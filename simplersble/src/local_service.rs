use std::sync::{Arc, Mutex};

use super::ffi;
use crate::local_characteristic::InnerLocalCharacteristic;
use crate::local_characteristic::LocalCharacteristic;
use crate::types::{call, CharacteristicCapability, Error};

pub(crate) struct InnerLocalService {
    internal: cxx::UniquePtr<ffi::RustyLocalService>,

    // Characteristics added through this service. We keep a single Rust object per native
    // characteristic, so that all clones share the same callbacks and event stream.
    characteristics: Mutex<Vec<LocalCharacteristic>>,
}

impl InnerLocalService {
    pub(crate) fn new(internal: cxx::UniquePtr<ffi::RustyLocalService>) -> Self {
        Self {
            internal,
            characteristics: Mutex::new(Vec::new()),
        }
    }

    fn uuid(&self) -> Result<String, Error> {
        call(|error| self.internal.uuid(error))
    }

    fn add_characteristic(
        &self,
        uuid: &str,
        capabilities: &[CharacteristicCapability],
    ) -> Result<LocalCharacteristic, Error> {
        let mask = CharacteristicCapability::to_mask(capabilities);
        let internal = call(|error| self.internal.add_characteristic(uuid, mask, error))?;
        let characteristic: LocalCharacteristic = InnerLocalCharacteristic::new(internal).into();

        self.characteristics.lock().unwrap().push(characteristic.clone());
        Ok(characteristic)
    }

    fn characteristics(&self) -> Vec<LocalCharacteristic> {
        self.characteristics.lock().unwrap().clone()
    }
}

unsafe impl Sync for InnerLocalService {}
unsafe impl Send for InnerLocalService {}

/// A primary GATT service of a [`LocalPeripheral`](crate::LocalPeripheral).
///
/// Clones refer to the same service.
#[derive(Clone)]
pub struct LocalService {
    inner: Arc<InnerLocalService>,
}

impl LocalService {
    pub fn uuid(&self) -> Result<String, Error> {
        self.inner.uuid()
    }

    /// Adds a characteristic offering the given operations to clients.
    ///
    /// Each characteristic needs at least one capability.
    pub fn add_characteristic(
        &self,
        uuid: &str,
        capabilities: &[CharacteristicCapability],
    ) -> Result<LocalCharacteristic, Error> {
        self.inner.add_characteristic(uuid, capabilities)
    }

    /// Returns the characteristics added with [`add_characteristic`](Self::add_characteristic),
    /// in the order they were added.
    pub fn characteristics(&self) -> Vec<LocalCharacteristic> {
        self.inner.characteristics()
    }
}

impl From<InnerLocalService> for LocalService {
    fn from(service: InnerLocalService) -> Self {
        Self {
            inner: Arc::new(service),
        }
    }
}
