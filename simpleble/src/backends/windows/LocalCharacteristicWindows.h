#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <set>

#include <kvn_safe_callback.hpp>

#include "../common/LocalCharacteristicBase.h"
#include "winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h"
#include "winrt/Windows.Foundation.Collections.h"

namespace SimpleBLE::Local {

class CharacteristicWindows : public CharacteristicBase, public std::enable_shared_from_this<CharacteristicWindows> {
  public:
    using GattSession = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattSession;
    using SessionObserver = std::function<void(const GattSession&)>;
    using ActivityObserver = std::function<bool()>;

    CharacteristicWindows(BluetoothUUID uuid, std::set<CharacteristicCapability> capabilities);
    ~CharacteristicWindows() override;

    void start_native(const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalService& service,
                      SessionObserver session_observer, ActivityObserver activity_observer);
    void stop_native() noexcept;

    BluetoothUUID uuid() override;
    std::set<CharacteristicCapability> capabilities() override;

    ByteArray value() override;
    void set_value(ByteArray value) override;

    void set_callback_on_read(std::function<ByteArray()> on_read) override;
    void set_callback_on_write(std::function<void(ByteArray value)> on_write) override;

    void set_callback_on_subscribed(std::function<void()> on_subscribed) override;
    void set_callback_on_unsubscribed(std::function<void()> on_unsubscribed) override;

  private:
    using GattLocalCharacteristic =
        winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalCharacteristic;
    using GattReadRequestedEventArgs =
        winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattReadRequestedEventArgs;
    using GattWriteRequestedEventArgs =
        winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattWriteRequestedEventArgs;

    struct NativeState {
        GattLocalCharacteristic characteristic{nullptr};
        SessionObserver session_observer;
        ActivityObserver activity_observer;
        std::atomic_bool subscribed{false};
        winrt::event_token read_requested_token{};
        winrt::event_token write_requested_token{};
        winrt::event_token subscribed_clients_changed_token{};

        bool active() const { return activity_observer && activity_observer(); }
    };

    BluetoothUUID _uuid;
    std::set<CharacteristicCapability> _capabilities;

    ByteArray _value;
    std::mutex _value_mutex;
    std::shared_ptr<NativeState> _native;
    std::mutex _native_mutex;

    kvn::safe_callback<ByteArray()> _callback_on_read;
    kvn::safe_callback<void(ByteArray)> _callback_on_write;
    kvn::safe_callback<void()> _callback_on_subscribed;
    kvn::safe_callback<void()> _callback_on_unsubscribed;

    std::shared_ptr<NativeState> _native_snapshot();
    static void _revoke_handlers(const std::shared_ptr<NativeState>& native) noexcept;
    void _on_read_requested(const std::shared_ptr<NativeState>& native, const GattReadRequestedEventArgs& args);
    void _on_write_requested(const std::shared_ptr<NativeState>& native, const GattWriteRequestedEventArgs& args);
    void _on_subscribed_clients_changed(const std::shared_ptr<NativeState>& native);
};

}  // namespace SimpleBLE::Local
