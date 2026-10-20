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

// Fills in `error` from the exception being handled.
void report_exception(Bindings::RustyError& error) noexcept {
    error.failed = true;

    try {
        throw;
    } catch (const SimpleBLE::Exception::BaseException& e) {
        error.code = e.make_error()->code;
        error.message = rust::String::lossy(e.what());
    } catch (const std::exception& e) {
        error.code = SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION;
        error.message = rust::String::lossy(e.what());
    } catch (...) {
        error.code = SimpleBLE::ErrorCode::UNCLASSIFIED_EXCEPTION;
        error.message = "Unknown exception";
    }
}

}  // namespace

rust::Vec<Bindings::RustyAdapterWrapper> Bindings::RustyAdapter_get_adapters(RustyError& error) try {
    rust::Vec<Bindings::RustyAdapterWrapper> result;

    for (auto& adapter : SimpleBLE::Adapter::get_adapters()) {
        Bindings::RustyAdapterWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyAdapter>(adapter);
        result.push_back(std::move(wrapper));
    }

    return result;
} catch (...) {
    report_exception(error);
    return {};
}

bool Bindings::RustyAdapter_bluetooth_enabled(RustyError& error) try {
    return SimpleBLE::Adapter::bluetooth_enabled();
} catch (...) {
    report_exception(error);
    return {};
}

rust::Vec<Bindings::RustyBackendWrapper> Bindings::RustyBackend_get_backends(RustyError& error) try {
    rust::Vec<Bindings::RustyBackendWrapper> result;

    for (auto& backend : SimpleBLE::Backend::get_backends()) {
        Bindings::RustyBackendWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyBackend>(backend);
        result.push_back(std::move(wrapper));
    }

    return result;
} catch (...) {
    report_exception(error);
    return {};
}

std::unique_ptr<Bindings::RustyBackend> Bindings::RustyBackend_get_native(RustyError& error) try {
    return std::make_unique<Bindings::RustyBackend>(SimpleBLE::Backend::get_native());
} catch (...) {
    report_exception(error);
    return {};
}

std::unique_ptr<Bindings::RustyBackend> Bindings::RustyBackend_get_dongl(RustyError& error) try {
    return std::make_unique<Bindings::RustyBackend>(SimpleBLE::Backend::get_dongl());
} catch (...) {
    report_exception(error);
    return {};
}

std::unique_ptr<Bindings::RustyBackend> Bindings::RustyBackend_get_simulator(RustyError& error) try {
    return std::make_unique<Bindings::RustyBackend>(SimpleBLE::Backend::get_simulator());
} catch (...) {
    report_exception(error);
    return {};
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

bool Bindings::RustyBackend::bluetooth_enabled(RustyError& error) const try {
    return _internal->bluetooth_enabled();
} catch (...) {
    report_exception(error);
    return {};
}

rust::Vec<Bindings::RustyAdapterWrapper> Bindings::RustyBackend::adapters(RustyError& error) const try {
    rust::Vec<Bindings::RustyAdapterWrapper> result;

    for (auto& adapter : _internal->adapters()) {
        Bindings::RustyAdapterWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyAdapter>(adapter);
        result.push_back(std::move(wrapper));
    }

    return result;
} catch (...) {
    report_exception(error);
    return {};
}

// InnerAdapter Bindings

void Bindings::RustyAdapter::link(SimpleRsBLE::InnerAdapter& target, RustyError& error) const try {
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

    // NOTE: Callbacks are only registered once the InnerAdapter(Rust) subscribes to its events.
    // They belong to the adapter, so registering them here would take them from every other handle.
    std::unique_lock<std::shared_mutex> lock(_callback_target->mutex);
    _callback_target->adapter = &target;  // THIS LINE IS SUPER IMPORTANT
} catch (...) {
    report_exception(error);
}

void Bindings::RustyAdapter::unlink(RustyError& error) const try {
    // NOTE: Callbacks stay registered, as they might belong to another handle by now.
    // Clearing the back-pointer stops ours for good, once any running one finishes.
    std::unique_lock<std::shared_mutex> lock(_callback_target->mutex);
    _callback_target->adapter = nullptr;
} catch (...) {
    report_exception(error);
}

void Bindings::RustyAdapter::register_scan_callbacks(RustyError& error) const try {
    auto target = _callback_target;

    _internal->set_callback_on_scan_start([target]() {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->adapter == nullptr) return;

        target->adapter->on_callback_scan_start();
    });

    _internal->set_callback_on_scan_stop([target]() {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->adapter == nullptr) return;

        target->adapter->on_callback_scan_stop();
    });

    _internal->set_callback_on_scan_found([target](SimpleBLE::Peripheral peripheral) {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->adapter == nullptr) return;

        Bindings::RustyPeripheralWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyPeripheral>(peripheral);
        target->adapter->on_callback_scan_found(wrapper);
    });

    _internal->set_callback_on_scan_updated([target](SimpleBLE::Peripheral peripheral) {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->adapter == nullptr) return;

        Bindings::RustyPeripheralWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyPeripheral>(peripheral);
        target->adapter->on_callback_scan_updated(wrapper);
    });
} catch (...) {
    report_exception(error);
}

