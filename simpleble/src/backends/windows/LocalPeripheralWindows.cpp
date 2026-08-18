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
#include "winrt/Windows.System.Threading.h"

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

    auto weak_self = weak_from_this();
    auto service = std::make_shared<ServiceWindows>(
        std::move(uuid),
        [weak_self](const GattSession& session, uint64_t expected_generation) {
            if (auto self = weak_self.lock()) {
                try {
                    self->_observe_session(session, expected_generation);
                } catch (const std::exception& ex) {
                    SIMPLEBLE_LOG_WARN(
                        fmt::format("Failed to observe a Windows local peripheral client: {}", ex.what()));
                } catch (...) {
                    SIMPLEBLE_LOG_WARN("Failed to observe a Windows local peripheral client");
                }
            }
        },
        [weak_self]() -> uint64_t {
            if (auto self = weak_self.lock()) {
                return self->_active_client_generation();
            }
            return 0;
        },
        [weak_self](uint64_t generation) {
            if (auto self = weak_self.lock()) {
                return self->_client_callbacks_are_enabled(generation);
            }
            return false;
        });
    service->initialize_handlers();
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
    std::unique_lock lock(_lifecycle_mutex);
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
    for (const auto& service : services_snapshot) {
        service->freeze();
    }

    uint64_t generation;
    {
        std::scoped_lock clients_lock(_clients_mutex);
        generation = ++_client_generation;
        _accepting_clients = true;
        _client_callbacks_enabled = false;
    }
    for (const auto& service : services_snapshot) {
        service->activate(generation);
    }

    bool rollback_failed = false;
    try {
        WinRT::MtaManager::get().execute_sync([&publication_order, &rollback_failed]() {
            size_t started_count = 0;
            try {
                for (const auto& service : publication_order) {
                    // WinRT publishes each primary GATT service through its own provider. Starting every provider
                    // keeps the complete GATT table available. Windows owns the shared advertisement payload and may
                    // omit UUIDs that do not fit, reporting StartedWithoutAllAdvertisementData for those providers.
                    service->start_advertising();
                    ++started_count;
                }
                for (size_t index = 0; index < started_count; ++index) {
                    publication_order[index]->wait_until_advertising();
                }
            } catch (...) {
                for (size_t index = 0; index < started_count; ++index) {
                    try {
                        publication_order[index]->stop_advertising();
                    } catch (...) {
                        rollback_failed = true;
                    }
                }
                throw;
            }
        });
        {
            std::scoped_lock clients_lock(_clients_mutex);
            _started.store(true);
        }
        for (const auto& service : services_snapshot) {
            service->reconcile_subscriptions(generation);
        }
    } catch (const std::exception& ex) {
        _clear_client_sessions();
        for (const auto& service : services_snapshot) {
            service->deactivate();
            service->reset_subscriptions();
            if (!rollback_failed) {
                service->unfreeze();
            }
        }
        _cleanup_required = rollback_failed;
        if (rollback_failed) {
            SIMPLEBLE_LOG_ERROR(
                "Failed to roll back every Windows local service; the peripheral remains frozen until stop() succeeds.");
        }
        SIMPLEBLE_LOG_ERROR(fmt::format("Failed to start Windows local peripheral: {}", ex.what()));
        throw;
    } catch (...) {
        _clear_client_sessions();
        for (const auto& service : services_snapshot) {
            service->deactivate();
            service->reset_subscriptions();
            if (!rollback_failed) {
                service->unfreeze();
            }
        }
        _cleanup_required = rollback_failed;
        if (rollback_failed) {
            SIMPLEBLE_LOG_ERROR(
                "Failed to roll back every Windows local service; the peripheral remains frozen until stop() succeeds.");
        }
        SIMPLEBLE_LOG_ERROR("Failed to start Windows local peripheral");
        throw;
    }

    lock.unlock();
    auto weak_self = weak_from_this();
    try {
        winrt::Windows::System::Threading::ThreadPool::RunAsync(
            [weak_self, generation, services_snapshot](const winrt::Windows::Foundation::IAsyncAction&) {
                if (auto self = weak_self.lock()) {
                    self->_enable_client_callbacks(generation);
                    for (const auto& service : services_snapshot) {
                        service->enable_subscription_callbacks(generation);
                    }
                }
            });
    } catch (...) {
        const auto schedule_error = std::current_exception();
        try {
            stop();
        } catch (const std::exception& ex) {
            SIMPLEBLE_LOG_ERROR(
                fmt::format("Failed to roll back Windows local peripheral after callback scheduling failed: {}",
                            ex.what()));
        } catch (...) {
            SIMPLEBLE_LOG_ERROR(
                "Failed to roll back Windows local peripheral after callback scheduling failed");
        }
        std::rethrow_exception(schedule_error);
    }
}

