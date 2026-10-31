#pragma once

#include "rust/cxx.h"
#include "simpleble/Adapter.h"
#include "simpleble/Backend.h"
#include "simpleble/Peripheral.h"
#include "simpleble/local/Characteristic.h"
#include "simpleble/local/Peripheral.h"
#include "simpleble/local/Service.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace SimpleRsBLE {

struct InnerAdapter;
struct InnerPeripheral;
struct InnerLocalPeripheral;
struct InnerLocalCharacteristic;

};  // namespace SimpleRsBLE

namespace Bindings {

struct RustyAdapterWrapper;
struct RustyPeripheralWrapper;
struct RustyServiceWrapper;
struct RustyCharacteristicWrapper;
struct RustyDescriptorWrapper;
struct RustyManufacturerDataWrapper;
struct RustyBackendWrapper;
struct RustyError;

class RustyBackend;
class RustyLocalPeripheral;
class RustyLocalService;
class RustyLocalCharacteristic;

rust::Vec<Bindings::RustyAdapterWrapper> RustyAdapter_get_adapters(RustyError& error);
bool RustyAdapter_bluetooth_enabled(RustyError& error);

rust::Vec<Bindings::RustyBackendWrapper> RustyBackend_get_backends(RustyError& error);
std::unique_ptr<Bindings::RustyBackend> RustyBackend_get_native(RustyError& error);
std::unique_ptr<Bindings::RustyBackend> RustyBackend_get_dongl(RustyError& error);
std::unique_ptr<Bindings::RustyBackend> RustyBackend_get_simulator(RustyError& error);

void RustyConfig_reset_all();

void RustyConfig_simplebluez_reset();
bool RustyConfig_simplebluez_use_system_bus();
void RustyConfig_simplebluez_set_use_system_bus(bool enabled);
uint64_t RustyConfig_simplebluez_connection_timeout_ns();
void RustyConfig_simplebluez_set_connection_timeout_ns(uint64_t timeout_ns);
uint64_t RustyConfig_simplebluez_disconnection_timeout_ns();
void RustyConfig_simplebluez_set_disconnection_timeout_ns(uint64_t timeout_ns);
bool RustyConfig_simplebluez_interactive_pairing();
void RustyConfig_simplebluez_set_interactive_pairing(bool enabled);

void RustyConfig_winrt_reset();
bool RustyConfig_winrt_experimental_use_own_mta_apartment();
void RustyConfig_winrt_set_experimental_use_own_mta_apartment(bool enabled);
bool RustyConfig_winrt_experimental_reinitialize_winrt_apartment_on_main_thread();
void RustyConfig_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(bool enabled);
bool RustyConfig_winrt_use_deferred_disconnect();
void RustyConfig_winrt_set_use_deferred_disconnect(bool enabled);

void RustyConfig_corebluetooth_reset();

void RustyConfig_android_reset();
bool RustyConfig_android_negotiate_mtu();
void RustyConfig_android_set_negotiate_mtu(bool enabled);

void RustyConfig_dongl_reset();
bool RustyConfig_dongl_use_dongl_backend();
void RustyConfig_dongl_set_use_dongl_backend(bool enabled);
bool RustyConfig_dongl_auto_update();
void RustyConfig_dongl_set_auto_update(bool enabled);
bool RustyConfig_dongl_force_update();
void RustyConfig_dongl_set_force_update(bool enabled);

class RustyBackend {
  public:
    RustyBackend() = default;
    virtual ~RustyBackend() = default;

    RustyBackend(SimpleBLE::Backend backend) : _internal(new SimpleBLE::Backend(std::move(backend))) {}

    rust::String identifier() const;
    bool bluetooth_enabled(RustyError& error) const;
    rust::Vec<Bindings::RustyAdapterWrapper> adapters(RustyError& error) const;

  private:
    std::unique_ptr<SimpleBLE::Backend> _internal;
};

class RustyAdapter {
  public:
    RustyAdapter() = default;
    virtual ~RustyAdapter() { _internal.reset(); }

    RustyAdapter(SimpleBLE::Adapter adapter) : _internal(new SimpleBLE::Adapter(std::move(adapter))) {};

    void link(SimpleRsBLE::InnerAdapter& target) const;
    void unlink() const;
    void register_scan_callbacks() const;
    void register_power_callbacks() const;

    bool initialized(RustyError& error) const;
    rust::String identifier(RustyError& error) const;
    rust::String address(RustyError& error) const;

    void power_on(RustyError& error) const;
    void power_off(RustyError& error) const;
    bool is_powered(RustyError& error) const;

    void scan_start(RustyError& error) const;
    void scan_stop(RustyError& error) const;
    void scan_for(int32_t timeout_ms, RustyError& error) const;
    bool scan_is_active(RustyError& error) const;
    rust::Vec<Bindings::RustyPeripheralWrapper> scan_get_results(RustyError& error) const;

    rust::Vec<Bindings::RustyPeripheralWrapper> get_paired_peripherals(RustyError& error) const;
    rust::Vec<Bindings::RustyPeripheralWrapper> get_connected_peripherals(RustyError& error) const;

