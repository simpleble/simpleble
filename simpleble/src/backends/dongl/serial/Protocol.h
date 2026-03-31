#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "protocol/d2h.pb.h"
#include "protocol/h2d.pb.h"

#include "ProtocolBase.h"

namespace SimpleBLE {
namespace Dongl {
namespace Serial {

class Protocol : public ProtocolBase {
  public:
    Protocol(const std::string& device_path);
    virtual ~Protocol();

    basic_WhoamiRsp basic_whoami();
    basic_ResetRsp basic_reset();
    basic_DfuStartRsp basic_dfu_start(uint32_t expected_version, uint32_t total_length, uint32_t crc32, bool force_update);
    basic_DfuChunkRsp basic_dfu_chunk(uint32_t page_index, uint32_t offset_in_page, const std::vector<uint8_t>& encrypted_data);
    basic_DfuMetadataRsp basic_dfu_metadata();
    basic_DfuVerifyRsp basic_dfu_verify();
    basic_DfuRebootRsp basic_dfu_reboot(bool force_update);
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
