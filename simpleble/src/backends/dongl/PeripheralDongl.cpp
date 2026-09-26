#include "PeripheralDongl.h"

#include "CharacteristicBase.h"
#include "DescriptorBase.h"
#include "ServiceBase.h"

#include <simpleble/Exceptions.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

#include "CommonUtils.h"
#include "LoggingInternal.h"
#include "Utils.h"
#include "fmt/chrono.h"
#include "protocol/simpleble.pb.h"
#include "simpleble/Types.h"

#include <fmt/core.h>

using namespace SimpleBLE;
using namespace std::chrono_literals;

PeripheralDongl::PeripheralDongl(std::shared_ptr<Dongl::Serial::Protocol> serial_protocol,
                                 Dongl::advertising_data_t advertising_data) {
    _serial_protocol = serial_protocol;
    _address_type = advertising_data.address_type;
    _address = advertising_data.mac_address;
    _connectable = advertising_data.connectable;
    update_advertising_data(advertising_data);
}

PeripheralDongl::~PeripheralDongl() {}

void* PeripheralDongl::underlying() const { return nullptr; }

std::string PeripheralDongl::identifier() {
    std::lock_guard<std::mutex> lock(_advertising_mutex);
    return _identifier;
}

BluetoothAddress PeripheralDongl::address() { return _address; }

BluetoothAddressType PeripheralDongl::address_type() { return _address_type; }

int16_t PeripheralDongl::rssi() {
    std::lock_guard<std::mutex> lock(_advertising_mutex);
    return _rssi;
}

int16_t PeripheralDongl::tx_power() {
    std::lock_guard<std::mutex> lock(_advertising_mutex);
    return _advertisement.tx_power != std::numeric_limits<int16_t>::min() ? _advertisement.tx_power
                                                                          : _scan_response.tx_power;
}

uint16_t PeripheralDongl::mtu() { return _mtu; }

void PeripheralDongl::connect() {
    if (is_connected()) {
        return;
    }

    _conn_handle = BLE_CONN_HANDLE_INVALID;
    _mtu = 0;
    _services.clear();
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        _connect_result.reset();
        _connect_pending = true;
    }

    // The Dongl owns the attempt, including retries, and reports its outcome exactly once within the timeout.
    simpleble_ConnectRsp response;
    try {
        response = _serial_protocol->simpleble_connect(static_cast<simpleble_BluetoothAddressType>(_address_type),
                                                       _address, CONNECT_TIMEOUT.count());
    } catch (...) {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        _connect_pending = false;
        throw;
    }
    if (response.ret_code != 0) {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        _connect_pending = false;
        throw Exception::OperationFailed(fmt::format("Error when attempting to connect: {}", response.ret_code));
    }

    simpleble_ConnectCompleteEvt result;
    {
        std::unique_lock<std::mutex> lock(connection_mutex_);
        // Waiting past the Dongl's own deadline means the Dongl stopped responding. Don't resend the command.
        const bool reported = connection_cv_.wait_for(lock, CONNECT_TIMEOUT + CONNECT_RESULT_MARGIN,
                                                      [this]() { return _connect_result.has_value(); });
        _connect_pending = false;
        if (!reported) {
            _conn_handle = BLE_CONN_HANDLE_INVALID;
            throw Exception::OperationFailed("Dongl did not report the connection result");
        }
        result = *_connect_result;
    }

    if (result.status != simpleble_ConnectStatus_CONNECT_SUCCESS) {
        _conn_handle = BLE_CONN_HANDLE_INVALID;
        throw Exception::OperationFailed(fmt::format("Connection failed: {} (HCI reason 0x{:02X})",
                                                     Dongl::connect_status_to_string(result.status),
                                                     result.hci_reason));
    }

    // ConnectionEvt set the handle. If the link dropped since, the disconnection cleared it: don't restore it.
    if (_conn_handle != result.conn_handle) {
        throw Exception::OperationFailed("Connection lost right after it was established");
    }
    _mtu = result.mtu;
    _resolve_missing_uuids();

    _connection_announced = true;
    SAFE_CALLBACK_CALL(this->_callback_on_connected);
}

