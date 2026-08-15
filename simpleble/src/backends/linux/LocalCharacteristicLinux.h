#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <simplebluez/standard/Characteristic.h>
#include <kvn_safe_callback.hpp>

#include "../common/LocalCharacteristicBase.h"

namespace SimpleBLE::Local {

class CharacteristicLinux : public CharacteristicBase {
  public:
    CharacteristicLinux(std::shared_ptr<SimpleBluez::Characteristic> characteristic, BluetoothUUID uuid,
                        std::set<CharacteristicCapability> capabilities);
    ~CharacteristicLinux() override;

    BluetoothUUID uuid() override;
    std::set<CharacteristicCapability> capabilities() override;

    ByteArray value() override;
    void set_value(ByteArray value) override;

    void set_callback_on_read(std::function<ByteArray()> on_read) override;
    void set_callback_on_write(std::function<void(ByteArray value)> on_write) override;

    void set_callback_on_subscribed(std::function<void()> on_subscribed) override;
    void set_callback_on_unsubscribed(std::function<void()> on_unsubscribed) override;

  private:
    std::shared_ptr<SimpleBluez::Characteristic> _characteristic;
    BluetoothUUID _uuid;
    std::set<CharacteristicCapability> _capabilities;
    std::atomic_bool _subscribed{false};

    kvn::safe_callback<ByteArray()> _callback_on_read;
    kvn::safe_callback<void(ByteArray)> _callback_on_write;
    kvn::safe_callback<void()> _callback_on_subscribed;
    kvn::safe_callback<void()> _callback_on_unsubscribed;

    static std::vector<std::string> _flags_from_capabilities(const std::set<CharacteristicCapability>& capabilities);
};

}  // namespace SimpleBLE::Local
