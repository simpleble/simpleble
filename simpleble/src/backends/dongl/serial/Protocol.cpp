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

basic_DfuStartRsp Protocol::basic_dfu_start() {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_basic_tag;
    command.cmd.basic.which_cmd = basic_Command_dfu_start_tag;
    basic_DfuStartCmd dfu_start_cmd = basic_DfuStartCmd_init_default;
    command.cmd.basic.cmd.dfu_start = dfu_start_cmd;

    dongl_Response response = exchange(command);
    return response.rsp.basic.rsp.dfu_start;
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

simpleble_ConnectRsp Protocol::simpleble_connect(simpleble_BluetoothAddressType address_type, const std::string& address) {
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_simpleble_tag;
    command.cmd.simpleble.which_cmd = simpleble_Command_connect_tag;
    simpleble_ConnectCmd connect_cmd = simpleble_ConnectCmd_init_default;
    command.cmd.simpleble.cmd.connect = connect_cmd;

    command.cmd.simpleble.cmd.connect.address_type = address_type;
    strncpy(command.cmd.simpleble.cmd.connect.address, address.c_str(), sizeof(command.cmd.simpleble.cmd.connect.address));

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

simpleble_WriteRsp Protocol::simpleble_write(uint16_t conn_handle, uint16_t handle, simpleble_WriteOperation operation, const std::vector<uint8_t>& data) {
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

// GAP commands
sd_GapAddrSetRsp Protocol::sd_gap_addr_set(bool has_addr, sd_types_BleGapAddr addr) {
    sd_GapAddrSetCmd cmd = {has_addr, addr};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_addr_set_tag;
    command.cmd.softdevice.cmd.gap_addr_set = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_addr_set;
}

sd_GapAddrGetRsp Protocol::sd_gap_addr_get() {
    sd_GapAddrGetCmd cmd = {0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_addr_get_tag;
    command.cmd.softdevice.cmd.gap_addr_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_addr_get;
}

sd_GapWhitelistSetRsp Protocol::sd_gap_whitelist_set(pb_size_t wl_addrs_count, sd_types_BleGapAddr wl_addrs[8], uint8_t len) {
    sd_GapWhitelistSetCmd cmd = {wl_addrs_count, {}, len};
    memcpy(cmd.wl_addrs, wl_addrs, sizeof(sd_types_BleGapAddr) * 8);
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_whitelist_set_tag;
    command.cmd.softdevice.cmd.gap_whitelist_set = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_whitelist_set;
}

sd_GapDeviceIdentitiesSetRsp Protocol::sd_gap_device_identities_set(pb_size_t id_keys_count, sd_types_BleGapIdKey id_keys[8], pb_size_t local_irks_count, sd_types_BleGapIrk local_irks[8], uint8_t len) {
    sd_GapDeviceIdentitiesSetCmd cmd = {id_keys_count, {}, local_irks_count, {}, len};
    memcpy(cmd.id_keys, id_keys, sizeof(sd_types_BleGapIdKey) * 8);
    memcpy(cmd.local_irks, local_irks, sizeof(sd_types_BleGapIrk) * 8);
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_device_identities_set_tag;
    command.cmd.softdevice.cmd.gap_device_identities_set = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_device_identities_set;
}

sd_GapPrivacySetRsp Protocol::sd_gap_privacy_set(bool has_privacy_params, sd_types_BleGapPrivacyParams privacy_params) {
    sd_GapPrivacySetCmd cmd = {has_privacy_params, privacy_params};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_privacy_set_tag;
    command.cmd.softdevice.cmd.gap_privacy_set = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_privacy_set;
}

sd_GapPrivacyGetRsp Protocol::sd_gap_privacy_get() {
    sd_GapPrivacyGetCmd cmd = {0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_privacy_get_tag;
    command.cmd.softdevice.cmd.gap_privacy_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_privacy_get;
}

sd_GapAdvSetConfigureRsp Protocol::sd_gap_adv_set_configure(uint8_t adv_handle, bool has_adv_data, sd_types_BleGapAdvData adv_data, bool has_adv_params, sd_types_BleGapAdvParams adv_params) {
    sd_GapAdvSetConfigureCmd cmd = {adv_handle, has_adv_data, adv_data, has_adv_params, adv_params};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_adv_set_configure_tag;
    command.cmd.softdevice.cmd.gap_adv_set_configure = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_adv_set_configure;
}

sd_GapAdvStartRsp Protocol::sd_gap_adv_start(uint8_t adv_handle, uint8_t conn_cfg_tag) {
    sd_GapAdvStartCmd cmd = {adv_handle, conn_cfg_tag};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_adv_start_tag;
    command.cmd.softdevice.cmd.gap_adv_start = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_adv_start;
}

sd_GapAdvStopRsp Protocol::sd_gap_adv_stop(uint8_t adv_handle) {
    sd_GapAdvStopCmd cmd = {adv_handle};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_adv_stop_tag;
    command.cmd.softdevice.cmd.gap_adv_stop = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_adv_stop;
}

sd_GapConnParamUpdateRsp Protocol::sd_gap_conn_param_update(uint16_t conn_handle, bool has_conn_params, sd_types_BleGapConnParams conn_params) {
    sd_GapConnParamUpdateCmd cmd = {conn_handle, has_conn_params, conn_params};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_conn_param_update_tag;
    command.cmd.softdevice.cmd.gap_conn_param_update = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_conn_param_update;
}

sd_GapDisconnectRsp Protocol::sd_gap_disconnect(uint16_t conn_handle, uint8_t hci_ret_code_code) {
    sd_GapDisconnectCmd cmd = {conn_handle, hci_ret_code_code};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_disconnect_tag;
    command.cmd.softdevice.cmd.gap_disconnect = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_disconnect;
}

sd_GapTxPowerSetRsp Protocol::sd_gap_tx_power_set(uint8_t role, uint16_t handle, int8_t tx_power) {
    sd_GapTxPowerSetCmd cmd = {role, handle, tx_power};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_tx_power_set_tag;
    command.cmd.softdevice.cmd.gap_tx_power_set = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_tx_power_set;
}

sd_GapAppearanceSetRsp Protocol::sd_gap_appearance_set(uint16_t appearance) {
    sd_GapAppearanceSetCmd cmd = {appearance};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_appearance_set_tag;
    command.cmd.softdevice.cmd.gap_appearance_set = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_appearance_set;
}

sd_GapAppearanceGetRsp Protocol::sd_gap_appearance_get() {
    sd_GapAppearanceGetCmd cmd = {0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_appearance_get_tag;
    command.cmd.softdevice.cmd.gap_appearance_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_appearance_get;
}

sd_GapPpcpSetRsp Protocol::sd_gap_ppcp_set(bool has_conn_params, sd_types_BleGapConnParams conn_params) {
    sd_GapPpcpSetCmd cmd = {has_conn_params, conn_params};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_ppcp_set_tag;
    command.cmd.softdevice.cmd.gap_ppcp_set = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_ppcp_set;
}

sd_GapPpcpGetRsp Protocol::sd_gap_ppcp_get() {
    sd_GapPpcpGetCmd cmd = {0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_ppcp_get_tag;
    command.cmd.softdevice.cmd.gap_ppcp_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_ppcp_get;
}

sd_GapDeviceNameSetRsp Protocol::sd_gap_device_name_set(bool has_write_perm, sd_types_BleGapConnSecMode write_perm, sd_GapDeviceNameSetCmd_dev_name_t dev_name, uint16_t len) {
    sd_GapDeviceNameSetCmd cmd = {has_write_perm, write_perm, dev_name, len};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_device_name_set_tag;
    command.cmd.softdevice.cmd.gap_device_name_set = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_device_name_set;
}

sd_GapDeviceNameGetRsp Protocol::sd_gap_device_name_get() {
    sd_GapDeviceNameGetCmd cmd = {0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_device_name_get_tag;
    command.cmd.softdevice.cmd.gap_device_name_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_device_name_get;
}

sd_GapAuthenticateRsp Protocol::sd_gap_authenticate(uint16_t conn_handle, bool has_sec_params, sd_types_BleGapSecParams sec_params) {
    sd_GapAuthenticateCmd cmd = {conn_handle, has_sec_params, sec_params};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_authenticate_tag;
    command.cmd.softdevice.cmd.gap_authenticate = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_authenticate;
}

sd_GapSecParamsReplyRsp Protocol::sd_gap_sec_params_reply(uint16_t conn_handle, sd_types_BleGapSecStatus sec_ret_code, bool has_sec_params, sd_types_BleGapSecParams sec_params, bool has_sec_keyset, sd_types_BleGapSecKeyset sec_keyset) {
    sd_GapSecParamsReplyCmd cmd = {conn_handle, sec_ret_code, has_sec_params, sec_params, has_sec_keyset, sec_keyset};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_sec_params_reply_tag;
    command.cmd.softdevice.cmd.gap_sec_params_reply = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_sec_params_reply;
}

sd_GapAuthKeyReplyRsp Protocol::sd_gap_auth_key_reply(uint16_t conn_handle, sd_types_BleGapAuthKeyType key_type, sd_GapAuthKeyReplyCmd_key_t key) {
    sd_GapAuthKeyReplyCmd cmd = {conn_handle, key_type, key};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_auth_key_reply_tag;
    command.cmd.softdevice.cmd.gap_auth_key_reply = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_auth_key_reply;
}

sd_GapLescDhkeyReplyRsp Protocol::sd_gap_lesc_dhkey_reply(uint16_t conn_handle, bool has_dhkey, sd_types_BleGapLescDhkey dhkey) {
    sd_GapLescDhkeyReplyCmd cmd = {conn_handle, has_dhkey, dhkey};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_lesc_dhkey_reply_tag;
    command.cmd.softdevice.cmd.gap_lesc_dhkey_reply = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_lesc_dhkey_reply;
}

sd_GapKeypressNotifyRsp Protocol::sd_gap_keypress_notify(uint16_t conn_handle, sd_types_BleGapKpNotType kp_not) {
    sd_GapKeypressNotifyCmd cmd = {conn_handle, kp_not};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_keypress_notify_tag;
    command.cmd.softdevice.cmd.gap_keypress_notify = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_keypress_notify;
}

sd_GapLescOobDataGetRsp Protocol::sd_gap_lesc_oob_data_get(uint16_t conn_handle, bool has_pk_own, sd_types_BleGapLescP256Pk pk_own) {
    sd_GapLescOobDataGetCmd cmd = {conn_handle, has_pk_own, pk_own};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_lesc_oob_data_get_tag;
    command.cmd.softdevice.cmd.gap_lesc_oob_data_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_lesc_oob_data_get;
}

sd_GapLescOobDataSetRsp Protocol::sd_gap_lesc_oob_data_set(uint16_t conn_handle, bool has_oobd_own, sd_types_BleGapLescOobData oobd_own, bool has_oobd_peer, sd_types_BleGapLescOobData oobd_peer) {
    sd_GapLescOobDataSetCmd cmd = {conn_handle, has_oobd_own, oobd_own, has_oobd_peer, oobd_peer};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_lesc_oob_data_set_tag;
    command.cmd.softdevice.cmd.gap_lesc_oob_data_set = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_lesc_oob_data_set;
}

sd_GapEncryptRsp Protocol::sd_gap_encrypt(uint16_t conn_handle, bool has_master_id, sd_types_BleGapMasterId master_id, bool has_enc_info, sd_types_BleGapEncInfo enc_info) {
    sd_GapEncryptCmd cmd = {conn_handle, has_master_id, master_id, has_enc_info, enc_info};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_encrypt_tag;
    command.cmd.softdevice.cmd.gap_encrypt = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_encrypt;
}

sd_GapSecInfoReplyRsp Protocol::sd_gap_sec_info_reply(uint16_t conn_handle, bool has_enc_info, sd_types_BleGapEncInfo enc_info, bool has_id_info, sd_types_BleGapIrk id_info, bool has_sign_info, sd_types_BleGapSignInfo sign_info) {
    sd_GapSecInfoReplyCmd cmd = {conn_handle, has_enc_info, enc_info, has_id_info, id_info, has_sign_info, sign_info};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_sec_info_reply_tag;
    command.cmd.softdevice.cmd.gap_sec_info_reply = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_sec_info_reply;
}

sd_GapConnSecGetRsp Protocol::sd_gap_conn_sec_get(uint16_t conn_handle) {
    sd_GapConnSecGetCmd cmd = {conn_handle};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_conn_sec_get_tag;
    command.cmd.softdevice.cmd.gap_conn_sec_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_conn_sec_get;
}

sd_GapRssiStartRsp Protocol::sd_gap_rssi_start(uint16_t conn_handle, uint8_t threshold_dbm, uint8_t skip_count) {
    sd_GapRssiStartCmd cmd = {conn_handle, threshold_dbm, skip_count};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_rssi_start_tag;
    command.cmd.softdevice.cmd.gap_rssi_start = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_rssi_start;
}

sd_GapRssiStopRsp Protocol::sd_gap_rssi_stop(uint16_t conn_handle) {
    sd_GapRssiStopCmd cmd = {conn_handle};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_rssi_stop_tag;
    command.cmd.softdevice.cmd.gap_rssi_stop = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_rssi_stop;
}

sd_GapScanStartRsp Protocol::sd_gap_scan_start(sd_types_BleGapScanParams scan_params) {
    sd_GapScanStartCmd cmd = {true, scan_params};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_scan_start_tag;
    command.cmd.softdevice.cmd.gap_scan_start = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_scan_start;
}

sd_GapScanStopRsp Protocol::sd_gap_scan_stop() {
    sd_GapScanStopCmd cmd = {0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_scan_stop_tag;
    command.cmd.softdevice.cmd.gap_scan_stop = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_scan_stop;
}

sd_GapConnectRsp Protocol::sd_gap_connect(bool has_peer_addr, sd_types_BleGapAddr peer_addr, bool has_scan_params, sd_types_BleGapScanParams scan_params, bool has_conn_params, sd_types_BleGapConnParams conn_params, uint8_t conn_cfg_tag) {
    sd_GapConnectCmd cmd = {has_peer_addr, peer_addr, has_scan_params, scan_params, has_conn_params, conn_params, conn_cfg_tag};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_connect_tag;
    command.cmd.softdevice.cmd.gap_connect = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_connect;
}

sd_GapConnectCancelRsp Protocol::sd_gap_connect_cancel() {
    sd_GapConnectCancelCmd cmd = {0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_connect_cancel_tag;
    command.cmd.softdevice.cmd.gap_connect_cancel = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_connect_cancel;
}

sd_GapRssiGetRsp Protocol::sd_gap_rssi_get(uint16_t conn_handle) {
    sd_GapRssiGetCmd cmd = {conn_handle};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_rssi_get_tag;
    command.cmd.softdevice.cmd.gap_rssi_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_rssi_get;
}

sd_GapPhyUpdateRsp Protocol::sd_gap_phy_update(uint16_t conn_handle, bool has_phys, sd_BleGapPhysCmd phys) {
    sd_GapPhyUpdateCmd cmd = {conn_handle, has_phys, phys};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_phy_update_tag;
    command.cmd.softdevice.cmd.gap_phy_update = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_phy_update;
}

sd_GapDataLengthUpdateRsp Protocol::sd_gap_data_length_update(uint16_t conn_handle, bool has_dl_params, sd_types_BleGapDataLengthParams dl_params, bool has_dl_limitation, sd_types_BleGapDataLengthLimitation dl_limitation) {
    sd_GapDataLengthUpdateCmd cmd = {conn_handle, has_dl_params, dl_params, has_dl_limitation, dl_limitation};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_data_length_update_tag;
    command.cmd.softdevice.cmd.gap_data_length_update = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_data_length_update;
}

sd_GapQosChannelSurveyStartRsp Protocol::sd_gap_qos_channel_survey_start(uint32_t interval_us) {
    sd_GapQosChannelSurveyStartCmd cmd = {interval_us};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_qos_channel_survey_start_tag;
    command.cmd.softdevice.cmd.gap_qos_channel_survey_start = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_qos_channel_survey_start;
}

sd_GapQosChannelSurveyStopRsp Protocol::sd_gap_qos_channel_survey_stop() {
    sd_GapQosChannelSurveyStopCmd cmd = {0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_qos_channel_survey_stop_tag;
    command.cmd.softdevice.cmd.gap_qos_channel_survey_stop = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_qos_channel_survey_stop;
}

sd_GapAdvAddrGetRsp Protocol::sd_gap_adv_addr_get(uint8_t adv_handle) {
    sd_GapAdvAddrGetCmd cmd = {adv_handle};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_adv_addr_get_tag;
    command.cmd.softdevice.cmd.gap_adv_addr_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_adv_addr_get;
}

sd_GapNextConnEvtCounterGetRsp Protocol::sd_gap_next_conn_evt_counter_get(uint16_t conn_handle) {
    sd_GapNextConnEvtCounterGetCmd cmd = {conn_handle};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_next_conn_evt_counter_get_tag;
    command.cmd.softdevice.cmd.gap_next_conn_evt_counter_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_next_conn_evt_counter_get;
}

sd_GapConnEvtTriggerStartRsp Protocol::sd_gap_conn_evt_trigger_start(uint16_t conn_handle, bool has_params, sd_types_BleGapConnEventTrigger params) {
    sd_GapConnEvtTriggerStartCmd cmd = {conn_handle, has_params, params};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_conn_evt_trigger_start_tag;
    command.cmd.softdevice.cmd.gap_conn_evt_trigger_start = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_conn_evt_trigger_start;
}

sd_GapConnEvtTriggerStopRsp Protocol::sd_gap_conn_evt_trigger_stop(uint16_t conn_handle) {
    sd_GapConnEvtTriggerStopCmd cmd = {conn_handle};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gap_conn_evt_trigger_stop_tag;
    command.cmd.softdevice.cmd.gap_conn_evt_trigger_stop = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gap_conn_evt_trigger_stop;
}

// GATTC commands
sd_GattcPrimaryServicesDiscoverRsp Protocol::sd_gattc_primary_services_discover(uint16_t conn_handle, uint16_t start_handle, bool has_srvc_uuid, sd_types_BleUuid srvc_uuid) {
    sd_GattcPrimaryServicesDiscoverCmd cmd = {conn_handle, start_handle, has_srvc_uuid, srvc_uuid};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gattc_primary_services_discover_tag;
    command.cmd.softdevice.cmd.gattc_primary_services_discover = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gattc_primary_services_discover;
}

sd_GattcRelationshipsDiscoverRsp Protocol::sd_gattc_relationships_discover(uint16_t conn_handle, bool has_handle_range, sd_types_BleGattcHandleRange handle_range) {
    sd_GattcRelationshipsDiscoverCmd cmd = {conn_handle, has_handle_range, handle_range};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gattc_relationships_discover_tag;
    command.cmd.softdevice.cmd.gattc_relationships_discover = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gattc_relationships_discover;
}

sd_GattcCharacteristicsDiscoverRsp Protocol::sd_gattc_characteristics_discover(uint16_t conn_handle, bool has_handle_range, sd_types_BleGattcHandleRange handle_range) {
    sd_GattcCharacteristicsDiscoverCmd cmd = {conn_handle, has_handle_range, handle_range};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gattc_characteristics_discover_tag;
    command.cmd.softdevice.cmd.gattc_characteristics_discover = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gattc_characteristics_discover;
}

sd_GattcDescriptorsDiscoverRsp Protocol::sd_gattc_descriptors_discover(uint16_t conn_handle, bool has_handle_range, sd_types_BleGattcHandleRange handle_range) {
    sd_GattcDescriptorsDiscoverCmd cmd = {conn_handle, has_handle_range, handle_range};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gattc_descriptors_discover_tag;
    command.cmd.softdevice.cmd.gattc_descriptors_discover = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gattc_descriptors_discover;
}

sd_GattcAttrInfoDiscoverRsp Protocol::sd_gattc_attr_info_discover(uint16_t conn_handle, bool has_handle_range, sd_types_BleGattcHandleRange handle_range) {
    sd_GattcAttrInfoDiscoverCmd cmd = {conn_handle, has_handle_range, handle_range};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gattc_attr_info_discover_tag;
    command.cmd.softdevice.cmd.gattc_attr_info_discover = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gattc_attr_info_discover;
}

sd_GattcCharValueByUuidReadRsp Protocol::sd_gattc_char_value_by_uuid_read(uint16_t conn_handle, bool has_uuid, sd_types_BleUuid uuid, bool has_handle_range, sd_types_BleGattcHandleRange handle_range) {
    sd_GattcCharValueByUuidReadCmd cmd = {conn_handle, has_uuid, uuid, has_handle_range, handle_range};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gattc_char_value_by_uuid_read_tag;
    command.cmd.softdevice.cmd.gattc_char_value_by_uuid_read = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gattc_char_value_by_uuid_read;
}

sd_GattcReadRsp Protocol::sd_gattc_read(uint16_t conn_handle, uint16_t handle, uint16_t offset) {
    sd_GattcReadCmd cmd = {conn_handle, handle, offset};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gattc_read_tag;
    command.cmd.softdevice.cmd.gattc_read = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gattc_read;
}

sd_GattcCharValuesReadRsp Protocol::sd_gattc_char_values_read(uint16_t conn_handle, pb_callback_t handles, uint8_t handle_count) {
    sd_GattcCharValuesReadCmd cmd = {conn_handle, handles, handle_count};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gattc_char_values_read_tag;
    command.cmd.softdevice.cmd.gattc_char_values_read = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gattc_char_values_read;
}

sd_GattcWriteRsp Protocol::sd_gattc_write(uint16_t conn_handle, bool has_write_params, sd_types_BleGattcWriteParams write_params) {
    sd_GattcWriteCmd cmd = {conn_handle, has_write_params, write_params};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gattc_write_tag;
    command.cmd.softdevice.cmd.gattc_write = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gattc_write;
}

sd_GattcHvConfirmRsp Protocol::sd_gattc_hv_confirm(uint16_t conn_handle, uint16_t handle) {
    sd_GattcHvConfirmCmd cmd = {conn_handle, handle};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gattc_hv_confirm_tag;
    command.cmd.softdevice.cmd.gattc_hv_confirm = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gattc_hv_confirm;
}

sd_GattcExchangeMtuRequestRsp Protocol::sd_gattc_exchange_mtu_request(uint16_t conn_handle, uint16_t client_rx_mtu) {
    sd_GattcExchangeMtuRequestCmd cmd = {conn_handle, client_rx_mtu};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gattc_exchange_mtu_request_tag;
    command.cmd.softdevice.cmd.gattc_exchange_mtu_request = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gattc_exchange_mtu_request;
}

// GATTS commands
sd_GattsServiceAddRsp Protocol::sd_gatts_service_add(uint8_t type, bool has_uuid, sd_types_BleUuid uuid) {
    sd_GattsServiceAddCmd cmd = {type, has_uuid, uuid, 0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_service_add_tag;
    command.cmd.softdevice.cmd.gatts_service_add = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_service_add;
}

sd_GattsIncludeAddRsp Protocol::sd_gatts_include_add(uint16_t service_handle, uint16_t inc_srvc_handle) {
    sd_GattsIncludeAddCmd cmd = {service_handle, inc_srvc_handle, 0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_include_add_tag;
    command.cmd.softdevice.cmd.gatts_include_add = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_include_add;
}

sd_GattsCharacteristicAddRsp Protocol::sd_gatts_characteristic_add(uint16_t service_handle, bool has_char_md, sd_types_BleGattsCharMd char_md, bool has_attr_char_value, sd_types_BleGattsAttr attr_char_value) {
    sd_GattsCharacteristicAddCmd cmd = {service_handle, has_char_md, char_md, has_attr_char_value, attr_char_value, false, {}};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_characteristic_add_tag;
    command.cmd.softdevice.cmd.gatts_characteristic_add = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_characteristic_add;
}

sd_GattsDescriptorAddRsp Protocol::sd_gatts_descriptor_add(uint16_t char_handle, bool has_attr, sd_types_BleGattsAttr attr) {
    sd_GattsDescriptorAddCmd cmd = {char_handle, has_attr, attr, 0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_descriptor_add_tag;
    command.cmd.softdevice.cmd.gatts_descriptor_add = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_descriptor_add;
}

sd_GattsValueSetRsp Protocol::sd_gatts_value_set(uint16_t conn_handle, uint16_t handle, bool has_value, sd_types_BleGattsValue value) {
    sd_GattsValueSetCmd cmd = {conn_handle, handle, has_value, value};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_value_set_tag;
    command.cmd.softdevice.cmd.gatts_value_set = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_value_set;
}

sd_GattsValueGetRsp Protocol::sd_gatts_value_get(uint16_t conn_handle, uint16_t handle) {
    sd_GattsValueGetCmd cmd = {conn_handle, handle, false, {}};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_value_get_tag;
    command.cmd.softdevice.cmd.gatts_value_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_value_get;
}

sd_GattsHvxRsp Protocol::sd_gatts_hvx(uint16_t conn_handle, bool has_hvx_params, sd_types_BleGattsHvxParams hvx_params) {
    sd_GattsHvxCmd cmd = {conn_handle, has_hvx_params, hvx_params};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_hvx_tag;
    command.cmd.softdevice.cmd.gatts_hvx = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_hvx;
}

sd_GattsServiceChangedRsp Protocol::sd_gatts_service_changed(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle) {
    sd_GattsServiceChangedCmd cmd = {conn_handle, start_handle, end_handle};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_service_changed_tag;
    command.cmd.softdevice.cmd.gatts_service_changed = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_service_changed;
}

sd_GattsRwAuthorizeReplyRsp Protocol::sd_gatts_rw_authorize_reply(uint16_t conn_handle, bool has_rw_authorize_reply_params, sd_types_BleGattsRwAuthorizeReplyParams rw_authorize_reply_params) {
    sd_GattsRwAuthorizeReplyCmd cmd = {conn_handle, has_rw_authorize_reply_params, rw_authorize_reply_params};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_rw_authorize_reply_tag;
    command.cmd.softdevice.cmd.gatts_rw_authorize_reply = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_rw_authorize_reply;
}

sd_GattsSysAttrSetRsp Protocol::sd_gatts_sys_attr_set(uint16_t conn_handle, sd_GattsSysAttrSetCmd_sys_attr_data_t sys_attr_data, uint16_t len, uint32_t flags) {
    sd_GattsSysAttrSetCmd cmd = {conn_handle, sys_attr_data, len, flags};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_sys_attr_set_tag;
    command.cmd.softdevice.cmd.gatts_sys_attr_set = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_sys_attr_set;
}

sd_GattsSysAttrGetRsp Protocol::sd_gatts_sys_attr_get(uint16_t conn_handle, uint32_t flags) {
    sd_GattsSysAttrGetCmd cmd = {conn_handle, {}, 0, flags};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_sys_attr_get_tag;
    command.cmd.softdevice.cmd.gatts_sys_attr_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_sys_attr_get;
}

sd_GattsInitialUserHandleGetRsp Protocol::sd_gatts_initial_user_handle_get() {
    sd_GattsInitialUserHandleGetCmd cmd = {0};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_initial_user_handle_get_tag;
    command.cmd.softdevice.cmd.gatts_initial_user_handle_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_initial_user_handle_get;
}

sd_GattsAttrGetRsp Protocol::sd_gatts_attr_get(uint16_t handle) {
    sd_GattsAttrGetCmd cmd = {handle, false, {}, false, {}};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_attr_get_tag;
    command.cmd.softdevice.cmd.gatts_attr_get = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_attr_get;
}

sd_GattsExchangeMtuReplyRsp Protocol::sd_gatts_exchange_mtu_reply(uint16_t conn_handle, uint16_t server_rx_mtu) {
    sd_GattsExchangeMtuReplyCmd cmd = {conn_handle, server_rx_mtu};
    dongl_Command command = dongl_Command_init_zero;
    command.which_cmd = dongl_Command_softdevice_tag;
    command.cmd.softdevice.which_cmd = sd_Command_gatts_exchange_mtu_reply_tag;
    command.cmd.softdevice.cmd.gatts_exchange_mtu_reply = cmd;

    dongl_Response response = exchange(command);
    return response.rsp.softdevice.rsp.gatts_exchange_mtu_reply;
}
