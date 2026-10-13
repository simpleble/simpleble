#include "Bindings.hpp"
#include <chrono>
#include <exception>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "simpleble/Advanced.h"
#include "simpleble/Config.h"
#include "simpleble/Exceptions.h"

#include "simplersble/simplersble/src/lib.rs.h"

namespace {

uint64_t duration_to_ns(std::chrono::steady_clock::duration duration) {
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    return ns < 0 ? 0 : static_cast<uint64_t>(ns);
}

std::chrono::steady_clock::duration duration_from_ns(uint64_t ns) {
    // The Rust side never passes more than INT64_MAX nanoseconds.
    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::nanoseconds(static_cast<int64_t>(ns)));
}

using LocalCapability = SimpleBLE::Local::CharacteristicCapability;
using Bindings::RustyCharacteristicCapability;

const std::pair<RustyCharacteristicCapability, LocalCapability> CAPABILITIES[] = {
    {RustyCharacteristicCapability::READ, LocalCapability::READ},
    {RustyCharacteristicCapability::WRITE_REQUEST, LocalCapability::WRITE_REQUEST},
    {RustyCharacteristicCapability::WRITE_COMMAND, LocalCapability::WRITE_COMMAND},
    {RustyCharacteristicCapability::NOTIFY, LocalCapability::NOTIFY},
    {RustyCharacteristicCapability::INDICATE, LocalCapability::INDICATE},
};

}  // namespace

rust::Vec<Bindings::RustyAdapterWrapper> Bindings::RustyAdapter_get_adapters() {
    rust::Vec<Bindings::RustyAdapterWrapper> result;

    for (auto& adapter : SimpleBLE::Adapter::get_adapters()) {
        Bindings::RustyAdapterWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyAdapter>(adapter);
        result.push_back(std::move(wrapper));
    }

    return result;
}

bool Bindings::RustyAdapter_bluetooth_enabled() { return SimpleBLE::Adapter::bluetooth_enabled(); }

rust::Vec<Bindings::RustyBackendWrapper> Bindings::RustyBackend_get_backends() {
    rust::Vec<Bindings::RustyBackendWrapper> result;

    for (auto& backend : SimpleBLE::Backend::get_backends()) {
        Bindings::RustyBackendWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyBackend>(backend);
        result.push_back(std::move(wrapper));
    }

    return result;
}

// Config Bindings

void Bindings::RustyConfig_reset_all() { SimpleBLE::Config::Base::reset_all(); }

void Bindings::RustyConfig_simplebluez_reset() { SimpleBLE::Config::SimpleBluez::reset(); }

bool Bindings::RustyConfig_simplebluez_use_system_bus() { return SimpleBLE::Config::SimpleBluez::use_system_bus; }

void Bindings::RustyConfig_simplebluez_set_use_system_bus(bool enabled) {
    SimpleBLE::Config::SimpleBluez::use_system_bus = enabled;
}

uint64_t Bindings::RustyConfig_simplebluez_connection_timeout_ns() {
    return duration_to_ns(SimpleBLE::Config::SimpleBluez::connection_timeout);
}

void Bindings::RustyConfig_simplebluez_set_connection_timeout_ns(uint64_t timeout_ns) {
    SimpleBLE::Config::SimpleBluez::connection_timeout = duration_from_ns(timeout_ns);
}

uint64_t Bindings::RustyConfig_simplebluez_disconnection_timeout_ns() {
    return duration_to_ns(SimpleBLE::Config::SimpleBluez::disconnection_timeout);
}

void Bindings::RustyConfig_simplebluez_set_disconnection_timeout_ns(uint64_t timeout_ns) {
    SimpleBLE::Config::SimpleBluez::disconnection_timeout = duration_from_ns(timeout_ns);
}

void Bindings::RustyConfig_winrt_reset() { SimpleBLE::Config::WinRT::reset(); }

bool Bindings::RustyConfig_winrt_experimental_use_own_mta_apartment() {
    return SimpleBLE::Config::WinRT::experimental_use_own_mta_apartment;
}

