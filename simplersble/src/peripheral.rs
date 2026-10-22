use std::collections::HashMap;
use std::mem;
use std::panic::{self, AssertUnwindSafe};
use std::pin::Pin;
use std::sync::{Arc, Mutex, PoisonError};

use futures::Stream;

use super::ffi;
use crate::events::EventSender;
use crate::service::Service;
use crate::service::InnerService;
use crate::types::{call, BluetoothAddressType, Error};

#[derive(Clone)]
pub enum ConnectionEvent {
    Connected,
    Disconnected,
}

#[derive(Clone)]
pub enum ValueChangedEvent {
    ValueUpdated(Vec<u8>),
}

pub(crate) type PasskeyRequestCallback = Arc<dyn Fn() -> Option<String> + Send + Sync>;
pub(crate) type PasskeyDisplayCallback = Arc<dyn Fn(&str) + Send + Sync>;
pub(crate) type NumericComparisonCallback = Arc<dyn Fn(&str) -> bool + Send + Sync>;

pub struct InnerPeripheral {
    internal: cxx::UniquePtr<ffi::RustyPeripheral>,

    on_connection_event: EventSender<ConnectionEvent>,
    on_characteristic_update_map: Mutex<HashMap<String, EventSender<ValueChangedEvent>>>,

    // NOTE: Pairing callbacks run on a native worker thread, possibly while `connect()` is still
    // running, so each one has a lock of its own.
    passkey_request_callback: Mutex<Option<PasskeyRequestCallback>>,
    passkey_display_callback: Mutex<Option<PasskeyDisplayCallback>>,
    numeric_comparison_callback: Mutex<Option<NumericComparisonCallback>>,
}

// Stores `callback` and then registers the native callback, so that the device uses this peripheral's callbacks.
// If the native call fails, the previous callback is put back.
// NOTE: The native callback forwards to whichever callback is stored and rejects the request without one, so it
// stays registered when `callback` is `None`. Clones changing callbacks at once can't leave the two out of step.
fn replace_pairing_callback<T>(
    slot: &Mutex<Option<T>>,
    callback: Option<T>,
    register: impl FnOnce() -> Result<(), Error>,
) -> Result<(), Error> {
    let previous = mem::replace(&mut *slot.lock().unwrap(), callback);

    if let Err(e) = register() {
        *slot.lock().unwrap() = previous;
        return Err(e);
    }

    Ok(())
}

fn current_callback<T: Clone>(slot: &Mutex<Option<T>>) -> Option<T> {
    slot.lock().unwrap_or_else(PoisonError::into_inner).clone()
}

impl InnerPeripheral {
    pub(crate) fn new(wrapper: &mut ffi::RustyPeripheralWrapper) -> Pin<Box<Self>> {
        let this = Self {
            internal: cxx::UniquePtr::<ffi::RustyPeripheral>::null(),
            on_connection_event: EventSender::new(),
            on_characteristic_update_map: Mutex::new(HashMap::new()),
            passkey_request_callback: Mutex::new(None),
            passkey_display_callback: Mutex::new(None),
            numeric_comparison_callback: Mutex::new(None),
        };

        let mut this_boxed = Box::pin(this);
        wrapper.internal.link(this_boxed.as_mut());
        mem::swap(&mut this_boxed.internal, &mut wrapper.internal);

        return this_boxed;
    }

    pub fn identifier(&self) -> Result<String, Error> {
        call(|error| self.internal.identifier(error))
    }

    pub fn initialized(&self) -> Result<bool, Error> {
        call(|error| self.internal.initialized(error))
    }

    pub fn address(&self) -> Result<String, Error> {
        call(|error| self.internal.address(error))
    }

    pub fn address_type(&self) -> Result<BluetoothAddressType, Error> {
        let address_type = call(|error| self.internal.address_type(error))?;

        return match address_type {
            ffi::BluetoothAddressType::PUBLIC => Ok(BluetoothAddressType::Public),
            ffi::BluetoothAddressType::RANDOM => Ok(BluetoothAddressType::Random),
            ffi::BluetoothAddressType::UNSPECIFIED => Ok(BluetoothAddressType::Unspecified),
            _ => Ok(BluetoothAddressType::Unspecified), // Or handle error appropriately
        };
    }