    std::unique_ptr<Bindings::RustyLocalPeripheral> create_local_peripheral(RustyError& error) const;

    rust::Vec<Bindings::RustyPeripheralWrapper> macos_retrieve_cached_peripherals(
        rust::Vec<rust::String> const& identifiers, RustyError& error) const;

    void linux_set_passkey_request_callback(bool enabled, RustyError& error) const;
    void linux_set_passkey_display_callback(bool enabled, RustyError& error) const;
    void linux_set_numeric_comparison_callback(bool enabled, RustyError& error) const;
    void linux_set_pairing_authorization_callback(bool enabled, RustyError& error) const;

  private:
    // NOTE: All internal properties need to be handled as pointers,
    // allowing the calls to RustyAdapter to always be const.
    // This might require us to store pointers to pointers, so it's
    // important to be careful when handling these.
    std::unique_ptr<SimpleBLE::Adapter> _internal;

    // NOTE: Callbacks belong to the adapter and can outlive this object,
    // so they reach the InnerAdapter(Rust) through this shared back-pointer.
    struct CallbackTarget {
        std::shared_mutex mutex;
        SimpleRsBLE::InnerAdapter const* adapter = nullptr;
    };
    std::shared_ptr<CallbackTarget> _callback_target = std::make_shared<CallbackTarget>();
};

class RustyPeripheral {
  public:
    RustyPeripheral() = default;
    virtual ~RustyPeripheral() { _internal.reset(); }

    RustyPeripheral(SimpleBLE::Peripheral peripheral) : _internal(new SimpleBLE::Peripheral(peripheral)) {}

    void link(SimpleRsBLE::InnerPeripheral& target) const;
    void unlink() const;
    void register_connection_callbacks() const;

    bool initialized(RustyError& error) const;
    rust::String identifier(RustyError& error) const;
    rust::String address(RustyError& error) const;
    SimpleBLE::BluetoothAddressType address_type(RustyError& error) const;
    int16_t rssi(RustyError& error) const;

    int16_t tx_power(RustyError& error) const;
    uint16_t mtu(RustyError& error) const;

    void connect(RustyError& error) const;
    void disconnect(RustyError& error) const;
    bool is_connected(RustyError& error) const;
    bool is_connectable(RustyError& error) const;
    bool is_paired(RustyError& error) const;
    void unpair(RustyError& error) const;

    rust::Vec<Bindings::RustyServiceWrapper> services(RustyError& error) const;
    rust::Vec<Bindings::RustyManufacturerDataWrapper> manufacturer_data(RustyError& error) const;

    rust::Vec<uint8_t> read(rust::String const& service, rust::String const& characteristic, RustyError& error) const;
    void write_request(rust::String const& service, rust::String const& characteristic,
                       rust::Vec<uint8_t> const& data, RustyError& error) const;
    void write_command(rust::String const& service, rust::String const& characteristic,
                       rust::Vec<uint8_t> const& data, RustyError& error) const;
    void notify(rust::String const& service, rust::String const& characteristic, RustyError& error) const;
    void indicate(rust::String const& service, rust::String const& characteristic, RustyError& error) const;
    void unsubscribe(rust::String const& service, rust::String const& characteristic, RustyError& error) const;

    rust::Vec<uint8_t> read_descriptor(rust::String const& service, rust::String const& characteristic,
                                       rust::String const& descriptor, RustyError& error) const;
    void write_descriptor(rust::String const& service, rust::String const& characteristic,
                          rust::String const& descriptor, rust::Vec<uint8_t> const& data, RustyError& error) const;

    void dongl_set_passkey_request_callback(RustyError& error) const;
    void dongl_set_passkey_display_callback(RustyError& error) const;
    void dongl_set_numeric_comparison_callback(RustyError& error) const;
    void dongl_secure_connection(bool mitm, RustyError& error) const;

  private:
    // NOTE: All internal properties need to be handled as pointers,
    // allowing the calls to RustyPeripheral to always be const.
    // This might require us to store pointers to pointers, so it's
    // important to be careful when handling these.
    std::unique_ptr<SimpleBLE::Peripheral> _internal;

    // NOTE: Callbacks belong to the peripheral and can outlive this object,
    // so they reach the InnerPeripheral(Rust) through this shared back-pointer.
    struct CallbackTarget {
        std::shared_mutex mutex;
        SimpleRsBLE::InnerPeripheral const* peripheral = nullptr;
    };
    std::shared_ptr<CallbackTarget> _callback_target = std::make_shared<CallbackTarget>();
};

class RustyService {
  public:
    RustyService() = default;
    virtual ~RustyService() = default;

    RustyService(SimpleBLE::Service service) : _internal(new SimpleBLE::Service(std::move(service))) {}

    bool initialized() const { return _internal->initialized(); }
    rust::String uuid() const { return rust::String(_internal->uuid()); }

    rust::Vec<uint8_t> data() const;

    rust::Vec<Bindings::RustyCharacteristicWrapper> characteristics() const;

