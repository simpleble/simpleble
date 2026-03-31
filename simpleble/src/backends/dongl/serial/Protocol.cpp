#include "Protocol.h"

#include <cstring>
#include "fmt/base.h"
#include "protocol/simpleble.pb.h"

using namespace SimpleBLE::Dongl::Serial;

Protocol::Protocol(const std::string& device_path) : ProtocolBase(device_path) {}

Protocol::~Protocol() {}

basic_WhoamiRsp Protocol::basic_whoami() {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_basic_tag;
    command.cmd.basic.which_cmd = basic_Command_whoami_tag;
    basic_WhoamiCmd whoami_cmd = basic_WhoamiCmd_init_default;
    command.cmd.basic.cmd.whoami = whoami_cmd;

    dongl_Response response = exchange(command);
    return response.rsp.basic.rsp.whoami;
}

basic_ResetRsp Protocol::basic_reset() {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_basic_tag;
    command.cmd.basic.which_cmd = basic_Command_reset_tag;
    basic_ResetCmd reset_cmd = basic_ResetCmd_init_default;
    command.cmd.basic.cmd.reset = reset_cmd;

    dongl_Response response = exchange(command);
    return response.rsp.basic.rsp.reset;
}

basic_DfuStartRsp Protocol::basic_dfu_start(uint32_t expected_version, uint32_t total_length, uint32_t crc32, bool force_update) {
    fmt::print("Protocol::basic_dfu_start: expected_version={}, total_length={}, crc32=0x{:08x}, force_update={}\n", expected_version,
               total_length, crc32, force_update);
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_basic_tag;
    command.cmd.basic.which_cmd = basic_Command_dfu_start_tag;

    command.cmd.basic.cmd.dfu_start.expected_version = expected_version;
    command.cmd.basic.cmd.dfu_start.total_length = total_length;
    command.cmd.basic.cmd.dfu_start.crc32 = crc32;
    command.cmd.basic.cmd.dfu_start.force_update = force_update;

    dongl_Response response = exchange(command);
    return response.rsp.basic.rsp.dfu_start;
}

basic_DfuChunkRsp Protocol::basic_dfu_chunk(uint32_t page_index, uint32_t offset_in_page,
                                            const std::vector<uint8_t>& encrypted_data) {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_basic_tag;
    command.cmd.basic.which_cmd = basic_Command_dfu_chunk_tag;

    command.cmd.basic.cmd.dfu_chunk.page_index = page_index;
    command.cmd.basic.cmd.dfu_chunk.offset_in_page = offset_in_page;
    command.cmd.basic.cmd.dfu_chunk.encrypted_data.size = encrypted_data.size();
    memcpy(command.cmd.basic.cmd.dfu_chunk.encrypted_data.bytes, encrypted_data.data(), encrypted_data.size());

    dongl_Response response = exchange(command);
    return response.rsp.basic.rsp.dfu_chunk;
}

basic_DfuMetadataRsp Protocol::basic_dfu_metadata() {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_basic_tag;
    command.cmd.basic.which_cmd = basic_Command_dfu_metadata_tag;

    dongl_Response response = exchange(command);
    return response.rsp.basic.rsp.dfu_metadata;
}

basic_DfuVerifyRsp Protocol::basic_dfu_verify() {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_basic_tag;
    command.cmd.basic.which_cmd = basic_Command_dfu_verify_tag;

    dongl_Response response = exchange(command);
    return response.rsp.basic.rsp.dfu_verify;
}

basic_DfuRebootRsp Protocol::basic_dfu_reboot(bool force_update) {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_basic_tag;
    command.cmd.basic.which_cmd = basic_Command_dfu_reboot_tag;
    command.cmd.basic.cmd.dfu_reboot.force_update = force_update;

    dongl_Response response = exchange(command);
    return response.rsp.basic.rsp.dfu_reboot;
}

basic_PowerOnRsp Protocol::basic_power_on() {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_basic_tag;
    command.cmd.basic.which_cmd = basic_Command_power_on_tag;
    basic_PowerOnCmd power_on_cmd = basic_PowerOnCmd_init_default;
    command.cmd.basic.cmd.power_on = power_on_cmd;

    dongl_Response response = exchange(command);
    return response.rsp.basic.rsp.power_on;
}

basic_PowerOffRsp Protocol::basic_power_off() {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_basic_tag;
    command.cmd.basic.which_cmd = basic_Command_power_off_tag;
    basic_PowerOffCmd power_off_cmd = basic_PowerOffCmd_init_default;
    command.cmd.basic.cmd.power_off = power_off_cmd;

    dongl_Response response = exchange(command);
    return response.rsp.basic.rsp.power_off;
}

