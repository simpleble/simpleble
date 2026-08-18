#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <kvn_safe_callback.hpp>

#include "../common/LocalPeripheralBase.h"
#include "winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h"
#include "winrt/Windows.Devices.Bluetooth.h"

namespace SimpleBLE::Local {

class ServiceWindows;

class PeripheralWindows : public PeripheralBase, public std::enable_shared_from_this<PeripheralWindows> {
  public:
    explicit PeripheralWindows(winrt::Windows::Devices::Bluetooth::BluetoothAdapter adapter);
    ~PeripheralWindows() override;

    void* underlying() const override;

    Advertisement advertisement() override;
    void set_advertisement(Advertisement advertisement) override;

    std::shared_ptr<ServiceBase> add_service(BluetoothUUID uuid) override;
    std::vector<std::shared_ptr<ServiceBase>> services() override;
    void remove_all_services() override;

    void start() override;
    void stop() override;

    bool is_started() override;
    bool is_advertising() override;

    void set_callback_on_client_connected(
        std::function<void(BluetoothAddress client_address)> on_client_connected) override;
    void set_callback_on_client_disconnected(
        std::function<void(BluetoothAddress client_address)> on_client_disconnected) override;

  private:
    using GattSession = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSession;
    using GattSessionStatusChangedEventArgs =
        winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSessionStatusChangedEventArgs;

    struct ClientSession {
        GattSession session;
        winrt::event_token status_changed_token;
        BluetoothAddress address;
        bool connected{false};
        bool connect_callback_pending{false};
        bool connect_callback_in_progress{false};
        bool connected_notified{false};
        bool closed{false};
        uint64_t generation;
        uint64_t sequence;
    };

    struct ClientCallbackState {
        uint64_t connected_sequence{0};
        uint64_t disconnect_sequence{0};
        bool disconnect_callback_in_progress{false};
    };

    struct PendingClientSession {
        GattSession session;
        uint64_t generation;
    };

    winrt::Windows::Devices::Bluetooth::BluetoothAdapter _adapter{nullptr};
    Advertisement _advertisement;
    std::vector<std::shared_ptr<ServiceWindows>> _services;
    std::atomic_bool _started{false};
    bool _cleanup_required{false};
    mutable std::mutex _lifecycle_mutex;

    std::map<std::string, ClientSession> _client_sessions;
    std::map<std::string, ClientCallbackState> _client_callback_states;
    std::map<std::string, PendingClientSession> _pending_client_sessions;
    std::mutex _clients_mutex;
    uint64_t _client_generation{0};
    uint64_t _next_client_sequence{0};
    bool _accepting_clients{false};
    bool _client_callbacks_enabled{false};
    kvn::safe_callback<void(BluetoothAddress)> _callback_on_client_connected;
    kvn::safe_callback<void(BluetoothAddress)> _callback_on_client_disconnected;

    void _ensure_mutable() const;
    void _observe_session(const GattSession& session, uint64_t expected_generation);
    void _on_session_status_changed(const std::string& key, uint64_t generation, uint64_t sequence,
                                    const GattSession& sender, const GattSessionStatusChangedEventArgs& args);
    void _clear_client_sessions() noexcept;
    uint64_t _active_client_generation();
    bool _client_callbacks_are_enabled(uint64_t generation);
    void _enable_client_callbacks(uint64_t generation);
    void _deliver_client_connected(const std::string& key, uint64_t generation, uint64_t sequence);
    void _deliver_client_disconnected(const std::string& key, uint64_t generation, uint64_t sequence,
                                      const BluetoothAddress& address);
    void _promote_pending_client_session(const std::string& key, uint64_t generation);
    static std::pair<std::string, BluetoothAddress> _session_identity(const GattSession& session);
};

}  // namespace SimpleBLE::Local
