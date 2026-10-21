#include "PeripheralAndroid.h"

#include "BuildVec.h"
#include "BuilderBase.h"
#include "CharacteristicBase.h"
#include "DescriptorBase.h"
#include "ServiceBase.h"

#include <simpleble/Exceptions.h>
#include <simpleble/Config.h>
#include <limits>
#include "CommonUtils.h"
#include "LoggingInternal.h"
#include "simpleble/Descriptor.h"

using namespace SimpleBLE;
using namespace std::chrono_literals;

PeripheralAndroid::PeripheralAndroid(Android::BluetoothDevice device) : _device(device) {
    _btGattCallback.set_callback_onConnectionStateChange([this](bool connected) {
        if (connected) {
            if (Config::Android::connection_priority_request != Config::Android::ConnectionPriorityRequest::DISABLED) {
                _gatt.requestConnectionPriority(static_cast<int>(Config::Android::connection_priority_request));
            }

            // If a connection has been established, request service discovery.
            _gatt.discoverServices();
        } else {
            _connection_pending = false;
            // If a connection has been lost, close the GATT object.
            _gatt.close();
            SAFE_CALLBACK_CALL(callback_on_disconnected_);
            _disconnection_cv.notify_all();
        }
    });

    _btGattCallback.set_callback_onServicesDiscovered([this]() {
        _services = _gatt.getServices();

        // Android can report a retained ACL connection's MTU during discovery.
        // Request it only when needed, after discovery and before completing connect.
        if (Config::Android::negotiate_mtu && !_mtu_observed) {
            if (_gatt.requestMtu(517)) {
                return;
            }

            SIMPLEBLE_LOG_WARN("Failed to start MTU negotiation; using the last known ATT MTU");
        }

        _complete_connection();
    });

    _btGattCallback.set_callback_onMtuChanged([this](int status) {
        if (status == 0) {
            _mtu_observed = true;
        }

        if (_btGattCallback.services_discovered) {
            if (status != 0 && _connection_pending) {
                SIMPLEBLE_LOG_WARN("MTU negotiation failed; using the last known ATT MTU");
            }

            _complete_connection();
        }
    });
}

PeripheralAndroid::~PeripheralAndroid() {}

void PeripheralAndroid::update_advertising_data(Android::ScanResult scan_result) {
    rssi_ = scan_result.getRssi();
    connectable_ = scan_result.isConnectable();

    auto scan_record = scan_result.getScanRecord();

    // ScanResult.getTxPower() is the extended advertising header field, not the TX Power Level AD type.
    // Keep the last advertised value when a result omits it.
    int tx_power_level = scan_record.getTxPowerLevel();
    if (tx_power_level != std::numeric_limits<int>::min()) {
        tx_power_ = tx_power_level;
    }

    // Each result carries the advertisement combined with its scan response, so it replaces the last one.
    advertised_services_ = scan_record.getServiceUuids();
    manufacturer_data_ = scan_record.getManufacturerData();
}

void* PeripheralAndroid::underlying() const { return nullptr; }

std::string PeripheralAndroid::identifier() { return _device.getName(); }

BluetoothAddress PeripheralAndroid::address() { return BluetoothAddress(_device.getAddress()); }

BluetoothAddressType PeripheralAndroid::address_type() {
    // TODO: This is only available on API level 35 and above, so we're forced to return UNSPECIFIED
    return BluetoothAddressType::UNSPECIFIED;
    // switch (_device.getAddressType()) {
    //     case Android::BluetoothDevice::ADDRESS_TYPE_PUBLIC:
    //         return BluetoothAddressType::PUBLIC;
    //     case Android::BluetoothDevice::ADDRESS_TYPE_RANDOM:
    //         return BluetoothAddressType::RANDOM;
    //     default:
    //         return BluetoothAddressType::UNSPECIFIED;
    // }
}

int16_t PeripheralAndroid::rssi() { return rssi_; }

int16_t PeripheralAndroid::tx_power() { return tx_power_; }

uint16_t PeripheralAndroid::mtu() {
    if (!is_connected()) {
        return 0;
    }

    return _btGattCallback.mtu.load() - 3;
}

void PeripheralAndroid::connect() {
    _btGattCallback.mtu = 23;
    _connection_pending = true;
    _mtu_observed = false;
    _gatt = _device.connectGatt(false, _btGattCallback);

    // Wait for the connection to be confirmed.
    // The condition variable will return false if the connection was not established.
    std::unique_lock<std::mutex> lock(_connection_mutex);
    bool connected = _connection_cv.wait_for(lock, 8s, [this]() { return is_connected(); });
    if (!connected) {
        _handle_connection_timeout();
    }
}

