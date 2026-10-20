use std::sync::Arc;

use super::ffi;
use crate::adapter::{Adapter, InnerAdapter};
use crate::types::{call, Error};

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
    /// Set the [configuration](crate::config) first, for example to leave out the Dongl backend.
    pub fn get_backends() -> Result<Vec<Backend>, Error> {
        let mut raw_backend_list = call(|error| ffi::RustyBackend_get_backends(error))?;

        let mut backends = Vec::<Backend>::new();
        for backend_wrapper in raw_backend_list.iter_mut() {
            let internal = std::mem::replace(&mut backend_wrapper.internal, cxx::UniquePtr::null());
            backends.push(Backend {
                inner: Arc::new(InnerBackend { internal }),
            });
        }

        Ok(backends)
    }

    /// Returns the backend of the operating system's Bluetooth stack.
    pub fn get_native() -> Result<Backend, Error> {
        Self::from_internal(call(|error| ffi::RustyBackend_get_native(error)))
    }

    /// Returns the Dongl backend, whether or not
    /// [`config::dongl::use_dongl_backend`](crate::config::dongl::use_dongl_backend) lists it in
    /// [`get_backends`](Self::get_backends).
    ///
    /// Fails if this build of SimpleBLE doesn't include the Dongl backend.
    pub fn get_dongl() -> Result<Backend, Error> {
        Self::from_internal(call(|error| ffi::RustyBackend_get_dongl(error)))
    }

    /// Returns the Simulator backend, which only has adapters while a simulation is active.
    ///
    /// Fails if this build of SimpleBLE doesn't include the Simulator backend.
    pub fn get_simulator() -> Result<Backend, Error> {
        Self::from_internal(call(|error| ffi::RustyBackend_get_simulator(error)))
    }

    fn from_internal(internal: Result<cxx::UniquePtr<ffi::RustyBackend>, Error>) -> Result<Backend, Error> {
        let internal = internal?;
        Ok(Backend {
            inner: Arc::new(InnerBackend { internal }),
        })
    }

    /// Returns the backend's identifier, such as `"Dongl"`.
    pub fn identifier(&self) -> String {
        self.inner.internal.identifier()
    }

    pub fn bluetooth_enabled(&self) -> Result<bool, Error> {
        call(|error| self.inner.internal.bluetooth_enabled(error))
    }

    /// Returns the adapters of this backend.
    pub fn adapters(&self) -> Result<Vec<Adapter>, Error> {
        let mut raw_adapter_list = call(|error| self.inner.internal.adapters(error))?;

        let mut adapters = Vec::<Adapter>::new();
        for adapter_wrapper in raw_adapter_list.iter_mut() {
            adapters.push(InnerAdapter::new(adapter_wrapper).into());
        }

        Ok(adapters)
    }
}
