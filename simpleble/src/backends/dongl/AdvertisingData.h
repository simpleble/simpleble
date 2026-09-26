#pragma once

#include <simpleble/Types.h>
#include <cstdint>
#include <limits>
#include <map>
#include <string>

namespace SimpleBLE {
namespace Dongl {

// Dongl-specific counterpart of SimpleBLE::advertising_data_t (backends/common/AdapterBaseTypes.h). It lives in its
// own namespace because two different definitions of the same type in one namespace violate the one-definition rule
// and corrupt fields on builds that compile both.
struct advertising_data_t {
    std::string identifier;
    bool identifier_complete = false;
    BluetoothAddressType address_type;
    BluetoothAddress mac_address;
    bool connectable;
    bool scan_response = false;  // The fields came from a scan response rather than the advertisement.
    bool scannable = false;      // The advertisement accepts scan requests.
    int16_t rssi = std::numeric_limits<int16_t>::min();
    int16_t tx_power = std::numeric_limits<int16_t>::min();

    std::map<uint16_t, ByteArray> manufacturer_data;
    std::map<BluetoothUUID, ByteArray> service_data;
};

}  // namespace Dongl
}  // namespace SimpleBLE
