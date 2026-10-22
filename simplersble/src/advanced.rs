//! Backend- and platform-specific features that are not part of the common API.

/// Pairing with peripherals connected through a Dongl adapter.
///
/// The Dongl pairs without the operating system, so the application has to answer passkey and
/// numeric comparison requests itself. Register the callbacks before calling
/// [`connect`](crate::Peripheral::connect), as pairing can start as soon as the connection is up.
///
/// NOTE: Callbacks run on a native worker thread, possibly while `connect()` is still running,
/// and they can block while waiting for the user. Requests without a callback, or whose callback
/// panics, are rejected. Peripherals from other backends return an error.
///
/// NOTE: A callback stays registered until it's cleared or the last clone of the `Peripheral`
/// it was registered with is dropped. Registering through another `Peripheral` object for the
/// same device replaces it.
pub mod dongl {
    use std::sync::Arc;

    use crate::peripheral::Peripheral;
    use crate::types::Error;

    /// Registers a callback that returns the passkey the device displays, as entered by the user.
    ///
    /// Return exactly six decimal digits, including leading zeroes, or `None` to reject pairing.
    /// Any other value also rejects pairing.
    pub fn set_passkey_request_callback<F>(peripheral: &Peripheral, callback: F) -> Result<(), Error>
    where
        F: Fn() -> Option<String> + Send + Sync + 'static,
    {
        peripheral.set_passkey_request_callback(Some(Arc::new(callback)))
    }

    pub fn clear_passkey_request_callback(peripheral: &Peripheral) -> Result<(), Error> {
        peripheral.set_passkey_request_callback(None)
    }

    /// Registers a callback that shows the user a passkey to enter on the device.
    ///
    /// The passkey is exactly six decimal digits, including leading zeroes. Display it unmodified.
    pub fn set_passkey_display_callback<F>(peripheral: &Peripheral, callback: F) -> Result<(), Error>
    where
        F: Fn(&str) + Send + Sync + 'static,
    {
        peripheral.set_passkey_display_callback(Some(Arc::new(callback)))
    }

    pub fn clear_passkey_display_callback(peripheral: &Peripheral) -> Result<(), Error> {
        peripheral.set_passkey_display_callback(None)
    }

    /// Registers a callback for numeric comparison.
    ///
    /// The callback receives six decimal digits, including leading zeroes. Return `true` only after
    /// the user confirms that the device shows the same number; return `false` to reject pairing.
    pub fn set_numeric_comparison_callback<F>(peripheral: &Peripheral, callback: F) -> Result<(), Error>
    where
        F: Fn(&str) -> bool + Send + Sync + 'static,
    {
        peripheral.set_numeric_comparison_callback(Some(Arc::new(callback)))
    }

    pub fn clear_numeric_comparison_callback(peripheral: &Peripheral) -> Result<(), Error> {
        peripheral.set_numeric_comparison_callback(None)
    }

    /// Secures the connection with LE Secure Connections.
    ///
    /// Pairs with the connected device and blocks until the link is encrypted with LE Secure
    /// Connections and a 128-bit key. With `mitm`, the device must also be authenticated through
    /// the passkey or numeric comparison callbacks. Pairing started by this call creates no bond.
    /// If it fails, the Dongl disconnects.
    pub fn secure_connection(peripheral: &Peripheral, mitm: bool) -> Result<(), Error> {
        peripheral.secure_connection(mitm)
    }
}

/// CoreBluetooth extensions. Available on macOS.
#[cfg(any(target_os = "macos", doc))]
pub mod macos {
    use crate::adapter::Adapter;
    use crate::local_peripheral::LocalPeripheral;
    use crate::peripheral::Peripheral;
    use crate::types::Error;

    /// Sets the local name included in the peripheral's advertisement, or removes it with `None`.
    ///
    /// This changes only the advertisement, not the system Bluetooth or GAP device name.
    /// Call it before [`LocalPeripheral::start`].
    pub fn set_advertisement_local_name(peripheral: &LocalPeripheral, local_name: Option<&str>) -> Result<(), Error> {
        peripheral.inner().macos_set_advertisement_local_name(local_name)
    }

    /// Returns the peripherals that CoreBluetooth can resolve from its system cache, without scanning.
    ///
    /// The identifiers are CoreBluetooth peripheral identifiers, as returned by
    /// [`Peripheral::address`]. Invalid and unknown identifiers are skipped, and a returned
    /// peripheral is not necessarily reachable. The adapter must be powered on.
    pub fn retrieve_cached_peripherals<S: AsRef<str>>(
        adapter: &Adapter,
        identifiers: &[S],
    ) -> Result<Vec<Peripheral>, Error> {
        let identifiers = identifiers.iter().map(|identifier| identifier.as_ref().to_string()).collect();
        adapter.macos_retrieve_cached_peripherals(&identifiers)
    }
}

/// BlueZ extensions. Available on Linux.
#[cfg(any(target_os = "linux", doc))]
pub mod linux {
    use crate::local_peripheral::LocalPeripheral;
    use crate::types::Error;

    /// Sets the local name included in the peripheral's BlueZ advertisement, or removes it with `None`.
    ///
    /// This changes only the advertisement, not the adapter alias or GAP device name.
    /// Call it before [`LocalPeripheral::start`].
    pub fn set_advertisement_local_name(peripheral: &LocalPeripheral, local_name: Option<&str>) -> Result<(), Error> {
        peripheral.inner().linux_set_advertisement_local_name(local_name)
    }
}
