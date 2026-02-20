#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "ProtocolBase.h"
#include "Wire.h"

#include "protocol/d2h.pb.h"
#include "protocol/h2d.pb.h"

namespace SimpleBLE {
namespace Dongl {
namespace Serial {

class Protocol : public ProtocolBase {
  public:
    Protocol(const std::string& device_path);
    ~Protocol();

    basic_WhoamiRsp basic_whoami();
    basic_ResetRsp basic_reset();
    basic_DfuStartRsp basic_dfu_start();
    basic_PowerOnRsp basic_power_on();
    basic_PowerOffRsp basic_power_off();

    simpleble_InitRsp simpleble_init();
    simpleble_ScanStartRsp simpleble_scan_start();
    simpleble_ScanStopRsp simpleble_scan_stop();
    simpleble_ConnectRsp simpleble_connect(simpleble_BluetoothAddressType address_type, const std::string& address);
    simpleble_DisconnectRsp simpleble_disconnect(uint16_t conn_handle);
    simpleble_ReadRsp simpleble_read(uint16_t conn_handle, uint16_t handle);
    simpleble_WriteRsp simpleble_write(uint16_t conn_handle, uint16_t handle, simpleble_WriteOperation operation, const std::vector<uint8_t>& data);
};

}  // namespace Serial
}  // namespace Dongl
}  // namespace SimpleBLE
