//! Library-wide configuration.
//!
//! SimpleBLE reads these settings when it creates backends and adapters. Set them before the first
//! call to [`Backend::get_backends`](crate::Backend::get_backends) or
//! [`Adapter::get_adapters`](crate::Adapter::get_adapters); changes made afterwards may not take
//! effect.
//!
//! ```no_run
//! simplersble::config::dongl::set_use_dongl_backend(false);
//! let backends = simplersble::Backend::get_backends().unwrap();
//! ```

use std::sync::{Mutex, PoisonError};
use std::time::Duration;

use super::ffi;

// The settings are plain globals on the native side, so we serialize access to them.
static CONFIG_LOCK: Mutex<()> = Mutex::new(());

fn locked<T>(access: impl FnOnce() -> T) -> T {
    let _guard = CONFIG_LOCK.lock().unwrap_or_else(PoisonError::into_inner);
    access()
}

fn duration_to_ns(duration: Duration) -> u64 {
    u64::try_from(duration.as_nanos()).unwrap_or(u64::MAX).min(i64::MAX as u64)
}

/// Restores every setting to its default value.
pub fn reset_all() {
    locked(ffi::RustyConfig_reset_all)
}

/// Settings for the Linux backend, which talks to BlueZ over D-Bus.
pub mod simplebluez {
    use super::*;

    /// Restores the SimpleBluez settings to their default values.
    pub fn reset() {
        locked(ffi::RustyConfig_simplebluez_reset)
    }

    /// Whether to connect to the system D-Bus instead of the session bus. Defaults to `true`.
    pub fn use_system_bus() -> bool {
        locked(ffi::RustyConfig_simplebluez_use_system_bus)
    }

    pub fn set_use_system_bus(enabled: bool) {
        locked(|| ffi::RustyConfig_simplebluez_set_use_system_bus(enabled))
    }

    /// How long to wait for a connection to complete. Defaults to 2 seconds.
    pub fn connection_timeout() -> Duration {
        Duration::from_nanos(locked(ffi::RustyConfig_simplebluez_connection_timeout_ns))
    }

    pub fn set_connection_timeout(timeout: Duration) {
        locked(|| ffi::RustyConfig_simplebluez_set_connection_timeout_ns(duration_to_ns(timeout)))
    }

    /// How long to wait for a disconnection to complete. Defaults to 1 second.
    pub fn disconnection_timeout() -> Duration {
        Duration::from_nanos(locked(ffi::RustyConfig_simplebluez_disconnection_timeout_ns))
    }

    pub fn set_disconnection_timeout(timeout: Duration) {
        locked(|| ffi::RustyConfig_simplebluez_set_disconnection_timeout_ns(duration_to_ns(timeout)))
    }
}

/// Settings for the Windows backend.
pub mod winrt {
    use super::*;

    /// Restores the WinRT settings to their default values.
    pub fn reset() {
        locked(ffi::RustyConfig_winrt_reset)
    }

    /// Whether WinRT calls run on SimpleBLE's own MTA thread. Defaults to `true`.
    #[deprecated(note = "SimpleBLE uses its own MTA apartment by default. This setting will be removed in a future release.")]
    pub fn experimental_use_own_mta_apartment() -> bool {
        locked(ffi::RustyConfig_winrt_experimental_use_own_mta_apartment)
    }

    #[deprecated(note = "SimpleBLE uses its own MTA apartment by default. This setting will be removed in a future release.")]
    pub fn set_experimental_use_own_mta_apartment(enabled: bool) {
        locked(|| ffi::RustyConfig_winrt_set_experimental_use_own_mta_apartment(enabled))
    }

    /// Experimental: whether to reinitialize the WinRT apartment on the calling thread when the
    /// backend starts. Defaults to `false`.
    pub fn experimental_reinitialize_winrt_apartment_on_main_thread() -> bool {
        locked(ffi::RustyConfig_winrt_experimental_reinitialize_winrt_apartment_on_main_thread)
    }

    pub fn set_experimental_reinitialize_winrt_apartment_on_main_thread(enabled: bool) {
        locked(|| ffi::RustyConfig_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(enabled))
    }

    /// Whether `disconnect()` returns immediately and cleans up in the background instead of
    /// blocking. Defaults to `true`.
    pub fn use_deferred_disconnect() -> bool {
        locked(ffi::RustyConfig_winrt_use_deferred_disconnect)
    }

    pub fn set_use_deferred_disconnect(enabled: bool) {
        locked(|| ffi::RustyConfig_winrt_set_use_deferred_disconnect(enabled))
    }
}

/// Settings for the macOS backend. There are none yet.
pub mod corebluetooth {
    use super::*;

    /// Restores the CoreBluetooth settings to their default values.
    pub fn reset() {
        locked(ffi::RustyConfig_corebluetooth_reset)
    }
}

/// Settings for the Android backend.
pub mod android {
    use super::*;

    pub fn reset() {
        locked(ffi::RustyConfig_android_reset)
    }

    /// Negotiate MTU on connect. Defaults to `true`.
    pub fn negotiate_mtu() -> bool {
        locked(ffi::RustyConfig_android_negotiate_mtu)
    }

    pub fn set_negotiate_mtu(enabled: bool) {
        locked(|| ffi::RustyConfig_android_set_negotiate_mtu(enabled))
    }
}

/// Settings for the Dongl USB adapter.
pub mod dongl {
    use super::*;

    /// Restores the Dongl settings to their default values.
    pub fn reset() {
        locked(ffi::RustyConfig_dongl_reset)
    }

    /// Whether to look for Dongl adapters. Defaults to `true`.
    ///
    /// When enabled, the backend whose identifier is `"Dongl"` provides one adapter per Dongl.
    pub fn use_dongl_backend() -> bool {
        locked(ffi::RustyConfig_dongl_use_dongl_backend)
    }

    pub fn set_use_dongl_backend(enabled: bool) {
        locked(|| ffi::RustyConfig_dongl_set_use_dongl_backend(enabled))
    }

    /// Reserved for Dongl firmware updates; the current backend ignores it. Defaults to `false`.
    pub fn auto_update() -> bool {
        locked(ffi::RustyConfig_dongl_auto_update)
    }

    pub fn set_auto_update(enabled: bool) {
        locked(|| ffi::RustyConfig_dongl_set_auto_update(enabled))
    }

    /// Reserved for Dongl firmware updates; the current backend ignores it. Defaults to `false`.
    pub fn force_update() -> bool {
        locked(ffi::RustyConfig_dongl_force_update)
    }

    pub fn set_force_update(enabled: bool) {
        locked(|| ffi::RustyConfig_dongl_set_force_update(enabled))
    }
}