void Bindings::RustyConfig_winrt_set_experimental_use_own_mta_apartment(bool enabled) {
    SimpleBLE::Config::WinRT::experimental_use_own_mta_apartment = enabled;
}

bool Bindings::RustyConfig_winrt_experimental_reinitialize_winrt_apartment_on_main_thread() {
    return SimpleBLE::Config::WinRT::experimental_reinitialize_winrt_apartment_on_main_thread;
}

void Bindings::RustyConfig_winrt_set_experimental_reinitialize_winrt_apartment_on_main_thread(bool enabled) {
    SimpleBLE::Config::WinRT::experimental_reinitialize_winrt_apartment_on_main_thread = enabled;
}

bool Bindings::RustyConfig_winrt_use_deferred_disconnect() { return SimpleBLE::Config::WinRT::use_deferred_disconnect; }

void Bindings::RustyConfig_winrt_set_use_deferred_disconnect(bool enabled) {
    SimpleBLE::Config::WinRT::use_deferred_disconnect = enabled;
}

void Bindings::RustyConfig_corebluetooth_reset() { SimpleBLE::Config::CoreBluetooth::reset(); }

void Bindings::RustyConfig_dongl_reset() { SimpleBLE::Config::Dongl::reset(); }

bool Bindings::RustyConfig_dongl_use_dongl_backend() { return SimpleBLE::Config::Dongl::use_dongl_backend; }

void Bindings::RustyConfig_dongl_set_use_dongl_backend(bool enabled) {
    SimpleBLE::Config::Dongl::use_dongl_backend = enabled;
}

bool Bindings::RustyConfig_dongl_auto_update() { return SimpleBLE::Config::Dongl::auto_update; }

void Bindings::RustyConfig_dongl_set_auto_update(bool enabled) { SimpleBLE::Config::Dongl::auto_update = enabled; }

bool Bindings::RustyConfig_dongl_force_update() { return SimpleBLE::Config::Dongl::force_update; }

void Bindings::RustyConfig_dongl_set_force_update(bool enabled) { SimpleBLE::Config::Dongl::force_update = enabled; }

// Backend Bindings

rust::String Bindings::RustyBackend::identifier() const { return rust::String(_internal->identifier()); }

bool Bindings::RustyBackend::bluetooth_enabled() const { return _internal->bluetooth_enabled(); }

rust::Vec<Bindings::RustyAdapterWrapper> Bindings::RustyBackend::adapters() const {
    rust::Vec<Bindings::RustyAdapterWrapper> result;

    for (auto& adapter : _internal->adapters()) {
        Bindings::RustyAdapterWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyAdapter>(adapter);
        result.push_back(std::move(wrapper));
    }

    return result;
}

// InnerAdapter Bindings

void Bindings::RustyAdapter::link(SimpleRsBLE::InnerAdapter& target) const {
    // Time to explain the weird shenanigan we're doing here:
    // The TL;DR is that we're making the InnerAdapter(Rust) and the RustyAdapter(C++)
    // point to each other in a safe way.
    // To achieve this, the InnerAdapter(Rust) owns a RustyAdapter(C++) via a UniquePtr,
    // which ensures that calls will always be made to a valid C++ object.
    // We now give the RustyAdapter(C++) a pointer back to the InnerAdapter(Rust),
    // so that callbacks can be forwarded back to the Rust domain.
    // In order to ensure that the InnerAdapter(Rust) is always valid (given
    // that Rust is keen on moving stuff around) the object is created as a
    // Pin<Box<T>>

    // `_adapter` is a pointer to a pointer, allowing us to manipulate the contents within const functions.
    std::lock_guard<std::mutex> lock(_adapter_mutex);
    _adapter = &target;  // THIS LINE IS SUPER IMPORTANT

    _internal->set_callback_on_scan_start([this]() {
        if (this->_adapter == nullptr) return;

        this->_adapter->on_callback_scan_start();
    });

    _internal->set_callback_on_scan_stop([this]() {
        if (this->_adapter == nullptr) return;

        this->_adapter->on_callback_scan_stop();
    });

    _internal->set_callback_on_scan_found([this](SimpleBLE::Peripheral peripheral) {
        if (this->_adapter == nullptr) return;

        Bindings::RustyPeripheralWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyPeripheral>(peripheral);
        this->_adapter->on_callback_scan_found(wrapper);
    });

    _internal->set_callback_on_scan_updated([this](SimpleBLE::Peripheral peripheral) {
        if (this->_adapter == nullptr) return;

        Bindings::RustyPeripheralWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyPeripheral>(peripheral);
        this->_adapter->on_callback_scan_updated(wrapper);
    });

    _internal->set_callback_on_power_on([this]() {
        if (this->_adapter == nullptr) return;

        this->_adapter->on_callback_power_on();
    });

    _internal->set_callback_on_power_off([this]() {
        if (this->_adapter == nullptr) return;

        this->_adapter->on_callback_power_off();
    });
}