    pub fn rssi(&self) -> Result<i16, Error> {
        call(|error| self.internal.rssi(error))
    }

    pub fn tx_power(&self) -> Result<i16, Error> {
        call(|error| self.internal.tx_power(error))
    }

    pub fn mtu(&self) -> Result<u16, Error> {
        call(|error| self.internal.mtu(error))
    }

    pub fn connect(&self) -> Result<(), Error> {
        call(|error| self.internal.connect(error))
    }

    pub fn disconnect(&self) -> Result<(), Error> {
        call(|error| self.internal.disconnect(error))
    }

    pub fn is_connected(&self) -> Result<bool, Error> {
        call(|error| self.internal.is_connected(error))
    }

    pub fn is_connectable(&self) -> Result<bool, Error> {
        call(|error| self.internal.is_connectable(error))
    }

    pub fn is_paired(&self) -> Result<bool, Error> {
        call(|error| self.internal.is_paired(error))
    }

    pub fn unpair(&self) -> Result<(), Error> {
        call(|error| self.internal.unpair(error))
    }

    pub fn services(&self) -> Result<Vec<Service>, Error> {
        let mut raw_services = call(|error| self.internal.services(error))?;

        let mut services = Vec::<Service>::new();
        for service_wrapper in raw_services.iter_mut() {
            services.push(InnerService::new(service_wrapper).into());
        }

        Ok(services)
    }

    pub fn manufacturer_data(&self) -> Result<HashMap<u16, Vec<u8>>, Error> {
        let raw_manufacturer_data = call(|error| self.internal.manufacturer_data(error))?;

        let mut manufacturer_data = HashMap::<u16, Vec<u8>>::new();
        for raw_manuf_data in raw_manufacturer_data.iter() {
            manufacturer_data.insert(raw_manuf_data.company_id, raw_manuf_data.data.clone());
        }

        Ok(manufacturer_data)
    }

    pub fn read(&self, service: &String, characteristic: &String) -> Result<Vec<u8>, Error> {
        call(|error| self.internal.read(service, characteristic, error))
    }

    pub fn write_request(
        &self,
        service: &String,
        characteristic: &String,
        data: &Vec<u8>,
    ) -> Result<(), Error> {
        call(|error| self.internal.write_request(service, characteristic, data, error))
    }

    pub fn write_command(
        &self,
        service: &String,
        characteristic: &String,
        data: &Vec<u8>,
    ) -> Result<(), Error> {
        call(|error| self.internal.write_command(service, characteristic, data, error))
    }

    pub fn notify(
        &self,
        service: &String,
        characteristic: &String
    ) -> Result<impl Stream<Item = ValueChangedEvent>, Error> {
        // NOTE: The stream subscribes first, so that it gets the values that arrive while notify() runs.
        let updates = self.subscribe_to_characteristic_updates(format!("{}{}", service, characteristic));

        call(|error| self.internal.notify(service, characteristic, error))?;

        Ok(updates)
    }

    pub fn indicate(
        &self,
        service: &String,
        characteristic: &String
    ) -> Result<impl Stream<Item = ValueChangedEvent>, Error> {
        // NOTE: The stream subscribes first, so that it gets the values that arrive while indicate() runs.
        let updates = self.subscribe_to_characteristic_updates(format!("{}{}", service, characteristic));

        call(|error| self.internal.indicate(service, characteristic, error))?;

        Ok(updates)
    }

    // NOTE: Clones can unsubscribe at the same time, so the entry is looked up and subscribed to
    // under a single lock.
    fn subscribe_to_characteristic_updates(
        &self,
        key: String,
    ) -> impl Stream<Item = ValueChangedEvent> {
        self.on_characteristic_update_map
            .lock()
            .unwrap()
            .entry(key)
            .or_insert_with(EventSender::new)
            .subscribe()
    }

    pub fn unsubscribe(
        &self,
        service: &String,
        characteristic: &String,
    ) -> Result<(), Error> {
        let key = format!("{}{}", service, characteristic);
        self.on_characteristic_update_map.lock().unwrap().remove(&key);

        call(|error| self.internal.unsubscribe(service, characteristic, error))
    }

