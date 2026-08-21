#include "LocalPeripheralAndroid.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <string>
#include <utility>

#include <simpleble/Exceptions.h>

#include "BackendAndroid.h"
#include "CommonUtils.h"
#include "LocalCharacteristicAndroid.h"
#include "LocalServiceAndroid.h"
#include "LoggingInternal.h"
#include "types/android/bluetooth/BluetoothManager.h"
#include "types/android/bluetooth/le/AdvertiseData.h"
#include "types/android/bluetooth/le/AdvertiseSettings.h"
#include "types/android/content/Context.h"
#include "types/android/os/ParcelUUID.h"
#include "types/java/util/UUID.h"

namespace {

constexpr int GATT_SUCCESS = 0;
constexpr int GATT_READ_NOT_PERMITTED = 2;
constexpr int GATT_WRITE_NOT_PERMITTED = 3;
constexpr int GATT_REQUEST_NOT_SUPPORTED = 6;
constexpr int GATT_INVALID_OFFSET = 7;
constexpr int GATT_INVALID_ATTRIBUTE_LENGTH = 13;
constexpr int STATE_DISCONNECTED = 0;
constexpr int STATE_CONNECTED = 2;
constexpr auto OPERATION_TIMEOUT = std::chrono::seconds(5);

std::string advertise_error(int code) {
    switch (code) {
        case 1:
            return "advertisement data is too large";
        case 2:
            return "too many advertisers are already active";
        case 3:
            return "this advertisement is already active";
        case 4:
            return "the Android Bluetooth stack reported an internal error";
        case 5:
            return "Bluetooth LE advertising is not supported";
        default:
            return "Android error " + std::to_string(code);
    }
}

}  // namespace