void Bindings::RustyAdapter::unlink() const {
    // Clear all callbacks
    _internal->set_callback_on_scan_start(nullptr);
    _internal->set_callback_on_scan_stop(nullptr);
    _internal->set_callback_on_scan_found(nullptr);
    _internal->set_callback_on_scan_updated(nullptr);
    _internal->set_callback_on_power_on(nullptr);
    _internal->set_callback_on_power_off(nullptr);

    // `_adapter` is a pointer to a pointer.
    std::lock_guard<std::mutex> lock(_adapter_mutex);
    _adapter = nullptr;
}

bool Bindings::RustyAdapter::initialized() const { return _internal->initialized(); }

rust::String Bindings::RustyAdapter::identifier() const { return rust::String(_internal->identifier()); }

rust::String Bindings::RustyAdapter::address() const { return rust::String(_internal->address()); }

void Bindings::RustyAdapter::power_on() const { _internal->power_on(); }

void Bindings::RustyAdapter::power_off() const { _internal->power_off(); }

bool Bindings::RustyAdapter::is_powered() const { return _internal->is_powered(); }

void Bindings::RustyAdapter::scan_start() const { _internal->scan_start(); }

void Bindings::RustyAdapter::scan_stop() const { _internal->scan_stop(); }

void Bindings::RustyAdapter::scan_for(int32_t timeout_ms) const { _internal->scan_for(timeout_ms); }

bool Bindings::RustyAdapter::scan_is_active() const { return _internal->scan_is_active(); }

rust::Vec<Bindings::RustyPeripheralWrapper> Bindings::RustyAdapter::scan_get_results() const {
    rust::Vec<Bindings::RustyPeripheralWrapper> result;

    for (auto& peripheral : _internal->scan_get_results()) {
        Bindings::RustyPeripheralWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyPeripheral>(peripheral);
        result.push_back(std::move(wrapper));
    }

    return result;
}

rust::Vec<Bindings::RustyPeripheralWrapper> Bindings::RustyAdapter::get_paired_peripherals() const {
    rust::Vec<Bindings::RustyPeripheralWrapper> result;

    for (auto& peripheral : _internal->get_paired_peripherals()) {
        Bindings::RustyPeripheralWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyPeripheral>(peripheral);
        result.push_back(std::move(wrapper));
    }

    return result;
}

rust::Vec<Bindings::RustyPeripheralWrapper> Bindings::RustyAdapter::get_connected_peripherals() const {
    rust::Vec<Bindings::RustyPeripheralWrapper> result;

    for (auto& peripheral : _internal->get_connected_peripherals()) {
        Bindings::RustyPeripheralWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyPeripheral>(peripheral);
        result.push_back(std::move(wrapper));
    }

    return result;
}

std::unique_ptr<Bindings::RustyLocalPeripheral> Bindings::RustyAdapter::create_local_peripheral() const {
    return std::make_unique<Bindings::RustyLocalPeripheral>(_internal->create_local_peripheral());
}

