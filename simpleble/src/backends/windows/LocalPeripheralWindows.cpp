#include "LocalPeripheralWindows.h"

#include <algorithm>
#include <exception>
#include <utility>

#include <simpleble/Exceptions.h>

#include "CommonUtils.h"
#include "LocalServiceWindows.h"
#include "LoggingInternal.h"
#include "MtaManager.h"
#include "Utils.h"
#include "winrt/Windows.Foundation.Metadata.h"

namespace SimpleBLE::Local {

using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;

PeripheralWindows::PeripheralWindows(winrt::Windows::Devices::Bluetooth::BluetoothAdapter adapter)
    : _adapter(std::move(adapter)) {}

PeripheralWindows::~PeripheralWindows() {
    _callback_on_client_connected.unload();
    _callback_on_client_disconnected.unload();
    try {
        stop();
    } catch (const std::exception& ex) {
        SIMPLEBLE_LOG_WARN(fmt::format("Failed to stop Windows local peripheral during cleanup: {}", ex.what()));
    } catch (...) {
        SIMPLEBLE_LOG_WARN("Failed to stop Windows local peripheral during cleanup");
    }

    std::scoped_lock lock(_lifecycle_mutex);
    _services.clear();
    _run.reset();
}

void* PeripheralWindows::underlying() const {
    return reinterpret_cast<void*>(const_cast<winrt::Windows::Devices::Bluetooth::BluetoothAdapter*>(&_adapter));
}

Advertisement PeripheralWindows::advertisement() {
    std::scoped_lock lock(_lifecycle_mutex);
    return _advertisement;
}

void PeripheralWindows::set_advertisement(Advertisement advertisement) {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();
    _advertisement = std::move(advertisement);
}

std::shared_ptr<ServiceBase> PeripheralWindows::add_service(BluetoothUUID uuid) {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();
    auto service = std::make_shared<ServiceWindows>(std::move(uuid));
    _services.push_back(service);
    return service;
}

std::vector<std::shared_ptr<ServiceBase>> PeripheralWindows::services() {
    std::scoped_lock lock(_lifecycle_mutex);
    return {_services.begin(), _services.end()};
}

void PeripheralWindows::remove_all_services() {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();
    _services.clear();
}

void PeripheralWindows::start() {
    std::scoped_lock lock(_lifecycle_mutex);
    if (_cleanup_required) {
        throw Exception::OperationFailed(
            "The previous Windows peripheral operation requires cleanup; call stop() before starting again.");
    }
    if (_started.load()) {
        return;
    }
    if (_services.empty()) {
        throw Exception::OperationFailed("Windows peripheral mode requires at least one local service.");
    }
    if (!winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
            L"Windows.Devices.Bluetooth.BluetoothAdapter", L"IsPeripheralRoleSupported") ||
        !_adapter.IsPeripheralRoleSupported()) {
        throw Exception::OperationNotSupported();
    }

    if (_advertisement.local_name.has_value()) {
        SIMPLEBLE_LOG_WARN(
            "Windows peripheral mode uses the system Bluetooth name; the requested local advertisement name cannot be "
            "applied.");
    }

    std::vector<std::shared_ptr<ServiceWindows>> publication_order;
    publication_order.reserve(_services.size());
    for (const auto& requested_uuid : _advertisement.service_uuids) {
        const auto requested_guid = uuid_to_guid(requested_uuid);
        const auto service = std::find_if(_services.begin(), _services.end(), [&](const auto& candidate) {
            return uuid_to_guid(candidate->uuid()) == requested_guid;
        });
        if (service == _services.end()) {
            throw Exception::OperationFailed(
                fmt::format("Advertisement service UUID {} is not hosted by this local peripheral.", requested_uuid));
        }
        if (std::find(publication_order.begin(), publication_order.end(), *service) == publication_order.end()) {
            publication_order.push_back(*service);
        }
    }
    for (const auto& service : _services) {
        if (std::find(publication_order.begin(), publication_order.end(), service) == publication_order.end()) {
            publication_order.push_back(service);
        }
    }

