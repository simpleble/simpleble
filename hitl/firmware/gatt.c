#include "gatt.h"

#include <components/libraries/util/app_error.h>
#include <simpleembed/nrf52/softdevice/common/nrf_sdh_ble.h>
#include <string.h>

#include "control.h"

static peripheral_state_t* state;
static uint8_t uuid_type;
static uint16_t control_request_handle;
static uint16_t control_response_handle;

static const uint8_t base_uuid[16] = {
    0x01, 0x43, 0x5f, 0x6e, 0x8d, 0x9c, 0xb0, 0xa1, 0x6f, 0x4b, 0x7a, 0x2e, 0x00, 0x00, 0x57, 0x7e,
};

static void encode_uint32(uint8_t* data, uint32_t value) {
    for (unsigned index = 0; index < 4; ++index) {
        data[index] = value >> (8 * index);
    }
}

void gatt_set_value(attribute_id_t id, const uint8_t* data, uint16_t length) {
    attribute_t* attribute = &state->attributes[id];
    ble_gatts_value_t value = {
        .len = length,
        .p_value = (uint8_t*)data,
    };
    APP_ERROR_CHECK(sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID, attribute->handle, &value));
    memmove(attribute->data, data, length);
    attribute->length = length;
}

void gatt_restore_values(void) {
    uint8_t data[32] = {0};
    for (unsigned index = 0; index < ATTRIBUTE_COUNT; ++index) {
        attribute_t* attribute = &state->attributes[index];
        attribute->write_count = 0;
        attribute->subscription = 0;
        attribute->last_write_length = 0;
        attribute->last_write_operation = 0;
        if (index != ATTRIBUTE_INFO && index != ATTRIBUTE_READ_VALUE && index != ATTRIBUTE_DESCRIPTOR &&
            index != ATTRIBUTE_BATTERY) {
            gatt_set_value((attribute_id_t)index, data, 0);
        }
    }

    encode_uint32(data, 1);
    encode_uint32(data + 4, NRF_FICR->DEVICEID[0]);
    encode_uint32(data + 8, NRF_FICR->DEVICEID[1]);
    encode_uint32(data + 12, state->test_id);
    gatt_set_value(ATTRIBUTE_INFO, data, 16);

    for (unsigned index = 0; index < sizeof(data); ++index) {
        data[index] = index;
    }
    gatt_set_value(ATTRIBUTE_READ_VALUE, data, sizeof(data));
    gatt_set_value(ATTRIBUTE_DESCRIPTOR, (const uint8_t*)"DESC", 4);
    data[0] = 50;
    gatt_set_value(ATTRIBUTE_BATTERY, data, 1);
}

static void update_control_response(void) {
    uint8_t page[CONTROL_PAGE_SIZE];
    ble_gatts_value_t value = {
        .len = control_read_page(page),
        .p_value = page,
    };
    APP_ERROR_CHECK(sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID, control_response_handle, &value));
}

static attribute_id_t find_attribute(uint16_t handle) {
    for (unsigned index = 0; index < ATTRIBUTE_COUNT; ++index) {
        if (state->attributes[index].handle == handle) {
            return (attribute_id_t)index;
        }
    }
    return ATTRIBUTE_COUNT;
}

static void record_write(attribute_id_t id, const ble_gatts_evt_write_t* write, bool accepted) {
    attribute_t* attribute = &state->attributes[id];
    if (accepted) {
        gatt_set_value(id, write->data, write->len);
        ++attribute->write_count;
        memcpy(attribute->last_write, write->data, write->len);
        attribute->last_write_length = write->len;
        attribute->last_write_operation = write->op;
    }

    static const char digits[] = "0123456789abcdef";
    char hex[FIXTURE_MAX_VALUE_LENGTH * 2 + 1];
    if (!write->len) {
        strcpy(hex, "-");
    } else {
        for (unsigned index = 0; index < write->len; ++index) {
            hex[index * 2] = digits[write->data[index] >> 4];
            hex[index * 2 + 1] = digits[write->data[index] & 15];
        }
        hex[write->len * 2] = '\0';
    }
    control_event("type=write attr=%s op=%u len=%u offset=%u accepted=%u data=%s", attribute->name, write->op,
                  write->len, write->offset, accepted, hex);
}

