#pragma once

#include <atomic>
#include <condition_variable>
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
    using SessionObserver = std::function<void(const GattSession&)>;
    using ActivityObserver = std::function<bool()>;

    explicit ServiceWindows(BluetoothUUID uuid);
    ~ServiceWindows() override;

    BluetoothUUID uuid() override;

    std::shared_ptr<CharacteristicBase> add_characteristic(BluetoothUUID uuid,
                                                           std::set<CharacteristicCapability> capabilities) override;
    std::vector<std::shared_ptr<CharacteristicBase>> characteristics() override;

    void freeze();
    void unfreeze();
    void create_native(SessionObserver session_observer, ActivityObserver activity_observer);
    void destroy_native() noexcept;
    void start_advertising();
    void wait_until_advertising();
    void stop_advertising();
    bool is_advertising() const;

  private:
    using GattServiceProvider = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattServiceProvider;
    using AdvertisementStatus =
        winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattServiceProviderAdvertisementStatus;

    struct NativeState {
        GattServiceProvider provider{nullptr};
        ActivityObserver activity_observer;
        winrt::event_token advertisement_status_changed_token{};
        AdvertisementStatus advertisement_status{AdvertisementStatus::Created};
        winrt::Windows::Devices::Bluetooth::BluetoothError advertisement_error{
            winrt::Windows::Devices::Bluetooth::BluetoothError::Success};
        bool advertising_requested{false};
        mutable std::mutex advertisement_mutex;
        std::condition_variable advertisement_cv;

        bool active() const { return activity_observer && activity_observer(); }
    };

    BluetoothUUID _uuid;
    std::vector<std::shared_ptr<CharacteristicWindows>> _characteristics;
    std::shared_ptr<NativeState> _native;
    mutable std::mutex _mutex;
    bool _frozen{false};

    std::shared_ptr<NativeState> _native_snapshot() const;
    static void _remove_advertisement_handler(const std::shared_ptr<NativeState>& native) noexcept;
};

}  // namespace SimpleBLE::Local
