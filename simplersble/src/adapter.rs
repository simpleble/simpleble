use std::pin::Pin;
use std::mem;
use std::sync::Arc;
use std::time::Duration;

use futures::Stream;

use super::ffi;
use crate::events::EventSender;
use crate::local_peripheral::InnerLocalPeripheral;
use crate::local_peripheral::LocalPeripheral;
use crate::peripheral::InnerPeripheral;
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

pub struct InnerAdapter {
    internal: cxx::UniquePtr<ffi::RustyAdapter>,
    on_scan_event: EventSender<ScanEvent>,
    on_power_event: EventSender<PowerEvent>,
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