void Bindings::RustyAdapter::register_power_callbacks(RustyError& error) const try {
    auto target = _callback_target;

    _internal->set_callback_on_power_on([target]() {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->adapter == nullptr) return;

        target->adapter->on_callback_power_on();
    });

    _internal->set_callback_on_power_off([target]() {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->adapter == nullptr) return;

        target->adapter->on_callback_power_off();
    });
} catch (...) {
    report_exception(error);
}

bool Bindings::RustyAdapter::initialized(RustyError& error) const try {
    return _internal->initialized();
} catch (...) {
    report_exception(error);
    return {};
}

rust::String Bindings::RustyAdapter::identifier(RustyError& error) const try {
    return rust::String(_internal->identifier());
} catch (...) {
    report_exception(error);
    return {};
}

rust::String Bindings::RustyAdapter::address(RustyError& error) const try {
    return rust::String(_internal->address());
} catch (...) {
    report_exception(error);
    return {};
}

void Bindings::RustyAdapter::power_on(RustyError& error) const try {
    _internal->power_on();
} catch (...) {
    report_exception(error);
}

void Bindings::RustyAdapter::power_off(RustyError& error) const try {
    _internal->power_off();
} catch (...) {
    report_exception(error);
}

bool Bindings::RustyAdapter::is_powered(RustyError& error) const try {
    return _internal->is_powered();
} catch (...) {
    report_exception(error);
    return {};
}

void Bindings::RustyAdapter::scan_start(RustyError& error) const try {
    _internal->scan_start();
} catch (...) {
    report_exception(error);
}

void Bindings::RustyAdapter::scan_stop(RustyError& error) const try {
    _internal->scan_stop();
} catch (...) {
    report_exception(error);
}

void Bindings::RustyAdapter::scan_for(int32_t timeout_ms, RustyError& error) const try {
    _internal->scan_for(timeout_ms);
} catch (...) {
    report_exception(error);
}

bool Bindings::RustyAdapter::scan_is_active(RustyError& error) const try {
    return _internal->scan_is_active();
} catch (...) {
    report_exception(error);
    return {};
}

rust::Vec<Bindings::RustyPeripheralWrapper> Bindings::RustyAdapter::scan_get_results(RustyError& error) const try {
    rust::Vec<Bindings::RustyPeripheralWrapper> result;

    for (auto& peripheral : _internal->scan_get_results()) {
        Bindings::RustyPeripheralWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyPeripheral>(peripheral);
        result.push_back(std::move(wrapper));
    }

    return result;
} catch (...) {
    report_exception(error);
    return {};
}