    pub fn descriptor_read(
        &self,
        service: &String,
        characteristic: &String,
        descriptor: &String,
    ) -> Result<Vec<u8>, Error> {
        call(|error| self.internal.read_descriptor(service, characteristic, descriptor, error))
    }

    pub fn descriptor_write(
        &self,
        service: &String,
        characteristic: &String,
        descriptor: &String,
        data: &Vec<u8>,
    ) -> Result<(), Error> {
        call(|error| self.internal.write_descriptor(service, characteristic, descriptor, data, error))
    }

    pub(crate) fn set_passkey_request_callback(&self, callback: Option<PasskeyRequestCallback>) -> Result<(), Error> {
        replace_pairing_callback(&self.passkey_request_callback, callback, || {
            call(|error| self.internal.dongl_set_passkey_request_callback(error))
        })
    }

    pub(crate) fn set_passkey_display_callback(&self, callback: Option<PasskeyDisplayCallback>) -> Result<(), Error> {
        replace_pairing_callback(&self.passkey_display_callback, callback, || {
            call(|error| self.internal.dongl_set_passkey_display_callback(error))
        })
    }

    pub(crate) fn set_numeric_comparison_callback(
        &self,
        callback: Option<NumericComparisonCallback>,
    ) -> Result<(), Error> {
        replace_pairing_callback(&self.numeric_comparison_callback, callback, || {
            call(|error| self.internal.dongl_set_numeric_comparison_callback(error))
        })
    }

    pub(crate) fn secure_connection(&self, mitm: bool) -> Result<(), Error> {
        call(|error| self.internal.dongl_secure_connection(mitm, error))
    }

    pub fn on_callback_connected(&self) {
        self.on_connection_event.send(ConnectionEvent::Connected);
    }

    pub fn on_callback_disconnected(&self) {
        self.on_connection_event.send(ConnectionEvent::Disconnected);
    }

    pub fn on_callback_characteristic_updated(
        &self,
        service: &String,
        characteristic: &String,
        data: &Vec<u8>,
    ) {
        let key = format!("{}{}", service, characteristic);

        if let Some(updates) = self.on_characteristic_update_map.lock().unwrap().get(&key) {
            updates.send(ValueChangedEvent::ValueUpdated(data.clone()));
        }
    }

    // The pairing request is rejected if there's no callback or the callback panics.
    // NOTE: A panic must never unwind into C++.

    pub(crate) fn on_callback_passkey_request(&self, passkey: &mut String) -> bool {
        let Some(callback) = current_callback(&self.passkey_request_callback) else {
            return false;
        };

        match panic::catch_unwind(AssertUnwindSafe(|| callback())) {
            Ok(Some(value)) => {
                *passkey = value;
                true
            }
            _ => false,
        }
    }

    pub(crate) fn on_callback_passkey_display(&self, passkey: &[u8]) {
        let Some(callback) = current_callback(&self.passkey_display_callback) else {
            return;
        };

        let passkey = String::from_utf8_lossy(passkey);
        let _ = panic::catch_unwind(AssertUnwindSafe(|| callback(&passkey)));
    }

    pub(crate) fn on_callback_numeric_comparison(&self, passkey: &[u8]) -> bool {
        let Some(callback) = current_callback(&self.numeric_comparison_callback) else {
            return false;
        };

        let passkey = String::from_utf8_lossy(passkey);
        panic::catch_unwind(AssertUnwindSafe(|| callback(&passkey))).unwrap_or(false)
    }
}

impl Drop for InnerPeripheral {
    fn drop(&mut self) {
        self.internal.unlink();
    }
}

unsafe impl Sync for InnerPeripheral {}
unsafe impl Send for InnerPeripheral {}

#[derive(Clone)]
pub struct Peripheral {
    inner: Arc<Pin<Box<InnerPeripheral>>>,
}

impl Peripheral {
    pub fn identifier(&self) -> Result<String, Error> {
        self.inner.identifier()
    }

    pub fn initialized(&self) -> Result<bool, Error> {
        self.inner.initialized()
    }

    pub fn address(&self) -> Result<String, Error> {
        self.inner.address()
    }