// InnerPeripheral Bindings

void Bindings::RustyPeripheral::link(SimpleRsBLE::InnerPeripheral& target) const {
    // Time to explain the weird shenanigan we're doing here:
    // The TL;DR is that we're making the InnerPeripheral(Rust) and the RustyPeripheral(C++)
    // point to each other in a safe way.
    // To achieve this, the InnerPeripheral(Rust) owns a RustyPeripheral(C++) via a UniquePtr,
    // which ensures that calls will always be made to a valid C++ object.
    // We now give the RustyPeripheral(C++) a pointer back to the InnerPeripheral(Rust),
    // so that callbacks can be forwarded back to the Rust domain.
    // In order to ensure that the InnerPeripheral(Rust) is always valid (given
    // that Rust is keen on moving stuff around) the object is created as a
    // Pin<Box<T>>

    // `_peripheral` is a pointer to a pointer, allowing us to manipulate the contents within const functions.
    _peripheral = &target;  // THIS LINE IS SUPER IMPORTANT

    {
        std::lock_guard<std::mutex> lock(_value_callback_target->mutex);
        _value_callback_target->peripheral = &target;
    }

    _internal->set_callback_on_connected([this]() {
        if (this->_peripheral == nullptr) return;

        this->_peripheral->on_callback_connected();
    });

    _internal->set_callback_on_disconnected([this]() {
        if (this->_peripheral == nullptr) return;

        this->_peripheral->on_callback_disconnected();
    });
}

void Bindings::RustyPeripheral::unlink() const {
    // Clear all callbacks
    _internal->set_callback_on_connected(nullptr);
    _internal->set_callback_on_disconnected(nullptr);

    // Pairing callbacks only exist on Dongl peripherals, so we only clear the ones registered here.
    if (_dongl_passkey_request) {
        dongl_set_passkey_request_callback(false);
    }
    if (_dongl_passkey_display) {
        dongl_set_passkey_display_callback(false);
    }
    if (_dongl_numeric_comparison) {
        dongl_set_numeric_comparison_callback(false);
    }

    // `_peripheral` is a pointer to a pointer.
    _peripheral = nullptr;

    // NOTE: Notify and indicate callbacks stay registered, so clearing their back-pointer stops them for good.
    std::lock_guard<std::mutex> lock(_value_callback_target->mutex);
    _value_callback_target->peripheral = nullptr;
}

bool Bindings::RustyPeripheral::initialized() const { return _internal->initialized(); }

rust::String Bindings::RustyPeripheral::identifier() const { return rust::String(_internal->identifier()); }

rust::String Bindings::RustyPeripheral::address() const { return rust::String(_internal->address()); }


SimpleBLE::BluetoothAddressType Bindings::RustyPeripheral::address_type() const { return _internal->address_type(); }

int16_t Bindings::RustyPeripheral::rssi() const { return _internal->rssi(); }

int16_t Bindings::RustyPeripheral::tx_power() const { return _internal->tx_power(); }

uint16_t Bindings::RustyPeripheral::mtu() const { return _internal->mtu(); }

void Bindings::RustyPeripheral::connect() const { _internal->connect(); }

void Bindings::RustyPeripheral::disconnect() const { _internal->disconnect(); }

bool Bindings::RustyPeripheral::is_connected() const { return _internal->is_connected(); }

bool Bindings::RustyPeripheral::is_connectable() const { return _internal->is_connectable(); }

bool Bindings::RustyPeripheral::is_paired() const { return _internal->is_paired(); }

void Bindings::RustyPeripheral::unpair() const { _internal->unpair(); }

rust::Vec<Bindings::RustyServiceWrapper> Bindings::RustyPeripheral::services() const {
    rust::Vec<Bindings::RustyServiceWrapper> result;

    for (auto& service : _internal->services()) {
        Bindings::RustyServiceWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyService>(service);
        result.push_back(std::move(wrapper));
    }

    return result;
}

