#include "LocalCharacteristicAndroid.h"

#include <optional>
#include <utility>

#include <simpleble/Exceptions.h>

#include "CommonUtils.h"
#include "LocalPeripheralAndroid.h"

namespace SimpleBLE::Local {

CharacteristicAndroid::CharacteristicAndroid(std::weak_ptr<PeripheralAndroid> peripheral, BluetoothUUID uuid,
                                             std::set<CharacteristicCapability> capabilities)
    : _peripheral(std::move(peripheral)), _uuid(std::move(uuid)), _capabilities(std::move(capabilities)) {
    if (_capabilities.empty()) {
        throw Exception::OperationFailed("A local characteristic requires at least one capability.");
    }

    using Native = Android::BluetoothGattCharacteristic;
    int properties = 0;
    int permissions = 0;
    for (const auto capability : _capabilities) {
        switch (capability) {
            case CharacteristicCapability::READ:
                properties |= Native::PROPERTY_READ;
                permissions |= Native::PERMISSION_READ;
                break;
            case CharacteristicCapability::WRITE_REQUEST:
                properties |= Native::PROPERTY_WRITE;
                permissions |= Native::PERMISSION_WRITE;
                break;
            case CharacteristicCapability::WRITE_COMMAND:
                properties |= Native::PROPERTY_WRITE_NO_RESPONSE;
                permissions |= Native::PERMISSION_WRITE;
                break;
            case CharacteristicCapability::NOTIFY:
                properties |= Native::PROPERTY_NOTIFY;
                break;
            case CharacteristicCapability::INDICATE:
                properties |= Native::PROPERTY_INDICATE;
                break;
        }
    }
    _characteristic = Native(_uuid, properties, permissions);

    if (_capabilities.count(CharacteristicCapability::NOTIFY) > 0 ||
        _capabilities.count(CharacteristicCapability::INDICATE) > 0) {
        _cccd.emplace(
            Android::BluetoothGattDescriptor::CLIENT_CHARACTERISTIC_CONFIG,
            Android::BluetoothGattDescriptor::PERMISSION_READ | Android::BluetoothGattDescriptor::PERMISSION_WRITE);
        if (!_characteristic.addDescriptor(*_cccd)) {
            throw Exception::OperationFailed("Android failed to add the client configuration descriptor.");
        }
    }
}

CharacteristicAndroid::~CharacteristicAndroid() {
    _callback_on_read.unload();
    _callback_on_write.unload();
    _callback_on_subscribed.unload();
    _callback_on_unsubscribed.unload();
}

BluetoothUUID CharacteristicAndroid::uuid() { return _uuid; }
std::set<CharacteristicCapability> CharacteristicAndroid::capabilities() { return _capabilities; }

ByteArray CharacteristicAndroid::value() {
    std::scoped_lock lock(_mutex);
    return _value;
}

void CharacteristicAndroid::set_value(ByteArray value) {
    ByteArray published;
    {
        std::scoped_lock lock(_mutex);
        _value = std::move(value);
        published = _value;
    }
    if (auto peripheral = _peripheral.lock()) peripheral->publish(shared_from_this(), published);
}

void CharacteristicAndroid::set_callback_on_read(std::function<ByteArray()> callback) {
    if (callback)
        _callback_on_read.load(std::move(callback));
    else
        _callback_on_read.unload();
}
void CharacteristicAndroid::set_callback_on_write(std::function<void(ByteArray)> callback) {
    if (callback)
        _callback_on_write.load(std::move(callback));
    else
        _callback_on_write.unload();
}
void CharacteristicAndroid::set_callback_on_subscribed(std::function<void()> callback) {
    if (callback)
        _callback_on_subscribed.load(std::move(callback));
    else
        _callback_on_subscribed.unload();
}
void CharacteristicAndroid::set_callback_on_unsubscribed(std::function<void()> callback) {
    if (callback)
        _callback_on_unsubscribed.load(std::move(callback));
    else
        _callback_on_unsubscribed.unload();
}

Android::BluetoothGattCharacteristic CharacteristicAndroid::native_characteristic() const { return _characteristic; }
std::optional<Android::BluetoothGattDescriptor> CharacteristicAndroid::native_cccd() const { return _cccd; }

bool CharacteristicAndroid::can_read() const { return _capabilities.count(CharacteristicCapability::READ) > 0; }
bool CharacteristicAndroid::can_write_request() const {
    return _capabilities.count(CharacteristicCapability::WRITE_REQUEST) > 0;
}
bool CharacteristicAndroid::can_write_command() const {
    return _capabilities.count(CharacteristicCapability::WRITE_COMMAND) > 0;
}

ByteArray CharacteristicAndroid::handle_read() {
    if (_callback_on_read) {
        ByteArray dynamic = _callback_on_read();
        std::scoped_lock lock(_mutex);
        _value = dynamic;
        return dynamic;
    }
    return value();
}

void CharacteristicAndroid::handle_write(ByteArray value) {
    {
        std::scoped_lock lock(_mutex);
        _value = value;
    }
    SAFE_CALLBACK_CALL(_callback_on_write, std::move(value));
}

bool CharacteristicAndroid::set_subscription(const Android::BluetoothDevice& device, const ByteArray& value) {
    const ByteArray disabled(Android::BluetoothGattDescriptor::DISABLE_NOTIFICATION_VALUE);
    const ByteArray notifications(Android::BluetoothGattDescriptor::ENABLE_NOTIFICATION_VALUE);
    const ByteArray indications(Android::BluetoothGattDescriptor::ENABLE_INDICATION_VALUE);
    const BluetoothAddress address(device.getAddress());
    bool subscribed = false;
    bool notify_subscribed = false;
    bool notify_unsubscribed = false;
    {
        std::scoped_lock lock(_mutex);
        const bool was_empty = _subscribers.empty();
        if (value == disabled) {
            if (_subscribers.erase(address) > 0 && _subscribers.empty()) notify_unsubscribed = true;
        } else if (value == notifications && _capabilities.count(CharacteristicCapability::NOTIFY) > 0) {
            _subscribers.insert_or_assign(address, Subscriber{device, false});
            subscribed = true;
            notify_subscribed = was_empty;
        } else if (value == indications && _capabilities.count(CharacteristicCapability::INDICATE) > 0) {
            _subscribers.insert_or_assign(address, Subscriber{device, true});
            subscribed = true;
            notify_subscribed = was_empty;
        } else {
            return false;
        }
    }
    if (notify_subscribed) SAFE_CALLBACK_CALL(_callback_on_subscribed);
    if (notify_unsubscribed) SAFE_CALLBACK_CALL(_callback_on_unsubscribed);
    return subscribed || value == disabled;
}

ByteArray CharacteristicAndroid::subscription_value(const BluetoothAddress& address) const {
    std::scoped_lock lock(_mutex);
    auto subscriber = _subscribers.find(address);
    if (subscriber == _subscribers.end()) {
        return ByteArray(Android::BluetoothGattDescriptor::DISABLE_NOTIFICATION_VALUE);
    }
    return subscriber->second.confirm ? ByteArray(Android::BluetoothGattDescriptor::ENABLE_INDICATION_VALUE)
                                      : ByteArray(Android::BluetoothGattDescriptor::ENABLE_NOTIFICATION_VALUE);
}

std::vector<CharacteristicAndroid::Subscriber> CharacteristicAndroid::subscribers() const {
    std::scoped_lock lock(_mutex);
    std::vector<Subscriber> result;
    result.reserve(_subscribers.size());
    for (const auto& [address, subscriber] : _subscribers) result.push_back(subscriber);
    return result;
}

void CharacteristicAndroid::remove_subscriber(const BluetoothAddress& address) {
    bool notify = false;
    {
        std::scoped_lock lock(_mutex);
        if (_subscribers.erase(address) > 0 && _subscribers.empty()) notify = true;
    }
    if (notify) SAFE_CALLBACK_CALL(_callback_on_unsubscribed);
}

void CharacteristicAndroid::clear_subscribers() {
    std::scoped_lock lock(_mutex);
    _subscribers.clear();
}

}  // namespace SimpleBLE::Local
