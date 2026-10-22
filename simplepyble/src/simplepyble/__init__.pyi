"""
Type stubs for SimplePyBLE - Python bindings for SimpleBLE

This module provides Bluetooth Low Energy (BLE) functionality for Python.
"""

from typing import Callable, Dict, Iterable, List, Optional, Set, Union
from enum import Enum

__version__: str

class OperatingSystem(Enum):
    """Operating system enumeration."""
    WINDOWS = ...
    MACOS = ...
    IOS = ...
    LINUX = ...
    ANDROID = ...

class BluetoothAddressType(Enum):
    """Bluetooth address type enumeration."""
    PUBLIC = ...
    RANDOM = ...
    UNSPECIFIED = ...

class AndroidConnectionPriority(Enum):
    """Android connection priority request."""
    DISABLED = ...
    BALANCED = ...
    HIGH = ...
    LOW_POWER = ...
    DCK = ...

class ErrorCode(Enum):
    """Classification of a SimpleBLE error."""
    INVALID_ARGUMENT = ...
    OUT_OF_MEMORY = ...
    OBJECT_NOT_INITIALIZED = ...
    INVALID_BACKEND_REFERENCE = ...
    PERIPHERAL_NOT_CONNECTED = ...
    GATT_SERVICE_NOT_FOUND = ...
    GATT_CHARACTERISTIC_NOT_FOUND = ...
    GATT_DESCRIPTOR_NOT_FOUND = ...
    OPERATION_NOT_SUPPORTED = ...
    OPERATION_FAILED = ...
    WINRT_ACCESS_DENIED = ...
    WINRT_EXCEPTION = ...
    CORE_BLUETOOTH_EXCEPTION = ...
    UNCLASSIFIED_EXCEPTION = ...
    ADAPTER_IN_USE = ...

class SimpleBLEError(RuntimeError):
    """Raised when a SimpleBLE operation fails."""
    code: ErrorCode
    """The error code."""
    native_code: Optional[int]
    """The HRESULT of a WinRT failure, otherwise None."""

class NotInitializedError(SimpleBLEError):
    """The object was not initialized."""

class InvalidReferenceError(SimpleBLEError):
    """The object's backend object is no longer valid."""

class NotConnectedError(SimpleBLEError):
    """The operation requires a connected peripheral."""

class ServiceNotFoundError(SimpleBLEError):
    """The peripheral has no service with that UUID."""

class CharacteristicNotFoundError(SimpleBLEError):
    """The service has no characteristic with that UUID."""

class DescriptorNotFoundError(SimpleBLEError):
    """The characteristic has no descriptor with that UUID."""

class OperationNotSupportedError(SimpleBLEError):
    """The adapter, characteristic or platform doesn't support the operation."""

class OperationFailedError(SimpleBLEError):
    """The operation failed. The message has any available detail."""

class WinRTError(SimpleBLEError):
    """A WinRT call failed."""

class WinRTAccessDeniedError(WinRTError):
    """Windows denied access to the operation."""

class CoreBluetoothError(SimpleBLEError):
    """A CoreBluetooth call failed."""

class AdapterInUseError(SimpleBLEError):
    """Another process holds the adapter."""

class InvalidArgumentError(SimpleBLEError, ValueError):
    """An argument is out of range. It is a ValueError too."""

class Descriptor:
    """Represents a GATT descriptor."""

    def initialized(self) -> bool:
        """Check if the descriptor is initialized."""
        ...
    
    def uuid(self) -> str:
        """
        Get the UUID of the descriptor.
        
        Returns:
            str: The descriptor UUID
        """
        ...

