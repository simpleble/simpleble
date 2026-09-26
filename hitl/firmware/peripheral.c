#include "peripheral.h"
#include "build_id.h"
#include "control.h"
#include "gatt.h"

#include <components/libraries/util/app_error.h>
#include <nrf_soc.h>
#include <simpleembed/nrf52/hal/nrf_timer.h>
#include <simpleembed/nrf52/softdevice/common/nrf_sdh.h>
#include <simpleembed/nrf52/softdevice/common/nrf_sdh_ble.h>
#include <simpleembed/support/delay.h>
#include <stdio.h>
#include <string.h>

#define CONNECTION_TAG 1

static peripheral_state_t state = {
    .connection_handle = BLE_CONN_HANDLE_INVALID,
    .mtu = 23,
    .mtu_ceiling = 247,
    .attributes =
        {
            [ATTRIBUTE_INFO] = {.name = "INFO", .uuid = 0x0002, .max_length = 20},
            [ATTRIBUTE_READ_VALUE] = {.name = "READ_VALUE", .uuid = 0x0003, .max_length = 512},
            [ATTRIBUTE_WRITE_REQUEST] = {.name = "WRITE_REQUEST", .uuid = 0x0004, .max_length = 512},
            [ATTRIBUTE_WRITE_COMMAND] = {.name = "WRITE_COMMAND", .uuid = 0x0005, .max_length = 512},
            [ATTRIBUTE_NOTIFY_A] = {.name = "NOTIFY_A", .uuid = 0x0006, .max_length = 244},
            [ATTRIBUTE_NOTIFY_B] = {.name = "NOTIFY_B", .uuid = 0x0007, .max_length = 244},
            [ATTRIBUTE_INDICATE] = {.name = "INDICATE", .uuid = 0x0008, .max_length = 244},
            [ATTRIBUTE_ERROR] = {.name = "ERROR", .uuid = 0x0009, .max_length = 1},
            [ATTRIBUTE_DESCRIPTOR] = {.name = "DESC", .uuid = 0x0010, .max_length = 32},
            [ATTRIBUTE_BATTERY] = {.name = "BATTERY", .uuid = 0x2a19, .max_length = 1},
        },
};

static const uint8_t base_uuid[16] = {
    0x01, 0x43, 0x5f, 0x6e, 0x8d, 0x9c, 0xb0, 0xa1, 0x6f, 0x4b, 0x7a, 0x2e, 0x00, 0x00, 0x57, 0x7e,
};

static uint8_t advertising_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
static uint8_t advertising_data[31];
static uint8_t scan_response_data[31];
static char device_name[13];
static advertising_profile_t advertising_profile;
static uint32_t advertising_token;
static uint32_t advertising_duration_ms;
static uint32_t advertising_deadline;

static uint16_t connection_mtu_ceiling = 247;
static int notification_owner = -1;
static int indication_owner = -1;
static unsigned next_notification;

static scheduled_action_t scheduled_action;
static uint32_t scheduled_at;
static uint32_t scheduled_delay_ms;
static bool waiting_for_reply;
static uint32_t reset_test_id;
static bool reset_from_ble;

static void write_u32(uint8_t* destination, uint32_t value) {
    for (unsigned byte = 0; byte < 4; ++byte) {
        destination[byte] = value >> (8 * byte);
    }
}

static uint32_t timer_now(void) {
    nrf_timer_task_trigger(NRF_TIMER2, NRF_TIMER_TASK_CAPTURE0);
    return nrf_timer_cc_read(NRF_TIMER2, NRF_TIMER_CC_CHANNEL0);
}

static uint32_t timer_ticks(uint32_t milliseconds) { return (milliseconds * 125 + 3) / 4; }

static bool timer_due(uint32_t deadline) { return (int32_t)(timer_now() - deadline) >= 0; }

const peripheral_state_t* peripheral_state(void) { return &state; }

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info) {
    const error_info_t* error = id == NRF_FAULT_ID_SDK_ERROR ? (const error_info_t*)info : NULL;
    control_event("type=fault id=%lu pc=%lu info=%lu code=%lu line=%lu", (unsigned long)id, (unsigned long)pc,
                  (unsigned long)info, error ? (unsigned long)error->err_code : 0,
                  error ? (unsigned long)error->line_num : 0);
    app_error_save_and_stop(id, pc, info);
}