rust::Vec<Bindings::RustyPeripheralWrapper> Bindings::RustyAdapter::get_paired_peripherals(
    RustyError& error) const try {
    rust::Vec<Bindings::RustyPeripheralWrapper> result;

    for (auto& peripheral : _internal->get_paired_peripherals()) {
        Bindings::RustyPeripheralWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyPeripheral>(peripheral);
        result.push_back(std::move(wrapper));
    }

    return result;
} catch (...) {
    report_exception(error);
    return {};
}

rust::Vec<Bindings::RustyPeripheralWrapper> Bindings::RustyAdapter::get_connected_peripherals(
    RustyError& error) const try {
    rust::Vec<Bindings::RustyPeripheralWrapper> result;

    for (auto& peripheral : _internal->get_connected_peripherals()) {
        Bindings::RustyPeripheralWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyPeripheral>(peripheral);
        result.push_back(std::move(wrapper));
    }

    return result;
} catch (...) {
    report_exception(error);
    return {};
}

std::unique_ptr<Bindings::RustyLocalPeripheral> Bindings::RustyAdapter::create_local_peripheral(
    RustyError& error) const try {
    return std::make_unique<Bindings::RustyLocalPeripheral>(_internal->create_local_peripheral());
} catch (...) {
    report_exception(error);
    return {};
}

// InnerPeripheral Bindings

void Bindings::RustyPeripheral::link(SimpleRsBLE::InnerPeripheral& target, RustyError& error) const try {
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

    // NOTE: Callbacks are only registered once the InnerPeripheral(Rust) subscribes to its events.
    // They belong to the peripheral, so registering them here would take them from every other handle.
    std::unique_lock<std::shared_mutex> lock(_callback_target->mutex);
    _callback_target->peripheral = &target;  // THIS LINE IS SUPER IMPORTANT
} catch (...) {
    report_exception(error);
}

void Bindings::RustyPeripheral::unlink(RustyError& error) const try {
    // NOTE: Callbacks stay registered, as they might belong to another handle by now.
    // Clearing the back-pointer stops ours for good, once any running one finishes.
    std::unique_lock<std::shared_mutex> lock(_callback_target->mutex);
    _callback_target->peripheral = nullptr;
} catch (...) {
    report_exception(error);
}

void Bindings::RustyPeripheral::register_connection_callbacks(RustyError& error) const try {
    auto target = _callback_target;

    _internal->set_callback_on_connected([target]() {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->peripheral == nullptr) return;

        target->peripheral->on_callback_connected();
    });

    _internal->set_callback_on_disconnected([target]() {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->peripheral == nullptr) return;

        target->peripheral->on_callback_disconnected();
    });
} catch (...) {
    report_exception(error);
}

bool Bindings::RustyPeripheral::initialized(RustyError& error) const try {
    return _internal->initialized();
} catch (...) {
    report_exception(error);
    return {};
}

rust::String Bindings::RustyPeripheral::identifier(RustyError& error) const try {
    return rust::String(_internal->identifier());
} catch (...) {
    report_exception(error);
    return {};
}

rust::String Bindings::RustyPeripheral::address(RustyError& error) const try {
    return rust::String(_internal->address());
} catch (...) {
    report_exception(error);
    return {};
}


SimpleBLE::BluetoothAddressType Bindings::RustyPeripheral::address_type(RustyError& error) const try {
    return _internal->address_type();
} catch (...) {
    report_exception(error);
    return {};
}

int16_t Bindings::RustyPeripheral::rssi(RustyError& error) const try {
    return _internal->rssi();
} catch (...) {
    report_exception(error);
    return {};
}

int16_t Bindings::RustyPeripheral::tx_power(RustyError& error) const try {
    return _internal->tx_power();
} catch (...) {
    report_exception(error);
    return {};
}

uint16_t Bindings::RustyPeripheral::mtu(RustyError& error) const try {
    return _internal->mtu();
} catch (...) {
    report_exception(error);
    return {};
}