class Characteristic:
    """Represents a GATT characteristic."""

    def initialized(self) -> bool:
        """Check if the characteristic is initialized."""
        ...
    
    def uuid(self) -> str:
        """
        Get the UUID of the characteristic.
        
        Returns:
            str: The characteristic UUID
        """
        ...
    
    def descriptors(self) -> List[Descriptor]:
        """
        Get the descriptors of the characteristic.
        
        Returns:
            List[Descriptor]: List of descriptors
        """
        ...
    
    def capabilities(self) -> List[str]:
        """
        Get the capabilities of the characteristic.
        
        Returns:
            List[str]: List of capability strings
        """
        ...
    
    def can_read(self) -> bool:
        """
        Check if the characteristic can be read.
        
        Returns:
            bool: True if readable, False otherwise
        """
        ...
    
    def can_write_request(self) -> bool:
        """
        Check if the characteristic can be written with a request.
        
        Returns:
            bool: True if writable with request, False otherwise
        """
        ...
    
    def can_write_command(self) -> bool:
        """
        Check if the characteristic can be written with a command.
        
        Returns:
            bool: True if writable with command, False otherwise
        """
        ...
    
    def can_notify(self) -> bool:
        """
        Check if the characteristic can send notifications.
        
        Returns:
            bool: True if notifications supported, False otherwise
        """
        ...
    
    def can_indicate(self) -> bool:
        """
        Check if the characteristic can send indications.
        
        Returns:
            bool: True if indications supported, False otherwise
        """
        ...

class Service:
    """Represents a GATT service."""

    def initialized(self) -> bool:
        """Check if the service is initialized."""
        ...
    
    def uuid(self) -> str:
        """
        Get the UUID of the service.
        
        Returns:
            str: The service UUID
        """
        ...
    
    def data(self) -> bytes:
        """
        Get the advertised service data.
        
        Returns:
            bytes: The service data
        """
        ...
    
    def characteristics(self) -> List[Characteristic]:
        """
        Get the characteristics of the service.
        
        Returns:
            List[Characteristic]: List of characteristics
        """
        ...

