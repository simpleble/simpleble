#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "../common/LocalServiceBase.h"
#include "types/android/bluetooth/BluetoothGattService.h"

namespace SimpleBLE::Local {

class CharacteristicAndroid;
class PeripheralAndroid;

class ServiceAndroid : public ServiceBase {
  public:
    ServiceAndroid(std::weak_ptr<PeripheralAndroid> peripheral, BluetoothUUID uuid);
    ~ServiceAndroid() override = default;

    BluetoothUUID uuid() override;
    std::shared_ptr<CharacteristicBase> add_characteristic(BluetoothUUID uuid,
                                                           std::set<CharacteristicCapability> capabilities) override;
    std::vector<std::shared_ptr<CharacteristicBase>> characteristics() override;

    Android::BluetoothGattService native_service() const;
    void freeze();
    void unfreeze();

  private:
    std::weak_ptr<PeripheralAndroid> _peripheral;
    Android::BluetoothGattService _service;
    BluetoothUUID _uuid;
    std::vector<std::shared_ptr<CharacteristicAndroid>> _characteristics;
    std::mutex _mutex;
    bool _frozen{false};
};

}  // namespace SimpleBLE::Local
