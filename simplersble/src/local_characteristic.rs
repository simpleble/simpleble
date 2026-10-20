use std::cell::Cell;
use std::panic::{self, AssertUnwindSafe};
use std::pin::Pin;
use std::sync::{Arc, Mutex, PoisonError};

use futures::Stream;

use super::ffi;
use crate::events::EventSender;
use crate::types::{call, CharacteristicCapability, Error, ErrorCode};

/// A client interacting with a [`LocalCharacteristic`].
#[derive(Clone, Debug)]
pub enum LocalCharacteristicEvent {
    /// A client wrote this value. [`LocalCharacteristic::value`] is updated before the event is sent.
    Written(Vec<u8>),
    /// The first client subscribed to notifications or indications.
    Subscribed,
    /// The last client unsubscribed from notifications and indications.
    Unsubscribed,
}

type ReadCallback = Arc<dyn Fn() -> Vec<u8> + Send + Sync>;

thread_local! {
    // Number of read callbacks running on this thread.
    // NOTE: The native side holds on to the callback while it runs, so it can't replace or clear itself.
    static READ_CALLBACK_DEPTH: Cell<usize> = const { Cell::new(0) };
}

pub struct InnerLocalCharacteristic {
    internal: cxx::UniquePtr<ffi::RustyLocalCharacteristic>,
    on_event: EventSender<LocalCharacteristicEvent>,

    // NOTE: These are used from native threads through the back-pointer, so they have their own
    // locks, and no lock is held while user code runs.
    read_callback: Mutex<Option<ReadCallback>>,
    // Registers and clears the native read callback one at a time, so that the native callback
    // is only registered while `read_callback` is set.
    read_registration: Mutex<()>,
}

impl InnerLocalCharacteristic {
    pub(crate) fn new(internal: cxx::UniquePtr<ffi::RustyLocalCharacteristic>) -> Result<Pin<Box<Self>>, Error> {
        let this = Box::pin(Self {
            internal,
            on_event: EventSender::new(),
            read_callback: Mutex::new(None),
            read_registration: Mutex::new(()),
        });

        // The back-pointer is only given to C++ once the object has its final address.
        call(|error| this.internal.link(&this, error))?;

        Ok(this)
    }

    fn uuid(&self) -> Result<String, Error> {
        call(|error| self.internal.uuid(error))
    }

    fn capabilities(&self) -> Result<Vec<CharacteristicCapability>, Error> {
        let mask = call(|error| self.internal.capabilities(error))?;
        Ok(CharacteristicCapability::from_mask(mask))
    }

    fn value(&self) -> Result<Vec<u8>, Error> {
        call(|error| self.internal.value(error))
    }

    fn set_value(&self, value: &[u8]) -> Result<(), Error> {
        call(|error| self.internal.set_value(value, error))
    }

    fn set_read_callback(&self, callback: Option<ReadCallback>) -> Result<(), Error> {
        if READ_CALLBACK_DEPTH.with(Cell::get) > 0 {
            return Err(Error::new(
                ErrorCode::OperationFailed,
                "Read callbacks cannot be changed from inside a read callback.".to_string(),
            ));
        }

        let _registration = self.read_registration.lock().unwrap();

        match callback {
            Some(callback) => {
                // Store the callback before the native side can ask for it.
                let previous = self.read_callback.lock().unwrap().replace(callback);
                if previous.is_none() {
                    if let Err(e) = call(|error| self.internal.set_read_callback_enabled(true, error)) {
                        self.read_callback.lock().unwrap().take();
                        return Err(e);
                    }
                }
            }
            None => {
                // Unregister natively first, so that reads return the stored value again.
                call(|error| self.internal.set_read_callback_enabled(false, error))?;
                let _previous = self.read_callback.lock().unwrap().take();
            }
        }

        Ok(())
    }

    pub(crate) fn on_callback_read(&self) -> Vec<u8> {
        let callback = self.read_callback.lock().unwrap_or_else(PoisonError::into_inner).clone();
        let Some(callback) = callback else {
            return Vec::new();
        };

        // If the callback panics, answer with an empty value instead of unwinding into C++.
        READ_CALLBACK_DEPTH.with(|depth| depth.set(depth.get() + 1));
        let value = panic::catch_unwind(AssertUnwindSafe(|| callback())).unwrap_or_default();
        READ_CALLBACK_DEPTH.with(|depth| depth.set(depth.get() - 1));

        value
    }

    pub(crate) fn on_callback_written(&self, value: &[u8]) {
        self.on_event.send(LocalCharacteristicEvent::Written(value.to_vec()));
    }

    pub(crate) fn on_callback_subscribed(&self) {
        self.on_event.send(LocalCharacteristicEvent::Subscribed);
    }

    pub(crate) fn on_callback_unsubscribed(&self) {
        self.on_event.send(LocalCharacteristicEvent::Unsubscribed);
    }
}

impl Drop for InnerLocalCharacteristic {
    fn drop(&mut self) {
        // Clear the native callbacks (waiting for any that are running) before this object goes away.
        call(|error| self.internal.unlink(error)).unwrap();
    }
}

unsafe impl Sync for InnerLocalCharacteristic {}
unsafe impl Send for InnerLocalCharacteristic {}

/// A characteristic of a [`LocalService`](crate::LocalService).
///
/// Clones refer to the same characteristic and share its read callback and events.
#[derive(Clone)]
pub struct LocalCharacteristic {
    inner: Arc<Pin<Box<InnerLocalCharacteristic>>>,
}

impl LocalCharacteristic {
    pub fn uuid(&self) -> Result<String, Error> {
        self.inner.uuid()
    }

    pub fn capabilities(&self) -> Result<Vec<CharacteristicCapability>, Error> {
        self.inner.capabilities()
    }

    /// Returns the stored value, without calling the read callback.
    pub fn value(&self) -> Result<Vec<u8>, Error> {
        self.inner.value()
    }

    /// Updates the stored value.
    ///
    /// For a characteristic with the `Notify` or `Indicate` capability, this also publishes
    /// the value to subscribed clients on a best-effort basis.
    pub fn set_value(&self, value: &[u8]) -> Result<(), Error> {
        self.inner.set_value(value)
    }

    /// Answers client reads with the value returned by `callback` instead of the stored value.
    ///
    /// NOTE: The callback runs on a native thread while the client waits, so it must return
    /// quickly and must not call blocking BLE operations. If it panics, the client gets an
    /// empty value.
    ///
    /// NOTE: The characteristic holds on to the callback until it's cleared or dropped, so a
    /// callback that captures a clone of this characteristic keeps it alive.
    pub fn set_callback_on_read<F>(&self, callback: F) -> Result<(), Error>
    where
        F: Fn() -> Vec<u8> + Send + Sync + 'static,
    {
        self.inner.set_read_callback(Some(Arc::new(callback)))
    }

    /// Clears the read callback, so that reads return the stored value again.
    ///
    /// Waits for a read callback that is running on another thread.
    pub fn clear_callback_on_read(&self) -> Result<(), Error> {
        self.inner.set_read_callback(None)
    }

    /// Returns a stream of client writes and subscription changes.
    pub fn on_event(&self) -> impl Stream<Item = Result<LocalCharacteristicEvent, Error>> {
        self.inner.on_event.subscribe()
    }
}

impl From<Pin<Box<InnerLocalCharacteristic>>> for LocalCharacteristic {
    fn from(characteristic: Pin<Box<InnerLocalCharacteristic>>) -> Self {
        Self {
            inner: Arc::new(characteristic),
        }
    }
}