void PeripheralDongl::disconnect() {
    if (!is_connected()) {
        return;
    }

    auto response = _serial_protocol->simpleble_disconnect(_conn_handle);
    if (response.ret_code != 0) {
        throw Exception::OperationFailed(fmt::format("Failed to disconnect: {}", response.ret_code));
    }

    // Wait for the disconnection to be confirmed.
    std::unique_lock<std::mutex> lock(disconnection_mutex_);
    disconnection_cv_.wait_for(lock, 500ms, [this]() { return !is_connected(); });

    if (is_connected()) {
        _conn_handle = BLE_CONN_HANDLE_INVALID;
        throw Exception::OperationFailed(fmt::format("Timeout while waiting for disconnection confirmation"));
    }
}

bool PeripheralDongl::is_connected() { return _conn_handle != BLE_CONN_HANDLE_INVALID; }

bool PeripheralDongl::is_connectable() {
    std::lock_guard<std::mutex> lock(_advertising_mutex);
    return _connectable;
}

bool PeripheralDongl::is_paired() {
    return _serial_protocol->simpleble_is_paired(static_cast<simpleble_BluetoothAddressType>(_address_type), _address)
        .is_paired;
}

void PeripheralDongl::unpair() {
    auto response = _serial_protocol->simpleble_unpair(static_cast<simpleble_BluetoothAddressType>(_address_type),
                                                       _address);
    if (response.ret_code != 0) {
        throw Exception::OperationFailed(fmt::format("Failed to unpair: {}", response.ret_code));
    }
}

void PeripheralDongl::set_passkey_request_callback(const std::function<std::optional<std::string>()>& callback) {
    if (callback) {
        passkey_request_callback_.load(callback);
    } else {
        passkey_request_callback_.unload();
    }
}

void PeripheralDongl::set_passkey_display_callback(const std::function<void(const std::string& passkey)>& callback) {
    if (callback) {
        passkey_display_callback_.load(callback);
    } else {
        passkey_display_callback_.unload();
    }
}

void PeripheralDongl::set_numeric_comparison_callback(const std::function<bool(const std::string& passkey)>& callback) {
    if (callback) {
        numeric_comparison_callback_.load(callback);
    } else {
        numeric_comparison_callback_.unload();
    }
}

SharedPtrVector<ServiceBase> PeripheralDongl::available_services() {
    SharedPtrVector<ServiceBase> service_list;
    for (auto& service : _services) {
        SharedPtrVector<CharacteristicBase> characteristic_list;
        for (auto& characteristic : service.characteristics) {
            SharedPtrVector<DescriptorBase> descriptor_list;
            for (auto& descriptor : characteristic.descriptors) {
                descriptor_list.push_back(std::make_shared<DescriptorBase>(descriptor.uuid));
            }
            characteristic_list.push_back(std::make_shared<CharacteristicBase>(
                characteristic.uuid, descriptor_list, characteristic.can_read, characteristic.can_write_request,
                characteristic.can_write_command, characteristic.can_notify, characteristic.can_indicate));
        }
        service_list.push_back(std::make_shared<ServiceBase>(service.uuid, characteristic_list));
    }

    return service_list;
}

SharedPtrVector<ServiceBase> PeripheralDongl::advertised_services() {
    std::lock_guard<std::mutex> lock(_advertising_mutex);
    SharedPtrVector<ServiceBase> service_list;
    for (auto& [service_uuid, data] : _service_data) {
        service_list.push_back(std::make_shared<ServiceBase>(service_uuid, data));
    }

    return service_list;
}

std::map<uint16_t, ByteArray> PeripheralDongl::manufacturer_data() {
    std::lock_guard<std::mutex> lock(_advertising_mutex);
    auto data = _scan_response.manufacturer_data;
    for (auto& [company_id, bytes] : _advertisement.manufacturer_data) {
        data[company_id] = bytes;
    }
    return data;
}

