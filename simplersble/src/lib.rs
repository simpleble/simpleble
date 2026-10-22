pub mod types;
pub mod adapter;
pub mod peripheral;
pub mod service;
pub mod characteristic;
pub mod descriptor;
pub mod backend;
pub mod config;
pub mod advanced;
pub mod local_peripheral;
pub mod local_service;
pub mod local_characteristic;
mod events;

use adapter::InnerAdapter;
use peripheral::InnerPeripheral;
use local_peripheral::InnerLocalPeripheral;
use local_characteristic::InnerLocalCharacteristic;

pub use types::{Error, ErrorCode, BluetoothAddressType, CharacteristicCapability};
pub use adapter::Adapter;
pub use adapter::ScanEvent;
pub use adapter::PowerEvent;
pub use peripheral::Peripheral;
pub use peripheral::ConnectionEvent;
pub use peripheral::ValueChangedEvent;
pub use service::Service;
pub use characteristic::Characteristic;
pub use descriptor::Descriptor;
pub use backend::Backend;
pub use local_peripheral::LocalPeripheral;
pub use local_peripheral::ClientEvent;
pub use local_service::LocalService;
pub use local_characteristic::LocalCharacteristic;
pub use local_characteristic::LocalCharacteristicEvent;

#[cxx::bridge]
mod ffi {

    // ----------------------------------------------------------------------------------

    // The following structs are used to wrap the underlying classes into the objects
    // that are used in the Rust bindings.

    #[namespace = "Bindings"]
    struct RustyAdapterWrapper {
        internal: UniquePtr<RustyAdapter>,
    }

    #[namespace = "Bindings"]
    struct RustyPeripheralWrapper {
        internal: UniquePtr<RustyPeripheral>,
    }

    #[namespace = "Bindings"]
    struct RustyServiceWrapper {
        internal: UniquePtr<RustyService>,
    }

    #[namespace = "Bindings"]
    struct RustyCharacteristicWrapper {
        internal: UniquePtr<RustyCharacteristic>,
    }

    #[namespace = "Bindings"]
    struct RustyDescriptorWrapper {
        internal: UniquePtr<RustyDescriptor>,
    }

    #[namespace = "Bindings"]
    struct RustyBackendWrapper {
        internal: UniquePtr<RustyBackend>,
    }

    // ----------------------------------------------------------------------------------

    // A failed call fills this in instead of throwing, as an exception only carries its message
    // across cxx.
    #[namespace = "Bindings"]
    struct RustyError {
        failed: bool,
        code: ErrorCode,
        message: String,
    }

    #[namespace = "Bindings"]
    struct RustyManufacturerDataWrapper {
        company_id: u16,
        data: Vec<u8>,
    }

    #[namespace = "SimpleBLE"]
    #[repr(i32)]
    enum BluetoothAddressType {
        PUBLIC,
        RANDOM,
        UNSPECIFIED,
    }

    #[namespace = "SimpleBLE"]
    #[repr(i32)]
    enum ErrorCode {
        INVALID_ARGUMENT = 0,
        OUT_OF_MEMORY = 1,
        OBJECT_NOT_INITIALIZED = 2,
        INVALID_BACKEND_REFERENCE = 3,
        PERIPHERAL_NOT_CONNECTED = 4,
        GATT_SERVICE_NOT_FOUND = 5,
        GATT_CHARACTERISTIC_NOT_FOUND = 6,
        GATT_DESCRIPTOR_NOT_FOUND = 7,
        OPERATION_NOT_SUPPORTED = 8,
        OPERATION_FAILED = 9,
        WINRT_ACCESS_DENIED = 10,
        WINRT_EXCEPTION = 11,
        CORE_BLUETOOTH_EXCEPTION = 12,
        UNCLASSIFIED_EXCEPTION = 13,
        ADAPTER_IN_USE = 14,
    }