  private:
    // NOTE: All internal properties need to be handled as pointers,
    // allowing the calls to RustyService to always be const.
    // This might require us to store pointers to pointers, so it's
    // important to be careful when handling these.
    std::shared_ptr<SimpleBLE::Service> _internal;
};

class RustyCharacteristic {
  public:
    RustyCharacteristic() = default;
    virtual ~RustyCharacteristic() = default;

    RustyCharacteristic(SimpleBLE::Characteristic characteristic)
        : _internal(new SimpleBLE::Characteristic(std::move(characteristic))) {}

    bool initialized() const { return _internal->initialized(); }
    rust::String uuid() const { return rust::String(_internal->uuid()); }

    rust::Vec<Bindings::RustyDescriptorWrapper> descriptors() const;
    rust::Vec<rust::String> capabilities() const;

    bool can_read() const { return _internal->can_read(); }
    bool can_write_request() const { return _internal->can_write_request(); }
    bool can_write_command() const { return _internal->can_write_command(); }
    bool can_notify() const { return _internal->can_notify(); }
    bool can_indicate() const { return _internal->can_indicate(); }

  private:
    // NOTE: All internal properties need to be handled as pointers,
    // allowing the calls to RustyCharacteristic to always be const.
    // This might require us to store pointers to pointers, so it's
    // important to be careful when handling these.
    std::shared_ptr<SimpleBLE::Characteristic> _internal;
};

class RustyDescriptor {
  public:
    RustyDescriptor() = default;
    virtual ~RustyDescriptor() = default;

    RustyDescriptor(SimpleBLE::Descriptor descriptor) : _internal(new SimpleBLE::Descriptor(std::move(descriptor))) {}

    bool initialized() const { return _internal->initialized(); }
    rust::String uuid() const { return rust::String(_internal->uuid()); }

  private:
    // NOTE: All internal properties need to be handled as pointers,
    // allowing the calls to RustyDescriptor to always be const.
    // This might require us to store pointers to pointers, so it's
    // important to be careful when handling these.
    std::shared_ptr<SimpleBLE::Descriptor> _internal;
};

class RustyLocalPeripheral {
  public:
    RustyLocalPeripheral() = default;
    virtual ~RustyLocalPeripheral() { _internal.reset(); }

    RustyLocalPeripheral(SimpleBLE::Local::Peripheral peripheral)
        : _internal(new SimpleBLE::Local::Peripheral(std::move(peripheral))) {}

    void link(SimpleRsBLE::InnerLocalPeripheral const& target) const;
    void unlink() const;

    void add_advertised_service(rust::Str uuid, RustyError& error) const;
    void set_advertised_manufacturer_data(uint16_t company_id, rust::Slice<const uint8_t> data,
    RustyError& error) const;
    void set_connectable(bool connectable, RustyError& error) const;
    std::unique_ptr<Bindings::RustyLocalService> add_service(rust::Str uuid, RustyError& error) const;
    void remove_all_services(RustyError& error) const;

    void start(RustyError& error) const;
    void stop(RustyError& error) const;
    bool is_started(RustyError& error) const;
    bool is_advertising(RustyError& error) const;

    void macos_set_advertisement_local_name(bool has_local_name, rust::Str local_name, RustyError& error) const;
    void linux_set_advertisement_local_name(bool has_local_name, rust::Str local_name, RustyError& error) const;

  private:
    std::unique_ptr<SimpleBLE::Local::Peripheral> _internal;
    mutable SimpleRsBLE::InnerLocalPeripheral const* _peripheral = nullptr;
};

class RustyLocalService {
  public:
    RustyLocalService() = default;
    virtual ~RustyLocalService() = default;

    RustyLocalService(SimpleBLE::Local::Service service) : _internal(new SimpleBLE::Local::Service(std::move(service))) {}

    rust::String uuid(RustyError& error) const;
    std::unique_ptr<Bindings::RustyLocalCharacteristic> add_characteristic(rust::Str uuid, uint8_t capabilities,
    RustyError& error) const;

  private:
    std::unique_ptr<SimpleBLE::Local::Service> _internal;
};

class RustyLocalCharacteristic {
  public:
    RustyLocalCharacteristic() = default;
    virtual ~RustyLocalCharacteristic() { _internal.reset(); }

    RustyLocalCharacteristic(SimpleBLE::Local::Characteristic characteristic)
        : _internal(new SimpleBLE::Local::Characteristic(std::move(characteristic))) {}

    void link(SimpleRsBLE::InnerLocalCharacteristic const& target) const;
    void unlink() const;

    rust::String uuid(RustyError& error) const;
    uint8_t capabilities(RustyError& error) const;
    rust::Vec<uint8_t> value(RustyError& error) const;
    void set_value(rust::Slice<const uint8_t> value, RustyError& error) const;
    void set_read_callback_enabled(bool enabled, RustyError& error) const;

  private:
    std::unique_ptr<SimpleBLE::Local::Characteristic> _internal;
    mutable SimpleRsBLE::InnerLocalCharacteristic const* _characteristic = nullptr;
};

};  // namespace Bindings
