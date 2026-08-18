#pragma once

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
    using CallbackObserver = std::function<bool(uint64_t)>;

    CharacteristicWindows(const winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattLocalService& service,
                          BluetoothUUID uuid, std::set<CharacteristicCapability> capabilities,
                          SessionObserver session_observer, ActivityObserver activity_observer,
                          CallbackObserver callback_observer);
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
    void reconcile_subscriptions(uint64_t generation) noexcept;
    void enable_subscription_callbacks(uint64_t generation);

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
    CallbackObserver _callback_observer;

    ByteArray _value;
    std::mutex _value_mutex;
    size_t _subscribed_clients{0};
    uint64_t _subscription_generation{0};
    bool _subscription_callback_delivered{false};
    bool _subscription_callback_in_progress{false};
    uint64_t _subscription_transition_sequence{0};
    uint64_t _subscription_delivery_generation{0};
    uint64_t _subscription_delivery_sequence{0};
    std::mutex _subscription_mutex;

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
                                        const winrt::Windows::Foundation::IInspectable& args,
                                        uint64_t expected_generation = 0);
    void _deliver_subscription_callback(uint64_t generation, uint64_t sequence, bool subscribed);
};

}  // namespace SimpleBLE::Local