class Peripheral:
    """Represents a BLE peripheral device."""
    
    def initialized(self) -> bool:
        """
        Check if the peripheral is initialized.
        
        Returns:
            bool: True if initialized, False otherwise
        """
        ...
    
    def identifier(self) -> str:
        """
        Get the identifier of the peripheral.
        
        Returns:
            str: The peripheral identifier
        """
        ...
    
    def address(self) -> str:
        """
        Get the address of the peripheral.
        
        Returns:
            str: The peripheral address
        """
        ...
    
    def address_type(self) -> BluetoothAddressType:
        """
        Get the address type of the peripheral.
        
        Returns:
            BluetoothAddressType: The address type
        """
        ...
    
    def rssi(self) -> int:
        """
        Get the RSSI (Received Signal Strength Indicator) of the peripheral.
        
        Returns:
            int: The RSSI value in dBm
        """
        ...
    
    def tx_power(self) -> int:
        """
        Get the transmit power of the peripheral.
        
        Returns:
            int: The transmit power in dBm
        """
        ...
    
    def mtu(self) -> int:
        """
        Get the negotiated MTU (Maximum Transmission Unit) value.
        
        Returns:
            int: The MTU value
        """
        ...
    
    def connect(self) -> None:
        """Connect to the peripheral."""
        ...
    
    def disconnect(self) -> None:
        """Disconnect from the peripheral."""
        ...
    
    def is_connected(self) -> bool:
        """
        Check if the peripheral is connected.
        
        Returns:
            bool: True if connected, False otherwise
        """
        ...
    
    def is_connectable(self) -> bool:
        """
        Check if the peripheral is connectable.
        
        Returns:
            bool: True if connectable, False otherwise
        """
        ...
    
    def is_paired(self) -> bool:
        """
        Check if the peripheral is paired.
        
        Returns:
            bool: True if paired, False otherwise
        """
        ...
    
    def unpair(self) -> None:
        """Unpair the peripheral."""
        ...
    
    def services(self) -> List[Service]:
        """
        Get the services of the peripheral.
        
        Returns:
            List[Service]: List of services
        """
        ...
    
    def manufacturer_data(self) -> Dict[int, bytes]:
        """
        Get the manufacturer data of the peripheral.
        
        Returns:
            Dict[int, bytes]: Dictionary mapping manufacturer ID to data
        """
        ...
    
    def read(self, service: str, characteristic: str) -> bytes:
        """
        Read a characteristic from the peripheral.
        
        Args:
            service: The service UUID
            characteristic: The characteristic UUID
            
        Returns:
            bytes: The characteristic value
        """
        ...
    
    def write_request(self, service: str, characteristic: str, payload: bytes) -> None:
        """
        Write a request to the peripheral.
        
        Args:
            service: The service UUID
            characteristic: The characteristic UUID
            payload: The data to write
        """
        ...
    
    def write_command(self, service: str, characteristic: str, payload: bytes) -> None:
        """
        Write a command to the peripheral.
        
        Args:
            service: The service UUID
            characteristic: The characteristic UUID
            payload: The data to write
        """
        ...
    
    def notify(self, service: str, characteristic: str, callback: Callable[[bytes], None]) -> None:
        """
        Subscribe to notifications from a characteristic.
        
        Args:
            service: The service UUID
            characteristic: The characteristic UUID
            callback: Callback function to receive notification data
        """
        ...
    
    def indicate(self, service: str, characteristic: str, callback: Callable[[bytes], None]) -> None:
        """
        Subscribe to indications from a characteristic.
        
        Args:
            service: The service UUID
            characteristic: The characteristic UUID
            callback: Callback function to receive indication data
        """
        ...
    
    def unsubscribe(self, service: str, characteristic: str) -> None:
        """
        Unsubscribe from a characteristic.
        
        Args:
            service: The service UUID
            characteristic: The characteristic UUID
        """
        ...
    
    def descriptor_read(self, service: str, characteristic: str, descriptor: str) -> bytes:
        """
        Read a descriptor from the peripheral.
        
        Args:
            service: The service UUID
            characteristic: The characteristic UUID
            descriptor: The descriptor UUID
            
        Returns:
            bytes: The descriptor value
        """
        ...
    
    def descriptor_write(self, service: str, characteristic: str, descriptor: str, payload: bytes) -> None:
        """
        Write a descriptor to the peripheral.
        
        Args:
            service: The service UUID
            characteristic: The characteristic UUID
            descriptor: The descriptor UUID
            payload: The data to write
        """
        ...
    
    def set_callback_on_connected(self, callback: Optional[Callable[[], None]]) -> None:
        """
        Set the callback to be called when the peripheral connects.
        
        Args:
            callback: Callback function to call on connection
        """
        ...
    
    def set_callback_on_disconnected(self, callback: Optional[Callable[[], None]]) -> None:
        """
        Set the callback to be called when the peripheral disconnects.

        Args:
            callback: Callback function to call on disconnection
        """
        ...

class LocalCharacteristicCapability(Enum):
    """Capability of a local characteristic."""
    READ = ...
    WRITE_REQUEST = ...
    WRITE_COMMAND = ...
    NOTIFY = ...
    INDICATE = ...

class LocalCharacteristic:
    """
    Represents a characteristic hosted by a local peripheral.

    Callbacks run on a SimpleBLE thread and should return promptly. Pass None
    to unregister a callback. If a callback raises, the exception is logged to
    the ``simplepyble`` logger.
    """

    def initialized(self) -> bool:
        """Check if the local characteristic is initialized."""
        ...

    def uuid(self) -> str:
        """
        Get the UUID of the local characteristic.

        Returns:
            str: The characteristic UUID
        """
        ...

    def capabilities(self) -> Set[LocalCharacteristicCapability]:
        """
        Get the capabilities of the local characteristic.

        Returns:
            Set[LocalCharacteristicCapability]: The characteristic capabilities
        """
        ...

    def value(self) -> bytes:
        """
        Get the current value of the local characteristic.

        Returns:
            bytes: The characteristic value
        """
        ...

    def set_value(self, value: Union[bytes, bytearray, memoryview]) -> None:
        """
        Update the value of the local characteristic.

        If the characteristic has the NOTIFY or INDICATE capability, the new
        value is published to subscribed clients on a best-effort basis.

        Args:
            value: The new value
        """
        ...

    def set_callback_on_read(self, callback: Optional[Callable[[], bytes]]) -> None:
        """
        Set the callback that supplies the value for client reads.

        Without a read callback, reads return value(). If the callback raises
        or returns something other than a bytes-like object, the error is
        logged and the client receives an error (BlueZ serves the stored value
        instead).

        Args:
            callback: Callback function returning the value to serve
        """
        ...

    def set_callback_on_write(self, callback: Optional[Callable[[bytes], None]]) -> None:
        """
        Set the callback to be called when a client writes the characteristic.

        The written value is stored before the callback runs. Calling
        set_value() from the callback publishes the value to subscribed clients.

        Args:
            callback: Callback function receiving the written value
        """
        ...

    def set_callback_on_subscribed(self, callback: Optional[Callable[[], None]]) -> None:
        """
        Set the callback to be called when the first client subscribes.

        Args:
            callback: Callback function to call on subscription
        """
        ...

    def set_callback_on_unsubscribed(self, callback: Optional[Callable[[], None]]) -> None:
        """
        Set the callback to be called when the last client unsubscribes.

        Args:
            callback: Callback function to call on unsubscription
        """
        ...

