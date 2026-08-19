#pragma once

#include <atomic>
#include <cstddef>
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
    using SessionObserver = std::function<void(const GattSession&, uint64_t expected_generation)>;
    using ActivityObserver = std::function<uint64_t()>;

    CharacteristicWindows(const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalService& service,
                          BluetoothUUID uuid, std::set<CharacteristicCapability> capabilities,
                          SessionObserver session_observer, ActivityObserver activity_observer);
    ~CharacteristicWindows() override;

    void initialize_handlers();

    BluetoothUUID uuid() override;
    std::set<CharacteristicCapability> capabilities() override;

    ByteArray value() override;
    void set_value(ByteArray value) override;

    void set_callback_on_read(std::function<ByteArray()> on_read) override;
    void set_callback_on_write(std::function<void(ByteArray value)> on_write) override;

    void set_callback_on_subscribed(std::function<void()> on_subscribed) override;
    void set_callback_on_unsubscribed(std::function<void()> on_unsubscribed) override;

    void reset_subscriptions();

  private:
    using GattLocalCharacteristic =
        winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalCharacteristic;
    using GattReadRequestedEventArgs =
        winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattReadRequestedEventArgs;
    using GattWriteRequestedEventArgs =
        winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattWriteRequestedEventArgs;

    GattLocalCharacteristic _characteristic{nullptr};
    BluetoothUUID _uuid;
    std::set<CharacteristicCapability> _capabilities;
    SessionObserver _session_observer;
    ActivityObserver _activity_observer;

    ByteArray _value;
    std::mutex _value_mutex;
    std::atomic_bool _subscribed{false};

    winrt::event_token _read_requested_token_{};
    winrt::event_token _write_requested_token_{};
    winrt::event_token _subscribed_clients_changed_token_{};

    kvn::safe_callback<ByteArray()> _callback_on_read;
    kvn::safe_callback<void(ByteArray)> _callback_on_write;
    kvn::safe_callback<void()> _callback_on_subscribed;
    kvn::safe_callback<void()> _callback_on_unsubscribed;

    void _on_read_requested(const GattLocalCharacteristic& sender, const GattReadRequestedEventArgs& args);
    void _on_write_requested(const GattLocalCharacteristic& sender, const GattWriteRequestedEventArgs& args);
    void _on_subscribed_clients_changed(const GattLocalCharacteristic& sender,
                                        const winrt::Windows::Foundation::IInspectable& args);
};

}  // namespace SimpleBLE::Local
