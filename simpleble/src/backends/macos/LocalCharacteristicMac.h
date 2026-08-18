#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <set>

#include <kvn_safe_callback.hpp>

#include "../common/LocalCharacteristicBase.h"

namespace SimpleBLE::Local {

class PeripheralMac;

class CharacteristicMac : public CharacteristicBase {
  public:
    CharacteristicMac(std::weak_ptr<PeripheralMac> peripheral, BluetoothUUID uuid,
                      std::set<CharacteristicCapability> capabilities);
    ~CharacteristicMac() override;

    BluetoothUUID uuid() override;
    std::set<CharacteristicCapability> capabilities() override;

    ByteArray value() override;
    void set_value(ByteArray value) override;

    void set_callback_on_read(std::function<ByteArray()> on_read) override;
    void set_callback_on_write(std::function<void(ByteArray value)> on_write) override;
    void set_callback_on_subscribed(std::function<void()> on_subscribed) override;
    void set_callback_on_unsubscribed(std::function<void()> on_unsubscribed) override;

    void* underlying() const;
    bool can_read() const;
    bool can_write() const;
    ByteArray handle_read();
    void handle_write(ByteArray value);
    bool handle_subscribed(const BluetoothAddress& client_address);
    bool handle_unsubscribed(const BluetoothAddress& client_address);
    void clear_subscribers();

  private:
    std::weak_ptr<PeripheralMac> _peripheral;
    void* _opaque_characteristic;
    BluetoothUUID _uuid;
    std::set<CharacteristicCapability> _capabilities;
    ByteArray _value;
    std::set<BluetoothAddress> _subscribers;
    mutable std::mutex _mutex;

    kvn::safe_callback<ByteArray()> _callback_on_read;
    kvn::safe_callback<void(ByteArray)> _callback_on_write;
    kvn::safe_callback<void()> _callback_on_subscribed;
    kvn::safe_callback<void()> _callback_on_unsubscribed;
};

}  // namespace SimpleBLE::Local
