#include "Utils.h"

#include <stdexcept>

#include <fmt/core.h>

namespace SimpleBLE {
namespace Dongl {

BluetoothUUID uuid_from_uuid16(uint16_t uuid) {
    return BluetoothUUID(fmt::format("0000{:04X}-0000-1000-8000-00805F9B34FB", uuid));
}

BluetoothUUID uuid_from_uuid32(uint32_t uuid) {
    return BluetoothUUID(fmt::format("{:08X}-0000-1000-8000-00805F9B34FB", uuid));
}

BluetoothUUID uuid_from_uuid128(const uint8_t uuid[16]) {
    return BluetoothUUID(fmt::format(
        "{:02X}{:02X}{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}-{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}",
        uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11],
        uuid[12], uuid[13], uuid[14], uuid[15]));
}

BluetoothUUID uuid_from_proto(const simpleble_UUID& uuid) {
    switch (uuid.which_uuid) {
        case simpleble_UUID_uuid16_tag:
            return uuid_from_uuid16(uuid.uuid.uuid16.uuid);
        case simpleble_UUID_uuid32_tag:
            return uuid_from_uuid32(uuid.uuid.uuid32.uuid);
        case simpleble_UUID_uuid128_tag:
            return uuid_from_uuid128(uuid.uuid.uuid128.uuid);
        default:
            throw std::runtime_error(fmt::format("Unknown UUID type: {}", uuid.which_uuid));
    }
}

const char* connect_status_to_string(simpleble_ConnectStatus status) {
    switch (status) {
        case simpleble_ConnectStatus_CONNECT_SUCCESS:
            return "success";
        case simpleble_ConnectStatus_CONNECT_TIMEOUT:
            return "timed out";
        case simpleble_ConnectStatus_CONNECT_CANCELLED:
            return "cancelled";
        case simpleble_ConnectStatus_CONNECT_DISCONNECTED:
            return "disconnected";
        case simpleble_ConnectStatus_CONNECT_DISCOVERY_FAILED:
            return "attribute discovery failed";
        default:
            return "unknown status";
    }
}

}  // namespace Dongl
}  // namespace SimpleBLE
