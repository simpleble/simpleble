use std::pin::Pin;
use std::mem;
use std::panic::{self, AssertUnwindSafe};
use std::sync::{Arc, Mutex};
use std::time::Duration;

use futures::Stream;

use super::ffi;
use crate::events::EventSender;
use crate::local_peripheral::InnerLocalPeripheral;
use crate::local_peripheral::LocalPeripheral;
use crate::peripheral::{current_callback, InnerPeripheral};
use crate::peripheral::Peripheral;
use crate::types::{call, Error, ErrorCode};

#[derive(Clone)]
pub enum ScanEvent {
    Start,
    Stop,
    Found(Peripheral),
    Updated(Peripheral),
}

#[derive(Clone)]
pub enum PowerEvent {
    On,
    Off,
}

pub(crate) type PasskeyRequestCallback = Arc<dyn Fn(&str) -> Option<String> + Send + Sync>;
pub(crate) type PasskeyDisplayCallback = Arc<dyn Fn(&str, &str) + Send + Sync>;
pub(crate) type NumericComparisonCallback = Arc<dyn Fn(&str, &str) -> bool + Send + Sync>;
pub(crate) type PairingAuthorizationCallback = Arc<dyn Fn(&str) -> bool + Send + Sync>;

pub struct InnerAdapter {
    internal: cxx::UniquePtr<ffi::RustyAdapter>,
    on_scan_event: EventSender<ScanEvent>,
    on_power_event: EventSender<PowerEvent>,

    // NOTE: Linux pairing callbacks run on SimpleBLE's BlueZ event thread, which waits for their answer, so each
    // one has a lock of its own. Changing one also takes `pairing_registration`, which keeps the stored callback
    // and the native one in step when clones change it at once.
    passkey_request_callback: Mutex<Option<PasskeyRequestCallback>>,
    passkey_display_callback: Mutex<Option<PasskeyDisplayCallback>>,
    numeric_comparison_callback: Mutex<Option<NumericComparisonCallback>>,
    pairing_authorization_callback: Mutex<Option<PairingAuthorizationCallback>>,
    #[cfg(any(target_os = "linux", doc))]
    pairing_registration: Mutex<()>,
}

// Registers or clears the native callback to match `callback`, storing it so that the native callback finds it.
// Clearing reaches SimpleBLE, which then answers as if no callback had been registered, so Just Works pairing is
// accepted again without an authorization callback. If the native call fails, the stored callback is unchanged.
#[cfg(any(target_os = "linux", doc))]
fn replace_pairing_callback<T>(
    registration: &Mutex<()>,
    slot: &Mutex<Option<T>>,
    callback: Option<T>,
    register: impl FnOnce(bool) -> Result<(), Error>,
) -> Result<(), Error> {
    use std::sync::PoisonError;

    let _registration = registration.lock().unwrap_or_else(PoisonError::into_inner);

    if callback.is_none() {
        register(false)?;
        *slot.lock().unwrap_or_else(PoisonError::into_inner) = None;
        return Ok(());
    }

    let previous = mem::replace(&mut *slot.lock().unwrap_or_else(PoisonError::into_inner), callback);
    if let Err(e) = register(true) {
        *slot.lock().unwrap_or_else(PoisonError::into_inner) = previous;
        return Err(e);
    }

    Ok(())
}

impl InnerAdapter {
    pub fn bluetooth_enabled() -> Result<bool, Error> {
        call(|error| ffi::RustyAdapter_bluetooth_enabled(error))
    }

    pub fn get_adapters() -> Result<Vec<Pin<Box<InnerAdapter>>>, Error> {
        let mut raw_adapter_list = call(|error| ffi::RustyAdapter_get_adapters(error))?;

        let mut adapters = Vec::<Pin<Box<InnerAdapter>>>::new();
        for adapter_wrapper in raw_adapter_list.iter_mut() {
            adapters.push(InnerAdapter::new(adapter_wrapper));
        }
        Ok(adapters)
    }

