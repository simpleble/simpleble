#include "LocalPeripheralMac.h"

#include <exception>
#include <utility>

#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

#import "CommonUtils.h"
#import "LocalCharacteristicMac.h"
#import "LocalPeripheralBaseMacOS.h"
#import "LocalServiceMac.h"
#import "Utils.h"

namespace {

NSData* dataFromByteArray(const SimpleBLE::ByteArray& value) {
    return [NSData dataWithBytes:value.empty() ? nullptr : value.data() length:value.size()];
}

}  // namespace

namespace SimpleBLE::Local {

PeripheralMac::PeripheralMac() { _opaque_internal = (__bridge_retained void*)[[LocalPeripheralBaseMacOS alloc] init:this]; }

PeripheralMac::~PeripheralMac() {
    _callback_on_client_connected.unload();
    _callback_on_client_disconnected.unload();

    try {
        stop();
    } catch (const std::exception& ex) {
        SIMPLEBLE_LOG_WARN(fmt::format("Failed to stop local peripheral during cleanup: {}", ex.what()));
    } catch (...) {
        SIMPLEBLE_LOG_WARN("Failed to stop local peripheral during cleanup");
    }

    LocalPeripheralBaseMacOS* internal = (__bridge LocalPeripheralBaseMacOS*)_opaque_internal;
    [internal detach];
    internal = (__bridge_transfer LocalPeripheralBaseMacOS*)_opaque_internal;
    internal = nil;
}

void* PeripheralMac::underlying() const {
    LocalPeripheralBaseMacOS* internal = (__bridge LocalPeripheralBaseMacOS*)_opaque_internal;
    return [internal underlying];
}

Advertisement PeripheralMac::advertisement() {
    std::scoped_lock lock(_lifecycle_mutex);
    return _advertisement;
}

void PeripheralMac::set_advertisement(Advertisement advertisement) {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();
    _advertisement = std::move(advertisement);
}

std::shared_ptr<ServiceBase> PeripheralMac::add_service(BluetoothUUID uuid) {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();
    auto service = std::make_shared<ServiceMac>(shared_from_this(), std::move(uuid));
    _services.push_back(service);
    return service;
}

std::vector<std::shared_ptr<ServiceBase>> PeripheralMac::services() {
    std::scoped_lock lock(_lifecycle_mutex);
    return {_services.begin(), _services.end()};
}

void PeripheralMac::remove_all_services() {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();
    _services.clear();
    std::scoped_lock characteristicsLock(_characteristics_mutex);
    _characteristics.clear();
}

void PeripheralMac::start() {
    std::scoped_lock lock(_lifecycle_mutex);
    if (_started.load()) {
        return;
    }

    NSMutableArray<CBMutableService*>* nativeServices = [NSMutableArray arrayWithCapacity:_services.size()];
    for (const auto& service : _services) {
        [nativeServices addObject:(__bridge CBMutableService*)service->underlying()];
    }

    NSMutableArray<CBUUID*>* serviceUuids = [NSMutableArray array];
    const auto& advertisedUuids = _advertisement.service_uuids;
    if (advertisedUuids.empty()) {
        for (const auto& service : _services) {
            [serviceUuids addObject:uuidFromSimpleBLE(service->uuid())];
        }
    } else {
        for (const auto& uuid : advertisedUuids) {
            [serviceUuids addObject:uuidFromSimpleBLE(uuid)];
        }
    }

    NSMutableDictionary<NSString*, id>* advertisementData = [NSMutableDictionary dictionary];
    if (_advertisement.local_name.has_value()) {
        const auto& localName = *_advertisement.local_name;
        NSString* nativeName = [[NSString alloc] initWithBytes:localName.data() length:localName.size() encoding:NSUTF8StringEncoding];
        if (nativeName == nil) {
            throw Exception::OperationFailed("The local peripheral name is not valid UTF-8.");
        }
        advertisementData[CBAdvertisementDataLocalNameKey] = nativeName;
    }
    if (serviceUuids.count > 0) {
        advertisementData[CBAdvertisementDataServiceUUIDsKey] = serviceUuids;
    }

    for (const auto& service : _services) {
        service->freeze();
    }

    LocalPeripheralBaseMacOS* internal = (__bridge LocalPeripheralBaseMacOS*)_opaque_internal;
    NSString* error = [internal startWithServices:nativeServices advertisementData:advertisementData];
    if (error != nil) {
        for (const auto& service : _services) {
            service->unfreeze();
        }
        throw Exception::OperationFailed(error.UTF8String);
    }
    _started.store(true);
}

void PeripheralMac::stop() {
    std::scoped_lock lock(_lifecycle_mutex);
    if (!_started.exchange(false)) {
        return;
    }

    LocalPeripheralBaseMacOS* internal = (__bridge LocalPeripheralBaseMacOS*)_opaque_internal;
    [internal stop];
    for (const auto& service : _services) {
        service->unfreeze();
    }
    _clear_subscribers();
}

bool PeripheralMac::is_started() { return _started.load(); }

bool PeripheralMac::is_advertising() {
    LocalPeripheralBaseMacOS* internal = (__bridge LocalPeripheralBaseMacOS*)_opaque_internal;
    return [internal isAdvertising];
}

void PeripheralMac::set_callback_on_client_connected(std::function<void(BluetoothAddress client_address)> on_client_connected) {
    if (on_client_connected) {
        _callback_on_client_connected.load(std::move(on_client_connected));
    } else {
        _callback_on_client_connected.unload();
    }
}

void PeripheralMac::set_callback_on_client_disconnected(std::function<void(BluetoothAddress client_address)> on_client_disconnected) {
    if (on_client_disconnected) {
        _callback_on_client_disconnected.load(std::move(on_client_disconnected));
    } else {
        _callback_on_client_disconnected.unload();
    }
}

void PeripheralMac::register_characteristic(const std::shared_ptr<CharacteristicMac>& characteristic) {
    std::scoped_lock lock(_characteristics_mutex);
    _characteristics[characteristic->underlying()] = characteristic;
}

std::shared_ptr<CharacteristicMac> PeripheralMac::characteristic_for(void* opaque_characteristic) {
    std::scoped_lock lock(_characteristics_mutex);
    auto item = _characteristics.find(opaque_characteristic);
    if (item == _characteristics.end()) {
        return {};
    }
    auto characteristic = item->second.lock();
    if (!characteristic) {
        _characteristics.erase(item);
    }
    return characteristic;
}

void PeripheralMac::publish(void* opaque_characteristic, const ByteArray& value) {
    if (!_started.load()) {
        return;
    }
    LocalPeripheralBaseMacOS* internal = (__bridge LocalPeripheralBaseMacOS*)_opaque_internal;
    [internal publishValue:dataFromByteArray(value) forCharacteristic:(__bridge CBMutableCharacteristic*)opaque_characteristic];
}

void PeripheralMac::handle_subscribed(void* opaque_characteristic, const BluetoothAddress& client_address) {
    auto keepAlive = shared_from_this();
    auto characteristic = characteristic_for(opaque_characteristic);
    if (!characteristic || !characteristic->handle_subscribed(client_address)) {
        return;
    }

    // CoreBluetooth does not expose peripheral-role connection events. Treat
    // the first active subscription as the observable client connection.
    if (!_started.load()) {
        return;
    }
    bool notify = false;
    {
        std::scoped_lock lock(_clients_mutex);
        notify = ++_client_subscriptions[client_address] == 1;
    }
    if (notify) {
        SAFE_CALLBACK_CALL(_callback_on_client_connected, client_address);
    }
}

void PeripheralMac::handle_unsubscribed(void* opaque_characteristic, const BluetoothAddress& client_address) {
    auto keepAlive = shared_from_this();
    auto characteristic = characteristic_for(opaque_characteristic);
    if (!characteristic || !characteristic->handle_unsubscribed(client_address)) {
        return;
    }

    if (!_started.load()) {
        return;
    }
    bool notify = false;
    {
        std::scoped_lock lock(_clients_mutex);
        auto item = _client_subscriptions.find(client_address);
        if (item != _client_subscriptions.end() && --item->second == 0) {
            _client_subscriptions.erase(item);
            notify = true;
        }
    }
    if (notify) {
        SAFE_CALLBACK_CALL(_callback_on_client_disconnected, client_address);
    }
}

void PeripheralMac::_ensure_mutable() const {
    if (_started.load()) {
        throw Exception::OperationFailed("The local peripheral cannot be changed while it is started.");
    }
}

void PeripheralMac::_clear_subscribers() {
    {
        std::scoped_lock lock(_clients_mutex);
        _client_subscriptions.clear();
    }

    std::vector<std::shared_ptr<CharacteristicMac>> characteristics;
    {
        std::scoped_lock lock(_characteristics_mutex);
        characteristics.reserve(_characteristics.size());
        for (auto& [opaque, weakCharacteristic] : _characteristics) {
            if (auto characteristic = weakCharacteristic.lock()) {
                characteristics.push_back(std::move(characteristic));
            }
        }
    }
    for (const auto& characteristic : characteristics) {
        characteristic->clear_subscribers();
    }
}

}  // namespace SimpleBLE::Local