void PeripheralAndroid::disconnect() {
    _gatt.disconnect();

    // Wait for the disconnection to be confirmed.
    std::unique_lock<std::mutex> lock(_disconnection_mutex);
    bool disconnected = _disconnection_cv.wait_for(lock, 8s, [this]() { return !is_connected(); });
    if (!disconnected) {
        throw SimpleBLE::Exception::OperationFailed("Failed to disconnect from device");
    }
}

bool PeripheralAndroid::is_connected() {
    return _btGattCallback.connected && _btGattCallback.services_discovered && !_connection_pending;
}

bool PeripheralAndroid::is_connectable() { return connectable_; }

bool PeripheralAndroid::is_paired() { return _device.getBondState() == Android::BluetoothDevice::BOND_BONDED; }

void PeripheralAndroid::unpair() {
    // IMPORTANT: This is a non-public API call, which might be blacklisted by the Android OS in the future.
    // NOTE: Doesn't seem to work, needs further investigation.
    //_device.removeBond();
}

SharedPtrVector<ServiceBase> PeripheralAndroid::available_services() {
    SharedPtrVector<ServiceBase> service_list;
    for (auto service : _services) {
        // Build the list of characteristics for the service.
        SharedPtrVector<CharacteristicBase> characteristic_list;
        for (auto characteristic : service.getCharacteristics()) {
            // Build the list of descriptors for the characteristic.
            SharedPtrVector<DescriptorBase> descriptor_list;
            for (auto descriptor : characteristic.getDescriptors()) {
                descriptor_list.push_back(std::make_shared<DescriptorBase>(descriptor.getUuid()));
            }

            int flags = characteristic.getProperties();

            bool can_read = flags & Android::BluetoothGattCharacteristic::PROPERTY_READ;
            bool can_write_request = flags & Android::BluetoothGattCharacteristic::PROPERTY_WRITE;
            bool can_write_command = flags & Android::BluetoothGattCharacteristic::PROPERTY_WRITE_NO_RESPONSE;
            bool can_notify = flags & Android::BluetoothGattCharacteristic::PROPERTY_NOTIFY;
            bool can_indicate = flags & Android::BluetoothGattCharacteristic::PROPERTY_INDICATE;

            characteristic_list.push_back(
                std::make_shared<CharacteristicBase>(characteristic.getUuid(), descriptor_list, can_read,
                                                     can_write_request, can_write_command, can_notify, can_indicate));
        }

        service_list.push_back(std::make_shared<ServiceBase>(service.getUuid(), characteristic_list));
    }

    return service_list;
}

SharedPtrVector<ServiceBase> PeripheralAndroid::advertised_services() {
    SharedPtrVector<ServiceBase> service_list;
    for (auto& service_uuid : advertised_services_) {
        service_list.push_back(std::make_shared<ServiceBase>(service_uuid));
    }
    return service_list;
}

std::map<uint16_t, ByteArray> PeripheralAndroid::manufacturer_data() { return manufacturer_data_; }

ByteArray PeripheralAndroid::read(BluetoothUUID const& service, BluetoothUUID const& characteristic) {
    SIMPLEBLE_LOG_INFO("Reading characteristic " + characteristic);

    auto characteristic_obj = _fetch_characteristic(service, characteristic);

    _btGattCallback.set_flag_characteristicReadPending(characteristic_obj.getObject());
    if (!_gatt.readCharacteristic(characteristic_obj)) {
        throw SimpleBLE::Exception::OperationFailed("Failed to read characteristic " + characteristic);
    }

    auto value = _btGattCallback.wait_flag_characteristicReadPending(characteristic_obj.getObject());
    return ByteArray(value);
}

void PeripheralAndroid::write_request(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                      ByteArray const& data) {
    SIMPLEBLE_LOG_INFO("Writing request to characteristic " + characteristic);

    auto characteristic_obj = _fetch_characteristic(service, characteristic);

    characteristic_obj.setWriteType(Android::BluetoothGattCharacteristic::WRITE_TYPE_DEFAULT);
    characteristic_obj.setValue(std::vector<uint8_t>(data.begin(), data.end()));

    _btGattCallback.set_flag_characteristicWritePending(characteristic_obj.getObject());
    if (!_gatt.writeCharacteristic(characteristic_obj)) {
        throw SimpleBLE::Exception::OperationFailed("Failed to write characteristic " + characteristic);
    }
    _btGattCallback.wait_flag_characteristicWritePending(characteristic_obj.getObject());
}