    pub(crate) fn new(wrapper: &mut ffi::RustyAdapterWrapper) -> Pin<Box<Self>> {
        let this = Self {
            internal: cxx::UniquePtr::<ffi::RustyAdapter>::null(),
            on_scan_event: EventSender::new(),
            on_power_event: EventSender::new(),
            passkey_request_callback: Mutex::new(None),
            passkey_display_callback: Mutex::new(None),
            numeric_comparison_callback: Mutex::new(None),
            pairing_authorization_callback: Mutex::new(None),
            #[cfg(any(target_os = "linux", doc))]
            pairing_registration: Mutex::new(()),
        };

        let mut this_boxed = Box::pin(this);
        wrapper.internal.link(this_boxed.as_mut());
        mem::swap(&mut this_boxed.internal, &mut wrapper.internal);

        return this_boxed;
    }

    pub fn initialized(&self) -> Result<bool, Error> {
        call(|error| self.internal.initialized(error))
    }

    pub fn identifier(&self) -> Result<String, Error> {
        call(|error| self.internal.identifier(error))
    }

    pub fn address(&self) -> Result<String, Error> {
        call(|error| self.internal.address(error))
    }

    pub fn power_on(&self) -> Result<(), Error> {
        call(|error| self.internal.power_on(error))
    }

    pub fn power_off(&self) -> Result<(), Error> {
        call(|error| self.internal.power_off(error))
    }

    pub fn is_powered(&self) -> Result<bool, Error> {
        call(|error| self.internal.is_powered(error))
    }

    pub fn scan_start(&self) -> Result<(), Error> {
        call(|error| self.internal.scan_start(error))
    }

    pub fn scan_stop(&self) -> Result<(), Error> {
        call(|error| self.internal.scan_stop(error))
    }

    pub fn scan_for(&self, duration: Duration) -> Result<(), Error> {
        // SimpleBLE scans for a whole number of milliseconds, so the duration is rounded up.
        let timeout_ms = i32::try_from(duration.as_nanos().div_ceil(1_000_000)).map_err(|_| {
            Error::new(ErrorCode::InvalidArgument, "scan duration must fit a 32-bit millisecond count".to_string())
        })?;

        call(|error| self.internal.scan_for(timeout_ms, error))
    }

    pub fn scan_is_active(&self) -> Result<bool, Error> {
        call(|error| self.internal.scan_is_active(error))
    }

    pub fn scan_get_results(&self) -> Result<Vec<Peripheral>, Error> {
        let mut raw_peripheral_list = call(|error| self.internal.scan_get_results(error))?;

        let mut peripherals = Vec::<Peripheral>::new();
        for peripheral_wrapper in raw_peripheral_list.iter_mut() {
            peripherals.push(InnerPeripheral::new(peripheral_wrapper).into());
        }

        return Ok(peripherals);
    }

    pub fn get_paired_peripherals(&self) -> Result<Vec<Peripheral>, Error> {
        let mut raw_peripheral_list =
            call(|error| self.internal.get_paired_peripherals(error))?;

        let mut peripherals = Vec::<Peripheral>::new();
        for peripheral_wrapper in raw_peripheral_list.iter_mut() {
            peripherals.push(InnerPeripheral::new(peripheral_wrapper).into());
        }

        return Ok(peripherals);
    }

    pub fn get_connected_peripherals(&self) -> Result<Vec<Peripheral>, Error> {
        let mut raw_peripheral_list =
            call(|error| self.internal.get_connected_peripherals(error))?;

        let mut peripherals = Vec::<Peripheral>::new();
        for peripheral_wrapper in raw_peripheral_list.iter_mut() {
            peripherals.push(InnerPeripheral::new(peripheral_wrapper).into());
        }

        return Ok(peripherals);
    }

    pub fn create_local_peripheral(&self) -> Result<LocalPeripheral, Error> {
        let internal = call(|error| self.internal.create_local_peripheral(error))?;
        Ok(InnerLocalPeripheral::new(internal).into())
    }

    #[cfg(any(target_os = "macos", doc))]
    pub(crate) fn macos_retrieve_cached_peripherals(&self, identifiers: &Vec<String>) -> Result<Vec<Peripheral>, Error> {
        let mut raw_peripheral_list =
            call(|error| self.internal.macos_retrieve_cached_peripherals(identifiers, error))?;

        let mut peripherals = Vec::<Peripheral>::new();
        for peripheral_wrapper in raw_peripheral_list.iter_mut() {
            peripherals.push(InnerPeripheral::new(peripheral_wrapper).into());
        }

        Ok(peripherals)
    }