    const auto services_snapshot = _services;
    auto run = std::make_shared<RunState>();
    auto weak_self = weak_from_this();
    std::weak_ptr<RunState> weak_run = run;
    const ServiceWindows::SessionObserver session_observer = [weak_self, weak_run](const GattSession& session) {
        const auto state = weak_run.lock();
        if (!state || !state->active.load()) {
            return;
        }
        if (auto self = weak_self.lock()) {
            try {
                self->_observe_session(state, session);
            } catch (const std::exception& ex) {
                SIMPLEBLE_LOG_WARN(fmt::format("Failed to observe a Windows local peripheral client: {}", ex.what()));
            } catch (...) {
                SIMPLEBLE_LOG_WARN("Failed to observe a Windows local peripheral client");
            }
        }
    };
    const ServiceWindows::ActivityObserver activity_observer = [weak_run]() {
        const auto state = weak_run.lock();
        return state && state->active.load();
    };

    for (const auto& service : services_snapshot) {
        service->freeze();
    }
    _run = run;

    bool rollback_failed = false;
    try {
        for (const auto& service : services_snapshot) {
            service->create_native(session_observer, activity_observer);
        }

        try {
            WinRT::MtaManager::get().execute_sync([&publication_order]() {
                for (const auto& service : publication_order) {
                    // WinRT publishes each primary GATT service through its own provider. Starting every provider
                    // keeps the complete GATT table available. Windows owns the shared advertisement payload and may
                    // omit UUIDs that do not fit, reporting StartedWithoutAllAdvertisementData for those providers.
                    service->start_advertising();
                }
            });
            // Wait on the calling thread so the process-wide MTA executor remains available to scanning, GATT I/O,
            // notifications, and status events while Windows finishes publishing the providers.
            for (const auto& service : publication_order) {
                service->wait_until_advertising();
            }
        } catch (...) {
            const auto start_error = std::current_exception();
            run->active.store(false);
            try {
                WinRT::MtaManager::get().execute_sync([&publication_order, &rollback_failed]() {
                    for (const auto& service : publication_order) {
                        try {
                            service->stop_advertising();
                        } catch (...) {
                            rollback_failed = true;
                        }
                    }
                });
            } catch (...) {
                rollback_failed = true;
            }
            std::rethrow_exception(start_error);
        }
        _started.store(true);
    } catch (const std::exception& ex) {
        run->active.store(false);
        _clear_client_sessions(run);
        if (!rollback_failed) {
            for (const auto& service : services_snapshot) {
                service->destroy_native();
                service->unfreeze();
            }
            _run.reset();
        }
        _cleanup_required = rollback_failed;
        if (rollback_failed) {
            SIMPLEBLE_LOG_ERROR(
                "Failed to roll back every Windows local service; the peripheral remains frozen until stop() "
                "succeeds.");
        }
        SIMPLEBLE_LOG_ERROR(fmt::format("Failed to start Windows local peripheral: {}", ex.what()));
        throw;
    } catch (...) {
        run->active.store(false);
        _clear_client_sessions(run);
        if (!rollback_failed) {
            for (const auto& service : services_snapshot) {
                service->destroy_native();
                service->unfreeze();
            }
            _run.reset();
        }
        _cleanup_required = rollback_failed;
        if (rollback_failed) {
            SIMPLEBLE_LOG_ERROR(
                "Failed to roll back every Windows local service; the peripheral remains frozen until stop() "
                "succeeds.");
        }
        SIMPLEBLE_LOG_ERROR("Failed to start Windows local peripheral");
        throw;
    }
}

void PeripheralWindows::stop() {
    std::scoped_lock lock(_lifecycle_mutex);
    if (!_started.load() && !_cleanup_required) {
        return;
    }

    const auto run = _run;
    if (run) {
        run->active.store(false);
        _clear_client_sessions(run);
    }

    std::exception_ptr first_error;
    try {
        WinRT::MtaManager::get().execute_sync([this]() {
            std::exception_ptr stop_error;
            for (const auto& service : _services) {
                try {
                    service->stop_advertising();
                } catch (...) {
                    if (!stop_error) {
                        stop_error = std::current_exception();
                    }
                }
            }
            if (stop_error) {
                std::rethrow_exception(stop_error);
            }
        });
    } catch (...) {
        first_error = std::current_exception();
    }

    if (first_error) {
        _cleanup_required = true;
        std::rethrow_exception(first_error);
    }

    for (const auto& service : _services) {
        service->destroy_native();
        service->unfreeze();
    }
    _run.reset();
    _started.store(false);
    _cleanup_required = false;
}

