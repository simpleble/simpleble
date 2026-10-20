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

pub use types::{Error, BluetoothAddressType, CharacteristicCapability};
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
        fn RustyAdapter_bluetooth_enabled() -> Result<bool>;

        #[namespace = "Bindings"]
        fn RustyAdapter_get_adapters() -> Result<Vec<RustyAdapterWrapper>>;

        #[namespace = "Bindings"]
        fn RustyBackend_get_backends() -> Result<Vec<RustyBackendWrapper>>;

        #[namespace = "Bindings"]
        fn RustyBackend_get_native() -> Result<UniquePtr<RustyBackend>>;

        #[namespace = "Bindings"]
        fn RustyBackend_get_dongl() -> Result<UniquePtr<RustyBackend>>;

        #[namespace = "Bindings"]
        fn RustyBackend_get_simulator() -> Result<UniquePtr<RustyBackend>>;

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
        fn bluetooth_enabled(self: &RustyBackend) -> Result<bool>;
        fn adapters(self: &RustyBackend) -> Result<Vec<RustyAdapterWrapper>>;

        // RustyAdapter functions

        fn link(self: &RustyAdapter, target: Pin<&mut InnerAdapter>) -> Result<()>;
        fn unlink(self: &RustyAdapter) -> Result<()>;
        fn register_scan_callbacks(self: &RustyAdapter) -> Result<()>;
        fn register_power_callbacks(self: &RustyAdapter) -> Result<()>;

        fn initialized(self: &RustyAdapter) -> Result<bool>;
        fn identifier(self: &RustyAdapter) -> Result<String>;
        fn address(self: &RustyAdapter) -> Result<String>;

        fn power_on(self: &RustyAdapter) -> Result<()>;
        fn power_off(self: &RustyAdapter) -> Result<()>;
        fn is_powered(self: &RustyAdapter) -> Result<bool>;

        fn scan_start(self: &RustyAdapter) -> Result<()>;
        fn scan_stop(self: &RustyAdapter) -> Result<()>;
        fn scan_for(self: &RustyAdapter, timeout_ms: i32) -> Result<()>;
        fn scan_is_active(self: &RustyAdapter) -> Result<bool>;
        fn scan_get_results(self: &RustyAdapter) -> Result<Vec<RustyPeripheralWrapper>>;

        fn get_paired_peripherals(self: &RustyAdapter) -> Result<Vec<RustyPeripheralWrapper>>;
        fn get_connected_peripherals(self: &RustyAdapter) -> Result<Vec<RustyPeripheralWrapper>>;

        fn create_local_peripheral(self: &RustyAdapter) -> Result<UniquePtr<RustyLocalPeripheral>>;

        #[cfg(any(target_os = "macos", doc))]
        fn macos_retrieve_cached_peripherals(
            self: &RustyAdapter,
            identifiers: &Vec<String>,
        ) -> Result<Vec<RustyPeripheralWrapper>>;

        // RustyPeripheral functions

        fn link(self: &RustyPeripheral, target: Pin<&mut InnerPeripheral>) -> Result<()>;
        fn unlink(self: &RustyPeripheral) -> Result<()>;
        fn register_connection_callbacks(self: &RustyPeripheral) -> Result<()>;

        fn initialized(self: &RustyPeripheral) -> Result<bool>;
        fn identifier(self: &RustyPeripheral) -> Result<String>;
        fn address(self: &RustyPeripheral) -> Result<String>;
        fn address_type(self: &RustyPeripheral) -> Result<BluetoothAddressType>;
        fn rssi(self: &RustyPeripheral) -> Result<i16>;

        fn tx_power(self: &RustyPeripheral) -> Result<i16>;
        fn mtu(self: &RustyPeripheral) -> Result<u16>;

        fn connect(self: &RustyPeripheral) -> Result<()>;
        fn disconnect(self: &RustyPeripheral) -> Result<()>;
        fn is_connected(self: &RustyPeripheral) -> Result<bool>;
        fn is_connectable(self: &RustyPeripheral) -> Result<bool>;
        fn is_paired(self: &RustyPeripheral) -> Result<bool>;
        fn unpair(self: &RustyPeripheral) -> Result<()>;

        fn services(self: &RustyPeripheral) -> Result<Vec<RustyServiceWrapper>>;
        fn manufacturer_data(self: &RustyPeripheral) -> Result<Vec<RustyManufacturerDataWrapper>>;

        fn read(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
        ) -> Result<Vec<u8>>;
        fn write_request(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
            data: &Vec<u8>,
        ) -> Result<()>;
        fn write_command(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
            data: &Vec<u8>,
        ) -> Result<()>;
        fn notify(self: &RustyPeripheral, service: &String, characteristic: &String) -> Result<()>;
        fn indicate(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
        ) -> Result<()>;
        fn unsubscribe(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
        ) -> Result<()>;

        fn read_descriptor(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
            descriptor: &String,
        ) -> Result<Vec<u8>>;
        fn write_descriptor(
            self: &RustyPeripheral,
            service: &String,
            characteristic: &String,
            descriptor: &String,
            data: &Vec<u8>,
        ) -> Result<()>;

        fn dongl_set_passkey_request_callback(self: &RustyPeripheral, enabled: bool) -> Result<()>;
        fn dongl_set_passkey_display_callback(self: &RustyPeripheral, enabled: bool) -> Result<()>;
        fn dongl_set_numeric_comparison_callback(self: &RustyPeripheral, enabled: bool) -> Result<()>;

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

        fn link(self: &RustyLocalPeripheral, target: &InnerLocalPeripheral) -> Result<()>;
        fn unlink(self: &RustyLocalPeripheral) -> Result<()>;

        fn add_advertised_service(self: &RustyLocalPeripheral, uuid: &str) -> Result<()>;
        fn set_advertised_manufacturer_data(self: &RustyLocalPeripheral, company_id: u16, data: &[u8]) -> Result<()>;
        fn set_connectable(self: &RustyLocalPeripheral, connectable: bool) -> Result<()>;
        fn add_service(self: &RustyLocalPeripheral, uuid: &str) -> Result<UniquePtr<RustyLocalService>>;
        fn remove_all_services(self: &RustyLocalPeripheral) -> Result<()>;

        fn start(self: &RustyLocalPeripheral) -> Result<()>;
        fn stop(self: &RustyLocalPeripheral) -> Result<()>;
        fn is_started(self: &RustyLocalPeripheral) -> Result<bool>;
        fn is_advertising(self: &RustyLocalPeripheral) -> Result<bool>;

        #[cfg(any(target_os = "macos", doc))]
        fn macos_set_advertisement_local_name(
            self: &RustyLocalPeripheral,
            has_local_name: bool,
            local_name: &str,
        ) -> Result<()>;
        #[cfg(any(target_os = "linux", doc))]
        fn linux_set_advertisement_local_name(
            self: &RustyLocalPeripheral,
            has_local_name: bool,
            local_name: &str,
        ) -> Result<()>;

        // RustyLocalService functions

        fn uuid(self: &RustyLocalService) -> Result<String>;
        fn add_characteristic(
            self: &RustyLocalService,
            uuid: &str,
            capabilities: u8,
        ) -> Result<UniquePtr<RustyLocalCharacteristic>>;

        // RustyLocalCharacteristic functions

        fn link(self: &RustyLocalCharacteristic, target: &InnerLocalCharacteristic) -> Result<()>;
        fn unlink(self: &RustyLocalCharacteristic) -> Result<()>;

        fn uuid(self: &RustyLocalCharacteristic) -> Result<String>;
        fn capabilities(self: &RustyLocalCharacteristic) -> Result<u8>;
        fn value(self: &RustyLocalCharacteristic) -> Result<Vec<u8>>;
        fn set_value(self: &RustyLocalCharacteristic, value: &[u8]) -> Result<()>;
        fn set_read_callback_enabled(self: &RustyLocalCharacteristic, enabled: bool) -> Result<()>;
    }
}