    // Bit values used to pass a set of local characteristic capabilities as a mask.
    #[namespace = "Bindings"]
    #[repr(u8)]
    enum RustyCharacteristicCapability {
        READ = 1,
        WRITE_REQUEST = 2,
        WRITE_COMMAND = 4,
        NOTIFY = 8,
        INDICATE = 16,
    }

    // ----------------------------------------------------------------------------------

    #[namespace = "SimpleRsBLE"]
    extern "Rust" {
        type InnerAdapter;

        pub fn on_callback_scan_start(self: &InnerAdapter);
        pub fn on_callback_scan_stop(self: &InnerAdapter);
        pub fn on_callback_scan_updated(self: &InnerAdapter, peripheral: &mut RustyPeripheralWrapper);
        pub fn on_callback_scan_found(self: &InnerAdapter, peripheral: &mut RustyPeripheralWrapper);
        pub fn on_callback_power_on(self: &InnerAdapter);
        pub fn on_callback_power_off(self: &InnerAdapter);

        type InnerPeripheral;

        pub fn on_callback_connected(self: &InnerPeripheral);
        pub fn on_callback_disconnected(self: &InnerPeripheral);
        pub fn on_callback_characteristic_updated(
            self: &InnerPeripheral,
            service: &String,
            Characteristic: &String,
            data: &Vec<u8>,
        );
        pub fn on_callback_passkey_request(self: &InnerPeripheral, passkey: &mut String) -> bool;
        pub fn on_callback_passkey_display(self: &InnerPeripheral, passkey: &[u8]);
        pub fn on_callback_numeric_comparison(self: &InnerPeripheral, passkey: &[u8]) -> bool;

        type InnerLocalPeripheral;

        pub fn on_callback_client_connected(self: &InnerLocalPeripheral, address: &[u8]);
        pub fn on_callback_client_disconnected(self: &InnerLocalPeripheral, address: &[u8]);

        type InnerLocalCharacteristic;

        pub fn on_callback_read(self: &InnerLocalCharacteristic) -> Vec<u8>;
        pub fn on_callback_written(self: &InnerLocalCharacteristic, value: &[u8]);
        pub fn on_callback_subscribed(self: &InnerLocalCharacteristic);
        pub fn on_callback_unsubscribed(self: &InnerLocalCharacteristic);
    }

