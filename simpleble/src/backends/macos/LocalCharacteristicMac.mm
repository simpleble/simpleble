#include "LocalCharacteristicMac.h"

#include <utility>

#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

#include <simpleble/Exceptions.h>

#import "CommonUtils.h"
#import "LocalPeripheralMac.h"
#import "Utils.h"

namespace {

CBCharacteristicProperties propertiesFromCapabilities(const std::set<SimpleBLE::Local::CharacteristicCapability>& capabilities) {
    CBCharacteristicProperties properties = 0;
    for (const auto capability : capabilities) {
        switch (capability) {
            case SimpleBLE::Local::CharacteristicCapability::READ:
                properties |= CBCharacteristicPropertyRead;
                break;
            case SimpleBLE::Local::CharacteristicCapability::WRITE_REQUEST:
                properties |= CBCharacteristicPropertyWrite;
                break;
            case SimpleBLE::Local::CharacteristicCapability::WRITE_COMMAND:
                properties |= CBCharacteristicPropertyWriteWithoutResponse;
                break;
            case SimpleBLE::Local::CharacteristicCapability::NOTIFY:
                properties |= CBCharacteristicPropertyNotify;
                break;
            case SimpleBLE::Local::CharacteristicCapability::INDICATE:
                properties |= CBCharacteristicPropertyIndicate;
                break;
        }
    }
    return properties;
}

CBAttributePermissions permissionsFromCapabilities(const std::set<SimpleBLE::Local::CharacteristicCapability>& capabilities) {
    CBAttributePermissions permissions = 0;
    if (capabilities.count(SimpleBLE::Local::CharacteristicCapability::READ) > 0) {
        permissions |= CBAttributePermissionsReadable;
    }
    if (capabilities.count(SimpleBLE::Local::CharacteristicCapability::WRITE_REQUEST) > 0 ||
        capabilities.count(SimpleBLE::Local::CharacteristicCapability::WRITE_COMMAND) > 0) {
        permissions |= CBAttributePermissionsWriteable;
    }
    return permissions;
}

}  // namespace

namespace SimpleBLE::Local {

CharacteristicMac::CharacteristicMac(std::weak_ptr<PeripheralMac> peripheral, BluetoothUUID uuid,
                                     std::set<CharacteristicCapability> capabilities)
    : _peripheral(std::move(peripheral)), _uuid(std::move(uuid)), _capabilities(std::move(capabilities)) {
    if (_capabilities.empty()) {
        throw Exception::OperationFailed("A local characteristic requires at least one capability.");
    }

    auto* characteristic = [[CBMutableCharacteristic alloc] initWithType:uuidFromSimpleBLE(_uuid)
                                                              properties:propertiesFromCapabilities(_capabilities)
                                                                   value:nil
                                                             permissions:permissionsFromCapabilities(_capabilities)];
    _opaque_characteristic = (__bridge_retained void*)characteristic;
}

CharacteristicMac::~CharacteristicMac() {
    _callback_on_read.unload();
    _callback_on_write.unload();
    _callback_on_subscribed.unload();
    _callback_on_unsubscribed.unload();

    CBMutableCharacteristic* characteristic = (__bridge_transfer CBMutableCharacteristic*)_opaque_characteristic;
    characteristic = nil;
}

BluetoothUUID CharacteristicMac::uuid() { return _uuid; }

std::set<CharacteristicCapability> CharacteristicMac::capabilities() { return _capabilities; }

ByteArray CharacteristicMac::value() {
    std::scoped_lock lock(_mutex);
    return _value;
}

void CharacteristicMac::set_value(ByteArray value) {
    ByteArray valueToPublish;
    {
        std::scoped_lock lock(_mutex);
        _value = std::move(value);
        valueToPublish = _value;
    }

    const bool canPublish = _capabilities.count(CharacteristicCapability::NOTIFY) > 0 ||
                            _capabilities.count(CharacteristicCapability::INDICATE) > 0;
    if (canPublish) {
        auto peripheral = _peripheral.lock();
        if (!peripheral) {
            return;
        }
        peripheral->publish(_opaque_characteristic, valueToPublish);
    }
}

void CharacteristicMac::set_callback_on_read(std::function<ByteArray()> on_read) {
    if (on_read) {
        _callback_on_read.load(std::move(on_read));
    } else {
        _callback_on_read.unload();
    }
}

void CharacteristicMac::set_callback_on_write(std::function<void(ByteArray value)> on_write) {
    if (on_write) {
        _callback_on_write.load(std::move(on_write));
    } else {
        _callback_on_write.unload();
    }
}

void CharacteristicMac::set_callback_on_subscribed(std::function<void()> on_subscribed) {
    if (on_subscribed) {
        _callback_on_subscribed.load(std::move(on_subscribed));
    } else {
        _callback_on_subscribed.unload();
    }
}

void CharacteristicMac::set_callback_on_unsubscribed(std::function<void()> on_unsubscribed) {
    if (on_unsubscribed) {
        _callback_on_unsubscribed.load(std::move(on_unsubscribed));
    } else {
        _callback_on_unsubscribed.unload();
    }
}

void* CharacteristicMac::underlying() const { return _opaque_characteristic; }

bool CharacteristicMac::can_read() const { return _capabilities.count(CharacteristicCapability::READ) > 0; }

bool CharacteristicMac::can_write() const {
    return _capabilities.count(CharacteristicCapability::WRITE_REQUEST) > 0 ||
           _capabilities.count(CharacteristicCapability::WRITE_COMMAND) > 0;
}

ByteArray CharacteristicMac::handle_read() {
    if (_callback_on_read) {
        ByteArray dynamicValue = _callback_on_read();
        std::scoped_lock lock(_mutex);
        _value = dynamicValue;
        return dynamicValue;
    }
    return value();
}

void CharacteristicMac::handle_write(ByteArray value) {
    {
        std::scoped_lock lock(_mutex);
        _value = value;
    }
    SAFE_CALLBACK_CALL(_callback_on_write, std::move(value));
}

bool CharacteristicMac::handle_subscribed(const BluetoothAddress& client_address) {
    bool notify = false;
    {
        std::scoped_lock lock(_mutex);
        if (!_subscribers.insert(client_address).second) {
            return false;
        }
        notify = _subscribers.size() == 1;
    }
    if (notify) {
        SAFE_CALLBACK_CALL(_callback_on_subscribed);
    }
    return true;
}

bool CharacteristicMac::handle_unsubscribed(const BluetoothAddress& client_address) {
    bool notify = false;
    {
        std::scoped_lock lock(_mutex);
        if (_subscribers.erase(client_address) == 0) {
            return false;
        }
        notify = _subscribers.empty();
    }
    if (notify) {
        SAFE_CALLBACK_CALL(_callback_on_unsubscribed);
    }
    return true;
}

void CharacteristicMac::clear_subscribers() {
    std::scoped_lock lock(_mutex);
    _subscribers.clear();
}

}  // namespace SimpleBLE::Local