static void authorize_read(const ble_gatts_evt_read_t* read) {
    bool control = read->handle == control_response_handle;
    ble_gatts_rw_authorize_reply_params_t reply = {.type = BLE_GATTS_AUTHORIZE_TYPE_READ};
    reply.params.read.gatt_status = control ? BLE_GATT_STATUS_SUCCESS : BLE_GATT_STATUS_ATTERR_APP_BEGIN;
    if (control && read->offset) {
        reply.params.read.gatt_status = BLE_GATT_STATUS_ATTERR_INVALID_OFFSET;
    }

    APP_ERROR_CHECK(sd_ble_gatts_rw_authorize_reply(state->connection_handle, &reply));
    if (control && !read->offset && control_final_page_selected()) {
        peripheral_acknowledge_action();
    }
}

static uint16_t validate_control_write(const ble_gatts_evt_write_t* write) {
    if (write->op != BLE_GATTS_OP_WRITE_REQ || write->offset) {
        return BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED;
    }
    if (write->len > state->mtu - 3 || !control_valid_write(write->data, write->len)) {
        return BLE_GATT_STATUS_ATTERR_INVALID_ATT_VAL_LENGTH;
    }
    return BLE_GATT_STATUS_SUCCESS;
}

static uint16_t validate_write(attribute_id_t attribute, const ble_gatts_evt_write_t* write) {
    if (write->op == BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL) {
        return BLE_GATT_STATUS_SUCCESS;
    }
    if (write->op == BLE_GATTS_OP_PREP_WRITE_REQ || write->op == BLE_GATTS_OP_EXEC_WRITE_REQ_NOW) {
        return BLE_GATT_STATUS_ATTERR_APP_BEGIN + 2;
    }
    if (write->handle == control_request_handle) {
        return validate_control_write(write);
    }
    if (attribute == ATTRIBUTE_ERROR) {
        return BLE_GATT_STATUS_ATTERR_APP_BEGIN;
    }
    if ((attribute != ATTRIBUTE_WRITE_REQUEST && attribute != ATTRIBUTE_DESCRIPTOR) ||
        write->op != BLE_GATTS_OP_WRITE_REQ) {
        return BLE_GATT_STATUS_ATTERR_REQUEST_NOT_SUPPORTED;
    }
    if (write->offset) {
        return BLE_GATT_STATUS_ATTERR_INVALID_OFFSET;
    }
    if (write->len > state->attributes[attribute].max_length || write->len > state->mtu - 3) {
        return BLE_GATT_STATUS_ATTERR_INVALID_ATT_VAL_LENGTH;
    }
    return BLE_GATT_STATUS_SUCCESS;
}

static void authorize_write(const ble_gatts_evt_write_t* write) {
    attribute_id_t attribute = find_attribute(write->handle);
    uint16_t status = validate_write(attribute, write);
    ble_gatts_rw_authorize_reply_params_t reply = {.type = BLE_GATTS_AUTHORIZE_TYPE_WRITE};
    reply.params.write.gatt_status = status;
    if (status == BLE_GATT_STATUS_SUCCESS && write->op == BLE_GATTS_OP_WRITE_REQ) {
        reply.params.write.update = 1;
        reply.params.write.len = write->len;
        reply.params.write.p_data = write->data;
    }
    APP_ERROR_CHECK(sd_ble_gatts_rw_authorize_reply(state->connection_handle, &reply));

    if (status == BLE_GATT_STATUS_SUCCESS && write->handle == control_request_handle &&
        write->op == BLE_GATTS_OP_WRITE_REQ) {
        control_write(write->data, write->len);
        update_control_response();
    }
    if (attribute != ATTRIBUTE_COUNT && write->len <= FIXTURE_MAX_VALUE_LENGTH &&
        write->op != BLE_GATTS_OP_EXEC_WRITE_REQ_CANCEL) {
        record_write(attribute, write, status == BLE_GATT_STATUS_SUCCESS);
    }
}

static void authorize_request(const ble_gatts_evt_rw_authorize_request_t* request) {
    if (request->type == BLE_GATTS_AUTHORIZE_TYPE_READ) {
        authorize_read(&request->request.read);
    } else if (request->type == BLE_GATTS_AUTHORIZE_TYPE_WRITE) {
        authorize_write(&request->request.write);
    }
}