rust::Vec<Bindings::RustyManufacturerDataWrapper> Bindings::RustyPeripheral::manufacturer_data() const {
    rust::Vec<Bindings::RustyManufacturerDataWrapper> result;

    for (auto& manufacturer_data : _internal->manufacturer_data()) {
        Bindings::RustyManufacturerDataWrapper wrapper;
        wrapper.company_id = manufacturer_data.first;

        for (auto& byte : manufacturer_data.second) {
            wrapper.data.push_back(byte);
        }

        result.push_back(std::move(wrapper));
    }

    return result;
}

rust::Vec<uint8_t> Bindings::RustyPeripheral::read(rust::String const& service,
                                                   rust::String const& characteristic) const {
    std::string read_result = _internal->read(std::string(service), std::string(characteristic));

    rust::Vec<uint8_t> result;
    for (auto& byte : read_result) {
        result.push_back(byte);
    }

    return result;
}

void Bindings::RustyPeripheral::write_request(rust::String const& service_rs, rust::String const& characteristic_rs,
                                              rust::Vec<uint8_t> const& data) const {
    std::string service(service_rs);
    std::string characteristic(characteristic_rs);
    std::string data_vec((char*)data.data(), data.size());

    _internal->write_request(service, characteristic, data_vec);
}

void Bindings::RustyPeripheral::write_command(rust::String const& service_rs, rust::String const& characteristic_rs,
                                              rust::Vec<uint8_t> const& data) const {
    std::string service(service_rs);
    std::string characteristic(characteristic_rs);
    std::string data_vec((char*)data.data(), data.size());

    _internal->write_command(service, characteristic, data_vec);
}

void Bindings::RustyPeripheral::notify(rust::String const& service_rs, rust::String const& characteristic_rs) const {
    std::string service(service_rs);
    std::string characteristic(characteristic_rs);

    auto target = _value_callback_target;
    _internal->notify(service, characteristic, [target, service_rs, characteristic_rs](std::string data) {
        // NOTE: In this two cases we need to lock the mutex to ensure thread safety, as
        // the callback cannot be cleared in the unlink() function. The callback keeps its
        // own reference to the target, as it can outlive this object.
        std::lock_guard<std::mutex> lock(target->mutex);
        if (target->peripheral == nullptr) return;

        rust::Vec<uint8_t> data_vec;
        for (auto& byte : data) {
            data_vec.push_back(byte);
        }

        target->peripheral->on_callback_characteristic_updated(service_rs, characteristic_rs, data_vec);
    });
}

void Bindings::RustyPeripheral::indicate(rust::String const& service_rs, rust::String const& characteristic_rs) const {
    std::string service(service_rs);
    std::string characteristic(characteristic_rs);

    auto target = _value_callback_target;
    _internal->indicate(service, characteristic, [target, service_rs, characteristic_rs](std::string data) {
        // NOTE: In this two cases we need to lock the mutex to ensure thread safety, as
        // the callback cannot be cleared in the unlink() function. The callback keeps its
        // own reference to the target, as it can outlive this object.
        std::lock_guard<std::mutex> lock(target->mutex);
        if (target->peripheral == nullptr) return;

        rust::Vec<uint8_t> data_vec;
        for (auto& byte : data) {
            data_vec.push_back(byte);
        }

        target->peripheral->on_callback_characteristic_updated(service_rs, characteristic_rs, data_vec);
    });
}

void Bindings::RustyPeripheral::unsubscribe(rust::String const& service_rs,
                                            rust::String const& characteristic_rs) const {
    std::string service(service_rs);
    std::string characteristic(characteristic_rs);

    _internal->unsubscribe(service, characteristic);
}

rust::Vec<uint8_t> Bindings::RustyPeripheral::read_descriptor(rust::String const& service,
                                                              rust::String const& characteristic,
                                                              rust::String const& descriptor) const {
    std::string read_result = _internal->read(std::string(service), std::string(characteristic),
                                              std::string(descriptor));

    rust::Vec<uint8_t> result;
    for (auto& byte : read_result) {
        result.push_back(byte);
    }

    return result;
}