    unsafe extern "C++" {
        include!("src/bindings/Bindings.hpp");

        #[namespace = "SimpleBLE"]
        type BluetoothAddressType;

        #[namespace = "SimpleBLE"]
        type ErrorCode;

        #[namespace = "Bindings"]
        type RustyAdapter;

        #[namespace = "Bindings"]
        type RustyPeripheral;

        #[namespace = "Bindings"]
        type RustyService;

        #[namespace = "Bindings"]
        type RustyCharacteristic;

        #[namespace = "Bindings"]
        type RustyDescriptor;

        #[namespace = "Bindings"]
        type RustyBackend;

        #[namespace = "Bindings"]
        type RustyLocalPeripheral;

        #[namespace = "Bindings"]
        type RustyLocalService;

        #[namespace = "Bindings"]
        type RustyLocalCharacteristic;

        // Common functions

        #[namespace = "Bindings"]
        fn RustyAdapter_bluetooth_enabled(error: &mut RustyError) -> bool;

        #[namespace = "Bindings"]
        fn RustyAdapter_get_adapters(error: &mut RustyError) -> Vec<RustyAdapterWrapper>;

        #[namespace = "Bindings"]
        fn RustyBackend_get_backends(error: &mut RustyError) -> Vec<RustyBackendWrapper>;

        #[namespace = "Bindings"]
        fn RustyBackend_get_native(error: &mut RustyError) -> UniquePtr<RustyBackend>;

        #[namespace = "Bindings"]
        fn RustyBackend_get_dongl(error: &mut RustyError) -> UniquePtr<RustyBackend>;

        #[namespace = "Bindings"]
        fn RustyBackend_get_simulator(error: &mut RustyError) -> UniquePtr<RustyBackend>;

        // Config functions

        #[namespace = "Bindings"]
        fn RustyConfig_reset_all();

        #[namespace = "Bindings"]
        fn RustyConfig_simplebluez_reset();
        #[namespace = "Bindings"]
        fn RustyConfig_simplebluez_use_system_bus() -> bool;
        #[namespace = "Bindings"]
        fn RustyConfig_simplebluez_set_use_system_bus(enabled: bool);
        #[namespace = "Bindings"]
        fn RustyConfig_simplebluez_connection_timeout_ns() -> u64;
        #[namespace = "Bindings"]
        fn RustyConfig_simplebluez_set_connection_timeout_ns(timeout_ns: u64);
        #[namespace = "Bindings"]
        fn RustyConfig_simplebluez_disconnection_timeout_ns() -> u64;
        #[namespace = "Bindings"]
        fn RustyConfig_simplebluez_set_disconnection_timeout_ns(timeout_ns: u64);

        #[namespace = "Bindings"]
        fn RustyConfig_winrt_reset();
        #[namespace = "Bindings"]
        fn RustyConfig_winrt_experimental_use_own_mta_apartment() -> bool;
        #[namespace = "Bindings"]
        fn RustyConfig_winrt_set_experimental_use_own_mta_apartment(enabled: bool);
        #[namespace = "Bindings"]
        fn RustyConfig_winrt_experimental_reinitialize_winrt_apartment_on_main_thread() -> bool;
        #[namespace = "Bindings"]
        fn RustyConfig_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(enabled: bool);
        #[namespace = "Bindings"]
        fn RustyConfig_winrt_use_deferred_disconnect() -> bool;
        #[namespace = "Bindings"]
        fn RustyConfig_winrt_set_use_deferred_disconnect(enabled: bool);

        #[namespace = "Bindings"]
        fn RustyConfig_corebluetooth_reset();

        #[namespace = "Bindings"]
        fn RustyConfig_android_reset();
        #[namespace = "Bindings"]
        fn RustyConfig_android_negotiate_mtu() -> bool;
        #[namespace = "Bindings"]
        fn RustyConfig_android_set_negotiate_mtu(enabled: bool);

        #[namespace = "Bindings"]
        fn RustyConfig_dongl_reset();
        #[namespace = "Bindings"]
        fn RustyConfig_dongl_use_dongl_backend() -> bool;
        #[namespace = "Bindings"]
        fn RustyConfig_dongl_set_use_dongl_backend(enabled: bool);
        #[namespace = "Bindings"]
        fn RustyConfig_dongl_auto_update() -> bool;
        #[namespace = "Bindings"]
        fn RustyConfig_dongl_set_auto_update(enabled: bool);
        #[namespace = "Bindings"]
        fn RustyConfig_dongl_force_update() -> bool;
        #[namespace = "Bindings"]
        fn RustyConfig_dongl_set_force_update(enabled: bool);

        // RustyBackend functions

        fn identifier(self: &RustyBackend) -> String;
        fn bluetooth_enabled(self: &RustyBackend, error: &mut RustyError) -> bool;
        fn adapters(self: &RustyBackend, error: &mut RustyError) -> Vec<RustyAdapterWrapper>;

        // RustyAdapter functions

        fn link(self: &RustyAdapter, target: Pin<&mut InnerAdapter>);
        fn unlink(self: &RustyAdapter);
        fn register_scan_callbacks(self: &RustyAdapter);
        fn register_power_callbacks(self: &RustyAdapter);

        fn initialized(self: &RustyAdapter, error: &mut RustyError) -> bool;
        fn identifier(self: &RustyAdapter, error: &mut RustyError) -> String;
        fn address(self: &RustyAdapter, error: &mut RustyError) -> String;

        fn power_on(self: &RustyAdapter, error: &mut RustyError);
        fn power_off(self: &RustyAdapter, error: &mut RustyError);
        fn is_powered(self: &RustyAdapter, error: &mut RustyError) -> bool;

        fn scan_start(self: &RustyAdapter, error: &mut RustyError);
        fn scan_stop(self: &RustyAdapter, error: &mut RustyError);
        fn scan_for(self: &RustyAdapter, timeout_ms: i32, error: &mut RustyError);
        fn scan_is_active(self: &RustyAdapter, error: &mut RustyError) -> bool;
        fn scan_get_results(self: &RustyAdapter, error: &mut RustyError) -> Vec<RustyPeripheralWrapper>;

        fn get_paired_peripherals(self: &RustyAdapter, error: &mut RustyError) -> Vec<RustyPeripheralWrapper>;
        fn get_connected_peripherals(self: &RustyAdapter, error: &mut RustyError) -> Vec<RustyPeripheralWrapper>;

        fn create_local_peripheral(self: &RustyAdapter, error: &mut RustyError) -> UniquePtr<RustyLocalPeripheral>;

        #[cfg(any(target_os = "macos", doc))]
        fn macos_retrieve_cached_peripherals(
            self: &RustyAdapter,
            identifiers: &Vec<String>,
            error: &mut RustyError,
        ) -> Vec<RustyPeripheralWrapper>;

        // RustyPeripheral functions

        fn link(self: &RustyPeripheral, target: Pin<&mut InnerPeripheral>);
        fn unlink(self: &RustyPeripheral);
        fn register_connection_callbacks(self: &RustyPeripheral);

        fn initialized(self: &RustyPeripheral, error: &mut RustyError) -> bool;
        fn identifier(self: &RustyPeripheral, error: &mut RustyError) -> String;
        fn address(self: &RustyPeripheral, error: &mut RustyError) -> String;
        fn address_type(self: &RustyPeripheral, error: &mut RustyError) -> BluetoothAddressType;
        fn rssi(self: &RustyPeripheral, error: &mut RustyError) -> i16;

        fn tx_power(self: &RustyPeripheral, error: &mut RustyError) -> i16;
        fn mtu(self: &RustyPeripheral, error: &mut RustyError) -> u16;

        fn connect(self: &RustyPeripheral, error: &mut RustyError);
        fn disconnect(self: &RustyPeripheral, error: &mut RustyError);
        fn is_connected(self: &RustyPeripheral, error: &mut RustyError) -> bool;
        fn is_connectable(self: &RustyPeripheral, error: &mut RustyError) -> bool;
        fn is_paired(self: &RustyPeripheral, error: &mut RustyError) -> bool;
        fn unpair(self: &RustyPeripheral, error: &mut RustyError);

        fn services(self: &RustyPeripheral, error: &mut RustyError) -> Vec<RustyServiceWrapper>;
        fn manufacturer_data(self: &RustyPeripheral, error: &mut RustyError) -> Vec<RustyManufacturerDataWrapper>;

        fn read(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
            error: &mut RustyError,
        ) -> Vec<u8>;
        fn write_request(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
            data: &Vec<u8>,
            error: &mut RustyError,
        );
        fn write_command(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
            data: &Vec<u8>,
            error: &mut RustyError,
        );
        fn notify(self: &RustyPeripheral, service: &String, characteristic: &String, error: &mut RustyError);
        fn indicate(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
            error: &mut RustyError,
        );
        fn unsubscribe(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
            error: &mut RustyError,
        );

        fn read_descriptor(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
            descriptor: &String,
            error: &mut RustyError,
        ) -> Vec<u8>;
        fn write_descriptor(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
            descriptor: &String,
            data: &Vec<u8>,
            error: &mut RustyError,
        );

        fn dongl_set_passkey_request_callback(self: &RustyPeripheral, error: &mut RustyError);
        fn dongl_set_passkey_display_callback(self: &RustyPeripheral, error: &mut RustyError);
        fn dongl_set_numeric_comparison_callback(self: &RustyPeripheral, error: &mut RustyError);
        fn dongl_secure_connection(self: &RustyPeripheral, mitm: bool, error: &mut RustyError);

        // RustyService functions

        fn initialized(self: &RustyService) -> bool;
        fn uuid(self: &RustyService) -> String;
        fn data(self: &RustyService) -> Vec<u8>;
        fn characteristics(self: &RustyService) -> Vec<RustyCharacteristicWrapper>;

        // RustyCharacteristic functions

        fn initialized(self: &RustyCharacteristic) -> bool;
        fn uuid(self: &RustyCharacteristic) -> String;
        fn descriptors(self: &RustyCharacteristic) -> Vec<RustyDescriptorWrapper>;
        fn capabilities(self: &RustyCharacteristic) -> Vec<String>;

        fn can_read(self: &RustyCharacteristic) -> bool;
        fn can_write_request(self: &RustyCharacteristic) -> bool;
        fn can_write_command(self: &RustyCharacteristic) -> bool;
        fn can_notify(self: &RustyCharacteristic) -> bool;
        fn can_indicate(self: &RustyCharacteristic) -> bool;

        // RustyDescriptor functions

        fn initialized(self: &RustyDescriptor) -> bool;
        fn uuid(self: &RustyDescriptor) -> String;

        // RustyLocalPeripheral functions

        fn link(self: &RustyLocalPeripheral, target: &InnerLocalPeripheral);
        fn unlink(self: &RustyLocalPeripheral);

        fn add_advertised_service(self: &RustyLocalPeripheral, uuid: &str, error: &mut RustyError);
        fn set_advertised_manufacturer_data(
            self: &RustyLocalPeripheral,
            company_id: u16,
            data: &[u8],
            error: &mut RustyError,
        );
        fn set_connectable(self: &RustyLocalPeripheral, connectable: bool, error: &mut RustyError);
        fn add_service(self: &RustyLocalPeripheral, uuid: &str, error: &mut RustyError) -> UniquePtr<RustyLocalService>;
        fn remove_all_services(self: &RustyLocalPeripheral, error: &mut RustyError);

        fn start(self: &RustyLocalPeripheral, error: &mut RustyError);
        fn stop(self: &RustyLocalPeripheral, error: &mut RustyError);
        fn is_started(self: &RustyLocalPeripheral, error: &mut RustyError) -> bool;
        fn is_advertising(self: &RustyLocalPeripheral, error: &mut RustyError) -> bool;

        #[cfg(any(target_os = "macos", doc))]
        fn macos_set_advertisement_local_name(
            self: &RustyLocalPeripheral,
            has_local_name: bool,
            local_name: &str,
            error: &mut RustyError,
        );
        #[cfg(any(target_os = "linux", doc))]
        fn linux_set_advertisement_local_name(
            self: &RustyLocalPeripheral,
            has_local_name: bool,
            local_name: &str,
            error: &mut RustyError,
        );

        // RustyLocalService functions

        fn uuid(self: &RustyLocalService, error: &mut RustyError) -> String;
        fn add_characteristic(
            self: &RustyLocalService,
            uuid: &str,
            capabilities: u8,
            error: &mut RustyError,
        ) -> UniquePtr<RustyLocalCharacteristic>;

        // RustyLocalCharacteristic functions

        fn link(self: &RustyLocalCharacteristic, target: &InnerLocalCharacteristic);
        fn unlink(self: &RustyLocalCharacteristic);

        fn uuid(self: &RustyLocalCharacteristic, error: &mut RustyError) -> String;
        fn capabilities(self: &RustyLocalCharacteristic, error: &mut RustyError) -> u8;
        fn value(self: &RustyLocalCharacteristic, error: &mut RustyError) -> Vec<u8>;
        fn set_value(self: &RustyLocalCharacteristic, value: &[u8], error: &mut RustyError);
        fn set_read_callback_enabled(self: &RustyLocalCharacteristic, enabled: bool, error: &mut RustyError);
    }
}


