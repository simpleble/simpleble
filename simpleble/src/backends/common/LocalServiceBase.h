#pragma once

#include <memory>
#include <vector>

#include <simpleble/Types.h>
#include <simpleble/local/Characteristic.h>

namespace SimpleBLE::Local {

class CharacteristicBase;

class ServiceBase {
  public:
    virtual ~ServiceBase() = default;

    virtual BluetoothUUID uuid() = 0;

    virtual std::shared_ptr<CharacteristicBase> add_characteristic(
        BluetoothUUID uuid, std::vector<CharacteristicCapability> capabilities) = 0;
    virtual std::vector<std::shared_ptr<CharacteristicBase>> characteristics() = 0;

  protected:
    ServiceBase() = default;
};

}  // namespace SimpleBLE::Local