void Bindings::RustyPeripheral::write_descriptor(rust::String const& service, rust::String const& characteristic,
                                                 rust::String const& descriptor, rust::Vec<uint8_t> const& data) const {
    _internal->write(std::string(service), std::string(characteristic), std::string(descriptor),
                     std::string((char*)data.data(), data.size()));
}

// NOTE: The pairing callbacks run on a Dongl worker thread, possibly while connect() is still
// in progress. Clearing a callback waits for it to finish, which is why unlink() can safely
// release the back-pointer afterwards.

void Bindings::RustyPeripheral::dongl_set_passkey_request_callback(bool enabled) const {
    if (!enabled) {
        SimpleBLE::Advanced::Dongl::set_passkey_request_callback(*_internal, nullptr);
        _dongl_passkey_request = false;
        return;
    }

    SimpleBLE::Advanced::Dongl::set_passkey_request_callback(*_internal, [this]() -> std::optional<std::string> {
        if (this->_peripheral == nullptr) return std::nullopt;

        rust::String passkey;
        if (!this->_peripheral->on_callback_passkey_request(passkey)) return std::nullopt;

        return std::string(passkey);
    });
    _dongl_passkey_request = true;
}

void Bindings::RustyPeripheral::dongl_set_passkey_display_callback(bool enabled) const {
    if (!enabled) {
        SimpleBLE::Advanced::Dongl::set_passkey_display_callback(*_internal, nullptr);
        _dongl_passkey_display = false;
        return;
    }

    SimpleBLE::Advanced::Dongl::set_passkey_display_callback(*_internal, [this](const std::string& passkey) {
        if (this->_peripheral == nullptr) return;

        this->_peripheral->on_callback_passkey_display(
            rust::Slice<const uint8_t>(reinterpret_cast<const uint8_t*>(passkey.data()), passkey.size()));
    });
    _dongl_passkey_display = true;
}

void Bindings::RustyPeripheral::dongl_set_numeric_comparison_callback(bool enabled) const {
    if (!enabled) {
        SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(*_internal, nullptr);
        _dongl_numeric_comparison = false;
        return;
    }

    SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(*_internal, [this](const std::string& passkey) {
        if (this->_peripheral == nullptr) return false;

        return this->_peripheral->on_callback_numeric_comparison(
            rust::Slice<const uint8_t>(reinterpret_cast<const uint8_t*>(passkey.data()), passkey.size()));
    });
    _dongl_numeric_comparison = true;
}

// Service Bindings

rust::Vec<uint8_t> Bindings::RustyService::data() const {
    rust::Vec<uint8_t> result;
    for (auto& byte : _internal->data()) {
        result.push_back(byte);
    }

    return result;
}

rust::Vec<Bindings::RustyCharacteristicWrapper> Bindings::RustyService::characteristics() const {
    rust::Vec<Bindings::RustyCharacteristicWrapper> result;

    for (auto& characteristic : _internal->characteristics()) {
        Bindings::RustyCharacteristicWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyCharacteristic>(characteristic);
        result.push_back(std::move(wrapper));
    }

    return result;
}

// Characteristic Bindings

rust::Vec<Bindings::RustyDescriptorWrapper> Bindings::RustyCharacteristic::descriptors() const {
    rust::Vec<Bindings::RustyDescriptorWrapper> result;

    for (auto& descriptor : _internal->descriptors()) {
        Bindings::RustyDescriptorWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyDescriptor>(descriptor);
        result.push_back(std::move(wrapper));
    }

    return result;
}

rust::Vec<rust::String> Bindings::RustyCharacteristic::capabilities() const {
    rust::Vec<rust::String> result;

    for (auto& capability : _internal->capabilities()) {
        result.push_back(rust::String(capability));
    }

    return result;
}