void PeripheralWindows::stop() {
    std::scoped_lock lock(_lifecycle_mutex);
    if (!_started.load() && !_cleanup_required) {
        return;
    }

    _clear_client_sessions();
    for (const auto& service : _services) {
        service->deactivate();
        service->reset_subscriptions();
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
        service->unfreeze();
    }
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

void PeripheralWindows::_observe_session(const GattSession& session, uint64_t expected_generation) {
    if (!session) {
        return;
    }

    const auto [key, address] = _session_identity(session);
    uint64_t generation;
    uint64_t sequence;
    {
        std::scoped_lock lock(_clients_mutex);
        if (!_accepting_clients || (expected_generation != 0 && _client_generation != expected_generation)) {
            return;
        }
        generation = _client_generation;
        const auto existing = _client_sessions.find(key);
        if (existing != _client_sessions.end()) {
            if (existing->second.closed || winrt::get_abi(existing->second.session) != winrt::get_abi(session)) {
                _pending_client_sessions.insert_or_assign(key, PendingClientSession{session, generation});
            }
            return;
        }
        const auto callback_state = _client_callback_states.find(key);
        if (callback_state != _client_callback_states.end() &&
            (callback_state->second.disconnect_sequence != 0 ||
             callback_state->second.disconnect_callback_in_progress)) {
            _pending_client_sessions.insert_or_assign(key, PendingClientSession{session, generation});
            return;
        }
        sequence = ++_next_client_sequence;
        _client_sessions.emplace(
            key, ClientSession{session, {}, address, false, false, false, false, false, generation, sequence});
    }

    auto weak_self = weak_from_this();
    winrt::event_token token{};
    try {
        token = session.SessionStatusChanged(
            [weak_self, key, generation, sequence](const GattSession& sender,
                                                   const GattSessionStatusChangedEventArgs& args) {
                if (auto self = weak_self.lock()) {
                    self->_on_session_status_changed(key, generation, sequence, sender, args);
                }
            });
    } catch (...) {
        {
            std::scoped_lock lock(_clients_mutex);
            const auto it = _client_sessions.find(key);
            if (it != _client_sessions.end() && it->second.generation == generation &&
                it->second.sequence == sequence) {
                _client_sessions.erase(it);
            }
        }
        _promote_pending_client_session(key, generation);
        throw;
    }

    bool keep_token = false;
    bool deliver_connected = false;
    bool deliver_disconnected = false;
    bool promote_pending = false;
    {
        std::scoped_lock lock(_clients_mutex);
        const auto it = _client_sessions.find(key);
        if (_accepting_clients && _client_generation == generation && it != _client_sessions.end() &&
            it->second.generation == generation && it->second.sequence == sequence) {
            it->second.status_changed_token = token;
            keep_token = true;
            const auto status = session.SessionStatus();
            if (status == GattSessionStatus::Active && !it->second.connected) {
                it->second.connected = true;
                it->second.connect_callback_pending = true;
                deliver_connected = true;
            } else if (status == GattSessionStatus::Closed) {
                if (it->second.connect_callback_pending) {
                    it->second.closed = true;
                    it->second.status_changed_token = {};
                } else {
                    deliver_disconnected = it->second.connected_notified;
                    if (deliver_disconnected) {
                        _client_callback_states[key].disconnect_sequence = sequence;
                    }
                    _client_sessions.erase(it);
                    promote_pending = !deliver_disconnected;
                }
                keep_token = false;
            }
        }
    }

    if (!keep_token) {
        try {
            session.SessionStatusChanged(token);
        } catch (...) {
        }
    }
    if (deliver_connected) {
        _deliver_client_connected(key, generation, sequence);
    } else if (deliver_disconnected) {
        _deliver_client_disconnected(key, generation, sequence, address);
    } else if (promote_pending) {
        _promote_pending_client_session(key, generation);
    }
}

void PeripheralWindows::_on_session_status_changed(const std::string& key, uint64_t generation, uint64_t sequence,
                                                   const GattSession&,
                                                   const GattSessionStatusChangedEventArgs& args) {
    BluetoothAddress address;
    GattSession session{nullptr};
    winrt::event_token token{};
    bool deliver_connected = false;
    bool deliver_disconnected = false;
    bool promote_pending = false;
    bool revoke_token = false;
    {
        std::scoped_lock lock(_clients_mutex);
        const auto it = _client_sessions.find(key);
        if (!_accepting_clients || _client_generation != generation || it == _client_sessions.end() ||
            it->second.generation != generation || it->second.sequence != sequence) {
            return;
        }

        address = it->second.address;
        if (args.Status() == GattSessionStatus::Active && !it->second.connected) {
            it->second.connected = true;
            it->second.connect_callback_pending = true;
            deliver_connected = true;
        } else if (args.Status() == GattSessionStatus::Closed) {
            session = it->second.session;
            token = it->second.status_changed_token;
            revoke_token = static_cast<bool>(token);
            it->second.status_changed_token = {};
            if (it->second.connect_callback_pending) {
                it->second.closed = true;
            } else {
                deliver_disconnected = it->second.connected_notified;
                if (deliver_disconnected) {
                    _client_callback_states[key].disconnect_sequence = sequence;
                }
                _client_sessions.erase(it);
                promote_pending = !deliver_disconnected;
            }
        }
    }

    if (revoke_token) {
        try {
            session.SessionStatusChanged(token);
        } catch (...) {
        }
    }
    if (deliver_connected) {
        _deliver_client_connected(key, generation, sequence);
    } else if (deliver_disconnected) {
        _deliver_client_disconnected(key, generation, sequence, address);
    } else if (promote_pending) {
        _promote_pending_client_session(key, generation);
    }
}

void PeripheralWindows::_clear_client_sessions() noexcept {
    std::map<std::string, ClientSession> sessions;
    {
        std::scoped_lock lock(_clients_mutex);
        _accepting_clients = false;
        _client_callbacks_enabled = false;
        ++_client_generation;
        sessions.swap(_client_sessions);
        _client_callback_states.clear();
        _pending_client_sessions.clear();
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
    }
}

uint64_t PeripheralWindows::_active_client_generation() {
    std::scoped_lock lock(_clients_mutex);
    if (!_accepting_clients) {
        return 0;
    }
    return _client_generation;
}

bool PeripheralWindows::_client_callbacks_are_enabled(uint64_t generation) {
    std::scoped_lock lock(_clients_mutex);
    return _started.load() && _accepting_clients && _client_callbacks_enabled && _client_generation == generation;
}

void PeripheralWindows::_enable_client_callbacks(uint64_t generation) {
    std::vector<std::pair<std::string, uint64_t>> pending_connections;
    {
        std::scoped_lock lock(_clients_mutex);
        if (!_started.load() || !_accepting_clients || _client_generation != generation) {
            return;
        }
        _client_callbacks_enabled = true;
        for (const auto& [key, client] : _client_sessions) {
            if (client.generation == generation && client.connect_callback_pending) {
                pending_connections.emplace_back(key, client.sequence);
            }
        }
    }
    for (const auto& [key, sequence] : pending_connections) {
        _deliver_client_connected(key, generation, sequence);
    }
}

void PeripheralWindows::_deliver_client_connected(const std::string& key, uint64_t generation, uint64_t sequence) {
    BluetoothAddress address;
    {
        std::scoped_lock lock(_clients_mutex);
        const auto it = _client_sessions.find(key);
        if (!_started.load() || !_accepting_clients || !_client_callbacks_enabled ||
            _client_generation != generation || it == _client_sessions.end() ||
            it->second.generation != generation || it->second.sequence != sequence ||
            !it->second.connect_callback_pending || it->second.connect_callback_in_progress) {
            return;
        }
        const auto callback_state = _client_callback_states.find(key);
        if (callback_state != _client_callback_states.end() &&
            (callback_state->second.disconnect_sequence != 0 ||
             callback_state->second.disconnect_callback_in_progress)) {
            return;
        }
        address = it->second.address;
        it->second.connect_callback_in_progress = true;
    }

    SAFE_CALLBACK_CALL(_callback_on_client_connected, address);

    bool deliver_disconnected = false;
    {
        std::scoped_lock lock(_clients_mutex);
        const auto it = _client_sessions.find(key);
        if (!_accepting_clients || _client_generation != generation || it == _client_sessions.end() ||
            it->second.generation != generation || it->second.sequence != sequence ||
            !it->second.connect_callback_pending || !it->second.connect_callback_in_progress) {
            return;
        }
        it->second.connect_callback_pending = false;
        it->second.connect_callback_in_progress = false;
        it->second.connected_notified = true;
        _client_callback_states[key].connected_sequence = sequence;
        if (it->second.closed) {
            _client_sessions.erase(it);
            _client_callback_states[key].disconnect_sequence = sequence;
            deliver_disconnected = true;
        }
    }

    if (deliver_disconnected) {
        _deliver_client_disconnected(key, generation, sequence, address);
    }
}

void PeripheralWindows::_deliver_client_disconnected(const std::string& key, uint64_t generation, uint64_t sequence,
                                                      const BluetoothAddress& address) {
    {
        std::scoped_lock lock(_clients_mutex);
        const auto state = _client_callback_states.find(key);
        if (!_started.load() || !_accepting_clients || !_client_callbacks_enabled ||
            _client_generation != generation || state == _client_callback_states.end() ||
            state->second.connected_sequence != sequence || state->second.disconnect_sequence != sequence ||
            state->second.disconnect_callback_in_progress) {
            return;
        }
        state->second.disconnect_callback_in_progress = true;
    }
    SAFE_CALLBACK_CALL(_callback_on_client_disconnected, address);

    {
        std::scoped_lock lock(_clients_mutex);
        const auto state = _client_callback_states.find(key);
        if (!_accepting_clients || _client_generation != generation || state == _client_callback_states.end() ||
            state->second.disconnect_sequence != sequence) {
            return;
        }
        state->second.connected_sequence = 0;
        state->second.disconnect_sequence = 0;
        state->second.disconnect_callback_in_progress = false;

        _client_callback_states.erase(state);
    }
    _promote_pending_client_session(key, generation);
}

void PeripheralWindows::_promote_pending_client_session(const std::string& key, uint64_t generation) {
    GattSession session{nullptr};
    {
        std::scoped_lock lock(_clients_mutex);
        const auto pending = _pending_client_sessions.find(key);
        if (!_accepting_clients || _client_generation != generation || _client_sessions.count(key) != 0 ||
            pending == _pending_client_sessions.end() || pending->second.generation != generation) {
            return;
        }
        session = pending->second.session;
        _pending_client_sessions.erase(pending);
    }

    try {
        _observe_session(session, generation);
    } catch (const std::exception& ex) {
        SIMPLEBLE_LOG_WARN(fmt::format("Failed to promote a Windows local peripheral client: {}", ex.what()));
    } catch (...) {
        SIMPLEBLE_LOG_WARN("Failed to promote a Windows local peripheral client");
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