void PeripheralAndroid::write_command(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                      ByteArray const& data) {
    SIMPLEBLE_LOG_INFO("Writing command to characteristic " + characteristic);

    const size_t max_payload_size = _btGattCallback.mtu.load() - 3;
    if (data.size() > max_payload_size) {
        throw SimpleBLE::Exception::OperationFailed("Write command payload size " + std::to_string(data.size()) +
                                                    " exceeds the negotiated maximum of " +
                                                    std::to_string(max_payload_size) + " bytes");
    }

    auto characteristic_obj = _fetch_characteristic(service, characteristic);

    characteristic_obj.setWriteType(Android::BluetoothGattCharacteristic::WRITE_TYPE_NO_RESPONSE);
    characteristic_obj.setValue(std::vector<uint8_t>(data.begin(), data.end()));

    _btGattCallback.set_flag_characteristicWritePending(characteristic_obj.getObject());
    if (!_gatt.writeCharacteristic(characteristic_obj)) {
        throw SimpleBLE::Exception::OperationFailed("Failed to write characteristic " + characteristic);
    }
    _btGattCallback.wait_flag_characteristicWritePending(characteristic_obj.getObject());
}

void PeripheralAndroid::notify(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                               std::function<void(ByteArray payload)> callback) {
    SIMPLEBLE_LOG_INFO("Subscribing to characteristic " + characteristic);

    auto characteristic_obj = _fetch_characteristic(service, characteristic);
    auto descriptor_obj = _fetch_descriptor(service, characteristic,
                                            Android::BluetoothGattDescriptor::CLIENT_CHARACTERISTIC_CONFIG);

    _btGattCallback.set_callback_onCharacteristicChanged(characteristic_obj.getObject(),
                                                         [callback](std::vector<uint8_t> data) {
                                                             ByteArray payload(data);
                                                             callback(payload);
                                                         });
    bool success = _gatt.setCharacteristicNotification(characteristic_obj, true);
    if (!success) {
        throw SimpleBLE::Exception::OperationFailed("Failed to subscribe to characteristic " + characteristic);
    }

    _btGattCallback.set_flag_descriptorWritePending(descriptor_obj.getObject());
    descriptor_obj.setValue(Android::BluetoothGattDescriptor::ENABLE_NOTIFICATION_VALUE);
    if (!_gatt.writeDescriptor(descriptor_obj)) {
        throw SimpleBLE::Exception::OperationFailed("Failed to write descriptor for characteristic " + characteristic);
    }
    _btGattCallback.wait_flag_descriptorWritePending(descriptor_obj.getObject());
}

void PeripheralAndroid::indicate(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                 std::function<void(ByteArray payload)> callback) {
    SIMPLEBLE_LOG_INFO("Subscribing to characteristic " + characteristic);

    auto characteristic_obj = _fetch_characteristic(service, characteristic);
    auto descriptor_obj = _fetch_descriptor(service, characteristic,
                                            Android::BluetoothGattDescriptor::CLIENT_CHARACTERISTIC_CONFIG);

    _btGattCallback.set_callback_onCharacteristicChanged(characteristic_obj.getObject(),
                                                         [callback](std::vector<uint8_t> data) {
                                                             ByteArray payload(data);
                                                             callback(payload);
                                                         });
    bool success = _gatt.setCharacteristicNotification(characteristic_obj, true);
    if (!success) {
        throw SimpleBLE::Exception::OperationFailed("Failed to subscribe to characteristic " + characteristic);
    }

    _btGattCallback.set_flag_descriptorWritePending(descriptor_obj.getObject());
    descriptor_obj.setValue(Android::BluetoothGattDescriptor::ENABLE_INDICATION_VALUE);
    if (!_gatt.writeDescriptor(descriptor_obj)) {
        throw SimpleBLE::Exception::OperationFailed("Failed to write descriptor for characteristic " + characteristic);
    }
    _btGattCallback.wait_flag_descriptorWritePending(descriptor_obj.getObject());
}

void PeripheralAndroid::unsubscribe(BluetoothUUID const& service, BluetoothUUID const& characteristic) {
    SIMPLEBLE_LOG_INFO("Unsubscribing from characteristic " + characteristic);

    auto characteristic_obj = _fetch_characteristic(service, characteristic);
    auto descriptor_obj = _fetch_descriptor(service, characteristic,
                                            Android::BluetoothGattDescriptor::CLIENT_CHARACTERISTIC_CONFIG);

    _btGattCallback.set_flag_descriptorWritePending(descriptor_obj.getObject());
    descriptor_obj.setValue(Android::BluetoothGattDescriptor::DISABLE_NOTIFICATION_VALUE);
    if (!_gatt.writeDescriptor(descriptor_obj)) {
        throw SimpleBLE::Exception::OperationFailed("Failed to write descriptor for characteristic " + characteristic);
    }
    _btGattCallback.wait_flag_descriptorWritePending(descriptor_obj.getObject());

    _btGattCallback.clear_callback_onCharacteristicChanged(characteristic_obj.getObject());
    bool success = _gatt.setCharacteristicNotification(characteristic_obj, false);
    if (!success) {
        throw SimpleBLE::Exception::OperationFailed("Failed to unsubscribe from characteristic " + characteristic);
    }
}