class LocalService:
    """Represents a primary GATT service hosted by a local peripheral."""

    def initialized(self) -> bool:
        """Check if the local service is initialized."""
        ...

    def uuid(self) -> str:
        """
        Get the UUID of the local service.

        Returns:
            str: The service UUID
        """
        ...

    def add_characteristic(
        self, uuid: str, capabilities: Iterable[LocalCharacteristicCapability]
    ) -> LocalCharacteristic:
        """
        Add a characteristic to the local service.

        Args:
            uuid: The characteristic UUID
            capabilities: At least one capability of the characteristic

        Returns:
            LocalCharacteristic: The new characteristic
        """
        ...

    def characteristics(self) -> List[LocalCharacteristic]:
        """
        Get the characteristics of the local service.

        Returns:
            List[LocalCharacteristic]: List of characteristics
        """
        ...

class LocalPeripheral:
    """
    Represents a BLE peripheral hosted by this process.

    Configure advertised services, services, and characteristics before
    start(); they cannot change while started. Advertising stops when stop()
    is called or the object is destroyed.
    """

    def initialized(self) -> bool:
        """Check if the local peripheral is initialized."""
        ...

    def add_advertised_service(self, service_uuids: Union[str, List[str]]) -> None:
        """
        Add one or more service UUIDs to the advertisement.

        Args:
            service_uuids: A service UUID or a list of service UUIDs
        """
        ...

    def set_advertised_manufacturer_data(self, company_id: int, data: Union[bytes, bytearray, memoryview]) -> None:
        """
        Add manufacturer specific data to the advertisement.

        Setting data for the same company identifier again replaces it.

        Args:
            company_id: The company identifier
            data: The manufacturer specific data
        """
        ...

    def set_connectable(self, connectable: bool) -> None:
        """
        Choose whether centrals can connect.

        Local peripherals are connectable by default.

        Args:
            connectable: Whether centrals can connect
        """
        ...

    def add_service(self, uuid: str) -> LocalService:
        """
        Add a primary GATT service.

        Args:
            uuid: The service UUID

        Returns:
            LocalService: The new service
        """
        ...

    def services(self) -> List[LocalService]:
        """
        Get the services of the local peripheral.

        Returns:
            List[LocalService]: List of services
        """
        ...

    def remove_all_services(self) -> None:
        """Remove all services from the local peripheral."""
        ...

    def start(self) -> None:
        """Publish services and start advertising."""
        ...

    def stop(self) -> None:
        """Stop advertising and hosting the services."""
        ...

    def is_started(self) -> bool:
        """
        Check if the local peripheral is started.

        Returns:
            bool: True if started, False otherwise
        """
        ...

    def is_advertising(self) -> bool:
        """
        Check if the local peripheral is advertising.

        Returns:
            bool: True if advertising, False otherwise
        """
        ...

    def set_callback_on_client_connected(self, callback: Optional[Callable[[str], None]]) -> None:
        """
        Set the callback to be called when a client connects.

        Args:
            callback: Callback function receiving the client address, or a
                platform-specific identifier where the address is unavailable
        """
        ...

    def set_callback_on_client_disconnected(self, callback: Optional[Callable[[str], None]]) -> None:
        """
        Set the callback to be called when a client disconnects.

        Args:
            callback: Callback function receiving the client address, or a
                platform-specific identifier where the address is unavailable
        """
        ...

