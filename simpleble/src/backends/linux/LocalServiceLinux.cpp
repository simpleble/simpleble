#include "LocalServiceLinux.h"

#include <utility>

#include <simpleble/Exceptions.h>

#include "LocalCharacteristicLinux.h"

namespace SimpleBLE::Local {

ServiceLinux::ServiceLinux(std::shared_ptr<SimpleBluez::Service> service, std::string name, BluetoothUUID uuid)
    : _service(std::move(service)), _name(std::move(name)), _uuid(std::move(uuid)) {
    _service->uuid(_uuid);
    _service->primary(true);
}

BluetoothUUID ServiceLinux::uuid() { return _uuid; }

std::shared_ptr<CharacteristicBase> ServiceLinux::add_characteristic(
    BluetoothUUID uuid, std::set<CharacteristicCapability> capabilities) {
    std::scoped_lock lock(_mutex);
    if (_frozen) {
        throw Exception::OperationFailed("The local service cannot be changed while its peripheral is started.");
    }

    const auto name = std::to_string(_next_characteristic_id++);
    auto bluez_characteristic = _service->characteristic_add(name);
    try {
        auto characteristic = std::make_shared<CharacteristicLinux>(bluez_characteristic, std::move(uuid),
                                                                    std::move(capabilities));
        _characteristics.push_back(characteristic);
        return characteristic;
    } catch (...) {
        _service->characteristic_remove(name);
        throw;
    }
}

std::vector<std::shared_ptr<CharacteristicBase>> ServiceLinux::characteristics() {
    std::scoped_lock lock(_mutex);
    return {_characteristics.begin(), _characteristics.end()};
}

const std::string& ServiceLinux::name() const { return _name; }

void ServiceLinux::freeze() {
    std::scoped_lock lock(_mutex);
    _frozen = true;
}

void ServiceLinux::unfreeze() {
    std::scoped_lock lock(_mutex);
    _frozen = false;
}

}  // namespace SimpleBLE::Local