void Bindings::RustyPeripheral::connect(RustyError& error) const try {
    _internal->connect();
} catch (...) {
    report_exception(error);
}

void Bindings::RustyPeripheral::disconnect(RustyError& error) const try {
    _internal->disconnect();
} catch (...) {
    report_exception(error);
}

bool Bindings::RustyPeripheral::is_connected(RustyError& error) const try {
    return _internal->is_connected();
} catch (...) {
    report_exception(error);
    return {};
}

bool Bindings::RustyPeripheral::is_connectable(RustyError& error) const try {
    return _internal->is_connectable();
} catch (...) {
    report_exception(error);
    return {};
}

bool Bindings::RustyPeripheral::is_paired(RustyError& error) const try {
    return _internal->is_paired();
} catch (...) {
    report_exception(error);
    return {};
}

void Bindings::RustyPeripheral::unpair(RustyError& error) const try {
    _internal->unpair();
} catch (...) {
    report_exception(error);
}

rust::Vec<Bindings::RustyServiceWrapper> Bindings::RustyPeripheral::services(RustyError& error) const try {
    rust::Vec<Bindings::RustyServiceWrapper> result;

    for (auto& service : _internal->services()) {
        Bindings::RustyServiceWrapper wrapper;
        wrapper.internal = std::make_unique<Bindings::RustyService>(service);
        result.push_back(std::move(wrapper));
    }

    return result;
} catch (...) {
    report_exception(error);
    return {};
}

rust::Vec<Bindings::RustyManufacturerDataWrapper> Bindings::RustyPeripheral::manufacturer_data(
    RustyError& error) const try {
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
} catch (...) {
    report_exception(error);
    return {};
}

rust::Vec<uint8_t> Bindings::RustyPeripheral::read(rust::String const& service,
                                                   rust::String const& characteristic, RustyError& error) const try {
    std::string read_result = _internal->read(std::string(service), std::string(characteristic));

    rust::Vec<uint8_t> result;
    for (auto& byte : read_result) {
        result.push_back(byte);
    }

    return result;
} catch (...) {
    report_exception(error);
    return {};
}

void Bindings::RustyPeripheral::write_request(rust::String const& service_rs, rust::String const& characteristic_rs,
                                              rust::Vec<uint8_t> const& data, RustyError& error) const try {
    std::string service(service_rs);
    std::string characteristic(characteristic_rs);
    std::string data_vec((char*)data.data(), data.size());

    _internal->write_request(service, characteristic, data_vec);
} catch (...) {
    report_exception(error);
}

void Bindings::RustyPeripheral::write_command(rust::String const& service_rs, rust::String const& characteristic_rs,
                                              rust::Vec<uint8_t> const& data, RustyError& error) const try {
    std::string service(service_rs);
    std::string characteristic(characteristic_rs);
    std::string data_vec((char*)data.data(), data.size());

    _internal->write_command(service, characteristic, data_vec);
} catch (...) {
    report_exception(error);
}

void Bindings::RustyPeripheral::notify(rust::String const& service_rs, rust::String const& characteristic_rs,
                                       RustyError& error) const try {
    std::string service(service_rs);
    std::string characteristic(characteristic_rs);

    auto target = _callback_target;
    _internal->notify(service, characteristic, [target, service_rs, characteristic_rs](std::string data) {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->peripheral == nullptr) return;

        rust::Vec<uint8_t> data_vec;
        for (auto& byte : data) {
            data_vec.push_back(byte);
        }

        target->peripheral->on_callback_characteristic_updated(service_rs, characteristic_rs, data_vec);
    });
} catch (...) {
    report_exception(error);
}

void Bindings::RustyPeripheral::indicate(rust::String const& service_rs, rust::String const& characteristic_rs,
                                         RustyError& error) const try {
    std::string service(service_rs);
    std::string characteristic(characteristic_rs);

    auto target = _callback_target;
    _internal->indicate(service, characteristic, [target, service_rs, characteristic_rs](std::string data) {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->peripheral == nullptr) return;

        rust::Vec<uint8_t> data_vec;
        for (auto& byte : data) {
            data_vec.push_back(byte);
        }

        target->peripheral->on_callback_characteristic_updated(service_rs, characteristic_rs, data_vec);
    });
} catch (...) {
    report_exception(error);
}

