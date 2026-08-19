#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "../common/LocalServiceBase.h"
#include "winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h"

namespace SimpleBLE::Local {

class CharacteristicWindows;

class ServiceWindows : public ServiceBase, public std::enable_shared_from_this<ServiceWindows> {
  public:
    using GattSession = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSession;
    using SessionObserver = std::function<void(const GattSession&, uint64_t expected_generation)>;
    using ActivityObserver = std::function<uint64_t()>;

    ServiceWindows(BluetoothUUID uuid, SessionObserver session_observer, ActivityObserver activity_observer);
    ~ServiceWindows() override;

    BluetoothUUID uuid() override;

    std::shared_ptr<CharacteristicBase> add_characteristic(BluetoothUUID uuid,
                                                           std::set<CharacteristicCapability> capabilities) override;
    std::vector<std::shared_ptr<CharacteristicBase>> characteristics() override;

    void freeze();
    void unfreeze();
    void activate(uint64_t generation);
    void deactivate() noexcept;
    void start_advertising(uint64_t generation);
    void wait_until_advertising(uint64_t generation);
    void stop_advertising();
    void reset_subscriptions();
    bool is_advertising() const;

  private:
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattServiceProvider _provider{nullptr};
    BluetoothUUID _uuid;
    SessionObserver _session_observer;
    ActivityObserver _activity_observer;
    std::atomic_uint64_t _active_generation{0};
    std::vector<std::shared_ptr<CharacteristicWindows>> _characteristics;
    mutable std::mutex _mutex;
    bool _frozen{false};

    winrt::event_token _advertisement_status_changed_token{};
    winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattServiceProviderAdvertisementStatus
        _advertisement_status{winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::
                                  GattServiceProviderAdvertisementStatus::Created};
    winrt::Windows::Devices::Bluetooth::BluetoothError _advertisement_error{
        winrt::Windows::Devices::Bluetooth::BluetoothError::Success};
    uint64_t _advertising_generation{0};
    uint64_t _advertisement_status_generation{0};
    bool _advertising_requested{false};
    mutable std::mutex _advertisement_mutex;
    std::condition_variable _advertisement_cv;

    uint64_t _current_generation();
    void _install_advertisement_handler(uint64_t generation);
    void _remove_advertisement_handler() noexcept;
};

}  // namespace SimpleBLE::Local
