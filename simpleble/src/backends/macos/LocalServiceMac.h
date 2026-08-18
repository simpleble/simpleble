#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "../common/LocalServiceBase.h"

namespace SimpleBLE::Local {

class CharacteristicMac;
class PeripheralMac;

class ServiceMac : public ServiceBase {
  public:
    ServiceMac(std::weak_ptr<PeripheralMac> peripheral, BluetoothUUID uuid);
    ~ServiceMac() override;

    BluetoothUUID uuid() override;
    std::shared_ptr<CharacteristicBase> add_characteristic(BluetoothUUID uuid,
                                                           std::set<CharacteristicCapability> capabilities) override;
    std::vector<std::shared_ptr<CharacteristicBase>> characteristics() override;

    void* underlying() const;
    void freeze();
    void unfreeze();

  private:
    std::weak_ptr<PeripheralMac> _peripheral;
    void* _opaque_service;
    BluetoothUUID _uuid;
    std::vector<std::shared_ptr<CharacteristicMac>> _characteristics;
    std::mutex _mutex;
    bool _frozen{false};
};

}  // namespace SimpleBLE::Local