ByteArray PeripheralDongl::read(BluetoothUUID const& service_uuid, BluetoothUUID const& characteristic_uuid) {
    auto& characteristic = _find_characteristic_from_uuid(service_uuid, characteristic_uuid);

    if (!characteristic.can_read) {
        throw Exception::OperationFailed(fmt::format("Characteristic {} is not readable", characteristic_uuid));
    }

    simpleble_ReadRsp rsp = _serial_protocol->simpleble_read(_conn_handle, characteristic.handle_value);
    if (rsp.ret_code != 0) {
        throw Exception::OperationFailed(
            fmt::format("Failed to read characteristic {} - ret_code: {}", characteristic_uuid, rsp.ret_code));
    }

    return ByteArray(rsp.data.bytes, rsp.data.size);
}

void PeripheralDongl::write_request(BluetoothUUID const& service_uuid, BluetoothUUID const& characteristic_uuid,
                                    ByteArray const& data) {
    auto& characteristic = _find_characteristic_from_uuid(service_uuid, characteristic_uuid);

    if (!characteristic.can_write_request) {
        throw Exception::OperationFailed(fmt::format("Characteristic {} is not writable", characteristic_uuid));
    }

    simpleble_WriteRsp rsp = _serial_protocol->simpleble_write(_conn_handle, characteristic.handle_value,
                                                               simpleble_WriteOperation_WRITE_REQ, data);
    if (rsp.ret_code != 0) {
        throw Exception::OperationFailed(
            fmt::format("Failed to write characteristic {} - ret_code: {}", characteristic_uuid, rsp.ret_code));
    }
}

void PeripheralDongl::write_command(BluetoothUUID const& service_uuid, BluetoothUUID const& characteristic_uuid,
                                    ByteArray const& data) {
    auto& characteristic = _find_characteristic_from_uuid(service_uuid, characteristic_uuid);

    if (!characteristic.can_write_command) {
        throw Exception::OperationFailed(fmt::format("Characteristic {} is not writable", characteristic_uuid));
    }

    simpleble_WriteRsp rsp = _serial_protocol->simpleble_write(_conn_handle, characteristic.handle_value,
                                                               simpleble_WriteOperation_WRITE_CMD, data);
    if (rsp.ret_code != 0) {
        throw Exception::OperationFailed(
            fmt::format("Failed to write characteristic {} - ret_code: {}", characteristic_uuid, rsp.ret_code));
    }
}

void PeripheralDongl::notify(BluetoothUUID const& service_uuid, BluetoothUUID const& characteristic_uuid,
                             std::function<void(ByteArray payload)> callback) {
    auto& characteristic = _find_characteristic_from_uuid(service_uuid, characteristic_uuid);

    if (!characteristic.can_notify) {
        throw Exception::OperationFailed(fmt::format("Characteristic {} is not notifyable", characteristic_uuid));
    }

    if (characteristic.handle_cccd == 0) {
        throw Exception::OperationFailed(fmt::format("Characteristic {} does not have a CCCD", characteristic_uuid));
    }

    _callbacks_on_value_changed[characteristic.handle_value] = std::move(callback);

    ByteArray data = {0x01, 0x00};
    simpleble_WriteRsp rsp = _serial_protocol->simpleble_write(_conn_handle, characteristic.handle_cccd,
                                                               simpleble_WriteOperation_WRITE_REQ, data);
    if (rsp.ret_code != 0) {
        throw Exception::OperationFailed(
            fmt::format("Failed to write characteristic {} - ret_code: {}", characteristic_uuid, rsp.ret_code));
    }
}

void PeripheralDongl::indicate(BluetoothUUID const& service_uuid, BluetoothUUID const& characteristic_uuid,
                               std::function<void(ByteArray payload)> callback) {
    auto& characteristic = _find_characteristic_from_uuid(service_uuid, characteristic_uuid);

    if (!characteristic.can_indicate) {
        throw Exception::OperationFailed(fmt::format("Characteristic {} is not indicateable", characteristic_uuid));
    }

    if (characteristic.handle_cccd == 0) {
        throw Exception::OperationFailed(fmt::format("Characteristic {} does not have a CCCD", characteristic_uuid));
    }

    _callbacks_on_value_changed[characteristic.handle_value] = callback;

    ByteArray data = {0x02, 0x00};
    simpleble_WriteRsp rsp = _serial_protocol->simpleble_write(_conn_handle, characteristic.handle_cccd,
                                                               simpleble_WriteOperation_WRITE_REQ, data);
    if (rsp.ret_code != 0) {
        throw Exception::OperationFailed(
            fmt::format("Failed to write characteristic {} - ret_code: {}", characteristic_uuid, rsp.ret_code));
    }
}

