#include "LocalServiceMac.h"

#include <utility>

#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

#include <simpleble/Exceptions.h>

#import "LocalCharacteristicMac.h"
#import "LocalPeripheralMac.h"
#import "Utils.h"

namespace SimpleBLE::Local {

ServiceMac::ServiceMac(std::weak_ptr<PeripheralMac> peripheral, BluetoothUUID uuid)
    : _peripheral(std::move(peripheral)), _uuid(std::move(uuid)) {
    auto* service = [[CBMutableService alloc] initWithType:uuidFromSimpleBLE(_uuid) primary:YES];
    _opaque_service = (__bridge_retained void*)service;
}

ServiceMac::~ServiceMac() {
    CBMutableService* service = (__bridge_transfer CBMutableService*)_opaque_service;
    service = nil;
}

BluetoothUUID ServiceMac::uuid() { return _uuid; }

std::shared_ptr<CharacteristicBase> ServiceMac::add_characteristic(BluetoothUUID uuid, std::set<CharacteristicCapability> capabilities) {
    std::scoped_lock lock(_mutex);
    if (_frozen) {
        throw Exception::OperationFailed("The local service cannot be changed while its peripheral is started.");
    }

    auto peripheral = _peripheral.lock();
    if (!peripheral) {
        throw Exception::InvalidReference();
    }

    auto characteristic = std::make_shared<CharacteristicMac>(peripheral, std::move(uuid), std::move(capabilities));
    _characteristics.push_back(characteristic);

    NSMutableArray<CBMutableCharacteristic*>* nativeCharacteristics = [NSMutableArray arrayWithCapacity:_characteristics.size()];
    for (const auto& item : _characteristics) {
        [nativeCharacteristics addObject:(__bridge CBMutableCharacteristic*)item->underlying()];
    }
    ((__bridge CBMutableService*)_opaque_service).characteristics = nativeCharacteristics;
    peripheral->register_characteristic(characteristic);
    return characteristic;
}

std::vector<std::shared_ptr<CharacteristicBase>> ServiceMac::characteristics() {
    std::scoped_lock lock(_mutex);
    return {_characteristics.begin(), _characteristics.end()};
}

void* ServiceMac::underlying() const { return _opaque_service; }

void ServiceMac::freeze() {
    std::scoped_lock lock(_mutex);
    _frozen = true;
}

void ServiceMac::unfreeze() {
    std::scoped_lock lock(_mutex);
    _frozen = false;
}

}  // namespace SimpleBLE::Local
