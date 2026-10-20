use std::fmt;

use super::ffi;

/// The kind of failure an [`Error`] reports. The codes are shared with SimpleBLE's other bindings.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[non_exhaustive]
pub enum ErrorCode {
    /// An argument is invalid.
    InvalidArgument,
    /// Memory allocation failed.
    OutOfMemory,
    /// The object has not been initialized.
    ObjectNotInitialized,
    /// The object's backend is no longer available.
    InvalidBackendReference,
    /// The operation requires a connected peripheral.
    PeripheralNotConnected,
    /// The service UUID is not among the peripheral's services.
    GattServiceNotFound,
    /// The characteristic UUID is not in the service.
    GattCharacteristicNotFound,
    /// The descriptor UUID is not in the characteristic.
    GattDescriptorNotFound,
    /// The adapter, backend or characteristic doesn't support the operation.
    OperationNotSupported,
    /// The operation failed; the message has the details.
    OperationFailed,
    /// Windows denied access to Bluetooth.
    WinRtAccessDenied,
    /// A WinRT call failed.
    WinRtException,
    /// A CoreBluetooth call failed.
    CoreBluetoothException,
    /// Any other failure, such as one from D-Bus.
    UnclassifiedException,
    /// Another process holds the adapter, such as a Dongl whose serial port is open elsewhere.
    AdapterInUse,
}

impl From<ffi::ErrorCode> for ErrorCode {
    fn from(code: ffi::ErrorCode) -> Self {
        match code {
            ffi::ErrorCode::INVALID_ARGUMENT => ErrorCode::InvalidArgument,
            ffi::ErrorCode::OUT_OF_MEMORY => ErrorCode::OutOfMemory,
            ffi::ErrorCode::OBJECT_NOT_INITIALIZED => ErrorCode::ObjectNotInitialized,
            ffi::ErrorCode::INVALID_BACKEND_REFERENCE => ErrorCode::InvalidBackendReference,
            ffi::ErrorCode::PERIPHERAL_NOT_CONNECTED => ErrorCode::PeripheralNotConnected,
            ffi::ErrorCode::GATT_SERVICE_NOT_FOUND => ErrorCode::GattServiceNotFound,
            ffi::ErrorCode::GATT_CHARACTERISTIC_NOT_FOUND => ErrorCode::GattCharacteristicNotFound,
            ffi::ErrorCode::GATT_DESCRIPTOR_NOT_FOUND => ErrorCode::GattDescriptorNotFound,
            ffi::ErrorCode::OPERATION_NOT_SUPPORTED => ErrorCode::OperationNotSupported,
            ffi::ErrorCode::OPERATION_FAILED => ErrorCode::OperationFailed,
            ffi::ErrorCode::WINRT_ACCESS_DENIED => ErrorCode::WinRtAccessDenied,
            ffi::ErrorCode::WINRT_EXCEPTION => ErrorCode::WinRtException,
            ffi::ErrorCode::CORE_BLUETOOTH_EXCEPTION => ErrorCode::CoreBluetoothException,
            ffi::ErrorCode::ADAPTER_IN_USE => ErrorCode::AdapterInUse,
            _ => ErrorCode::UnclassifiedException,
        }
    }
}

/// A failed SimpleBLE call. It displays SimpleBLE's message, and [`code`](Self::code) tells failures apart.
#[derive(Debug, Clone)]
pub struct Error {
    code: ErrorCode,
    msg: String,
}

impl Error {
    pub(crate) fn new(code: ErrorCode, msg: String) -> Self {
        Error { code, msg }
    }

    pub fn code(&self) -> ErrorCode {
        self.code
    }
}

// Makes a bridge call, which reports a failure in `error` instead of throwing.
pub(crate) fn call<T>(func: impl FnOnce(&mut ffi::RustyError) -> T) -> Result<T, Error> {
    let mut error = ffi::RustyError {
        failed: false,
        code: ffi::ErrorCode::UNCLASSIFIED_EXCEPTION,
        message: String::new(),
    };
    let value = func(&mut error);

    if error.failed {
        return Err(Error::new(error.code.into(), error.message));
    }
    Ok(value)
}

#[derive(Debug)]
pub enum BluetoothAddressType {
    Public,
    Random,
    Unspecified,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum CharacteristicCapability {
    Read,
    WriteRequest,
    WriteCommand,
    Notify,
    Indicate,
}

impl CharacteristicCapability {
    const ALL: [CharacteristicCapability; 5] = [
        CharacteristicCapability::Read,
        CharacteristicCapability::WriteRequest,
        CharacteristicCapability::WriteCommand,
        CharacteristicCapability::Notify,
        CharacteristicCapability::Indicate,
    ];

    fn bit(self) -> u8 {
        match self {
            CharacteristicCapability::Read => ffi::RustyCharacteristicCapability::READ.repr,
            CharacteristicCapability::WriteRequest => ffi::RustyCharacteristicCapability::WRITE_REQUEST.repr,
            CharacteristicCapability::WriteCommand => ffi::RustyCharacteristicCapability::WRITE_COMMAND.repr,
            CharacteristicCapability::Notify => ffi::RustyCharacteristicCapability::NOTIFY.repr,
            CharacteristicCapability::Indicate => ffi::RustyCharacteristicCapability::INDICATE.repr,
        }
    }

    pub(crate) fn to_mask(capabilities: &[CharacteristicCapability]) -> u8 {
        capabilities.iter().fold(0, |mask, capability| mask | capability.bit())
    }

    pub(crate) fn from_mask(mask: u8) -> Vec<CharacteristicCapability> {
        Self::ALL.into_iter().filter(|capability| mask & capability.bit() != 0).collect()
    }
}

impl std::error::Error for Error {}

impl fmt::Display for BluetoothAddressType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            BluetoothAddressType::Public => write!(f, "Public"),
            BluetoothAddressType::Random => write!(f, "Random"),
            BluetoothAddressType::Unspecified => write!(f, "Unspecified"),
        }
    }
}

impl fmt::Display for CharacteristicCapability {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            CharacteristicCapability::Read => write!(f, "Read"),
            CharacteristicCapability::WriteRequest => write!(f, "WriteRequest"),
            CharacteristicCapability::WriteCommand => write!(f, "WriteCommand"),
            CharacteristicCapability::Notify => write!(f, "Notify"),
            CharacteristicCapability::Indicate => write!(f, "Indicate"),
        }
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "SimpleBLE error: {}", self.msg)
    }
}