void PeripheralDongl::unsubscribe(BluetoothUUID const& service_uuid, BluetoothUUID const& characteristic_uuid) {
    auto& characteristic = _find_characteristic_from_uuid(service_uuid, characteristic_uuid);

    if (characteristic.handle_cccd == 0) {
        throw Exception::OperationFailed(fmt::format("Characteristic {} does not have a CCCD", characteristic_uuid));
    }

    _callbacks_on_value_changed.erase(characteristic.handle_value);

    ByteArray data = {0x00, 0x00};
    simpleble_WriteRsp rsp = _serial_protocol->simpleble_write(_conn_handle, characteristic.handle_cccd,
                                                               simpleble_WriteOperation_WRITE_REQ, data);
    if (rsp.ret_code != 0) {
        throw Exception::OperationFailed(
            fmt::format("Failed to write characteristic {} - ret_code: {}", characteristic_uuid, rsp.ret_code));
    }
}

ByteArray PeripheralDongl::read(BluetoothUUID const& service_uuid, BluetoothUUID const& characteristic_uuid,
                                BluetoothUUID const& descriptor_uuid) {
    auto& descriptor = _find_descriptor_from_uuid(service_uuid, characteristic_uuid, descriptor_uuid);
    simpleble_ReadRsp rsp = _serial_protocol->simpleble_read(_conn_handle, descriptor.handle);
    if (rsp.ret_code != 0) {
        throw Exception::OperationFailed(
            fmt::format("Failed to read descriptor {} - ret_code: {}", descriptor_uuid, rsp.ret_code));
    }

    return ByteArray(rsp.data.bytes, rsp.data.size);
}

void PeripheralDongl::write(BluetoothUUID const& service_uuid, BluetoothUUID const& characteristic_uuid,
                            BluetoothUUID const& descriptor_uuid, ByteArray const& data) {
    auto& descriptor = _find_descriptor_from_uuid(service_uuid, characteristic_uuid, descriptor_uuid);
    simpleble_WriteRsp rsp = _serial_protocol->simpleble_write(_conn_handle, descriptor.handle,
                                                               simpleble_WriteOperation_WRITE_REQ, data);
    if (rsp.ret_code != 0) {
        throw Exception::OperationFailed(
            fmt::format("Failed to write descriptor {} - ret_code: {}", descriptor_uuid, rsp.ret_code));
    }
}

void PeripheralDongl::set_callback_on_connected(std::function<void()> on_connected) {
    if (on_connected) {
        _callback_on_connected.load(std::move(on_connected));
    } else {
        _callback_on_connected.unload();
    }
}

void PeripheralDongl::set_callback_on_disconnected(std::function<void()> on_disconnected) {
    if (on_disconnected) {
        _callback_on_disconnected.load(std::move(on_disconnected));
    } else {
        _callback_on_disconnected.unload();
    }
}

uint16_t PeripheralDongl::conn_handle() const { return _conn_handle; }

void PeripheralDongl::update_advertising_data(Dongl::advertising_data_t advertising_data) {
    std::lock_guard<std::mutex> lock(_advertising_mutex);
    if (!advertising_data.identifier.empty() && (advertising_data.identifier_complete || !_identifier_complete)) {
        _identifier = advertising_data.identifier;
        _identifier_complete = advertising_data.identifier_complete;
    }
    _rssi = advertising_data.rssi;
    if (!advertising_data.scan_response) {
        _connectable = advertising_data.connectable;
        if (!advertising_data.scannable) {
            // No scan response follows a non-scannable advertisement, so the last one no longer applies.
            _scan_response = {};
        }
    }

    auto& packet = advertising_data.scan_response ? _scan_response : _advertisement;
    packet.tx_power = advertising_data.tx_power;
    packet.manufacturer_data = std::move(advertising_data.manufacturer_data);

    for (auto& [uuid, data] : advertising_data.service_data) {
        _service_data[uuid] = std::move(data);
    }
}

