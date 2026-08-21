#pragma once

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <vector>

#include <kvn_safe_callback.hpp>

#include "../common/LocalCharacteristicBase.h"
#include "types/android/bluetooth/BluetoothDevice.h"
#include "types/android/bluetooth/BluetoothGattCharacteristic.h"
#include "types/android/bluetooth/BluetoothGattDescriptor.h"

namespace SimpleBLE::Local {

class PeripheralAndroid;

class CharacteristicAndroid : public CharacteristicBase, public std::enable_shared_from_this<CharacteristicAndroid> {
  public:
    struct Subscriber {
        Android::BluetoothDevice device;
        bool confirm;
    };

    CharacteristicAndroid(std::weak_ptr<PeripheralAndroid> peripheral, BluetoothUUID uuid,
                          std::set<CharacteristicCapability> capabilities);
    ~CharacteristicAndroid() override;

    BluetoothUUID uuid() override;
    std::set<CharacteristicCapability> capabilities() override;
    ByteArray value() override;
    void set_value(ByteArray value) override;
    void set_callback_on_read(std::function<ByteArray()> on_read) override;
    void set_callback_on_write(std::function<void(ByteArray value)> on_write) override;
    void set_callback_on_subscribed(std::function<void()> on_subscribed) override;
    void set_callback_on_unsubscribed(std::function<void()> on_unsubscribed) override;

    Android::BluetoothGattCharacteristic native_characteristic() const;
    std::optional<Android::BluetoothGattDescriptor> native_cccd() const;

    bool can_read() const;
    bool can_write_request() const;
    bool can_write_command() const;
    ByteArray handle_read();
    void handle_write(ByteArray value);
    bool set_subscription(const Android::BluetoothDevice& device, const ByteArray& value);
    ByteArray subscription_value(const BluetoothAddress& address) const;
    std::vector<Subscriber> subscribers() const;
    void remove_subscriber(const BluetoothAddress& address);
    void clear_subscribers();

  private:
    std::weak_ptr<PeripheralAndroid> _peripheral;
    Android::BluetoothGattCharacteristic _characteristic;
    std::optional<Android::BluetoothGattDescriptor> _cccd;
    BluetoothUUID _uuid;
    std::set<CharacteristicCapability> _capabilities;
    ByteArray _value;
    std::map<BluetoothAddress, Subscriber> _subscribers;
    mutable std::mutex _mutex;

    kvn::safe_callback<ByteArray()> _callback_on_read;
    kvn::safe_callback<void(ByteArray)> _callback_on_write;
    kvn::safe_callback<void()> _callback_on_subscribed;
    kvn::safe_callback<void()> _callback_on_unsubscribed;
};

}  // namespace SimpleBLE::Local
