use std::sync::Arc;

use super::ffi;
use crate::adapter::{Adapter, InnerAdapter};
use crate::types::Error;

struct InnerBackend {
    internal: cxx::UniquePtr<ffi::RustyBackend>,
}

unsafe impl Sync for InnerBackend {}
unsafe impl Send for InnerBackend {}

/// A source of Bluetooth adapters, such as the operating system's Bluetooth stack or the Dongl
/// USB adapters.
///
/// Use it to pick adapters from one backend, for example Dongl adapters, while
/// [`Adapter::get_adapters`] returns the adapters of every backend.
#[derive(Clone)]
pub struct Backend {
    inner: Arc<InnerBackend>,
}

impl Backend {
    /// Returns the available backends, initializing them if needed.
    ///
    /// Set the [configuration](crate::config) first, for example to enable the Dongl backend.
    pub fn get_backends() -> Result<Vec<Backend>, Error> {
        let mut raw_backend_list = ffi::RustyBackend_get_backends().map_err(Error::from_cxx_exception)?;

        let mut backends = Vec::<Backend>::new();
        for backend_wrapper in raw_backend_list.iter_mut() {
            let internal = std::mem::replace(&mut backend_wrapper.internal, cxx::UniquePtr::null());
            backends.push(Backend {
                inner: Arc::new(InnerBackend { internal }),
            });
        }

        Ok(backends)
    }

    /// Returns the backend's identifier, such as `"Dongl"`.
    pub fn identifier(&self) -> String {
        self.inner.internal.identifier()
    }

    pub fn bluetooth_enabled(&self) -> Result<bool, Error> {
        self.inner.internal.bluetooth_enabled().map_err(Error::from_cxx_exception)
    }

    /// Returns the adapters of this backend.
    pub fn adapters(&self) -> Result<Vec<Adapter>, Error> {
        let mut raw_adapter_list = self.inner.internal.adapters().map_err(Error::from_cxx_exception)?;

        let mut adapters = Vec::<Adapter>::new();
        for adapter_wrapper in raw_adapter_list.iter_mut() {
            adapters.push(InnerAdapter::new(adapter_wrapper).into());
        }

        Ok(adapters)
    }
}