simpleble_InitRsp Protocol::simpleble_init() {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_simpleble_tag;
    command.cmd.simpleble.which_cmd = simpleble_Command_init_tag;
    simpleble_InitCmd init_cmd = simpleble_InitCmd_init_default;
    command.cmd.simpleble.cmd.init = init_cmd;

    dongl_Response response = exchange(command);
    return response.rsp.simpleble.rsp.init;
}

simpleble_ScanStartRsp Protocol::simpleble_scan_start() {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_simpleble_tag;
    command.cmd.simpleble.which_cmd = simpleble_Command_scan_start_tag;
    simpleble_ScanStartCmd scan_start_cmd = simpleble_ScanStartCmd_init_default;
    command.cmd.simpleble.cmd.scan_start = scan_start_cmd;

    dongl_Response response = exchange(command);
    return response.rsp.simpleble.rsp.scan_start;
}

simpleble_ScanStopRsp Protocol::simpleble_scan_stop() {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_simpleble_tag;
    command.cmd.simpleble.which_cmd = simpleble_Command_scan_stop_tag;
    simpleble_ScanStopCmd scan_stop_cmd = simpleble_ScanStopCmd_init_default;
    command.cmd.simpleble.cmd.scan_stop = scan_stop_cmd;

    dongl_Response response = exchange(command);
    return response.rsp.simpleble.rsp.scan_stop;
}

simpleble_ConnectRsp Protocol::simpleble_connect(simpleble_BluetoothAddressType address_type,
                                                 const std::string& address) {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_simpleble_tag;
    command.cmd.simpleble.which_cmd = simpleble_Command_connect_tag;
    simpleble_ConnectCmd connect_cmd = simpleble_ConnectCmd_init_default;
    command.cmd.simpleble.cmd.connect = connect_cmd;

    command.cmd.simpleble.cmd.connect.address_type = address_type;
    strncpy(command.cmd.simpleble.cmd.connect.address, address.c_str(),
            sizeof(command.cmd.simpleble.cmd.connect.address));

    dongl_Response response = exchange(command);
    return response.rsp.simpleble.rsp.connect;
}

simpleble_DisconnectRsp Protocol::simpleble_disconnect(uint16_t conn_handle) {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_simpleble_tag;
    command.cmd.simpleble.which_cmd = simpleble_Command_disconnect_tag;
    simpleble_DisconnectCmd disconnect_cmd = simpleble_DisconnectCmd_init_default;
    command.cmd.simpleble.cmd.disconnect = disconnect_cmd;

    dongl_Response response = exchange(command);
    return response.rsp.simpleble.rsp.disconnect;
}

simpleble_ReadRsp Protocol::simpleble_read(uint16_t conn_handle, uint16_t handle) {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_simpleble_tag;
    command.cmd.simpleble.which_cmd = simpleble_Command_read_tag;
    simpleble_ReadCmd read_cmd = simpleble_ReadCmd_init_default;
    command.cmd.simpleble.cmd.read = read_cmd;
    command.cmd.simpleble.cmd.read.conn_handle = conn_handle;
    command.cmd.simpleble.cmd.read.handle = handle;

    fmt::print("simpleble_read: conn_handle: {}, handle: {}\n", conn_handle, handle);
    dongl_Response response = exchange(command);
    fmt::print("simpleble_read: response: {}\n", response.rsp.simpleble.rsp.read.ret_code);
    return response.rsp.simpleble.rsp.read;
}

simpleble_WriteRsp Protocol::simpleble_write(uint16_t conn_handle, uint16_t handle, simpleble_WriteOperation operation,
                                             const std::vector<uint8_t>& data) {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_simpleble_tag;
    command.cmd.simpleble.which_cmd = simpleble_Command_write_tag;
    simpleble_WriteCmd write_cmd = simpleble_WriteCmd_init_default;
    command.cmd.simpleble.cmd.write = write_cmd;
    command.cmd.simpleble.cmd.write.conn_handle = conn_handle;
    command.cmd.simpleble.cmd.write.handle = handle;
    command.cmd.simpleble.cmd.write.op = operation;
    command.cmd.simpleble.cmd.write.data.size = data.size();
    memcpy(command.cmd.simpleble.cmd.write.data.bytes, data.data(), data.size());

    dongl_Response response = exchange(command);
    return response.rsp.simpleble.rsp.write;
}