class Adapter:
    """Represents a BLE adapter."""
    
    @staticmethod
    def bluetooth_enabled() -> bool:
        """
        Check if Bluetooth is enabled.
        
        Returns:
            bool: True if Bluetooth is enabled, False otherwise
        """
        ...
    
    @staticmethod
    def get_adapters() -> List['Adapter']:
        """
        Get all available adapters.
        
        Returns:
            List[Adapter]: List of available adapters
        """
        ...
    
    def initialized(self) -> bool:
        """
        Check if the adapter is initialized.
        
        Returns:
            bool: True if initialized, False otherwise
        """
        ...
    
    def identifier(self) -> str:
        """
        Get the identifier of the adapter.
        
        Returns:
            str: The adapter identifier
        """
        ...
    
    def address(self) -> str:
        """
        Get the address of the adapter.
        
        Returns:
            str: The adapter address
        """
        ...
    
    def power_on(self) -> None:
        """Power on the adapter."""
        ...
    
    def power_off(self) -> None:
        """Power off the adapter."""
        ...
    
    def is_powered(self) -> bool:
        """
        Check if the adapter is powered.
        
        Returns:
            bool: True if powered, False otherwise
        """
        ...
    
    def scan_start(self) -> None:
        """Start scanning for peripherals."""
        ...
    
    def scan_stop(self) -> None:
        """Stop scanning for peripherals."""
        ...
    
    def scan_is_active(self) -> bool:
        """
        Check if the adapter is scanning.
        
        Returns:
            bool: True if scanning, False otherwise
        """
        ...
    
    def scan_for(self, timeout_ms: int) -> None:
        """
        Scan for peripherals for a given duration.
        
        Args:
            timeout_ms: Scan duration in milliseconds

        Raises:
            InvalidArgumentError: If timeout_ms is negative
        """
        ...
    
    def scan_get_results(self) -> List[Peripheral]:
        """
        Get the results of the last scan.
        
        Returns:
            List[Peripheral]: List of discovered peripherals
        """
        ...
    
    def set_callback_on_scan_start(self, callback: Optional[Callable[[], None]]) -> None:
        """
        Set the callback to be called when scanning starts.
        
        Args:
            callback: Callback function to call when scan starts
        """
        ...
    
    def set_callback_on_scan_stop(self, callback: Optional[Callable[[], None]]) -> None:
        """
        Set the callback to be called when scanning stops.
        
        Args:
            callback: Callback function to call when scan stops
        """
        ...
    
    def set_callback_on_scan_found(self, callback: Optional[Callable[[Peripheral], None]]) -> None:
        """
        Set the callback to be called when a peripheral is found.
        
        Args:
            callback: Callback function to call when a peripheral is found
        """
        ...
    
    def set_callback_on_scan_updated(self, callback: Optional[Callable[[Peripheral], None]]) -> None:
        """
        Set the callback to be called when a peripheral is updated.
        
        Args:
            callback: Callback function to call when a peripheral is updated
        """
        ...
    
    def get_paired_peripherals(self) -> List[Peripheral]:
        """
        Get all paired peripherals.
        
        Returns:
            List[Peripheral]: List of paired peripherals
        """
        ...

    def set_callback_on_power_on(self, callback: Optional[Callable[[], None]]) -> None:
        """Set the callback to be called when the adapter is powered on."""
        ...

    def set_callback_on_power_off(self, callback: Optional[Callable[[], None]]) -> None:
        """Set the callback to be called when the adapter is powered off."""
        ...

    def get_connected_peripherals(self) -> List[Peripheral]:
        """Get all connected peripherals."""
        ...

    def create_local_peripheral(self) -> LocalPeripheral:
        """
        Create a local peripheral hosted by this adapter.

        Returns:
            LocalPeripheral: The new local peripheral
        """
        ...

