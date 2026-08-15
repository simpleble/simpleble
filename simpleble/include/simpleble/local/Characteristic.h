#pragma once

#include <functional>
#include <memory>
#include <set>

#include <simpleble/export.h>

#include <simpleble/Exceptions.h>
#include <simpleble/Types.h>

namespace SimpleBLE::Local {

class CharacteristicBase;

enum class CharacteristicCapability {
    READ,
    WRITE_REQUEST,
    WRITE_COMMAND,
    NOTIFY,
    INDICATE,
};

class SIMPLEBLE_EXPORT Characteristic {
  public:
    Characteristic() = default;
    virtual ~Characteristic() = default;

    bool initialized() const;

    BluetoothUUID uuid();
    std::set<CharacteristicCapability> capabilities();

    /**
     * Read or update the characteristic's current value.
     *
     * Updating a characteristic with NOTIFY or INDICATE capability publishes
     * the latest value to subscribed clients on a best-effort basis.
     */
    ByteArray value();
    void set_value(ByteArray value);

    /**
     * Optional value callbacks.
     *
     * Reads return `value()` unless the read callback supplies a dynamic value.
     * Incoming writes update `value()` before the write callback is invoked.
     * Calling `set_value()` from the write callback can publish that value to
     * subscribed clients.
     */
    void set_callback_on_read(std::function<ByteArray()> on_read);
    void set_callback_on_write(std::function<void(ByteArray value)> on_write);

    /**
     * Observe whether this characteristic has any subscribed clients.
     *
     * The subscribed callback is invoked when the first client subscribes. The
     * unsubscribed callback is invoked when the last client unsubscribes.
     */
    void set_callback_on_subscribed(std::function<void()> on_subscribed);
    void set_callback_on_unsubscribed(std::function<void()> on_unsubscribed);

  protected:
    CharacteristicBase* operator->();
    const CharacteristicBase* operator->() const;

    std::shared_ptr<CharacteristicBase> internal_;
};

}  // namespace SimpleBLE::Local