ByteArray PeripheralAndroid::read(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                                  BluetoothUUID const& descriptor) {
    SIMPLEBLE_LOG_INFO("Reading descriptor " + descriptor);

    auto descriptor_obj = _fetch_descriptor(service, characteristic, descriptor);

    _btGattCallback.set_flag_descriptorReadPending(descriptor_obj.getObject());
    if (!_gatt.readDescriptor(descriptor_obj)) {
        throw SimpleBLE::Exception::OperationFailed("Failed to read descriptor " + descriptor);
    }

    auto value = _btGattCallback.wait_flag_descriptorReadPending(descriptor_obj.getObject());
    return ByteArray(value);
}

void PeripheralAndroid::write(BluetoothUUID const& service, BluetoothUUID const& characteristic,
                              BluetoothUUID const& descriptor, ByteArray const& data) {
    SIMPLEBLE_LOG_INFO("Writing descriptor " + descriptor);

    auto descriptor_obj = _fetch_descriptor(service, characteristic, descriptor);

    _btGattCallback.set_flag_descriptorWritePending(descriptor_obj.getObject());
    descriptor_obj.setValue(std::vector<uint8_t>(data.begin(), data.end()));
    if (!_gatt.writeDescriptor(descriptor_obj)) {
        throw SimpleBLE::Exception::OperationFailed("Failed to write descriptor " + descriptor);
    }
    _btGattCallback.wait_flag_descriptorWritePending(descriptor_obj.getObject());
}

void PeripheralAndroid::set_callback_on_connected(std::function<void()> on_connected) {
    if (on_connected) {
        callback_on_connected_.load(std::move(on_connected));
    } else {
        callback_on_connected_.unload();
    }
}

void PeripheralAndroid::set_callback_on_disconnected(std::function<void()> on_disconnected) {
    if (on_disconnected) {
        callback_on_disconnected_.load(std::move(on_disconnected));
    } else {
        callback_on_disconnected_.unload();
    }
}

void PeripheralAndroid::_complete_connection() {
    bool connection_pending = _connection_pending.exchange(false);
    if (!connection_pending) {
        return;
    }

    SAFE_CALLBACK_CALL(callback_on_connected_);
    _connection_cv.notify_all();
}

void PeripheralAndroid::_handle_connection_timeout() {
    bool waiting_for_mtu = _btGattCallback.services_discovered && _connection_pending;
    _btGattCallback.connected = false;
    _btGattCallback.services_discovered = false;
    _connection_pending = false;
    _gatt.disconnect();
    _gatt.close();
    if (waiting_for_mtu) {
        throw SimpleBLE::Exception::OperationFailed("Timed out waiting for MTU negotiation");
    }
    throw SimpleBLE::Exception::OperationFailed("Failed to connect to device");
}

// NOTE: This approach to retrieve the characteristic and descriptor objects is not ideal, as it involves
//       iterating on Java objects and returning a copy of the desired object. This will be improved in the future
//       if performance becomes a bottleneck.
Android::BluetoothGattCharacteristic PeripheralAndroid::_fetch_characteristic(
    const BluetoothUUID& service_uuid, const BluetoothUUID& characteristic_uuid) {
    for (auto& service : _services) {
        if (service.getUuid() == service_uuid) {
            for (auto& characteristic : service.getCharacteristics()) {
                if (characteristic.getUuid() == characteristic_uuid) {
                    return characteristic;
                }
            }
            throw SimpleBLE::Exception::CharacteristicNotFound(characteristic_uuid);
        }
    }
    throw SimpleBLE::Exception::ServiceNotFound(service_uuid);
}

Android::BluetoothGattDescriptor PeripheralAndroid::_fetch_descriptor(const BluetoothUUID& service_uuid,
                                                                      const BluetoothUUID& characteristic_uuid,
                                                                      const BluetoothUUID& descriptor_uuid) {
    for (auto& service : _services) {
        if (service.getUuid() == service_uuid) {
            for (auto& characteristic : service.getCharacteristics()) {
                if (characteristic.getUuid() == characteristic_uuid) {
                    for (auto& descriptor : characteristic.getDescriptors()) {
                        if (descriptor.getUuid() == descriptor_uuid) {
                            return descriptor;
                        }
                    }
                    throw SimpleBLE::Exception::DescriptorNotFound(descriptor_uuid);
                }
            }
            throw SimpleBLE::Exception::CharacteristicNotFound(characteristic_uuid);
        }
    }
    throw SimpleBLE::Exception::ServiceNotFound(service_uuid);
}
