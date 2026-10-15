use std::pin::Pin;
use std::sync::{Arc, Mutex};

use tokio::sync::broadcast;
use tokio_stream::wrappers::BroadcastStream;
use futures::{Stream, TryStreamExt};

use super::ffi;
use crate::local_service::InnerLocalService;
use crate::local_service::LocalService;
use crate::types::Error;

/// A client connecting to or disconnecting from a [`LocalPeripheral`].
///
/// Each event carries the client's Bluetooth address. On platforms that do not
/// expose the address, it carries a platform-specific identifier instead.
#[derive(Clone, Debug)]
pub enum ClientEvent {
    Connected(String),
    Disconnected(String),
}

pub struct InnerLocalPeripheral {
    internal: cxx::UniquePtr<ffi::RustyLocalPeripheral>,
    on_client_event: broadcast::Sender<ClientEvent>,

    // Services added through this peripheral. We keep a single Rust object per native service,
    // so that all clones share the same characteristics and callbacks.
    services: Mutex<Vec<LocalService>>,
}

impl InnerLocalPeripheral {
    pub(crate) fn new(internal: cxx::UniquePtr<ffi::RustyLocalPeripheral>) -> Result<Pin<Box<Self>>, Error> {
        let (client_event_sender, _) = broadcast::channel(128);

        let this = Box::pin(Self {
            internal,
            on_client_event: client_event_sender,
            services: Mutex::new(Vec::new()),
        });

        // The back-pointer is only given to C++ once the object has its final address.
        this.internal.link(&this).map_err(Error::from_cxx_exception)?;

        Ok(this)
    }

    fn add_advertised_service(&self, uuid: &str) -> Result<(), Error> {
        self.internal.add_advertised_service(uuid).map_err(Error::from_cxx_exception)
    }

    fn set_advertised_manufacturer_data(&self, company_id: u16, data: &[u8]) -> Result<(), Error> {
        self.internal
            .set_advertised_manufacturer_data(company_id, data)
            .map_err(Error::from_cxx_exception)
    }

    fn set_connectable(&self, connectable: bool) -> Result<(), Error> {
        self.internal.set_connectable(connectable).map_err(Error::from_cxx_exception)
    }

    fn add_service(&self, uuid: &str) -> Result<LocalService, Error> {
        let internal = self.internal.add_service(uuid).map_err(Error::from_cxx_exception)?;
        let service: LocalService = InnerLocalService::new(internal).into();

        self.services.lock().unwrap().push(service.clone());
        Ok(service)
    }

    fn services(&self) -> Vec<LocalService> {
        self.services.lock().unwrap().clone()
    }

    fn remove_all_services(&self) -> Result<(), Error> {
        self.internal.remove_all_services().map_err(Error::from_cxx_exception)?;
        self.services.lock().unwrap().clear();
        Ok(())
    }

    fn start(&self) -> Result<(), Error> {
        self.internal.start().map_err(Error::from_cxx_exception)
    }

    fn stop(&self) -> Result<(), Error> {
        self.internal.stop().map_err(Error::from_cxx_exception)
    }

    fn is_started(&self) -> Result<bool, Error> {
        self.internal.is_started().map_err(Error::from_cxx_exception)
    }

    fn is_advertising(&self) -> Result<bool, Error> {
        self.internal.is_advertising().map_err(Error::from_cxx_exception)
    }

    #[cfg(any(target_os = "macos", doc))]
    pub(crate) fn macos_set_advertisement_local_name(&self, local_name: Option<&str>) -> Result<(), Error> {
        self.internal
            .macos_set_advertisement_local_name(local_name.is_some(), local_name.unwrap_or_default())
            .map_err(Error::from_cxx_exception)
    }

    #[cfg(any(target_os = "linux", doc))]
    pub(crate) fn linux_set_advertisement_local_name(&self, local_name: Option<&str>) -> Result<(), Error> {
        self.internal
            .linux_set_advertisement_local_name(local_name.is_some(), local_name.unwrap_or_default())
            .map_err(Error::from_cxx_exception)
    }

