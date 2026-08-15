#pragma once

#include <functional>
#include <set>

#include <simpleble/Types.h>
#include <simpleble/local/Characteristic.h>

namespace SimpleBLE::Local {

class CharacteristicBase {
  public:
    virtual ~CharacteristicBase() = default;

    virtual BluetoothUUID uuid() = 0;
    virtual std::set<CharacteristicCapability> capabilities() = 0;

    virtual ByteArray value() = 0;
    virtual void set_value(ByteArray value) = 0;

    virtual void set_callback_on_read(std::function<ByteArray()> on_read) = 0;
    virtual void set_callback_on_write(std::function<void(ByteArray value)> on_write) = 0;

    virtual void set_callback_on_subscribed(std::function<void()> on_subscribed) = 0;
    virtual void set_callback_on_unsubscribed(std::function<void()> on_unsubscribed) = 0;

  protected:
    CharacteristicBase() = default;
};

}  // namespace SimpleBLE::Local