class Backend:
    """Represents a SimpleBLE backend."""

    @staticmethod
    def get_backends() -> List['Backend']:
        """Get all enabled SimpleBLE backends."""
        ...

    @staticmethod
    def get_native() -> 'Backend':
        """Get the backend of the operating system's Bluetooth stack."""
        ...

    @staticmethod
    def get_dongl() -> 'Backend':
        """Get the Dongl backend, whether or not config.dongl.use_dongl_backend lists it in get_backends().

        Raises:
            RuntimeError: If this build of SimplePyBLE doesn't include the Dongl backend.
        """
        ...

    @staticmethod
    def get_simulator() -> 'Backend':
        """Get the Simulator backend, which only has adapters while a simulation is active.

        Raises:
            RuntimeError: If this build of SimplePyBLE doesn't include the Simulator backend.
        """
        ...

    def initialized(self) -> bool:
        """Check if the backend is initialized."""
        ...

    def adapters(self) -> List[Adapter]:
        """Get all adapters provided by this backend."""
        ...

    def bluetooth_enabled(self) -> bool:
        """Check if Bluetooth is enabled for this backend."""
        ...

    def identifier(self) -> str:
        """Get the backend identifier."""
        ...

class _WinRTConfig:
    """WinRT-specific configuration options."""
    
    experimental_use_own_mta_apartment: bool
    """Deprecated: SimpleBLE uses its own MTA apartment by default."""
    
    experimental_reinitialize_winrt_apartment_on_main_thread: bool
    """Reinitialize the WinRT apartment on the main thread (experimental)"""

    use_deferred_disconnect: bool
    """Defer WinRT disconnect completion so disconnect calls return without waiting for the OS."""
    
    @staticmethod
    def reset() -> None:
        """Reset WinRT configuration options to their default values."""
        ...

class _SimpleBluezConfig:
    """SimpleBluez-specific configuration options."""

    use_system_bus: bool
    """Use the system D-Bus instead of the session D-Bus."""

    connection_timeout_ms: int
    """Connection timeout in milliseconds."""

    disconnection_timeout_ms: int
    """Disconnection timeout in milliseconds."""
    
    @staticmethod
    def reset() -> None:
        """Reset SimpleBluez configuration options to their default values."""
        ...

class _CoreBluetoothConfig:
    """CoreBluetooth-specific configuration options."""
    
    @staticmethod
    def reset() -> None:
        """Reset CoreBluetooth configuration options to their default values."""
        ...

class _AndroidConfig:
    """Android-specific configuration options."""

    negotiate_mtu: bool
    """Negotiate MTU on connect. Defaults to true."""

    connection_priority_request: AndroidConnectionPriority
    """Android connection priority request."""
    
    @staticmethod
    def reset() -> None:
        """Reset Android configuration options to their default values."""
        ...

class _DonglConfig:
    """SimpleBLE Dongl configuration options."""

    use_dongl_backend: bool
    """Include the SimpleBLE Dongl backend. Defaults to True."""

    auto_update: bool
    """Automatically update SimpleBLE Dongl firmware."""

    force_update: bool
    """Force a SimpleBLE Dongl firmware update."""

    @staticmethod
    def reset() -> None:
        """Reset SimpleBLE Dongl configuration options to their default values."""
        ...

class _BaseConfig:
    """Base configuration options."""
    
    @staticmethod
    def reset_all() -> None:
        """Reset all configuration options to their default values."""
        ...

class _Config:
    """Configuration options for SimpleBLE. Set values before retrieving any adapter."""
    
    winrt: _WinRTConfig
    """WinRT-specific configuration"""
    
    simplebluez: _SimpleBluezConfig
    """SimpleBluez-specific configuration"""
    
    corebluetooth: _CoreBluetoothConfig
    """CoreBluetooth-specific configuration"""
    
    android: _AndroidConfig
    """Android-specific configuration"""

    dongl: _DonglConfig
    """SimpleBLE Dongl configuration"""
    
    base: _BaseConfig
    """Base configuration"""

