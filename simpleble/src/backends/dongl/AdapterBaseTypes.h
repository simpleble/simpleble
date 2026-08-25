#pragma once

#include <simpleble/Types.h>
#include <cstdint>
#include <limits>
#include <map>
#include <string>

namespace SimpleBLE {

struct advertising_data_t {
    std::string identifier;
    bool identifier_complete = false;
    BluetoothAddressType address_type;
    BluetoothAddress mac_address;
    bool connectable;
    int16_t rssi = std::numeric_limits<int16_t>::min();
    int16_t tx_power = std::numeric_limits<int16_t>::min();

    std::map<uint16_t, ByteArray> manufacturer_data;
    std::map<BluetoothUUID, ByteArray> service_data;
};

}  // namespace SimpleBLE