    pub fn address_type(&self) -> Result<BluetoothAddressType, Error> {
        self.inner.address_type()
    }

    pub fn rssi(&self) -> Result<i16, Error> {
        self.inner.rssi()
    }

    pub fn tx_power(&self) -> Result<i16, Error> {
        self.inner.tx_power()
    }

    pub fn mtu(&self) -> Result<u16, Error> {
        self.inner.mtu()
    }

    pub fn connect(&self) -> Result<(), Error> {
        self.inner.connect()
    }

    pub fn disconnect(&self) -> Result<(), Error> {
        self.inner.disconnect()
    }

    pub fn is_connected(&self) -> Result<bool, Error> {
        self.inner.is_connected()
    }

    pub fn is_connectable(&self) -> Result<bool, Error> {
        self.inner.is_connectable()
    }

    pub fn is_paired(&self) -> Result<bool, Error> {
        self.inner.is_paired()
    }

    pub fn unpair(&self) -> Result<(), Error> {
        self.inner.unpair()
    }

    pub fn services(&self) -> Result<Vec<Service>, Error> {
        self.inner.services()
    }

    pub fn manufacturer_data(&self) -> Result<HashMap<u16, Vec<u8>>, Error> {
        self.inner.manufacturer_data()
    }

    pub fn read(&self, service: &String, characteristic: &String) -> Result<Vec<u8>, Error> {
        self.inner.read(service, characteristic)
    }

    pub fn write_request(
        &self,
        service: &String,
        characteristic: &String,
        data: &Vec<u8>,
    ) -> Result<(), Error> {
        self.inner.write_request(service, characteristic, data)
    }

    pub fn write_command(
        &self,
        service: &String,
        characteristic: &String,
        data: &Vec<u8>,
    ) -> Result<(), Error> {
        self.inner.write_command(service, characteristic, data)
    }

    pub fn notify(
        &self,
        service: &String,
        characteristic: &String
    ) -> Result<impl Stream<Item = ValueChangedEvent>, Error> {
        self.inner.notify(service, characteristic)
    }

    pub fn indicate(
        &self,
        service: &String,
        characteristic: &String
    ) -> Result<impl Stream<Item = ValueChangedEvent>, Error> {
        self.inner.indicate(service, characteristic)
    }

    pub fn unsubscribe(&self, service: &String, characteristic: &String) -> Result<(), Error> {
        self.inner.unsubscribe(service, characteristic)
    }

    pub fn descriptor_read(
        &self,
        service: &String,
        characteristic: &String,
        descriptor: &String,
    ) -> Result<Vec<u8>, Error> {
        self.inner.descriptor_read(service, characteristic, descriptor)
    }

    pub fn descriptor_write(
        &self,
        service: &String,
        characteristic: &String,
        descriptor: &String,
        data: &Vec<u8>,
    ) -> Result<(), Error> {
        self.inner.descriptor_write(service, characteristic, descriptor, data)
    }

    pub fn on_connection_event(&self) -> impl Stream<Item = ConnectionEvent> {
        let events = self.inner.on_connection_event.subscribe();
        self.inner.internal.register_connection_callbacks();
        events
    }
}

impl Peripheral {
    pub(crate) fn set_passkey_request_callback(&self, callback: Option<PasskeyRequestCallback>) -> Result<(), Error> {
        self.inner.set_passkey_request_callback(callback)
    }

    pub(crate) fn set_passkey_display_callback(&self, callback: Option<PasskeyDisplayCallback>) -> Result<(), Error> {
        self.inner.set_passkey_display_callback(callback)
    }

    pub(crate) fn set_numeric_comparison_callback(
        &self,
        callback: Option<NumericComparisonCallback>,
    ) -> Result<(), Error> {
        self.inner.set_numeric_comparison_callback(callback)
    }

    pub(crate) fn secure_connection(&self, mitm: bool) -> Result<(), Error> {
        self.inner.secure_connection(mitm)
    }
}

impl From<Pin<Box<InnerPeripheral>>> for Peripheral {
    fn from(peripheral: Pin<Box<InnerPeripheral>>) -> Self {
        Self {
            inner: Arc::new(peripheral),
        }
    }
}