void PeripheralDongl::_resolve_missing_uuids() {
    // The Dongl omits 128-bit UUIDs it could not resolve, so read those declarations.
    for (auto& service : _services) {
        // Fetch the service UUID if missing.
        if (service.uuid.empty()) {
            simpleble_ReadRsp rsp = _serial_protocol->simpleble_read(_conn_handle, service.start_handle);
            if (rsp.ret_code != 0) {
                SIMPLEBLE_LOG_ERROR(fmt::format("Failed to read UUID for service {} - ret_code: {}",
                                                service.start_handle, rsp.ret_code));
                continue;
            }

            if (rsp.data.size == 2) {
                service.uuid = Dongl::uuid_from_uuid16(rsp.data.bytes[1] << 8 | rsp.data.bytes[0]);
            } else if (rsp.data.size == 16) {
                uint8_t uuid_128[16];
                for (int i = 0; i < 16; i++) {
                    uuid_128[i] = rsp.data.bytes[15 - i];
                }
                service.uuid = Dongl::uuid_from_uuid128(uuid_128);
            } else {
                SIMPLEBLE_LOG_ERROR(fmt::format("Unexpected UUID size: {}", rsp.data.size));
                continue;
            }
        }

        for (auto& characteristic : service.characteristics) {
            // Fetch the characteristic UUID if missing.
            if (characteristic.uuid.empty()) {
                simpleble_ReadRsp rsp = _serial_protocol->simpleble_read(_conn_handle, characteristic.handle_decl);
                if (rsp.ret_code != 0) {
                    SIMPLEBLE_LOG_ERROR(fmt::format("Failed to read UUID for characteristic {} - ret_code: {}",
                                                    characteristic.handle_decl, rsp.ret_code));
                    continue;
                }

                if (rsp.data.size == 5) {
                    characteristic.uuid = Dongl::uuid_from_uuid16(rsp.data.bytes[4] << 8 | rsp.data.bytes[3]);
                } else if (rsp.data.size == 19) {
                    uint8_t uuid_128[16];
                    for (int i = 0; i < 16; i++) {
                        uuid_128[i] = rsp.data.bytes[15 - i + 3];
                    }
                    characteristic.uuid = Dongl::uuid_from_uuid128(uuid_128);
                }
            }
        }
    }
}

void PeripheralDongl::notify_connected(uint16_t conn_handle) {
    _conn_handle = conn_handle;
    connection_cv_.notify_all();
}

void PeripheralDongl::notify_disconnected() {
    const bool notify = _connection_announced.exchange(false);
    _conn_handle = BLE_CONN_HANDLE_INVALID;
    _mtu = 0;
    disconnection_cv_.notify_all();

    if (notify) {
        SAFE_CALLBACK_CALL(this->_callback_on_disconnected);
    }
}

void PeripheralDongl::notify_service_discovered(simpleble_ServiceDiscoveredEvt const& evt) {
    BluetoothUUID uuid;
    if (evt.has_uuid) {
        uuid = Dongl::uuid_from_proto(evt.uuid);
    }

    _services.emplace_back(ServiceDefinition{
        uuid,
        evt.start_handle,
        evt.end_handle,
    });
}

void PeripheralDongl::notify_characteristic_discovered(simpleble_CharacteristicDiscoveredEvt const& evt) {
    auto& service = _find_service_from_handle(evt.handle_decl);

    BluetoothUUID uuid;
    if (evt.has_uuid) {
        uuid = Dongl::uuid_from_proto(evt.uuid);
    }

    service.characteristics.emplace_back(CharacteristicDefinition{
        uuid,
        evt.handle_decl,
        evt.handle_value,
        0,
        evt.props.read,
        evt.props.write,
        evt.props.write_wo_resp,
        evt.props.notify,
        evt.props.indicate,
    });
}

