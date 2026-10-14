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

class RustyBackend;
class RustyLocalPeripheral;
class RustyLocalService;
class RustyLocalCharacteristic;

rust::Vec<Bindings::RustyAdapterWrapper> RustyAdapter_get_adapters();
bool RustyAdapter_bluetooth_enabled();

rust::Vec<Bindings::RustyBackendWrapper> RustyBackend_get_backends();
std::unique_ptr<Bindings::RustyBackend> RustyBackend_get_native();
std::unique_ptr<Bindings::RustyBackend> RustyBackend_get_dongl();
std::unique_ptr<Bindings::RustyBackend> RustyBackend_get_simulator();

void RustyConfig_reset_all();

void RustyConfig_simplebluez_reset();
bool RustyConfig_simplebluez_use_system_bus();
void RustyConfig_simplebluez_set_use_system_bus(bool enabled);
uint64_t RustyConfig_simplebluez_connection_timeout_ns();
void RustyConfig_simplebluez_set_connection_timeout_ns(uint64_t timeout_ns);
uint64_t RustyConfig_simplebluez_disconnection_timeout_ns();
void RustyConfig_simplebluez_set_disconnection_timeout_ns(uint64_t timeout_ns);

void RustyConfig_winrt_reset();
bool RustyConfig_winrt_experimental_use_own_mta_apartment();
void RustyConfig_winrt_set_experimental_use_own_mta_apartment(bool enabled);
bool RustyConfig_winrt_experimental_reinitialize_winrt_apartment_on_main_thread();
void RustyConfig_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(bool enabled);
bool RustyConfig_winrt_use_deferred_disconnect();
void RustyConfig_winrt_set_use_deferred_disconnect(bool enabled);

void RustyConfig_corebluetooth_reset();

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
    bool bluetooth_enabled() const;
    rust::Vec<Bindings::RustyAdapterWrapper> adapters() const;

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

    bool initialized() const;
    rust::String identifier() const;
    rust::String address() const;

    void power_on() const;
    void power_off() const;
    bool is_powered() const;

    void scan_start() const;
    void scan_stop() const;
    void scan_for(int32_t timeout_ms) const;
    bool scan_is_active() const;
    rust::Vec<Bindings::RustyPeripheralWrapper> scan_get_results() const;

    rust::Vec<Bindings::RustyPeripheralWrapper> get_paired_peripherals() const;
    rust::Vec<Bindings::RustyPeripheralWrapper> get_connected_peripherals() const;

    std::unique_ptr<Bindings::RustyLocalPeripheral> create_local_peripheral() const;

    rust::Vec<Bindings::RustyPeripheralWrapper> macos_retrieve_cached_peripherals(
        rust::Vec<rust::String> const& identifiers) const;

  private:
    // NOTE: All internal properties need to be handled as pointers,
    // allowing the calls to RustyAdapter to always be const.
    // This might require us to store pointers to pointers, so it's
    // important to be careful when handling these.
    std::unique_ptr<SimpleBLE::Adapter> _internal;
    mutable SimpleRsBLE::InnerAdapter* _adapter = nullptr;
    mutable std::mutex _adapter_mutex;
};

class RustyPeripheral {
  public:
    RustyPeripheral() = default;
    virtual ~RustyPeripheral() { _internal.reset(); }

    RustyPeripheral(SimpleBLE::Peripheral peripheral) : _internal(new SimpleBLE::Peripheral(peripheral)) {}

    void link(SimpleRsBLE::InnerPeripheral& target) const;
    void unlink() const;

    bool initialized() const;
    rust::String identifier() const;
    rust::String address() const;
    SimpleBLE::BluetoothAddressType address_type() const;
    int16_t rssi() const;

    int16_t tx_power() const;
    uint16_t mtu() const;

    void connect() const;
    void disconnect() const;
    bool is_connected() const;
    bool is_connectable() const;
    bool is_paired() const;
    void unpair() const;