bool PeripheralWindows::is_started() { return _started.load(); }

bool PeripheralWindows::is_advertising() {
    std::scoped_lock lock(_lifecycle_mutex);
    if (!_started.load() && !_cleanup_required) {
        return false;
    }
    return WinRT::MtaManager::get().execute_sync<bool>([this]() {
        return std::all_of(_services.begin(), _services.end(),
                           [](const auto& service) { return service->is_advertising(); });
    });
}

void PeripheralWindows::set_callback_on_client_connected(std::function<void(BluetoothAddress)> on_client_connected) {
    if (on_client_connected) {
        _callback_on_client_connected.load(std::move(on_client_connected));
    } else {
        _callback_on_client_connected.unload();
    }
}

void PeripheralWindows::set_callback_on_client_disconnected(
    std::function<void(BluetoothAddress)> on_client_disconnected) {
    if (on_client_disconnected) {
        _callback_on_client_disconnected.load(std::move(on_client_disconnected));
    } else {
        _callback_on_client_disconnected.unload();
    }
}

void PeripheralWindows::_ensure_mutable() const {
    if (_started.load() || _cleanup_required) {
        throw Exception::OperationFailed("The local peripheral cannot be changed while it is started.");
    }
}

void PeripheralWindows::_observe_session(const std::shared_ptr<RunState>& run, const GattSession& session) {
    if (!run || !run->active.load() || !session) {
        return;
    }

    const auto [key, address] = _session_identity(session);
    GattSession previous_session{nullptr};
    winrt::event_token previous_token{};
    {
        std::scoped_lock lock(run->clients_mutex);
        if (!run->active.load()) {
            return;
        }
        const auto existing = run->client_sessions.find(key);
        if (existing != run->client_sessions.end() &&
            winrt::get_abi(existing->second.session) == winrt::get_abi(session)) {
            return;
        }
        if (existing == run->client_sessions.end()) {
            run->client_sessions.emplace(key, ClientSession{session, {}, address, false});
        } else {
            previous_session = existing->second.session;
            previous_token = existing->second.status_changed_token;
            existing->second = ClientSession{session, {}, address, existing->second.connected};
        }
    }

    if (previous_session) {
        try {
            if (previous_token) {
                previous_session.SessionStatusChanged(previous_token);
            }
        } catch (...) {
        }
        try {
            previous_session.Close();
        } catch (...) {
        }
    }

    auto weak_self = weak_from_this();
    std::weak_ptr<RunState> weak_run = run;
    winrt::event_token token{};
    try {
        token = session.SessionStatusChanged([weak_self, weak_run, key](const GattSession& sender,
                                                                        const GattSessionStatusChangedEventArgs& args) {
            const auto state = weak_run.lock();
            if (!state || !state->active.load()) {
                return;
            }
            if (auto self = weak_self.lock()) {
                try {
                    self->_on_session_status_changed(state, key, sender, args);
                } catch (const std::exception& ex) {
                    SIMPLEBLE_LOG_WARN(fmt::format("Failed to handle a Windows client session change: {}", ex.what()));
                } catch (...) {
                    SIMPLEBLE_LOG_WARN("Failed to handle a Windows client session change");
                }
            }
        });
    } catch (...) {
        {
            std::scoped_lock lock(run->clients_mutex);
            const auto it = run->client_sessions.find(key);
            if (it != run->client_sessions.end() && winrt::get_abi(it->second.session) == winrt::get_abi(session)) {
                run->client_sessions.erase(it);
            }
        }
        try {
            session.Close();
        } catch (...) {
        }
        throw;
    }

    GattSessionStatus status;
    try {
        status = session.SessionStatus();
    } catch (...) {
        {
            std::scoped_lock lock(run->clients_mutex);
            const auto it = run->client_sessions.find(key);
            if (it != run->client_sessions.end() && winrt::get_abi(it->second.session) == winrt::get_abi(session)) {
                run->client_sessions.erase(it);
            }
        }
        try {
            session.SessionStatusChanged(token);
        } catch (...) {
        }
        try {
            session.Close();
        } catch (...) {
        }
        throw;
    }
    bool keep_token = false;
    bool close_session = false;
    bool notify_connected = false;
    bool notify_disconnected = false;
    {
        std::scoped_lock lock(run->clients_mutex);
        const auto it = run->client_sessions.find(key);
        if (run->active.load() && it != run->client_sessions.end() &&
            winrt::get_abi(it->second.session) == winrt::get_abi(session)) {
            it->second.status_changed_token = token;
            keep_token = true;
            if (status == GattSessionStatus::Active && !it->second.connected) {
                it->second.connected = true;
                notify_connected = true;
            } else if (status == GattSessionStatus::Closed) {
                notify_disconnected = it->second.connected;
                run->client_sessions.erase(it);
                keep_token = false;
                close_session = true;
            }
        }
    }

    if (!keep_token) {
        try {
            session.SessionStatusChanged(token);
        } catch (...) {
        }
    }
    if (close_session) {
        try {
            session.Close();
        } catch (...) {
        }
    }
    if (notify_connected && run->active.load()) {
        SAFE_CALLBACK_CALL(_callback_on_client_connected, address);
    } else if (notify_disconnected && run->active.load()) {
        SAFE_CALLBACK_CALL(_callback_on_client_disconnected, address);
    }
}