void PeripheralDongl::notify_descriptor_discovered(simpleble_DescriptorDiscoveredEvt const& evt) {
    auto& service = _find_service_from_handle(evt.handle);

    for (auto& characteristic : service.characteristics) {
        // If the descriptor matches the characteristic declaration handle or value handle, we can ignore it.
        if (characteristic.handle_decl == evt.handle || characteristic.handle_value == evt.handle) {
            return;
        }
    }

    // At this point we know we have a real descriptor that we shouldn't ignore.

    auto& characteristic = _find_characteristic_from_handle(evt.handle);
    characteristic.descriptors.emplace_back(DescriptorDefinition{
        Dongl::uuid_from_proto(evt.uuid),
        evt.handle,
    });

    // If the descriptor is a client characteristic configuration descriptor (CCCD),
    // save that handle number for the characteristic.
    if (evt.uuid.which_uuid == simpleble_UUID_uuid16_tag && evt.uuid.uuid.uuid16.uuid == 0x2902) {
        characteristic.handle_cccd = evt.handle;
    }
}

void PeripheralDongl::notify_connect_complete(simpleble_ConnectCompleteEvt const& evt) {
    bool orphaned;
    {
        std::lock_guard<std::mutex> lock(connection_mutex_);
        _connect_result = evt;
        orphaned = !_connect_pending && evt.status == simpleble_ConnectStatus_CONNECT_SUCCESS;
    }
    connection_cv_.notify_all();

    if (orphaned) {
        // connect() already gave up on this attempt, so nobody will use the link.
        SIMPLEBLE_LOG_WARN("Connection completed after connect() gave up; disconnecting");
        task_runner_.dispatch(
            [this, conn_handle = evt.conn_handle]() -> std::optional<std::chrono::milliseconds> {
                try {
                    _serial_protocol->simpleble_disconnect(conn_handle);
                } catch (const std::exception& e) {
                    SIMPLEBLE_LOG_ERROR(fmt::format("Failed to disconnect abandoned connection: {}", e.what()));
                }
                return std::nullopt;
            },
            0ms);
    }
}

void PeripheralDongl::notify_value_changed(simpleble_ValueChangedEvt const& evt) {
    ByteArray data(evt.data.bytes, evt.data.bytes + evt.data.size);
    std::function<void(ByteArray)> callback = _callbacks_on_value_changed[evt.handle];
    if (callback) {
        callback(data);
    }
}

void PeripheralDongl::notify_passkey_display(simpleble_PasskeyDisplayEvt const& evt) {
    const std::string passkey = evt.passkey;
    if (evt.match_request) {
        task_runner_.dispatch(
            [this, conn_handle = evt.conn_handle, request_id = evt.request_id,
             passkey]() -> std::optional<std::chrono::milliseconds> {
                bool accept = false;
                try {
                    accept = numeric_comparison_callback_(passkey);
                } catch (const std::exception& e) {
                    SIMPLEBLE_LOG_ERROR(fmt::format("Numeric comparison callback failed: {}", e.what()));
                } catch (...) {
                    SIMPLEBLE_LOG_ERROR("Numeric comparison callback failed with an unknown exception");
                }

                _send_auth_key_reply(conn_handle, request_id, {}, accept);
                return std::nullopt;
            },
            0ms);
        return;
    }

    task_runner_.dispatch(
        [this, passkey]() -> std::optional<std::chrono::milliseconds> {
            try {
                passkey_display_callback_(passkey);
            } catch (const std::exception& e) {
                SIMPLEBLE_LOG_ERROR(fmt::format("Passkey display callback failed: {}", e.what()));
            } catch (...) {
                SIMPLEBLE_LOG_ERROR("Passkey display callback failed with an unknown exception");
            }
            return std::nullopt;
        },
        0ms);
}