config: _Config
"""Configuration module for SimpleBLE"""

class _AdvancedMacOS:
    """Advanced CoreBluetooth APIs."""

    def set_advertisement_local_name(self, peripheral: LocalPeripheral, local_name: Optional[str]) -> None:
        """
        Set the local name included in the peripheral's advertisement.

        This changes only the advertisement payload, not the system Bluetooth
        name. Call before start().

        Args:
            peripheral: The local peripheral
            local_name: The advertised name, or None to remove the override
        """
        ...

    def retrieve_cached_peripherals(self, adapter: Adapter, identifiers: List[str]) -> List[Peripheral]:
        """Retrieve peripherals that CoreBluetooth can resolve from its system cache."""
        ...

class _AdvancedLinux:
    """Advanced BlueZ APIs."""

    def set_advertisement_local_name(self, peripheral: LocalPeripheral, local_name: Optional[str]) -> None:
        """
        Set the local name included in the peripheral's BlueZ advertisement.

        This changes only the advertisement payload, not the adapter alias.
        Call before start().

        Args:
            peripheral: The local peripheral
            local_name: The advertised name, or None to remove the override
        """
        ...

class _AdvancedDongl:
    """
    Pairing callbacks for peripherals reached through a Dongl adapter.

    Register them before connect(), since pairing can start as soon as the
    link is up. Callbacks run on a SimpleBLE worker thread and may block while
    the user answers. Passing None unregisters a callback. A request with no
    callback, or whose callback raises, is rejected. Registering on a
    peripheral from another backend raises RuntimeError.
    """

    def set_passkey_request_callback(
        self, peripheral: Peripheral, callback: Optional[Callable[[], Optional[str]]]
    ) -> None:
        """
        Register a callback for entering a passkey requested by the peer.

        Args:
            peripheral: The peripheral to pair with
            callback: Callback function returning the passkey as a str of
                exactly six decimal digits, including leading zeroes, or None
                to reject pairing. Any other result rejects pairing.
        """
        ...

    def set_passkey_display_callback(
        self, peripheral: Peripheral, callback: Optional[Callable[[str], None]]
    ) -> None:
        """
        Register a callback for a passkey that the peer must enter.

        Args:
            peripheral: The peripheral to pair with
            callback: Callback function receiving the six-digit passkey to show
        """
        ...

    def set_numeric_comparison_callback(
        self, peripheral: Peripheral, callback: Optional[Callable[[str], bool]]
    ) -> None:
        """
        Register a callback for numeric comparison during pairing.

        Args:
            peripheral: The peripheral to pair with
            callback: Callback function receiving the six-digit number and
                returning True only if the peer shows the same number. Any
                other result rejects pairing.
        """
        ...

    def secure_connection(self, peripheral: Peripheral, mitm: bool = False) -> None:
        """
        Secure the connection with LE Secure Connections.

        Pairs with the connected peer and waits until the link is encrypted
        with LE Secure Connections and a 128-bit key. Pairing started by this
        call creates no bond. If it fails, the Dongl disconnects.

        Args:
            peripheral: The connected peripheral
            mitm: Whether the peer must also be authenticated through the
                passkey or numeric comparison callbacks
        """
        ...

class _Advanced:
    """Platform-specific advanced SimpleBLE APIs."""

    dongl: _AdvancedDongl
    """Available on every platform."""

    macos: _AdvancedMacOS
    """Available on macOS."""

    linux: _AdvancedLinux
    """Available on Linux."""

advanced: _Advanced

def get_operating_system() -> OperatingSystem:
    """
    Get the currently-running operating system.
    
    Returns:
        OperatingSystem: The current operating system
    """
    ...

def get_simpleble_version() -> str:
    """
    Get the version of SimpleBLE.
    
    Returns:
        str: The SimpleBLE version string
    """
    ...
