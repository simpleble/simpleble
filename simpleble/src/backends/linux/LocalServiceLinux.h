#pragma once

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include <simplebluez/standard/Service.h>

#include "../common/LocalServiceBase.h"

namespace SimpleBLE::Local {

class CharacteristicLinux;

class ServiceLinux : public ServiceBase {
  public:
    ServiceLinux(std::shared_ptr<SimpleBluez::Service> service, std::string name, BluetoothUUID uuid);
    ~ServiceLinux() override = default;

    BluetoothUUID uuid() override;

    std::shared_ptr<CharacteristicBase> add_characteristic(BluetoothUUID uuid,
                                                           std::set<CharacteristicCapability> capabilities) override;
    std::vector<std::shared_ptr<CharacteristicBase>> characteristics() override;

    const std::string& name() const;
    void freeze();
    void unfreeze();

  private:
    std::shared_ptr<SimpleBluez::Service> _service;
    std::string _name;
    BluetoothUUID _uuid;
    std::vector<std::shared_ptr<CharacteristicLinux>> _characteristics;
    std::mutex _mutex;
    size_t _next_characteristic_id{0};
    bool _frozen{false};
};

}  // namespace SimpleBLE::Local