void PeripheralDongl::notify_auth_key_request(simpleble_AuthKeyRequestEvt const& evt) {
    const bool request_passkey = evt.key_type == simpleble_PairingAuthKeyType_PAIRING_AUTH_KEY_PASSKEY;
    if (!request_passkey) {
        SIMPLEBLE_LOG_WARN(fmt::format("Unsupported pairing key type: {}", static_cast<int>(evt.key_type)));
    }

    task_runner_.dispatch(
        [this, request_passkey, conn_handle = evt.conn_handle,
         request_id = evt.request_id]() -> std::optional<std::chrono::milliseconds> {
            std::optional<std::string> passkey;
            try {
                if (request_passkey) {
                    passkey = passkey_request_callback_();
                }
            } catch (const std::exception& e) {
                SIMPLEBLE_LOG_ERROR(fmt::format("Passkey request callback failed: {}", e.what()));
            } catch (...) {
                SIMPLEBLE_LOG_ERROR("Passkey request callback failed with an unknown exception");
            }

            std::vector<uint8_t> key;
            const bool accept = passkey && passkey->size() == 6 &&
                                std::all_of(passkey->begin(), passkey->end(),
                                            [](char c) { return c >= '0' && c <= '9'; });
            if (accept) {
                key.assign(passkey->begin(), passkey->end());
            } else if (passkey) {
                SIMPLEBLE_LOG_WARN("Passkey request callback returned an invalid passkey");
            }

            _send_auth_key_reply(conn_handle, request_id, key, accept);
            return std::nullopt;
        },
        0ms);
}

void PeripheralDongl::_send_auth_key_reply(uint16_t conn_handle, uint32_t request_id, const std::vector<uint8_t>& key,
                                           bool accept) {
    try {
        auto response = _serial_protocol->simpleble_auth_key_reply(conn_handle, request_id, key, accept);
        if (response.ret_code != 0) {
            SIMPLEBLE_LOG_ERROR(fmt::format("Failed to reply to pairing request: {}", response.ret_code));
        }
    } catch (const std::exception& e) {
        SIMPLEBLE_LOG_ERROR(fmt::format("Failed to send pairing reply: {}", e.what()));
    } catch (...) {
        SIMPLEBLE_LOG_ERROR("Failed to send pairing reply with an unknown exception");
    }
}

PeripheralDongl::ServiceDefinition& PeripheralDongl::_find_service_from_handle(uint16_t handle) {
    for (auto& service : _services) {
        if (service.start_handle <= handle && service.end_handle >= handle) {
            return service;
        }
    }

    throw std::runtime_error(fmt::format("Service not found for handle {}", handle));
}

PeripheralDongl::CharacteristicDefinition& PeripheralDongl::_find_characteristic_from_handle(uint16_t handle) {
    for (auto& service : _services) {
        if (service.start_handle <= handle && service.end_handle >= handle) {
            // For the given service handle, loop the characteristics backwards and select the first characteristic
            // where the handle_value is less than the descriptor handle.
            for (auto it = service.characteristics.rbegin(); it != service.characteristics.rend(); ++it) {
                if (it->handle_value < handle) {
                    return *it;
                }
            }
        }
    }

    throw std::runtime_error(fmt::format("Characteristic not found for handle {}", handle));
}

PeripheralDongl::CharacteristicDefinition& PeripheralDongl::_find_characteristic_from_uuid(
    BluetoothUUID const& service_uuid, BluetoothUUID const& characteristic_uuid) {
    for (auto& service : _services) {
        if (service.uuid == service_uuid) {
            for (auto& characteristic : service.characteristics) {
                if (characteristic.uuid == characteristic_uuid) {
                    return characteristic;
                }
            }
        }
    }

    throw std::runtime_error(fmt::format("Characteristic {} not found", characteristic_uuid));
}

PeripheralDongl::DescriptorDefinition& PeripheralDongl::_find_descriptor_from_uuid(
    BluetoothUUID const& service_uuid, BluetoothUUID const& characteristic_uuid, BluetoothUUID const& descriptor_uuid) {
    auto& characteristic = _find_characteristic_from_uuid(service_uuid, characteristic_uuid);
    for (auto& descriptor : characteristic.descriptors) {
        if (descriptor.uuid == descriptor_uuid) {
            return descriptor;
        }
    }

    throw std::runtime_error(fmt::format("Descriptor {} not found", descriptor_uuid));
}