    #[cfg(any(target_os = "linux", doc))]
    pub(crate) fn linux_set_passkey_request_callback(&self, callback: Option<PasskeyRequestCallback>) -> Result<(), Error> {
        replace_pairing_callback(&self.pairing_registration, &self.passkey_request_callback, callback, |enabled| {
            call(|error| self.internal.linux_set_passkey_request_callback(enabled, error))
        })
    }

    #[cfg(any(target_os = "linux", doc))]
    pub(crate) fn linux_set_passkey_display_callback(&self, callback: Option<PasskeyDisplayCallback>) -> Result<(), Error> {
        replace_pairing_callback(&self.pairing_registration, &self.passkey_display_callback, callback, |enabled| {
            call(|error| self.internal.linux_set_passkey_display_callback(enabled, error))
        })
    }

    #[cfg(any(target_os = "linux", doc))]
    pub(crate) fn linux_set_numeric_comparison_callback(
        &self,
        callback: Option<NumericComparisonCallback>,
    ) -> Result<(), Error> {
        replace_pairing_callback(&self.pairing_registration, &self.numeric_comparison_callback, callback, |enabled| {
            call(|error| self.internal.linux_set_numeric_comparison_callback(enabled, error))
        })
    }

    #[cfg(any(target_os = "linux", doc))]
    pub(crate) fn linux_set_pairing_authorization_callback(
        &self,
        callback: Option<PairingAuthorizationCallback>,
    ) -> Result<(), Error> {
        replace_pairing_callback(&self.pairing_registration, &self.pairing_authorization_callback, callback, |enabled| {
            call(|error| self.internal.linux_set_pairing_authorization_callback(enabled, error))
        })
    }

    // The pairing request is rejected if there's no callback or the callback panics.
    // NOTE: A panic must never unwind into C++.

    pub(crate) fn on_callback_passkey_request(&self, peer: &[u8], passkey: &mut String) -> bool {
        let Some(callback) = current_callback(&self.passkey_request_callback) else {
            return false;
        };

        let peer = String::from_utf8_lossy(peer);
        match panic::catch_unwind(AssertUnwindSafe(|| callback(&peer))) {
            Ok(Some(value)) => {
                *passkey = value;
                true
            }
            _ => false,
        }
    }

    pub(crate) fn on_callback_passkey_display(&self, peer: &[u8], passkey: &[u8]) {
        let Some(callback) = current_callback(&self.passkey_display_callback) else {
            return;
        };

        let (peer, passkey) = (String::from_utf8_lossy(peer), String::from_utf8_lossy(passkey));
        let _ = panic::catch_unwind(AssertUnwindSafe(|| callback(&peer, &passkey)));
    }

    pub(crate) fn on_callback_numeric_comparison(&self, peer: &[u8], passkey: &[u8]) -> bool {
        let Some(callback) = current_callback(&self.numeric_comparison_callback) else {
            return false;
        };

        let (peer, passkey) = (String::from_utf8_lossy(peer), String::from_utf8_lossy(passkey));
        panic::catch_unwind(AssertUnwindSafe(|| callback(&peer, &passkey))).unwrap_or(false)
    }

    pub(crate) fn on_callback_pairing_authorization(&self, peer: &[u8]) -> bool {
        let Some(callback) = current_callback(&self.pairing_authorization_callback) else {
            return false;
        };

        let peer = String::from_utf8_lossy(peer);
        panic::catch_unwind(AssertUnwindSafe(|| callback(&peer))).unwrap_or(false)
    }

    pub fn on_callback_scan_start(&self) {
        self.on_scan_event.send(ScanEvent::Start);
    }

    pub fn on_callback_scan_stop(&self) {
        self.on_scan_event.send(ScanEvent::Stop);
    }

    pub fn on_callback_scan_updated(&self, peripheral: &mut ffi::RustyPeripheralWrapper) {
        let peripheral: Peripheral = InnerPeripheral::new(peripheral).into();
        self.on_scan_event.send(ScanEvent::Updated(peripheral));
    }

    pub fn on_callback_scan_found(&self, peripheral: &mut ffi::RustyPeripheralWrapper) {
        let peripheral: Peripheral = InnerPeripheral::new(peripheral).into();
        self.on_scan_event.send(ScanEvent::Found(peripheral));
    }