void PeripheralWindows::_on_session_status_changed(const std::shared_ptr<RunState>& run, const std::string& key,
                                                   const GattSession& sender,
                                                   const GattSessionStatusChangedEventArgs& args) {
    BluetoothAddress address;
    GattSession session{nullptr};
    winrt::event_token token{};
    bool notify_connected = false;
    bool notify_disconnected = false;
    {
        std::scoped_lock lock(run->clients_mutex);
        const auto it = run->client_sessions.find(key);
        if (!run->active.load() || it == run->client_sessions.end() ||
            winrt::get_abi(it->second.session) != winrt::get_abi(sender)) {
            return;
        }

        address = it->second.address;
        if (args.Status() == GattSessionStatus::Active && !it->second.connected) {
            it->second.connected = true;
            notify_connected = true;
        } else if (args.Status() == GattSessionStatus::Closed) {
            session = it->second.session;
            token = it->second.status_changed_token;
            notify_disconnected = it->second.connected;
            run->client_sessions.erase(it);
        }
    }

    if (session) {
        try {
            if (token) {
                session.SessionStatusChanged(token);
            }
        } catch (...) {
        }
        try {
            session.Close();
        } catch (...) {
        }
    }
    if (notify_connected && run->active.load()) {
        SAFE_CALLBACK_CALL(_callback_on_client_connected, address);
    } else if (notify_disconnected && run->active.load()) {
        SAFE_CALLBACK_CALL(_callback_on_client_disconnected, address);
    }
}

void PeripheralWindows::_clear_client_sessions(const std::shared_ptr<RunState>& run) noexcept {
    if (!run) {
        return;
    }
    run->active.store(false);

    std::map<std::string, ClientSession> sessions;
    {
        std::scoped_lock lock(run->clients_mutex);
        sessions.swap(run->client_sessions);
    }
    for (auto& [key, client] : sessions) {
        try {
            if (client.status_changed_token) {
                client.session.SessionStatusChanged(client.status_changed_token);
            }
        } catch (const std::exception& ex) {
            SIMPLEBLE_LOG_WARN(fmt::format("Failed to remove Windows client session handler: {}", ex.what()));
        } catch (...) {
            SIMPLEBLE_LOG_WARN("Failed to remove Windows client session handler");
        }
        try {
            client.session.Close();
        } catch (const std::exception& ex) {
            SIMPLEBLE_LOG_WARN(fmt::format("Failed to close Windows client session: {}", ex.what()));
        } catch (...) {
            SIMPLEBLE_LOG_WARN("Failed to close Windows client session");
        }
    }
}

std::pair<std::string, BluetoothAddress> PeripheralWindows::_session_identity(const GattSession& session) {
    const std::string device_id = winrt::to_string(session.DeviceId().Id());
    const BluetoothAddress address = _bluetooth_address_from_id(device_id);
    if (!address.empty()) {
        return {device_id, address};
    }
    return {device_id, device_id};
}

}  // namespace SimpleBLE::Local