    pub(crate) fn on_callback_client_connected(&self, address: &[u8]) {
        let address = String::from_utf8_lossy(address).into_owned();
        let _ = self.on_client_event.send(ClientEvent::Connected(address));
    }

    pub(crate) fn on_callback_client_disconnected(&self, address: &[u8]) {
        let address = String::from_utf8_lossy(address).into_owned();
        let _ = self.on_client_event.send(ClientEvent::Disconnected(address));
    }
}

impl Drop for InnerLocalPeripheral {
    fn drop(&mut self) {
        // Clear the native callbacks before this object (and the back-pointer to it) goes away.
        self.internal.unlink().unwrap();
    }
}

unsafe impl Sync for InnerLocalPeripheral {}
unsafe impl Send for InnerLocalPeripheral {}

/// A BLE peripheral hosted by this process, created with
/// [`Adapter::create_local_peripheral`](crate::Adapter::create_local_peripheral).
///
/// A [`Peripheral`](crate::Peripheral) is a remote device; a `LocalPeripheral` is this host
/// exposing GATT services to remote clients. Add services, characteristics and advertised
/// service UUIDs before [`start`](Self::start); they cannot change while started.
///
/// Clones refer to the same peripheral. Dropping the last clone stops the peripheral.
#[derive(Clone)]
pub struct LocalPeripheral {
    inner: Arc<Pin<Box<InnerLocalPeripheral>>>,
}

impl LocalPeripheral {
    /// Adds a service UUID to the advertisement.
    pub fn add_advertised_service(&self, uuid: &str) -> Result<(), Error> {
        self.inner.add_advertised_service(uuid)
    }

    /// Adds manufacturer specific data to the advertisement.
    ///
    /// Setting data for the same company identifier again replaces it.
    pub fn set_advertised_manufacturer_data(&self, company_id: u16, data: &[u8]) -> Result<(), Error> {
        self.inner.set_advertised_manufacturer_data(company_id, data)
    }

    /// Chooses whether centrals can connect. Local peripherals are connectable by default.
    pub fn set_connectable(&self, connectable: bool) -> Result<(), Error> {
        self.inner.set_connectable(connectable)
    }

    /// Adds a primary GATT service.
    pub fn add_service(&self, uuid: &str) -> Result<LocalService, Error> {
        self.inner.add_service(uuid)
    }

    /// Returns the services added with [`add_service`](Self::add_service), in the order they were added.
    pub fn services(&self) -> Vec<LocalService> {
        self.inner.services()
    }

    /// Removes all GATT services.
    pub fn remove_all_services(&self) -> Result<(), Error> {
        self.inner.remove_all_services()
    }

    /// Publishes the services and starts advertising.
    pub fn start(&self) -> Result<(), Error> {
        self.inner.start()
    }

    /// Stops advertising and hosting the services.
    pub fn stop(&self) -> Result<(), Error> {
        self.inner.stop()
    }

    pub fn is_started(&self) -> Result<bool, Error> {
        self.inner.is_started()
    }

    pub fn is_advertising(&self) -> Result<bool, Error> {
        self.inner.is_advertising()
    }

    /// Returns a stream of clients connecting and disconnecting.
    pub fn on_client_event(&self) -> impl Stream<Item = Result<ClientEvent, Error>> {
        BroadcastStream::new(self.inner.on_client_event.subscribe())
            .map_err(|e| Error::from_string(e.to_string()))
    }

    #[cfg(any(target_os = "macos", target_os = "linux", doc))]
    pub(crate) fn inner(&self) -> &InnerLocalPeripheral {
        &self.inner
    }
}

impl From<Pin<Box<InnerLocalPeripheral>>> for LocalPeripheral {
    fn from(peripheral: Pin<Box<InnerLocalPeripheral>>) -> Self {
        Self {
            inner: Arc::new(peripheral),
        }
    }
}
