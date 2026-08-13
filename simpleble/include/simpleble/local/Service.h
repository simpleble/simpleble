#pragma once

#include <memory>
#include <vector>

#include <simpleble/export.h>

#include <simpleble/Exceptions.h>
#include <simpleble/Types.h>
#include <simpleble/local/Characteristic.h>

namespace SimpleBLE::Local {

class ServiceBase;

class SIMPLEBLE_EXPORT Service {
  public:
    Service() = default;
    virtual ~Service() = default;

    bool initialized() const;

    BluetoothUUID uuid();

    Characteristic add_characteristic(BluetoothUUID uuid, std::vector<CharacteristicCapability> capabilities);
    std::vector<Characteristic> characteristics();

  protected:
    ServiceBase* operator->();
    const ServiceBase* operator->() const;

    std::shared_ptr<ServiceBase> internal_;
};

}  // namespace SimpleBLE::Local
