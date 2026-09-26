#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FIXTURE_MAX_VALUE_LENGTH 512
#define FIXTURE_MAX_SEND_LENGTH 244
#define FIXTURE_INVALID_CONNECTION UINT16_MAX

typedef enum {
    ATTRIBUTE_INFO,
    ATTRIBUTE_READ_VALUE,
    ATTRIBUTE_WRITE_REQUEST,
    ATTRIBUTE_WRITE_COMMAND,
    ATTRIBUTE_NOTIFY_A,
    ATTRIBUTE_NOTIFY_B,
    ATTRIBUTE_INDICATE,
    ATTRIBUTE_ERROR,
    ATTRIBUTE_DESCRIPTOR,
    ATTRIBUTE_BATTERY,
    ATTRIBUTE_COUNT,
} attribute_id_t;

typedef enum {
    ADVERTISING_DEFAULT,
    ADVERTISING_SERVICE_DATA,
    ADVERTISING_NO_TX_POWER,
    ADVERTISING_NONCONNECTABLE,
    ADVERTISING_NONSCANNABLE,
} advertising_profile_t;

typedef enum {
    ACTION_NONE,
    ACTION_DISCONNECT,
    ACTION_REBOOT,
    ACTION_RESET,
} scheduled_action_t;

typedef struct {
    const char* name;
    uint16_t uuid;
    uint16_t max_length;
    uint16_t length;
    uint16_t handle;
    uint16_t cccd_handle;
    uint16_t subscription;
    uint32_t write_count;
    uint8_t data[FIXTURE_MAX_VALUE_LENGTH];
    uint16_t last_write_length;
    uint8_t last_write_operation;
    uint8_t last_write[FIXTURE_MAX_VALUE_LENGTH];
} attribute_t;

typedef struct {
    bool active;
    bool is_stream;
    uint32_t stream_id;
    uint32_t packet_count;
    uint32_t sent;
    uint32_t completed;
    uint32_t interval_ticks;
    uint32_t next_send_at;
    uint16_t length;
    const char* reason;
    uint8_t data[FIXTURE_MAX_SEND_LENGTH];
} delivery_t;

typedef struct {
    char board_id[17];
    char boot_id[17];
    uint16_t connection_handle;
    uint16_t mtu;
    uint16_t mtu_ceiling;
    uint32_t connection_id;
    uint32_t test_id;
    uint32_t reset_request_id;
    uint32_t accepted;
    uint32_t transmitted;
    uint32_t confirmed;
    bool advertising;
    attribute_t attributes[ATTRIBUTE_COUNT];
    delivery_t deliveries[ATTRIBUTE_COUNT];
} peripheral_state_t;

void peripheral_init(void);
void peripheral_poll(void);
const peripheral_state_t* peripheral_state(void);

void peripheral_reset(uint32_t request_id, uint32_t test_id, bool from_ble);
void peripheral_stop(void);
void peripheral_set_mtu(uint16_t ceiling);
void peripheral_set_advertising(advertising_profile_t profile, uint32_t token, uint32_t duration_ms);
const char* peripheral_schedule(scheduled_action_t action, uint32_t delay_ms, bool from_ble);
void peripheral_acknowledge_action(void);

bool peripheral_attribute_busy(attribute_id_t attribute);
const char* peripheral_set_value(attribute_id_t attribute, const uint8_t* data, uint16_t length);
const char* peripheral_send(attribute_id_t attribute, const uint8_t* data, uint16_t length);
const char* peripheral_stream(attribute_id_t attribute, uint32_t stream_id, uint16_t length, uint32_t count,
                              uint32_t interval_ms);

// SoftDevice event handling, called by the GATT observer.
void peripheral_connected(uint16_t connection_handle);
void peripheral_disconnected(uint8_t reason);
void peripheral_exchange_mtu(uint16_t requested);
void peripheral_notification_complete(uint16_t count);
void peripheral_indication_confirmed(void);
void peripheral_finish_delivery(attribute_id_t attribute, const char* reason);
