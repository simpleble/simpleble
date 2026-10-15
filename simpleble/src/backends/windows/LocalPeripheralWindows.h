#pragma once

#include <atomic>
#include <condition_variable>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <kvn_safe_callback.hpp>

#include "../common/LocalPeripheralBase.h"
#include "winrt/Windows.Devices.Bluetooth.Advertisement.h"
#include "winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h"
#include "winrt/Windows.Devices.Bluetooth.h"

namespace SimpleBLE::Local {

class ServiceWindows;

class PeripheralWindows : public PeripheralBase, public std::enable_shared_from_this<PeripheralWindows> {
  public:
    explicit PeripheralWindows(winrt::Windows::Devices::Bluetooth::BluetoothAdapter adapter);
    ~PeripheralWindows() override;

    void* underlying() const override;

    void add_advertised_service(BluetoothUUID service_uuid) override;
    void add_advertised_service(std::vector<BluetoothUUID> service_uuids) override;
    void set_advertised_manufacturer_data(uint16_t company_id, ByteArray data) override;
    void set_connectable(bool connectable) override;

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
        GattSession session{nullptr};
        winrt::event_token status_changed_token{};
        BluetoothAddress address;
        bool connected{false};

        void close() noexcept;
    };

    struct RunState {
        std::shared_ptr<std::atomic_bool> active{std::make_shared<std::atomic_bool>(true)};
        std::map<std::string, ClientSession> client_sessions;
        std::mutex clients_mutex;
    };

    // NOTE: GATT service providers can't advertise manufacturer data, so a peripheral without services advertises it
    // through a publisher instead. Publisher advertisements aren't connectable.
    struct Publisher {
        winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementPublisher publisher{nullptr};
        winrt::event_token status_changed_token{};
        std::mutex mutex;
        std::condition_variable cv;
        winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementPublisherStatus status{
            winrt::Windows::Devices::Bluetooth::Advertisement::BluetoothLEAdvertisementPublisherStatus::Created};
        winrt::Windows::Devices::Bluetooth::BluetoothError error{
            winrt::Windows::Devices::Bluetooth::BluetoothError::Success};
    };

    winrt::Windows::Devices::Bluetooth::BluetoothAdapter _adapter{nullptr};
    std::vector<BluetoothUUID> _advertised_service_uuids;
    std::map<uint16_t, ByteArray> _advertised_manufacturer_data;
    bool _connectable = true;
    std::shared_ptr<Publisher> _publisher;
    std::vector<std::shared_ptr<ServiceWindows>> _services;
    std::shared_ptr<RunState> _run;
    std::atomic_bool _started{false};
    mutable std::mutex _lifecycle_mutex;

    kvn::safe_callback<void(BluetoothAddress)> _callback_on_client_connected;
    kvn::safe_callback<void(BluetoothAddress)> _callback_on_client_disconnected;

    void _ensure_mutable() const;
    void _start_publisher();
    void _observe_session(const std::shared_ptr<RunState>& run, const GattSession& session);
    void _on_session_status_changed(const std::shared_ptr<RunState>& run, const std::string& key,
                                    const GattSession& sender, const GattSessionStatusChangedEventArgs& args);
    std::exception_ptr _stop_advertising() noexcept;
    void _clear_client_sessions(const std::shared_ptr<RunState>& run) noexcept;
    static std::pair<std::string, BluetoothAddress> _session_identity(const GattSession& session);
    static bool _is_session_closed(const GattSession& session) noexcept;
};

}  // namespace SimpleBLE::Local