void Bindings::RustyPeripheral::unsubscribe(rust::String const& service_rs,
                                            rust::String const& characteristic_rs, RustyError& error) const try {
    std::string service(service_rs);
    std::string characteristic(characteristic_rs);

    _internal->unsubscribe(service, characteristic);
} catch (...) {
    report_exception(error);
}

rust::Vec<uint8_t> Bindings::RustyPeripheral::read_descriptor(rust::String const& service,
                                                              rust::String const& characteristic,
                                                              rust::String const& descriptor,
                                                              RustyError& error) const try {
    std::string read_result = _internal->read(std::string(service), std::string(characteristic),
                                              std::string(descriptor));

    rust::Vec<uint8_t> result;
    for (auto& byte : read_result) {
        result.push_back(byte);
    }

    return result;
} catch (...) {
    report_exception(error);
    return {};
}

void Bindings::RustyPeripheral::write_descriptor(rust::String const& service, rust::String const& characteristic,
                                                 rust::String const& descriptor, rust::Vec<uint8_t> const& data,
                                                 RustyError& error) const try {
    _internal->write(std::string(service), std::string(characteristic), std::string(descriptor),
                     std::string((char*)data.data(), data.size()));
} catch (...) {
    report_exception(error);
}

// NOTE: The pairing callbacks run on a Dongl worker thread, possibly while connect() is still
// in progress. They hold the target lock while waiting for the user, so unlink() waits for them.

void Bindings::RustyPeripheral::dongl_set_passkey_request_callback(bool enabled, RustyError& error) const try {
    if (!enabled) {
        SimpleBLE::Advanced::Dongl::set_passkey_request_callback(*_internal, nullptr);
        return;
    }

    auto target = _callback_target;
    SimpleBLE::Advanced::Dongl::set_passkey_request_callback(*_internal, [target]() -> std::optional<std::string> {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->peripheral == nullptr) return std::nullopt;

        rust::String passkey;
        if (!target->peripheral->on_callback_passkey_request(passkey)) return std::nullopt;

        return std::string(passkey);
    });
} catch (...) {
    report_exception(error);
}

void Bindings::RustyPeripheral::dongl_set_passkey_display_callback(bool enabled, RustyError& error) const try {
    if (!enabled) {
        SimpleBLE::Advanced::Dongl::set_passkey_display_callback(*_internal, nullptr);
        return;
    }

    auto target = _callback_target;
    SimpleBLE::Advanced::Dongl::set_passkey_display_callback(*_internal, [target](const std::string& passkey) {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->peripheral == nullptr) return;

        target->peripheral->on_callback_passkey_display(
            rust::Slice<const uint8_t>(reinterpret_cast<const uint8_t*>(passkey.data()), passkey.size()));
    });
} catch (...) {
    report_exception(error);
}

