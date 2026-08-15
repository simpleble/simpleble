#include "LocalPeripheralLinux.h"

#include <exception>
#include <utility>

#include "BackendBluez.h"
#include "CommonUtils.h"
#include "LocalServiceLinux.h"

namespace SimpleBLE::Local {

PeripheralLinux::PeripheralLinux(std::shared_ptr<SimpleBluez::Adapter> adapter, std::string name)
    : _adapter(std::move(adapter)),
      _root(BackendBluez::get()->bluez.root_custom()),
      _name(std::move(name)) {
    _service_manager = _root->service_mgr_add(_name);
}

PeripheralLinux::~PeripheralLinux() {
    _callback_on_client_connected.unload();
    _callback_on_client_disconnected.unload();
    try {
        stop();
    } catch (const std::exception& ex) {
        SIMPLEBLE_LOG_WARN(fmt::format("Failed to stop local peripheral during cleanup: {}", ex.what()));
    } catch (...) {
        SIMPLEBLE_LOG_WARN("Failed to stop local peripheral during cleanup");
    }

    {
        std::scoped_lock lock(_lifecycle_mutex);
        _services.clear();
    }
    _root->service_mgr_remove(_name);
}

void* PeripheralLinux::underlying() const { return _service_manager.get(); }

Advertisement PeripheralLinux::advertisement() {
    std::scoped_lock lock(_lifecycle_mutex);
    return _advertisement;
}

void PeripheralLinux::set_advertisement(Advertisement advertisement) {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();
    _advertisement = std::move(advertisement);
}

std::shared_ptr<ServiceBase> PeripheralLinux::add_service(BluetoothUUID uuid) {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();

    const auto name = std::to_string(_next_service_id++);
    auto bluez_service = _service_manager->service_add(name);
    try {
        auto service = std::make_shared<ServiceLinux>(bluez_service, name, std::move(uuid));
        _services.push_back(service);
        return service;
    } catch (...) {
        _service_manager->service_remove("service_" + name);
        throw;
    }
}

std::vector<std::shared_ptr<ServiceBase>> PeripheralLinux::services() {
    std::scoped_lock lock(_lifecycle_mutex);
    return {_services.begin(), _services.end()};
}

void PeripheralLinux::remove_all_services() {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();

    for (const auto& service : _services) {
        _service_manager->service_remove("service_" + service->name());
    }
    _services.clear();
}

void PeripheralLinux::start() {
    std::scoped_lock lock(_lifecycle_mutex);
    if (_started.load()) {
        return;
    }

    std::vector<std::string> service_uuids = _advertisement.service_uuids;
    if (service_uuids.empty()) {
        service_uuids.reserve(_services.size());
        for (const auto& service : _services) {
            service_uuids.push_back(service->uuid());
        }
    }

    _bluez_advertisement = _root->advertisement_add(_name);
    _bluez_advertisement->adv_type("peripheral");
    _bluez_advertisement->discoverable(true);
    if (_advertisement.local_name.has_value()) {
        _bluez_advertisement->local_name(*_advertisement.local_name);
    }
    if (!service_uuids.empty()) {
        _bluez_advertisement->service_uuids(service_uuids);
    }

    for (const auto& service : _services) {
        service->freeze();
    }

    bool application_registered = false;
    try {
        _adapter->register_application(_service_manager->path());
        application_registered = true;
        _adapter->register_advertisement(_bluez_advertisement);
        _started.store(true);
    } catch (...) {
        if (application_registered) {
            try {
                _adapter->unregister_application(_service_manager->path());
            } catch (...) {
            }
        }
        _root->advertisement_remove(_name);
        _bluez_advertisement.reset();
        for (const auto& service : _services) {
            service->unfreeze();
        }
        throw;
    }
}

void PeripheralLinux::stop() {
    std::scoped_lock lock(_lifecycle_mutex);
    if (!_started.exchange(false)) {
        return;
    }

    std::exception_ptr first_error;
    try {
        _adapter->unregister_advertisement(_bluez_advertisement);
    } catch (...) {
        first_error = std::current_exception();
    }

    try {
        _adapter->unregister_application(_service_manager->path());
    } catch (...) {
        if (!first_error) {
            first_error = std::current_exception();
        }
    }

    _root->advertisement_remove(_name);
    _bluez_advertisement.reset();
    for (const auto& service : _services) {
        service->unfreeze();
    }

    {
        std::scoped_lock clients_lock(_clients_mutex);
        _connected_clients.clear();
    }

    if (first_error) {
        std::rethrow_exception(first_error);
    }
}

bool PeripheralLinux::is_started() { return _started.load(); }

bool PeripheralLinux::is_advertising() {
    std::scoped_lock lock(_lifecycle_mutex);
    return _bluez_advertisement && _bluez_advertisement->active();
}

void PeripheralLinux::set_callback_on_client_connected(
    std::function<void(BluetoothAddress client_address)> on_client_connected) {
    if (on_client_connected) {
        _callback_on_client_connected.load(std::move(on_client_connected));
    } else {
        _callback_on_client_connected.unload();
    }
}

void PeripheralLinux::set_callback_on_client_disconnected(
    std::function<void(BluetoothAddress client_address)> on_client_disconnected) {
    if (on_client_disconnected) {
        _callback_on_client_disconnected.load(std::move(on_client_disconnected));
    } else {
        _callback_on_client_disconnected.unload();
    }
}

void PeripheralLinux::handle_connected_changed(const BluetoothAddress& address, bool connected) {
    if (!_started.load()) {
        return;
    }

    bool notify_connected = false;
    bool notify_disconnected = false;
    {
        std::scoped_lock lock(_clients_mutex);
        if (connected) {
            notify_connected = _connected_clients.insert(address).second;
        } else {
            notify_disconnected = _connected_clients.erase(address) > 0;
        }
    }

    if (notify_connected) {
        SAFE_CALLBACK_CALL(_callback_on_client_connected, address);
    } else if (notify_disconnected) {
        SAFE_CALLBACK_CALL(_callback_on_client_disconnected, address);
    }
}

void PeripheralLinux::_ensure_mutable() const {
    if (_started.load()) {
        throw Exception::OperationFailed("The local peripheral cannot be changed while it is started.");
    }
}

}  // namespace SimpleBLE::Local