static ble_gap_adv_data_t build_advertising_data(void) {
    uint8_t* payload = advertising_data;
    *payload++ = 2;
    *payload++ = 1;  // Flags.
    *payload++ = 6;

    if (advertising_profile == ADVERTISING_SERVICE_DATA) {
        *payload++ = 22;
        *payload++ = 0x21;  // 128-bit service data.
        memcpy(payload, base_uuid, sizeof(base_uuid));
        payload[12] = 1;
        payload += sizeof(base_uuid);
        *payload++ = 1;
        write_u32(payload, advertising_token);
        payload += 4;
    } else {
        *payload++ = 17;
        *payload++ = 7;  // Complete 128-bit service UUID list.
        memcpy(payload, base_uuid, sizeof(base_uuid));
        payload[12] = 1;
        payload += sizeof(base_uuid);
        *payload++ = 8;
        *payload++ = 0xff;  // Manufacturer data, company ID 0xffff.
        *payload++ = 0xff;
        *payload++ = 0xff;
        *payload++ = 1;
        write_u32(payload, advertising_token);
        payload += 4;
    }

    uint8_t* scan_response = scan_response_data;
    *scan_response++ = 13;
    *scan_response++ = 9;  // Complete local name.
    memcpy(scan_response, device_name, 12);
    scan_response += 12;
    if (advertising_profile != ADVERTISING_NO_TX_POWER) {
        *scan_response++ = 2;
        *scan_response++ = 0x0a;
        *scan_response++ = 0;
    }

    ble_gap_adv_data_t data = {
        .adv_data = {.p_data = advertising_data, .len = payload - advertising_data},
        .scan_rsp_data = {.p_data = scan_response_data, .len = scan_response - scan_response_data},
    };
    if (advertising_profile == ADVERTISING_NONSCANNABLE) {
        // Non-scannable advertising cannot carry scan response data.
        data.scan_rsp_data.p_data = NULL;
        data.scan_rsp_data.len = 0;
    }
    return data;
}

static void start_advertising(void) {
    if (state.advertising) {
        APP_ERROR_CHECK(sd_ble_gap_adv_stop(advertising_handle));
    }

    ble_gap_adv_data_t data = build_advertising_data();
    ble_gap_adv_params_t parameters = {
        .properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED,
        .primary_phy = BLE_GAP_PHY_1MBPS,
        .interval = 160,
    };
    if (advertising_profile == ADVERTISING_NONCONNECTABLE) {
        parameters.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED;
    } else if (advertising_profile == ADVERTISING_NONSCANNABLE) {
        parameters.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    }

    APP_ERROR_CHECK(sd_ble_gap_adv_set_configure(&advertising_handle, &data, &parameters));
    APP_ERROR_CHECK(sd_ble_gap_adv_start(advertising_handle, CONNECTION_TAG));
    state.advertising = true;
    if (advertising_duration_ms) {
        advertising_deadline = timer_now() + timer_ticks(advertising_duration_ms);
    }
}

void peripheral_finish_delivery(attribute_id_t id, const char* reason) {
    delivery_t* delivery = &state.deliveries[id];
    if (!delivery->active) {
        return;
    }

    control_event("type=complete attr=%s kind=%s stream=%lu sent=%lu completed=%lu reason=%s",
                  state.attributes[id].name, delivery->is_stream ? "stream" : "send",
                  (unsigned long)delivery->stream_id, (unsigned long)delivery->sent, (unsigned long)delivery->completed,
                  reason);
    delivery->active = false;
    delivery->reason = reason;
}

static void cancel_deliveries(const char* reason) {
    for (unsigned id = 0; id < ATTRIBUTE_COUNT; ++id) {
        peripheral_finish_delivery(id, reason);
    }
}

static void finish_reset(void) {
    scheduled_action = ACTION_NONE;
    waiting_for_reply = false;
    memset(state.deliveries, 0, sizeof(state.deliveries));
    state.test_id = reset_test_id;
    advertising_profile = ADVERTISING_DEFAULT;
    advertising_duration_ms = 0;
    advertising_token = 0;
    state.mtu_ceiling = 247;
    state.mtu = 23;
    state.accepted = 0;
    state.transmitted = 0;
    state.confirmed = 0;

    gatt_restore_values();
    start_advertising();
    if (!reset_from_ble) {
        control_reply("%lu OK test=%lu", (unsigned long)state.reset_request_id, (unsigned long)state.test_id);
    }
    state.reset_request_id = 0;
}