void Bindings::RustyPeripheral::dongl_set_numeric_comparison_callback(bool enabled, RustyError& error) const try {
    if (!enabled) {
        SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(*_internal, nullptr);
        return;
    }

    auto target = _callback_target;
    SimpleBLE::Advanced::Dongl::set_numeric_comparison_callback(*_internal, [target](const std::string& passkey) {
        std::shared_lock<std::shared_mutex> lock(target->mutex);
        if (target->peripheral == nullptr) return false;

        return target->peripheral->on_callback_numeric_comparison(
            rust::Slice<const uint8_t>(reinterpret_cast<const uint8_t*>(passkey.data()), passkey.size()));
    });
} catch (...) {
    report_exception(error);
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

void Bindings::RustyLocalPeripheral::link(SimpleRsBLE::InnerLocalPeripheral const& target,
                                          RustyError& error) const try {
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
} catch (...) {
    report_exception(error);
}

void Bindings::RustyLocalPeripheral::unlink(RustyError& error) const try {
    // Clear all callbacks
    _internal->set_callback_on_client_connected(nullptr);
    _internal->set_callback_on_client_disconnected(nullptr);

    _peripheral = nullptr;
} catch (...) {
    report_exception(error);
}

void Bindings::RustyLocalPeripheral::add_advertised_service(rust::Str uuid, RustyError& error) const try {
    _internal->add_advertised_service(std::string(uuid));
} catch (...) {
    report_exception(error);
}

void Bindings::RustyLocalPeripheral::set_advertised_manufacturer_data(uint16_t company_id,
                                                                      rust::Slice<const uint8_t> data,
                                                                      RustyError& error) const try {
    _internal->set_advertised_manufacturer_data(company_id, SimpleBLE::ByteArray(data.data(), data.size()));
} catch (...) {
    report_exception(error);
}

void Bindings::RustyLocalPeripheral::set_connectable(bool connectable, RustyError& error) const try {
    _internal->set_connectable(connectable);
} catch (...) {
    report_exception(error);
}

std::unique_ptr<Bindings::RustyLocalService> Bindings::RustyLocalPeripheral::add_service(rust::Str uuid,
                                                                                         RustyError& error) const try {
    return std::make_unique<Bindings::RustyLocalService>(_internal->add_service(std::string(uuid)));
} catch (...) {
    report_exception(error);
    return {};
}

void Bindings::RustyLocalPeripheral::remove_all_services(RustyError& error) const try {
    _internal->remove_all_services();
} catch (...) {
    report_exception(error);
}

void Bindings::RustyLocalPeripheral::start(RustyError& error) const try {
    _internal->start();
} catch (...) {
    report_exception(error);
}

void Bindings::RustyLocalPeripheral::stop(RustyError& error) const try {
    _internal->stop();
} catch (...) {
    report_exception(error);
}

bool Bindings::RustyLocalPeripheral::is_started(RustyError& error) const try {
    return _internal->is_started();
} catch (...) {
    report_exception(error);
    return {};
}

bool Bindings::RustyLocalPeripheral::is_advertising(RustyError& error) const try {
    return _internal->is_advertising();
} catch (...) {
    report_exception(error);
    return {};
}

// LocalService Bindings

rust::String Bindings::RustyLocalService::uuid(RustyError& error) const try {
    return rust::String(_internal->uuid());
} catch (...) {
    report_exception(error);
    return {};
}

std::unique_ptr<Bindings::RustyLocalCharacteristic> Bindings::RustyLocalService::add_characteristic(
    rust::Str uuid, uint8_t capabilities, RustyError& error) const try {
    std::set<LocalCapability> capability_set;
    for (auto& [bit, capability] : CAPABILITIES) {
        if (capabilities & static_cast<uint8_t>(bit)) {
            capability_set.insert(capability);
        }
    }

    return std::make_unique<Bindings::RustyLocalCharacteristic>(
        _internal->add_characteristic(std::string(uuid), capability_set));
} catch (...) {
    report_exception(error);
    return {};
}

// LocalCharacteristic Bindings

void Bindings::RustyLocalCharacteristic::link(SimpleRsBLE::InnerLocalCharacteristic const& target,
                                              RustyError& error) const try {
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
} catch (...) {
    report_exception(error);
}

void Bindings::RustyLocalCharacteristic::unlink(RustyError& error) const try {
    // Clear all callbacks
    _internal->set_callback_on_read(nullptr);
    _internal->set_callback_on_write(nullptr);
    _internal->set_callback_on_subscribed(nullptr);
    _internal->set_callback_on_unsubscribed(nullptr);

    _characteristic = nullptr;
} catch (...) {
    report_exception(error);
}

rust::String Bindings::RustyLocalCharacteristic::uuid(RustyError& error) const try {
    return rust::String(_internal->uuid());
} catch (...) {
    report_exception(error);
    return {};
}

uint8_t Bindings::RustyLocalCharacteristic::capabilities(RustyError& error) const try {
    auto capability_set = _internal->capabilities();

    uint8_t result = 0;
    for (auto& [bit, capability] : CAPABILITIES) {
        if (capability_set.count(capability) > 0) {
            result |= static_cast<uint8_t>(bit);
        }
    }

    return result;
} catch (...) {
    report_exception(error);
    return {};
}

rust::Vec<uint8_t> Bindings::RustyLocalCharacteristic::value(RustyError& error) const try {
    rust::Vec<uint8_t> result;
    for (auto& byte : _internal->value()) {
        result.push_back(byte);
    }

    return result;
} catch (...) {
    report_exception(error);
    return {};
}

void Bindings::RustyLocalCharacteristic::set_value(rust::Slice<const uint8_t> value, RustyError& error) const try {
    _internal->set_value(SimpleBLE::ByteArray(value.data(), value.size()));
} catch (...) {
    report_exception(error);
}

void Bindings::RustyLocalCharacteristic::set_read_callback_enabled(bool enabled, RustyError& error) const try {
    if (!enabled) {
        _internal->set_callback_on_read(nullptr);
        return;
    }

    _internal->set_callback_on_read([this]() {
        if (this->_characteristic == nullptr) return SimpleBLE::ByteArray();

        rust::Vec<uint8_t> value = this->_characteristic->on_callback_read();
        return SimpleBLE::ByteArray(value.data(), value.size());
    });
} catch (...) {
    report_exception(error);
}

// Advanced Bindings
//
// Each platform has its own block. If the native backend isn't part of the build (for example,
// when using the simulated backend), these functions report that the operation isn't supported.

#if SIMPLEBLE_BACKEND_MACOS

void Bindings::RustyLocalPeripheral::macos_set_advertisement_local_name(bool has_local_name,
                                                                        rust::Str local_name,
                                                                        RustyError& error) const try {
    auto name = has_local_name ? std::make_optional<std::string>(local_name) : std::nullopt;
    SimpleBLE::Advanced::MacOS::set_advertisement_local_name(*_internal, std::move(name));
} catch (...) {
    report_exception(error);
}

rust::Vec<Bindings::RustyPeripheralWrapper> Bindings::RustyAdapter::macos_retrieve_cached_peripherals(
    rust::Vec<rust::String> const& identifiers, RustyError& error) const try {
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
} catch (...) {
    report_exception(error);
    return {};
}

#else

void Bindings::RustyLocalPeripheral::macos_set_advertisement_local_name(bool, rust::Str, RustyError& error) const try {
    throw SimpleBLE::Exception::OperationNotSupported();
} catch (...) {
    report_exception(error);
}

rust::Vec<Bindings::RustyPeripheralWrapper> Bindings::RustyAdapter::macos_retrieve_cached_peripherals(
    rust::Vec<rust::String> const&, RustyError& error) const try {
    throw SimpleBLE::Exception::OperationNotSupported();
} catch (...) {
    report_exception(error);
    return {};
}

#endif

#if SIMPLEBLE_BACKEND_LINUX

void Bindings::RustyLocalPeripheral::linux_set_advertisement_local_name(bool has_local_name,
                                                                        rust::Str local_name,
                                                                        RustyError& error) const try {
    auto name = has_local_name ? std::make_optional<std::string>(local_name) : std::nullopt;
    SimpleBLE::Advanced::Linux::set_advertisement_local_name(*_internal, std::move(name));
} catch (...) {
    report_exception(error);
}

#else

void Bindings::RustyLocalPeripheral::linux_set_advertisement_local_name(bool, rust::Str, RustyError& error) const try {
    throw SimpleBLE::Exception::OperationNotSupported();
} catch (...) {
    report_exception(error);
}

#endif
