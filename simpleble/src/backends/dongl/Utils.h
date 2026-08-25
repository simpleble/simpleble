#pragma once

#include <cstdint>

#include <simpleble/Types.h>

#include "protocol/simpleble.pb.h"

namespace SimpleBLE {
namespace Dongl {

BluetoothUUID uuid_from_uuid16(uint16_t uuid);
BluetoothUUID uuid_from_uuid32(uint32_t uuid);
BluetoothUUID uuid_from_uuid128(const uint8_t uuid[16]);
BluetoothUUID uuid_from_proto(const simpleble_UUID& uuid);

}  // namespace Dongl
}  // namespace SimpleBLE