    rust::Vec<Bindings::RustyServiceWrapper> services() const;
    rust::Vec<Bindings::RustyManufacturerDataWrapper> manufacturer_data() const;

    rust::Vec<uint8_t> read(rust::String const& service, rust::String const& characteristic) const;
    void write_request(rust::String const& service, rust::String const& characteristic,
                       rust::Vec<uint8_t> const& data) const;
    void write_command(rust::String const& service, rust::String const& characteristic,
                       rust::Vec<uint8_t> const& data) const;
    void notify(rust::String const& service, rust::String const& characteristic) const;
    void indicate(rust::String const& service, rust::String const& characteristic) const;
    void unsubscribe(rust::String const& service, rust::String const& characteristic) const;

    rust::Vec<uint8_t> read_descriptor(rust::String const& service, rust::String const& characteristic,
                                       rust::String const& descriptor) const;
    void write_descriptor(rust::String const& service, rust::String const& characteristic,
                          rust::String const& descriptor, rust::Vec<uint8_t> const& data) const;

    void dongl_set_passkey_request_callback(bool enabled) const;
    void dongl_set_passkey_display_callback(bool enabled) const;
    void dongl_set_numeric_comparison_callback(bool enabled) const;

  private:
    // NOTE: All internal properties need to be handled as pointers,
    // allowing the calls to RustyPeripheral to always be const.
    // This might require us to store pointers to pointers, so it's
    // important to be careful when handling these.
    std::unique_ptr<SimpleBLE::Peripheral> _internal;
    mutable SimpleRsBLE::InnerPeripheral* _peripheral = nullptr;

    // NOTE: Notify and indicate callbacks cannot be cleared in unlink() and can outlive this object,
    // so they reach the InnerPeripheral(Rust) through this shared back-pointer instead.
    struct ValueCallbackTarget {
        std::mutex mutex;
        SimpleRsBLE::InnerPeripheral* peripheral = nullptr;
    };
    std::shared_ptr<ValueCallbackTarget> _value_callback_target = std::make_shared<ValueCallbackTarget>();

    // Pairing callbacks registered by this wrapper, so that unlink() knows which ones to clear.
    mutable bool _dongl_passkey_request = false;
    mutable bool _dongl_passkey_display = false;
    mutable bool _dongl_numeric_comparison = false;
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

    void add_advertised_service(rust::Str uuid) const;
    std::unique_ptr<Bindings::RustyLocalService> add_service(rust::Str uuid) const;
    void remove_all_services() const;

    void start() const;
    void stop() const;
    bool is_started() const;
    bool is_advertising() const;

    void macos_set_advertisement_local_name(bool has_local_name, rust::Str local_name) const;
    void linux_set_advertisement_local_name(bool has_local_name, rust::Str local_name) const;

  private:
    std::unique_ptr<SimpleBLE::Local::Peripheral> _internal;
    mutable SimpleRsBLE::InnerLocalPeripheral const* _peripheral = nullptr;
};

class RustyLocalService {
  public:
    RustyLocalService() = default;
    virtual ~RustyLocalService() = default;

    RustyLocalService(SimpleBLE::Local::Service service) : _internal(new SimpleBLE::Local::Service(std::move(service))) {}

    rust::String uuid() const;
    std::unique_ptr<Bindings::RustyLocalCharacteristic> add_characteristic(rust::Str uuid, uint8_t capabilities) const;

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

    rust::String uuid() const;
    uint8_t capabilities() const;
    rust::Vec<uint8_t> value() const;
    void set_value(rust::Slice<const uint8_t> value) const;
    void set_read_callback_enabled(bool enabled) const;

  private:
    std::unique_ptr<SimpleBLE::Local::Characteristic> _internal;
    mutable SimpleRsBLE::InnerLocalCharacteristic const* _characteristic = nullptr;
};

};  // namespace Bindings
