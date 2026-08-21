#include "LocalServiceAndroid.h"

#include <utility>

#include <simpleble/Exceptions.h>

#include "LocalCharacteristicAndroid.h"
#include "LocalPeripheralAndroid.h"

namespace SimpleBLE::Local {

ServiceAndroid::ServiceAndroid(std::weak_ptr<PeripheralAndroid> peripheral, BluetoothUUID uuid)
    : _peripheral(std::move(peripheral)), _service(uuid), _uuid(std::move(uuid)) {}

BluetoothUUID ServiceAndroid::uuid() { return _uuid; }

std::shared_ptr<CharacteristicBase> ServiceAndroid::add_characteristic(
    BluetoothUUID uuid, std::set<CharacteristicCapability> capabilities) {
    std::scoped_lock lock(_mutex);
    if (_frozen) {
        throw Exception::OperationFailed("The local service cannot be changed while its peripheral is started.");
    }
    auto peripheral = _peripheral.lock();
    if (!peripheral) throw Exception::InvalidReference();

    auto characteristic = std::make_shared<CharacteristicAndroid>(peripheral, std::move(uuid), std::move(capabilities));
    if (!_service.addCharacteristic(characteristic->native_characteristic())) {
        throw Exception::OperationFailed("Android failed to add a local characteristic.");
    }
    _characteristics.push_back(characteristic);
    peripheral->register_characteristic(characteristic);
    return characteristic;
}

std::vector<std::shared_ptr<CharacteristicBase>> ServiceAndroid::characteristics() {
    std::scoped_lock lock(_mutex);
    return {_characteristics.begin(), _characteristics.end()};
}

Android::BluetoothGattService ServiceAndroid::native_service() const { return _service; }

void ServiceAndroid::freeze() {
    std::scoped_lock lock(_mutex);
    _frozen = true;
}

void ServiceAndroid::unfreeze() {
    std::scoped_lock lock(_mutex);
    _frozen = false;
}

}  // namespace SimpleBLE::Local