static void handle_write(const ble_gatts_evt_write_t* write) {
    attribute_t* command = &state->attributes[ATTRIBUTE_WRITE_COMMAND];
    if (write->handle == command->handle && write->len <= FIXTURE_MAX_VALUE_LENGTH) {
        bool accepted = write->op == BLE_GATTS_OP_WRITE_CMD && !write->offset && write->len <= state->mtu - 3;
        record_write(ATTRIBUTE_WRITE_COMMAND, write, accepted);
        if (!accepted) {
            gatt_set_value(ATTRIBUTE_WRITE_COMMAND, command->data, command->length);
        }
    }

    for (unsigned index = 0; index < ATTRIBUTE_COUNT; ++index) {
        attribute_t* attribute = &state->attributes[index];
        if (!attribute->cccd_handle || write->handle != attribute->cccd_handle || write->len != 2) {
            continue;
        }
        attribute->subscription = write->data[0] | ((uint16_t)write->data[1] << 8);
        control_event("type=cccd attr=%s value=%u", attribute->name, attribute->subscription);
        unsigned required_subscription = index == ATTRIBUTE_INDICATE ? 2 : 1;
        if (!(attribute->subscription & required_subscription)) {
            peripheral_finish_delivery((attribute_id_t)index, "unsubscribe");
        }
    }
}

static void ble_event(const ble_evt_t* event, void* context) {
    (void)context;

    switch (event->header.evt_id) {
        case BLE_GAP_EVT_CONNECTED:
            peripheral_connected(event->evt.gap_evt.conn_handle);
            break;
        case BLE_GAP_EVT_DISCONNECTED:
            peripheral_disconnected(event->evt.gap_evt.params.disconnected.reason);
            break;
        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            APP_ERROR_CHECK(
                sd_ble_gap_sec_params_reply(state->connection_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL));
            break;
        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            APP_ERROR_CHECK(sd_ble_gatts_sys_attr_set(state->connection_handle, NULL, 0, 0));
            break;
        case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
            ble_gap_phys_t phys = {
                .tx_phys = BLE_GAP_PHY_1MBPS,
                .rx_phys = BLE_GAP_PHY_1MBPS,
            };
            APP_ERROR_CHECK(sd_ble_gap_phy_update(state->connection_handle, &phys));
            break;
        }
        case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
            APP_ERROR_CHECK(sd_ble_gap_data_length_update(state->connection_handle, NULL, NULL));
            break;
        case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
            peripheral_exchange_mtu(event->evt.gatts_evt.params.exchange_mtu_request.client_rx_mtu);
            break;
        case BLE_EVT_USER_MEM_REQUEST:
            APP_ERROR_CHECK(sd_ble_user_mem_reply(state->connection_handle, NULL));
            break;
        case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
            authorize_request(&event->evt.gatts_evt.params.authorize_request);
            break;
        case BLE_GATTS_EVT_WRITE:
            handle_write(&event->evt.gatts_evt.params.write);
            break;
        case BLE_GATTS_EVT_HVN_TX_COMPLETE:
            peripheral_notification_complete(event->evt.gatts_evt.params.hvn_tx_complete.count);
            break;
        case BLE_GATTS_EVT_HVC:
            peripheral_indication_confirmed();
            break;
        case BLE_GATTS_EVT_TIMEOUT:
            APP_ERROR_CHECK(sd_ble_gap_disconnect(state->connection_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
            break;
        default:
            break;
    }
}
NRF_SDH_BLE_OBSERVER(hitl_observer, 3, ble_event, NULL);

static void add_attribute(uint16_t service, attribute_id_t id, uint8_t type) {
    attribute_t* attribute = &state->attributes[id];
    ble_uuid_t uuid = {
        .uuid = attribute->uuid,
        .type = type,
    };
    ble_gatts_attr_md_t metadata = {
        .vlen = 1,
        .vloc = BLE_GATTS_VLOC_STACK,
        .wr_auth = id == ATTRIBUTE_WRITE_REQUEST || id == ATTRIBUTE_ERROR || id == ATTRIBUTE_DESCRIPTOR,
        .rd_auth = id == ATTRIBUTE_ERROR,
    };
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&metadata.read_perm);
    if (id == ATTRIBUTE_WRITE_REQUEST || id == ATTRIBUTE_WRITE_COMMAND || id == ATTRIBUTE_ERROR ||
        id == ATTRIBUTE_DESCRIPTOR) {
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&metadata.write_perm);
    }
    ble_gatts_attr_t value = {
        .p_uuid = &uuid,
        .p_attr_md = &metadata,
        .max_len = attribute->max_length,
        .p_value = attribute->data,
    };
    if (id == ATTRIBUTE_DESCRIPTOR) {
        APP_ERROR_CHECK(
            sd_ble_gatts_descriptor_add(state->attributes[ATTRIBUTE_READ_VALUE].handle, &value, &attribute->handle));
        return;
    }

    ble_gatts_char_md_t characteristic = {0};
    characteristic.char_props.read = id != ATTRIBUTE_WRITE_COMMAND;
    characteristic.char_props.write = id == ATTRIBUTE_WRITE_REQUEST || id == ATTRIBUTE_ERROR;
    characteristic.char_props.write_wo_resp = id == ATTRIBUTE_WRITE_COMMAND;
    characteristic.char_props.notify = id == ATTRIBUTE_NOTIFY_A || id == ATTRIBUTE_NOTIFY_B || id == ATTRIBUTE_BATTERY;
    characteristic.char_props.indicate = id == ATTRIBUTE_INDICATE;

    ble_gatts_attr_md_t subscription = {.vloc = BLE_GATTS_VLOC_STACK};
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&subscription.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&subscription.write_perm);
    if (characteristic.char_props.notify || characteristic.char_props.indicate) {
        characteristic.p_cccd_md = &subscription;
    }
    if (id == ATTRIBUTE_READ_VALUE) {
        characteristic.p_char_user_desc = (uint8_t*)"SimpleBLE HITL read";
        characteristic.char_user_desc_size = sizeof("SimpleBLE HITL read") - 1;
        characteristic.char_user_desc_max_size = characteristic.char_user_desc_size;
    }

    ble_gatts_char_handles_t handles;
    APP_ERROR_CHECK(sd_ble_gatts_characteristic_add(service, &characteristic, &value, &handles));
    attribute->handle = handles.value_handle;
    attribute->cccd_handle = handles.cccd_handle;
}