// LocalPeripheral Bindings

void Bindings::RustyLocalPeripheral::link(SimpleRsBLE::InnerLocalPeripheral const& target) const {
    // Same approach as RustyAdapter::link(): the pinned InnerLocalPeripheral(Rust) owns this object,
    // and unlink() clears all native callbacks before releasing the back-pointer.
    _peripheral = &target;

    _internal->set_callback_on_client_connected([this](SimpleBLE::BluetoothAddress address) {
        if (this->_peripheral == nullptr) return;

        this->_peripheral->on_callback_client_connected(
            rust::Slice<const uint8_t>(reinterpret_cast<const uint8_t*>(address.data()), address.size()));
    });

    _internal->set_callback_on_client_disconnected([this](SimpleBLE::BluetoothAddress address) {
        if (this->_peripheral == nullptr) return;

        this->_peripheral->on_callback_client_disconnected(
            rust::Slice<const uint8_t>(reinterpret_cast<const uint8_t*>(address.data()), address.size()));
    });
}

void Bindings::RustyLocalPeripheral::unlink() const {
    // Clear all callbacks
    _internal->set_callback_on_client_connected(nullptr);
    _internal->set_callback_on_client_disconnected(nullptr);

    _peripheral = nullptr;
}

void Bindings::RustyLocalPeripheral::add_advertised_service(rust::Str uuid) const {
    _internal->add_advertised_service(std::string(uuid));
}

std::unique_ptr<Bindings::RustyLocalService> Bindings::RustyLocalPeripheral::add_service(rust::Str uuid) const {
    return std::make_unique<Bindings::RustyLocalService>(_internal->add_service(std::string(uuid)));
}

void Bindings::RustyLocalPeripheral::remove_all_services() const { _internal->remove_all_services(); }

void Bindings::RustyLocalPeripheral::start() const { _internal->start(); }

void Bindings::RustyLocalPeripheral::stop() const { _internal->stop(); }

bool Bindings::RustyLocalPeripheral::is_started() const { return _internal->is_started(); }

bool Bindings::RustyLocalPeripheral::is_advertising() const { return _internal->is_advertising(); }

// LocalService Bindings

rust::String Bindings::RustyLocalService::uuid() const { return rust::String(_internal->uuid()); }

std::unique_ptr<Bindings::RustyLocalCharacteristic> Bindings::RustyLocalService::add_characteristic(
    rust::Str uuid, uint8_t capabilities) const {
    std::set<LocalCapability> capability_set;
    for (auto& [bit, capability] : CAPABILITIES) {
        if (capabilities & static_cast<uint8_t>(bit)) {
            capability_set.insert(capability);
        }
    }

    return std::make_unique<Bindings::RustyLocalCharacteristic>(
        _internal->add_characteristic(std::string(uuid), capability_set));
}

// LocalCharacteristic Bindings

void Bindings::RustyLocalCharacteristic::link(SimpleRsBLE::InnerLocalCharacteristic const& target) const {
    // Same approach as RustyAdapter::link(): the pinned InnerLocalCharacteristic(Rust) owns this object,
    // and unlink() clears all native callbacks before releasing the back-pointer.
    // NOTE: The read callback is only registered when needed, as having one changes how reads are answered.
    _characteristic = &target;

    _internal->set_callback_on_write([this](SimpleBLE::ByteArray value) {
        if (this->_characteristic == nullptr) return;

        this->_characteristic->on_callback_written(rust::Slice<const uint8_t>(value.data(), value.size()));
    });

    _internal->set_callback_on_subscribed([this]() {
        if (this->_characteristic == nullptr) return;

        this->_characteristic->on_callback_subscribed();
    });

    _internal->set_callback_on_unsubscribed([this]() {
        if (this->_characteristic == nullptr) return;

        this->_characteristic->on_callback_unsubscribed();
    });
}

