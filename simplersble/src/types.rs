use std::fmt;

use super::ffi;

#[derive(Debug, Clone)]
pub struct Error {
    msg: String,
}

impl Error {
    // Helper function to create an error from a C++ exception string
    #[allow(dead_code)]
    pub(crate) fn from_string(s: String) -> Self {
        Error { msg: s }
    }

    // Helper function to create an error from a C++ exception
    pub(crate) fn from_cxx_exception(e: cxx::Exception) -> Self {
        Error { msg: e.what().to_string() }
    }
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