    pub fn on_callback_power_on(&self) {
        self.on_power_event.send(PowerEvent::On);
    }

    pub fn on_callback_power_off(&self) {
        self.on_power_event.send(PowerEvent::Off);
    }
}

impl Drop for InnerAdapter {
    fn drop(&mut self) {
        self.internal.unlink();
    }
}

unsafe impl Sync for InnerAdapter {}
unsafe impl Send for InnerAdapter {}

#[derive(Clone)]
pub struct Adapter {
    inner: Arc<Pin<Box<InnerAdapter>>>,
}

impl Adapter {

    pub fn bluetooth_enabled() -> Result<bool, Error> {
        call(|error| ffi::RustyAdapter_bluetooth_enabled(error))
    }

    pub fn get_adapters() -> Result<Vec<Adapter>, Error> {
        let mut raw_adapter_list = call(|error| ffi::RustyAdapter_get_adapters(error))?;

        let mut adapters = Vec::<Adapter>::new();
        for adapter_wrapper in raw_adapter_list.iter_mut() {
            adapters.push(InnerAdapter::new(adapter_wrapper).into());
        }

        return Ok(adapters);
    }

    pub fn initialized(&self) -> Result<bool, Error> {
        self.inner.initialized()
    }

    pub fn identifier(&self) -> Result<String, Error> {
        self.inner.identifier()
    }

    pub fn address(&self) -> Result<String, Error> {
        self.inner.address()
    }

    pub fn power_on(&self) -> Result<(), Error> {
        self.inner.power_on()
    }

    pub fn power_off(&self) -> Result<(), Error> {
        self.inner.power_off()
    }

    pub fn is_powered(&self) -> Result<bool, Error> {
        self.inner.is_powered()
    }

    pub fn scan_start(&self) -> Result<(), Error> {
        self.inner.scan_start()
    }

    pub fn scan_stop(&self) -> Result<(), Error> {
        self.inner.scan_stop()
    }

    /// Scans for `duration`, rounded up to whole milliseconds, and returns once the scan stopped.
    ///
    /// Fails with [`ErrorCode::InvalidArgument`] for durations longer than `i32::MAX` milliseconds.
    pub fn scan_for(&self, duration: Duration) -> Result<(), Error> {
        self.inner.scan_for(duration)
    }

    pub fn scan_is_active(&self) -> Result<bool, Error> {
        self.inner.scan_is_active()
    }

    pub fn scan_get_results(&self) -> Result<Vec<Peripheral>, Error> {
        self.inner.scan_get_results()
    }

    pub fn get_paired_peripherals(&self) -> Result<Vec<Peripheral>, Error> {
        self.inner.get_paired_peripherals()
    }

    pub fn get_connected_peripherals(&self) -> Result<Vec<Peripheral>, Error> {
        self.inner.get_connected_peripherals()
    }

    /// Creates a local peripheral that hosts GATT services on this adapter.
    ///
    /// Configure its services, characteristics and advertised service UUIDs, then call
    /// [`LocalPeripheral::start`] to start advertising.
    pub fn create_local_peripheral(&self) -> Result<LocalPeripheral, Error> {
        self.inner.create_local_peripheral()
    }

    #[cfg(any(target_os = "macos", doc))]
    pub(crate) fn macos_retrieve_cached_peripherals(&self, identifiers: &Vec<String>) -> Result<Vec<Peripheral>, Error> {
        self.inner.macos_retrieve_cached_peripherals(identifiers)
    }

    #[cfg(any(target_os = "linux", doc))]
    pub(crate) fn inner(&self) -> &InnerAdapter {
        &self.inner
    }

    pub fn on_scan_event(&self) -> impl Stream<Item = ScanEvent> {
        let events = self.inner.on_scan_event.subscribe();
        self.inner.internal.register_scan_callbacks();
        events
    }

    pub fn on_power_event(&self) -> impl Stream<Item = PowerEvent> {
        let events = self.inner.on_power_event.subscribe();
        self.inner.internal.register_power_callbacks();
        events
    }
}


impl From<Pin<Box<InnerAdapter>>> for Adapter {
    fn from(adapter: Pin<Box<InnerAdapter>>) -> Self {
        Self {
            inner: Arc::new(adapter),
        }
    }
}