namespace SimpleBLE::Local {

PeripheralAndroid::PeripheralAndroid(Android::BluetoothAdapter adapter) : _adapter(std::move(adapter)) {
    _server_callback.set_callback_onConnectionStateChange(
        [this](Android::BluetoothDevice device, int status, int state) {
            _handle_connection(std::move(device), status, state);
        });
    _server_callback.set_callback_onServiceAdded([this](int status, Android::BluetoothGattService) {
        {
            std::scoped_lock lock(_service_status_mutex);
            _service_status = status;
            _service_status_received = true;
        }
        _service_status_cv.notify_all();
    });
    _server_callback.set_callback_onCharacteristicReadRequest(
        [this](Android::BluetoothDevice device, int request_id, int offset,
               Android::BluetoothGattCharacteristic characteristic) {
        _handle_characteristic_read(std::move(device), request_id, offset, std::move(characteristic));
    });
    _server_callback.set_callback_onCharacteristicWriteRequest(
        [this](Android::BluetoothDevice device, int request_id, Android::BluetoothGattCharacteristic characteristic,
               bool prepared, bool response_needed, int offset, std::vector<uint8_t> value) {
            _handle_characteristic_write(std::move(device), request_id, std::move(characteristic), prepared,
                                         response_needed, offset, ByteArray(value));
        });
    _server_callback.set_callback_onDescriptorReadRequest(
        [this](Android::BluetoothDevice device, int request_id, int offset,
               Android::BluetoothGattDescriptor descriptor) {
        _handle_descriptor_read(std::move(device), request_id, offset, std::move(descriptor));
    });
    _server_callback.set_callback_onDescriptorWriteRequest(
        [this](Android::BluetoothDevice device, int request_id, Android::BluetoothGattDescriptor descriptor,
               bool prepared, bool response_needed, int offset, std::vector<uint8_t> value) {
        _handle_descriptor_write(std::move(device), request_id, std::move(descriptor), prepared, response_needed,
                                 offset, ByteArray(value));
    });
    _server_callback.set_callback_onExecuteWrite([this](Android::BluetoothDevice device, int request_id, bool) {
        _respond(device, request_id, GATT_REQUEST_NOT_SUPPORTED, 0);
    });
    _server_callback.set_callback_onNotificationSent([this](Android::BluetoothDevice, int) {
        std::scoped_lock lock(_notification_mutex);
        if (!_notifications.empty()) _notifications.pop_front();
        _send_next_notification_locked();
    });

    _advertise_callback.set_callback_onStartSuccess([this] {
        {
            std::scoped_lock lock(_advertise_status_mutex);
            _advertising.store(true);
            _advertise_error = 0;
            _advertise_status_received = true;
        }
        _advertise_status_cv.notify_all();
    });
    _advertise_callback.set_callback_onStartFailure([this](int error) {
        {
            std::scoped_lock lock(_advertise_status_mutex);
            _advertising.store(false);
            _advertise_error = error;
            _advertise_status_received = true;
        }
        _advertise_status_cv.notify_all();
    });
}

PeripheralAndroid::~PeripheralAndroid() {
    _callback_on_client_connected.unload();
    _callback_on_client_disconnected.unload();
    stop();
}

void* PeripheralAndroid::underlying() const { return nullptr; }

Advertisement PeripheralAndroid::advertisement() {
    std::scoped_lock lock(_lifecycle_mutex);
    return _advertisement;
}

void PeripheralAndroid::set_advertisement(Advertisement advertisement) {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();
    _advertisement = std::move(advertisement);
}

std::shared_ptr<ServiceBase> PeripheralAndroid::add_service(BluetoothUUID uuid) {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();
    auto service = std::make_shared<ServiceAndroid>(shared_from_this(), std::move(uuid));
    _services.push_back(service);
    return service;
}

std::vector<std::shared_ptr<ServiceBase>> PeripheralAndroid::services() {
    std::scoped_lock lock(_lifecycle_mutex);
    return {_services.begin(), _services.end()};
}

void PeripheralAndroid::remove_all_services() {
    std::scoped_lock lock(_lifecycle_mutex);
    _ensure_mutable();
    _services.clear();
    std::scoped_lock characteristic_lock(_characteristics_mutex);
    _characteristics.clear();
    _descriptors.clear();
}

void PeripheralAndroid::start() {
    std::scoped_lock lock(_lifecycle_mutex);
    if (_started.load()) return;
    if (!_adapter.isEnabled()) throw Exception::OperationFailed("Bluetooth is turned off.");
    if (!_adapter.isMultipleAdvertisementSupported()) {
        throw Exception::OperationFailed("This Android device does not support Bluetooth LE advertising.");
    }

    const std::string adapter_name = _adapter.getName();
    if (_advertisement.local_name.has_value() && *_advertisement.local_name != adapter_name) {
        throw Exception::OperationFailed("Android can only advertise the device Bluetooth name (\"" + adapter_name +
                                         "\"). Omit local_name or set it to that value.");
    }

    Android::Context context = BackendAndroid::application_context();
    Android::BluetoothManager manager(context.getSystemService("bluetooth"));
    _server = manager.openGattServer(context, _server_callback);
    for (const auto& service : _services) service->freeze();

    try {
        for (const auto& service : _services) {
            {
                std::scoped_lock status_lock(_service_status_mutex);
                _service_status_received = false;
                _service_status = 0;
            }
            {
                std::scoped_lock server_lock(_server_mutex);
                if (!_server.addService(service->native_service())) {
                    throw Exception::OperationFailed("Android rejected local GATT service " + service->uuid() + ".");
                }
            }

            std::unique_lock status_lock(_service_status_mutex);
            if (!_service_status_cv.wait_for(status_lock, OPERATION_TIMEOUT,
                                             [this] { return _service_status_received; })) {
                throw Exception::OperationFailed("Timed out while publishing Android GATT service " + service->uuid() +
                                                 ".");
            }
            if (_service_status != GATT_SUCCESS) {
                throw Exception::OperationFailed("Android failed to publish GATT service " + service->uuid() +
                                                 " (status " + std::to_string(_service_status) + ").");
            }
        }

        std::vector<BluetoothUUID> service_uuids = _advertisement.service_uuids;
        if (service_uuids.empty()) {
            service_uuids.reserve(_services.size());
            for (const auto& service : _services) service_uuids.push_back(service->uuid());
        }

        Android::AdvertiseData::Builder data_builder;
        for (const auto& uuid : service_uuids) {
            data_builder.addServiceUuid(Android::ParcelUUID(Android::UUID::fromString(uuid)));
        }
        auto data = data_builder.build();

        Android::AdvertiseData::Builder response_builder;
        response_builder.setIncludeDeviceName(_advertisement.local_name.has_value());
        auto scan_response = response_builder.build();

        auto settings = Android::AdvertiseSettings::Builder()
                            .setConnectable(true)
                            .setAdvertiseMode(Android::AdvertiseSettings::ADVERTISE_MODE_BALANCED)
                            .build();
        _advertiser = _adapter.getBluetoothLeAdvertiser();
        if (!_advertiser) {
            throw Exception::OperationFailed("Android Bluetooth LE advertising is unavailable.");
        }
        {
            std::scoped_lock status_lock(_advertise_status_mutex);
            _advertise_status_received = false;
            _advertise_error = 0;
        }
        _advertiser.startAdvertising(settings, data, scan_response, _advertise_callback);

        std::unique_lock status_lock(_advertise_status_mutex);
        if (!_advertise_status_cv.wait_for(status_lock, OPERATION_TIMEOUT,
                                           [this] { return _advertise_status_received; })) {
            throw Exception::OperationFailed("Timed out while starting Android Bluetooth LE advertising.");
        }
        if (_advertise_error != 0) {
            throw Exception::OperationFailed(
                "Failed to start Android advertising: " + advertise_error(_advertise_error) + ".");
        }
        _started.store(true);
    } catch (...) {
        _shutdown();
        for (const auto& service : _services) service->unfreeze();
        throw;
    }
}

void PeripheralAndroid::stop() {
    std::scoped_lock lock(_lifecycle_mutex);
    if (!_started.exchange(false) && !_server && !_advertising.load()) return;
    _shutdown();
    for (const auto& service : _services) service->unfreeze();
    std::vector<std::shared_ptr<CharacteristicAndroid>> characteristics;
    {
        std::scoped_lock characteristics_lock(_characteristics_mutex);
        for (auto& [object, weak] : _characteristics) {
            if (auto characteristic = weak.lock()) characteristics.push_back(std::move(characteristic));
        }
    }
    for (const auto& characteristic : characteristics) characteristic->clear_subscribers();
}

bool PeripheralAndroid::is_started() { return _started.load(); }
bool PeripheralAndroid::is_advertising() { return _advertising.load(); }

void PeripheralAndroid::set_callback_on_client_connected(std::function<void(BluetoothAddress)> callback) {
    if (callback)
        _callback_on_client_connected.load(std::move(callback));
    else
        _callback_on_client_connected.unload();
}

void PeripheralAndroid::set_callback_on_client_disconnected(std::function<void(BluetoothAddress)> callback) {
    if (callback)
        _callback_on_client_disconnected.load(std::move(callback));
    else
        _callback_on_client_disconnected.unload();
}

void PeripheralAndroid::register_characteristic(const std::shared_ptr<CharacteristicAndroid>& characteristic) {
    std::scoped_lock lock(_characteristics_mutex);
    _characteristics[characteristic->native_characteristic().getObject()] = characteristic;
    if (auto descriptor = characteristic->native_cccd()) {
        _descriptors[descriptor->getObject()] = characteristic;
    }
}

void PeripheralAndroid::publish(const std::shared_ptr<CharacteristicAndroid>& characteristic, const ByteArray& value) {
    if (!_started.load()) return;
    auto subscribers = characteristic->subscribers();
    if (subscribers.empty()) return;

    std::scoped_lock lock(_notification_mutex);
    const bool idle = _notifications.empty();
    for (const auto& subscriber : subscribers) {
        _notifications.push_back(
            PendingNotification{subscriber.device, characteristic->native_characteristic(), value, subscriber.confirm});
    }
    if (idle) _send_next_notification_locked();
}

void PeripheralAndroid::_ensure_mutable() const {
    if (_started.load()) {
        throw Exception::OperationFailed("The local peripheral cannot be changed while it is started.");
    }
}

void PeripheralAndroid::_shutdown() noexcept {
    _advertising.store(false);
    try {
        if (_advertiser) _advertiser.stopAdvertising(_advertise_callback);
    } catch (...) {
    }
    try {
        std::scoped_lock lock(_server_mutex);
        if (_server) {
            _server.clearServices();
            _server.close();
        }
    } catch (...) {
    }
    {
        std::scoped_lock lock(_notification_mutex);
        _notifications.clear();
    }
    {
        std::scoped_lock lock(_clients_mutex);
        _connected_clients.clear();
    }
}

void PeripheralAndroid::_respond(const Android::BluetoothDevice& device, int request_id, int status, int offset,
                                 const ByteArray& value) {
    try {
        std::scoped_lock lock(_server_mutex);
        if (_server && !_server.sendResponse(device, request_id, status, offset, value)) {
            SIMPLEBLE_LOG_WARN("Android rejected a GATT server response.");
        }
    } catch (const std::exception& exception) {
        SIMPLEBLE_LOG_ERROR(std::string("Failed to send Android GATT response: ") + exception.what());
    }
}

std::shared_ptr<CharacteristicAndroid> PeripheralAndroid::_characteristic_for(const JavaObject& object,
                                                                              bool descriptor) {
    std::scoped_lock lock(_characteristics_mutex);
    auto& items = descriptor ? _descriptors : _characteristics;
    auto item = items.find(object);
    if (item == items.end()) return {};
    auto characteristic = item->second.lock();
    if (!characteristic) items.erase(item);
    return characteristic;
}

void PeripheralAndroid::_handle_connection(Android::BluetoothDevice device, int status, int new_state) {
    const BluetoothAddress address(device.getAddress());
    bool connected = status == GATT_SUCCESS && new_state == STATE_CONNECTED;
    bool notify_connected = false;
    bool notify_disconnected = false;
    {
        std::scoped_lock lock(_clients_mutex);
        if (connected) {
            notify_connected = _connected_clients.insert(address).second;
        } else if (new_state == STATE_DISCONNECTED) {
            notify_disconnected = _connected_clients.erase(address) > 0;
        }
    }
    if (notify_disconnected) {
        std::vector<std::shared_ptr<CharacteristicAndroid>> characteristics;
        {
            std::scoped_lock lock(_characteristics_mutex);
            for (auto& [object, weak] : _characteristics) {
                if (auto characteristic = weak.lock()) characteristics.push_back(std::move(characteristic));
            }
        }
        for (const auto& characteristic : characteristics) characteristic->remove_subscriber(address);
        std::scoped_lock lock(_notification_mutex);
        _notifications.clear();
    }
    if (notify_connected) SAFE_CALLBACK_CALL(_callback_on_client_connected, address);
    if (notify_disconnected) SAFE_CALLBACK_CALL(_callback_on_client_disconnected, address);
}

void PeripheralAndroid::_handle_characteristic_read(Android::BluetoothDevice device, int request_id, int offset,
                                                    Android::BluetoothGattCharacteristic native) {
    auto characteristic = _characteristic_for(native.getObject(), false);
    if (!characteristic || !characteristic->can_read()) {
        _respond(device, request_id, GATT_READ_NOT_PERMITTED, offset);
        return;
    }
    try {
        auto value = characteristic->handle_read();
        if (offset < 0 || static_cast<size_t>(offset) > value.size()) {
            _respond(device, request_id, GATT_INVALID_OFFSET, offset);
            return;
        }
        _respond(device, request_id, GATT_SUCCESS, offset, ByteArray(value.begin() + offset, value.end()));
    } catch (const std::exception& exception) {
        SIMPLEBLE_LOG_ERROR(std::string("Android local read callback failed: ") + exception.what());
        _respond(device, request_id, GATT_REQUEST_NOT_SUPPORTED, offset);
    }
}

void PeripheralAndroid::_handle_characteristic_write(Android::BluetoothDevice device, int request_id,
                                                     Android::BluetoothGattCharacteristic native, bool prepared_write,
                                                     bool response_needed, int offset, ByteArray value) {
    auto characteristic = _characteristic_for(native.getObject(), false);
    int result = GATT_SUCCESS;
    if (!characteristic || (response_needed && !characteristic->can_write_request()) ||
        (!response_needed && !characteristic->can_write_command())) {
        result = GATT_WRITE_NOT_PERMITTED;
    } else if (prepared_write) {
        result = GATT_REQUEST_NOT_SUPPORTED;
    } else if (offset != 0) {
        result = GATT_INVALID_OFFSET;
    } else if (value.size() > 512) {
        result = GATT_INVALID_ATTRIBUTE_LENGTH;
    } else {
        try {
            characteristic->handle_write(std::move(value));
        } catch (const std::exception& exception) {
            SIMPLEBLE_LOG_ERROR(std::string("Android local write callback failed: ") + exception.what());
            result = GATT_REQUEST_NOT_SUPPORTED;
        }
    }
    if (response_needed) _respond(device, request_id, result, offset);
}

void PeripheralAndroid::_handle_descriptor_read(Android::BluetoothDevice device, int request_id, int offset,
                                                Android::BluetoothGattDescriptor descriptor) {
    auto characteristic = _characteristic_for(descriptor.getObject(), true);
    if (!characteristic) {
        _respond(device, request_id, GATT_READ_NOT_PERMITTED, offset);
        return;
    }
    auto value = characteristic->subscription_value(device.getAddress());
    if (offset < 0 || static_cast<size_t>(offset) > value.size()) {
        _respond(device, request_id, GATT_INVALID_OFFSET, offset);
        return;
    }
    _respond(device, request_id, GATT_SUCCESS, offset, ByteArray(value.begin() + offset, value.end()));
}

void PeripheralAndroid::_handle_descriptor_write(Android::BluetoothDevice device, int request_id,
                                                 Android::BluetoothGattDescriptor descriptor, bool prepared_write,
                                                 bool response_needed, int offset, ByteArray value) {
    auto characteristic = _characteristic_for(descriptor.getObject(), true);
    int result = GATT_SUCCESS;
    if (!characteristic)
        result = GATT_WRITE_NOT_PERMITTED;
    else if (prepared_write)
        result = GATT_REQUEST_NOT_SUPPORTED;
    else if (offset != 0)
        result = GATT_INVALID_OFFSET;
    else if (!characteristic->set_subscription(device, value))
        result = GATT_REQUEST_NOT_SUPPORTED;
    if (response_needed) _respond(device, request_id, result, offset);
}

void PeripheralAndroid::_send_next_notification_locked() {
    while (_started.load() && !_notifications.empty()) {
        auto& notification = _notifications.front();
        try {
            notification.characteristic.setValue(notification.value);
            std::scoped_lock server_lock(_server_mutex);
            if (_server && _server.notifyCharacteristicChanged(notification.device, notification.characteristic,
                                                               notification.confirm)) {
                return;
            }
            SIMPLEBLE_LOG_WARN("Android rejected local value notification.");
        } catch (const std::exception& exception) {
            SIMPLEBLE_LOG_WARN(std::string("Failed to publish Android local value: ") + exception.what());
        }
        _notifications.pop_front();
    }
}

}  // namespace SimpleBLE::Local