void Bindings::RustyLocalCharacteristic::unlink() const {
    // Clear all callbacks
    _internal->set_callback_on_read(nullptr);
    _internal->set_callback_on_write(nullptr);
    _internal->set_callback_on_subscribed(nullptr);
    _internal->set_callback_on_unsubscribed(nullptr);

    _characteristic = nullptr;
}

rust::String Bindings::RustyLocalCharacteristic::uuid() const { return rust::String(_internal->uuid()); }

uint8_t Bindings::RustyLocalCharacteristic::capabilities() const {
    auto capability_set = _internal->capabilities();

    uint8_t result = 0;
    for (auto& [bit, capability] : CAPABILITIES) {
        if (capability_set.count(capability) > 0) {
            result |= static_cast<uint8_t>(bit);
        }
    }

    return result;
}

rust::Vec<uint8_t> Bindings::RustyLocalCharacteristic::value() const {
    rust::Vec<uint8_t> result;
    for (auto& byte : _internal->value()) {
        result.push_back(byte);
    }

    return result;
}

void Bindings::RustyLocalCharacteristic::set_value(rust::Slice<const uint8_t> value) const {
    _internal->set_value(SimpleBLE::ByteArray(value.data(), value.size()));
}

void Bindings::RustyLocalCharacteristic::set_read_callback_enabled(bool enabled) const {
    if (!enabled) {
        _internal->set_callback_on_read(nullptr);
        return;
    }

    _internal->set_callback_on_read([this]() {
        if (this->_characteristic == nullptr) return SimpleBLE::ByteArray();

        rust::Vec<uint8_t> value = this->_characteristic->on_callback_read();
        return SimpleBLE::ByteArray(value.data(), value.size());
    });
}

// Advanced Bindings
//
// Each platform has its own block. If the native backend isn't part of the build (for example,
// when using the simulated backend), these functions report that the operation isn't supported.

#if SIMPLEBLE_BACKEND_MACOS

void Bindings::RustyLocalPeripheral::macos_set_advertisement_local_name(bool has_local_name,
                                                                        rust::Str local_name) const {
    auto name = has_local_name ? std::make_optional<std::string>(local_name) : std::nullopt;
    SimpleBLE::Advanced::MacOS::set_advertisement_local_name(*_internal, std::move(name));
}

rust::Vec<Bindings::RustyPeripheralWrapper> Bindings::RustyAdapter::macos_retrieve_cached_peripherals(
    rust::Vec<rust::String> const& identifiers) const {
    std::vector<SimpleBLE::BluetoothAddress> addresses;
    for (auto& identifier : identifiers) {
        addresses.emplace_back(identifier);
    }

    rust::Vec<Bindings::RustyPeripheralWrapper> result;

    for (auto& peripheral : SimpleBLE::Advanced::MacOS::retrieve_cached_peripherals(*_internal, addresses)) {
        Bindings::RustyPeripheralWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyPeripheral>(peripheral);
        result.push_back(std::move(wrapper));
    }

    return result;
}

#else

void Bindings::RustyLocalPeripheral::macos_set_advertisement_local_name(bool, rust::Str) const {
    throw SimpleBLE::Exception::OperationNotSupported();
}

rust::Vec<Bindings::RustyPeripheralWrapper> Bindings::RustyAdapter::macos_retrieve_cached_peripherals(
    rust::Vec<rust::String> const&) const {
    throw SimpleBLE::Exception::OperationNotSupported();
}

#endif

#if SIMPLEBLE_BACKEND_LINUX

void Bindings::RustyLocalPeripheral::linux_set_advertisement_local_name(bool has_local_name,
                                                                        rust::Str local_name) const {
    auto name = has_local_name ? std::make_optional<std::string>(local_name) : std::nullopt;
    SimpleBLE::Advanced::Linux::set_advertisement_local_name(*_internal, std::move(name));
}

#else

void Bindings::RustyLocalPeripheral::linux_set_advertisement_local_name(bool, rust::Str) const {
    throw SimpleBLE::Exception::OperationNotSupported();
}

#endif