void peripheral_reset(uint32_t request_id, uint32_t test_id, bool from_ble) {
    cancel_deliveries("reset");
    scheduled_action = ACTION_NONE;
    state.reset_request_id = request_id;
    reset_test_id = test_id;
    reset_from_ble = from_ble;

    if (from_ble) {
        scheduled_action = ACTION_RESET;
        scheduled_delay_ms = 100;
        waiting_for_reply = true;
        control_reply("%lu OK test=%lu", (unsigned long)request_id, (unsigned long)test_id);
    } else if (state.connection_handle != BLE_CONN_HANDLE_INVALID) {
        APP_ERROR_CHECK(sd_ble_gap_disconnect(state.connection_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
    } else {
        finish_reset();
    }
}

void peripheral_stop(void) { cancel_deliveries("stop"); }

void peripheral_set_mtu(uint16_t ceiling) { state.mtu_ceiling = ceiling; }

void peripheral_set_advertising(advertising_profile_t profile, uint32_t token, uint32_t duration_ms) {
    advertising_profile = profile;
    advertising_token = token;
    advertising_duration_ms = duration_ms;
    if (state.connection_handle == BLE_CONN_HANDLE_INVALID) {
        start_advertising();
    }
}

const char* peripheral_schedule(scheduled_action_t action, uint32_t delay_ms, bool from_ble) {
    if (scheduled_action != ACTION_NONE) {
        return "busy";
    }
    if (action == ACTION_DISCONNECT && state.connection_handle == BLE_CONN_HANDLE_INVALID) {
        return "disconnected";
    }

    scheduled_action = action;
    scheduled_delay_ms = delay_ms;
    waiting_for_reply = from_ble;
    scheduled_at = timer_now() + timer_ticks(delay_ms);
    return NULL;
}

void peripheral_acknowledge_action(void) {
    if (waiting_for_reply) {
        waiting_for_reply = false;
        scheduled_at = timer_now() + timer_ticks(scheduled_delay_ms);
    }
}

bool peripheral_attribute_busy(attribute_id_t id) {
    return state.deliveries[id].active || notification_owner == (int)id || indication_owner == (int)id;
}

static const char* validate_value(attribute_id_t id, const uint8_t* data, uint16_t length) {
    if (id == ATTRIBUTE_INFO || id == ATTRIBUTE_ERROR) {
        return "immutable";
    }
    if (length > state.attributes[id].max_length) {
        return "length";
    }
    if (id == ATTRIBUTE_BATTERY && (length != 1 || data[0] > 100)) {
        return "length";
    }
    return NULL;
}

const char* peripheral_set_value(attribute_id_t id, const uint8_t* data, uint16_t length) {
    const char* error = validate_value(id, data, length);
    if (!error) {
        gatt_set_value(id, data, length);
    }
    return error;
}

static const char* prepare_delivery(attribute_id_t id, uint16_t length) {
    const attribute_t* attribute = &state.attributes[id];
    uint16_t subscription = id == ATTRIBUTE_INDICATE ? 2 : 1;
    if (!attribute->cccd_handle || state.connection_handle == BLE_CONN_HANDLE_INVALID ||
        !(attribute->subscription & subscription)) {
        return "subscription";
    }
    if (length > state.mtu - 3) {
        return "length";
    }

    delivery_t* delivery = &state.deliveries[id];
    memset(delivery, 0, sizeof(*delivery));
    delivery->active = true;
    delivery->length = length;
    delivery->next_send_at = timer_now();
    return NULL;
}

const char* peripheral_send(attribute_id_t id, const uint8_t* data, uint16_t length) {
    const char* error = validate_value(id, data, length);
    if (error) {
        return error;
    }
    error = prepare_delivery(id, length);
    if (error) {
        return error;
    }

    delivery_t* delivery = &state.deliveries[id];
    delivery->packet_count = 1;
    delivery->interval_ticks = timer_ticks(20);
    memcpy(delivery->data, data, length);
    return NULL;
}

const char* peripheral_stream(attribute_id_t id, uint32_t stream_id, uint16_t length, uint32_t count,
                              uint32_t interval_ms) {
    const char* error = prepare_delivery(id, length);
    if (error) {
        return error;
    }

    delivery_t* delivery = &state.deliveries[id];
    delivery->is_stream = true;
    delivery->stream_id = stream_id;
    delivery->packet_count = count;
    delivery->interval_ticks = timer_ticks(interval_ms);
    return NULL;
}

void peripheral_connected(uint16_t connection_handle) {
    state.connection_handle = connection_handle;
    ++state.connection_id;
    state.advertising = false;
    state.mtu = 23;
    connection_mtu_ceiling = state.mtu_ceiling;

    control_clear_input();
    APP_ERROR_CHECK(sd_ble_gatts_sys_attr_set(connection_handle, NULL, 0, 0));
    control_event("type=connected");
}

void peripheral_disconnected(uint8_t reason) {
    control_event("type=disconnected reason=%u", reason);
    cancel_deliveries("disconnect");
    state.connection_handle = BLE_CONN_HANDLE_INVALID;
    waiting_for_reply = false;
    notification_owner = -1;
    indication_owner = -1;
    control_clear_input();

    for (unsigned id = 0; id < ATTRIBUTE_COUNT; ++id) {
        state.attributes[id].subscription = 0;
    }
    if (state.reset_request_id) {
        finish_reset();
    } else {
        start_advertising();
    }
}

void peripheral_exchange_mtu(uint16_t requested) {
    APP_ERROR_CHECK(sd_ble_gatts_exchange_mtu_reply(state.connection_handle, connection_mtu_ceiling));
    state.mtu = requested < connection_mtu_ceiling ? requested : connection_mtu_ceiling;
    control_event("type=mtu mtu=%u", state.mtu);
}

void peripheral_notification_complete(uint16_t count) {
    state.transmitted += count;
    if (notification_owner < 0) {
        return;
    }

    attribute_id_t id = notification_owner;
    notification_owner = -1;
    delivery_t* delivery = &state.deliveries[id];
    if (delivery->active && ++delivery->completed == delivery->packet_count) {
        peripheral_finish_delivery(id, "done");
    }
}

void peripheral_indication_confirmed(void) {
    ++state.confirmed;
    if (indication_owner < 0) {
        return;
    }

    attribute_id_t id = indication_owner;
    indication_owner = -1;
    delivery_t* delivery = &state.deliveries[id];
    if (delivery->active && ++delivery->completed == delivery->packet_count) {
        peripheral_finish_delivery(id, "done");
    }
}

static void initialize_identity(void) {
    uint32_t boot_nonce[2];
    while (sd_rand_application_vector_get((uint8_t*)boot_nonce, sizeof(boot_nonce)) != NRF_SUCCESS) {
        nrf_delay_ms(1);
    }
    snprintf(state.boot_id, sizeof(state.boot_id), "%08lx%08lx", (unsigned long)boot_nonce[1],
             (unsigned long)boot_nonce[0]);
    snprintf(state.board_id, sizeof(state.board_id), "%08lx%08lx", (unsigned long)NRF_FICR->DEVICEID[1],
             (unsigned long)NRF_FICR->DEVICEID[0]);
    snprintf(device_name, sizeof(device_name), "SBH-%08lX", (unsigned long)NRF_FICR->DEVICEID[0]);

    ble_gap_addr_t address;
    APP_ERROR_CHECK(sd_ble_gap_addr_get(&address));
    address.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
    address.addr[0] ^= 0x48;
    address.addr[5] |= 0xc0;
    APP_ERROR_CHECK(sd_ble_gap_addr_set(&address));
}

void peripheral_init(void) {
    nrf_timer_mode_set(NRF_TIMER2, NRF_TIMER_MODE_TIMER);
    nrf_timer_bit_width_set(NRF_TIMER2, NRF_TIMER_BIT_WIDTH_32);
    nrf_timer_frequency_set(NRF_TIMER2, NRF_TIMER_FREQ_31250Hz);
    nrf_timer_task_trigger(NRF_TIMER2, NRF_TIMER_TASK_START);

    APP_ERROR_CHECK(nrf_sdh_enable_request());
    uint32_t ram_start = 0;
    APP_ERROR_CHECK(nrf_sdh_ble_default_cfg_set(CONNECTION_TAG, &ram_start));
    APP_ERROR_CHECK(nrf_sdh_ble_enable(&ram_start));
    initialize_identity();

    ble_gap_conn_sec_mode_t security;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&security);
    APP_ERROR_CHECK(sd_ble_gap_device_name_set(&security, (uint8_t*)device_name, strlen(device_name)));
    ble_gap_conn_params_t parameters = {
        .min_conn_interval = 24,
        .max_conn_interval = 40,
        .conn_sup_timeout = 400,
    };
    APP_ERROR_CHECK(sd_ble_gap_ppcp_set(&parameters));

    gatt_init(&state);
    start_advertising();
    control_event("type=boot board=%s build=%s", state.board_id, BUILD_ID);
}

static void poll_advertising(void) {
    if (state.advertising && advertising_duration_ms && timer_due(advertising_deadline)) {
        advertising_profile = ADVERTISING_DEFAULT;
        advertising_duration_ms = 0;
        start_advertising();
    }
}

static void poll_scheduled_action(void) {
    if (scheduled_action == ACTION_NONE || waiting_for_reply || !timer_due(scheduled_at)) {
        return;
    }

    scheduled_action_t action = scheduled_action;
    scheduled_action = ACTION_NONE;
    control_event("type=scheduled action=%s", action == ACTION_REBOOT ? "reboot" : "disconnect");
    if (action == ACTION_REBOOT) {
        NVIC_SystemReset();
    }
    if (state.connection_handle != BLE_CONN_HANDLE_INVALID) {
        APP_ERROR_CHECK(sd_ble_gap_disconnect(state.connection_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION));
    }
}

static void prepare_stream_packet(delivery_t* delivery) {
    write_u32(delivery->data, state.test_id);
    write_u32(delivery->data + 4, delivery->stream_id);
    write_u32(delivery->data + 8, delivery->sent);
    for (unsigned byte = 12; byte < delivery->length; ++byte) {
        delivery->data[byte] = (delivery->sent + byte) & 255;
    }
}

static void send_next_packet(attribute_id_t id) {
    delivery_t* delivery = &state.deliveries[id];
    int* owner = id == ATTRIBUTE_INDICATE ? &indication_owner : &notification_owner;
    if (*owner >= 0) {
        return;
    }
    if (delivery->is_stream) {
        prepare_stream_packet(delivery);
    }

    uint16_t length = delivery->length;
    ble_gatts_hvx_params_t packet = {
        .handle = state.attributes[id].handle,
        .type = id == ATTRIBUTE_INDICATE ? BLE_GATT_HVX_INDICATION : BLE_GATT_HVX_NOTIFICATION,
        .p_len = &length,
        .p_data = delivery->data,
    };
    uint32_t result = sd_ble_gatts_hvx(state.connection_handle, &packet);
    if (result == NRF_ERROR_RESOURCES || result == NRF_ERROR_BUSY) {
        return;
    }
    if (result != NRF_SUCCESS) {
        control_event("type=send_error attr=%s code=%lu", state.attributes[id].name, (unsigned long)result);
        peripheral_finish_delivery(id, "error");
        return;
    }

    gatt_set_value(id, delivery->data, delivery->length);
    ++delivery->sent;
    ++state.accepted;
    *owner = id;
    if (id != ATTRIBUTE_INDICATE) {
        next_notification = (id + 1) % ATTRIBUTE_COUNT;
    }
    delivery->next_send_at = timer_now() + delivery->interval_ticks;
}

static void poll_deliveries(void) {
    if (state.connection_handle == BLE_CONN_HANDLE_INVALID) {
        return;
    }

    // Rotate priority so a long stream cannot starve the other characteristics.
    unsigned start = next_notification;
    for (unsigned offset = 0; offset < ATTRIBUTE_COUNT; ++offset) {
        attribute_id_t id = (start + offset) % ATTRIBUTE_COUNT;
        const delivery_t* delivery = &state.deliveries[id];
        if (!delivery->active || delivery->sent == delivery->packet_count || !timer_due(delivery->next_send_at)) {
            continue;
        }
        send_next_packet(id);
    }
}

void peripheral_poll(void) {
    poll_advertising();
    poll_scheduled_action();
    poll_deliveries();
}