static void add_fixture_service(void) {
    ble_uuid_t uuid = {.uuid = 1, .type = uuid_type};
    uint16_t service;
    APP_ERROR_CHECK(sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &uuid, &service));

    // Attach the custom descriptor before adding the next characteristic.
    static const attribute_id_t order[] = {
        ATTRIBUTE_INFO,          ATTRIBUTE_READ_VALUE,    ATTRIBUTE_DESCRIPTOR,
        ATTRIBUTE_WRITE_REQUEST, ATTRIBUTE_WRITE_COMMAND, ATTRIBUTE_NOTIFY_A,
        ATTRIBUTE_NOTIFY_B,      ATTRIBUTE_INDICATE,      ATTRIBUTE_ERROR,
    };
    for (unsigned index = 0; index < sizeof(order) / sizeof(order[0]); ++index) {
        add_attribute(service, order[index], uuid_type);
    }
}

static void add_battery_service(void) {
    ble_uuid_t uuid = {.uuid = 0x180f, .type = BLE_UUID_TYPE_BLE};
    uint16_t service;
    APP_ERROR_CHECK(sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &uuid, &service));
    add_attribute(service, ATTRIBUTE_BATTERY, BLE_UUID_TYPE_BLE);
}

static uint16_t add_control_characteristic(uint16_t service, bool response) {
    ble_uuid_t uuid = {
        .uuid = response ? 0x13 : 0x12,
        .type = uuid_type,
    };
    ble_gatts_attr_md_t metadata = {
        .vlen = 1,
        .vloc = BLE_GATTS_VLOC_STACK,
    };
    ble_gatts_char_md_t characteristic = {0};
    if (response) {
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&metadata.read_perm);
        metadata.rd_auth = 1;
        characteristic.char_props.read = 1;
    } else {
        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&metadata.write_perm);
        metadata.wr_auth = 1;
        characteristic.char_props.write = 1;
    }
    ble_gatts_attr_t value = {
        .p_uuid = &uuid,
        .p_attr_md = &metadata,
        .max_len = response ? CONTROL_PAGE_SIZE : FIXTURE_MAX_SEND_LENGTH,
    };
    ble_gatts_char_handles_t handles;
    APP_ERROR_CHECK(sd_ble_gatts_characteristic_add(service, &characteristic, &value, &handles));
    return handles.value_handle;
}

static void add_control_service(void) {
    ble_uuid_t uuid = {.uuid = 0x11, .type = uuid_type};
    uint16_t service;
    APP_ERROR_CHECK(sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &uuid, &service));
    control_request_handle = add_control_characteristic(service, false);
    control_response_handle = add_control_characteristic(service, true);
    update_control_response();
}

void gatt_init(peripheral_state_t* peripheral) {
    state = peripheral;
    ble_uuid128_t uuid;
    memcpy(uuid.uuid128, base_uuid, sizeof(base_uuid));
    APP_ERROR_CHECK(sd_ble_uuid_vs_add(&uuid, &uuid_type));

    add_fixture_service();
    add_battery_service();
    add_control_service();
    gatt_restore_values();
}
