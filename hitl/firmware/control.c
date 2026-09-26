#include "control.h"

#include <SEGGER_RTT.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "build_id.h"
#include "peripheral.h"

#define COMMAND_MAX_ARGUMENTS 9
#define RTT_BYTES_PER_POLL 256
#define RESPONSE_PAGE_PAYLOAD (CONTROL_PAGE_SIZE - 4)

typedef struct {
    uint32_t id;
    unsigned count;
    char* arguments[COMMAND_MAX_ARGUMENTS];
} command_t;

static uint32_t event_sequence;
static uint32_t dropped_records;
static bool handling_ble;
static char ble_input[CONTROL_LINE_SIZE];
static uint16_t ble_input_length;
static bool ble_input_invalid;
static char ble_response[CONTROL_LINE_SIZE];
static uint16_t response_length;
static uint16_t response_offset;

static void write_record(char* line, int length, bool use_ble) {
    if (length < 0 || length >= CONTROL_LINE_SIZE - 1) {
        ++dropped_records;
        return;
    }

    line[length++] = '\n';
    if (use_ble) {
        memcpy(ble_response, line, length);
        response_length = length;
        response_offset = 0;
    } else if (SEGGER_RTT_Write(0, line, length) != (unsigned)length) {
        ++dropped_records;
    }
}

void control_reply(const char* format, ...) {
    char line[CONTROL_LINE_SIZE];
    va_list arguments;
    va_start(arguments, format);
    int length = vsnprintf(line, sizeof(line) - 1, format, arguments);
    va_end(arguments);

    write_record(line, length, handling_ble);
}

void control_event(const char* format, ...) {
    char detail[1200];
    va_list arguments;
    va_start(arguments, format);
    int length = vsnprintf(detail, sizeof(detail), format, arguments);
    va_end(arguments);

    ++event_sequence;
    if (length < 0 || length >= (int)sizeof(detail)) {
        ++dropped_records;
        return;
    }

    const peripheral_state_t* state = peripheral_state();
    char line[CONTROL_LINE_SIZE];
    length = snprintf(line, sizeof(line) - 1, "EV boot=%s conn=%lu test=%lu seq=%lu %s", state->boot_id,
                      (unsigned long)state->connection_id, (unsigned long)state->test_id, (unsigned long)event_sequence,
                      detail);
    write_record(line, length, false);
}

static void reply_result(uint32_t request_id, const char* error) {
    if (error) {
        control_reply("%lu ERR code=%s", (unsigned long)request_id, error);
    } else {
        control_reply("%lu OK", (unsigned long)request_id);
    }
}

static bool parse_number(const char* text, uint32_t* result) {
    if (!text || !*text) {
        return false;
    }

    uint32_t value = 0;
    for (; *text; ++text) {
        if (*text < '0' || *text > '9') {
            return false;
        }
        unsigned digit = *text - '0';
        if (value > (UINT32_MAX - digit) / 10) {
            return false;
        }
        value = value * 10 + digit;
    }

    *result = value;
    return true;
}

static int hex_digit(char digit) {
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    }
    if (digit >= 'a' && digit <= 'f') {
        return digit - 'a' + 10;
    }
    if (digit >= 'A' && digit <= 'F') {
        return digit - 'A' + 10;
    }
    return -1;
}

static bool decode_hex(const char* text, uint8_t* data, uint16_t* length) {
    if (!strcmp(text, "-")) {
        *length = 0;
        return true;
    }

    size_t characters = strlen(text);
    if (!characters || characters % 2 || characters > FIXTURE_MAX_VALUE_LENGTH * 2) {
        return false;
    }

    for (unsigned index = 0; index < characters / 2; ++index) {
        int high = hex_digit(text[index * 2]);
        int low = hex_digit(text[index * 2 + 1]);
        if (high < 0 || low < 0) {
            return false;
        }
        data[index] = (high << 4) | low;
    }

    *length = characters / 2;
    return true;
}

static void encode_hex(const uint8_t* data, uint16_t length, char* text) {
    static const char digits[] = "0123456789abcdef";
    if (!length) {
        strcpy(text, "-");
        return;
    }

    for (unsigned index = 0; index < length; ++index) {
        text[index * 2] = digits[data[index] >> 4];
        text[index * 2 + 1] = digits[data[index] & 15];
    }
    text[length * 2] = '\0';
}

static bool parse_command(char* line, command_t* command) {
    command->count = 0;
    for (char* argument = strtok(line, " "); argument; argument = strtok(NULL, " ")) {
        if (command->count == COMMAND_MAX_ARGUMENTS) {
            return false;
        }
        command->arguments[command->count++] = argument;
    }

    return command->count >= 2 && parse_number(command->arguments[0], &command->id) && command->id != 0;
}

static void reply_hello(uint32_t request_id) {
    const peripheral_state_t* state = peripheral_state();
    control_reply("%lu OK version=1 board=%s boot=%s build=%s max_value=512 max_send=244 line=1536",
                  (unsigned long)request_id, state->board_id, state->boot_id, BUILD_ID);
}

static void reply_status(uint32_t request_id) {
    const peripheral_state_t* state = peripheral_state();
    unsigned active = 0;
    for (unsigned index = 0; index < ATTRIBUTE_COUNT; ++index) {
        active += state->deliveries[index].active;
    }

    control_reply(
        "%lu OK boot=%s conn=%lu test=%lu connected=%u advertising=%u mtu=%u ceiling=%u active=%u accepted=%lu "
        "tx=%lu confirmed=%lu dropped=%lu cccd_a=%u cccd_b=%u cccd_i=%u cccd_battery=%u",
        (unsigned long)request_id, state->boot_id, (unsigned long)state->connection_id, (unsigned long)state->test_id,
        state->connection_handle != FIXTURE_INVALID_CONNECTION, state->advertising, state->mtu, state->mtu_ceiling,
        active, (unsigned long)state->accepted, (unsigned long)state->transmitted, (unsigned long)state->confirmed,
        (unsigned long)dropped_records, state->attributes[ATTRIBUTE_NOTIFY_A].subscription,
        state->attributes[ATTRIBUTE_NOTIFY_B].subscription, state->attributes[ATTRIBUTE_INDICATE].subscription,
        state->attributes[ATTRIBUTE_BATTERY].subscription);
}

static const char* configure_advertising(const command_t* command, uint32_t token) {
    static const char* profiles[] = {"default", "service_data", "no_tx_power", "nonconnectable", "nonscannable"};
    unsigned profile;
    for (profile = 0; profile < sizeof(profiles) / sizeof(profiles[0]); ++profile) {
        if (!strcmp(command->arguments[2], profiles[profile])) {
            break;
        }
    }
    if (profile == sizeof(profiles) / sizeof(profiles[0])) {
        return "syntax";
    }

    uint32_t duration = 0;
    if (command->count == 5 &&
        (!parse_number(command->arguments[4], &duration) || duration < 1000 || duration > 60000)) {
        return "syntax";
    }
    // The host could not reconnect to restore a connectable profile.
    if (handling_ble && (profile == ADVERTISING_NONCONNECTABLE || profile == ADVERTISING_NONSCANNABLE) && !duration) {
        return "syntax";
    }

    peripheral_set_advertising((advertising_profile_t)profile, token, duration);
    return NULL;
}

static bool handle_device_command(const command_t* command) {
    const char* name = command->arguments[1];
    uint32_t value;

    if (!strcmp(name, "HELLO") && command->count == 2) {
        reply_hello(command->id);
    } else if (!strcmp(name, "STATUS") && command->count == 2) {
        reply_status(command->id);
    } else if (!strcmp(name, "RESET") && command->count == 3 && parse_number(command->arguments[2], &value)) {
        peripheral_reset(command->id, value, handling_ble);
    } else if (!strcmp(name, "STOP") && command->count == 2) {
        peripheral_stop();
        reply_result(command->id, NULL);
    } else if (!strcmp(name, "MTU") && command->count == 3 && parse_number(command->arguments[2], &value) &&
               (value == 23 || value == 247)) {
        peripheral_set_mtu(value);
        reply_result(command->id, NULL);
    } else if (!strcmp(name, "ADV") && (command->count == 4 || command->count == 5) &&
               parse_number(command->arguments[3], &value)) {
        reply_result(command->id, configure_advertising(command, value));
    } else if ((!strcmp(name, "DISCONNECT") || !strcmp(name, "REBOOT")) && command->count == 3 &&
               parse_number(command->arguments[2], &value) && value >= 100 && value <= 10000) {
        scheduled_action_t action = !strcmp(name, "DISCONNECT") ? ACTION_DISCONNECT : ACTION_REBOOT;
        reply_result(command->id, peripheral_schedule(action, value, handling_ble));
    } else {
        return false;
    }
    return true;
}

static attribute_id_t find_attribute(const char* name) {
    const peripheral_state_t* state = peripheral_state();
    for (unsigned index = 0; index < ATTRIBUTE_COUNT; ++index) {
        if (!strcmp(name, state->attributes[index].name)) {
            return (attribute_id_t)index;
        }
    }
    return ATTRIBUTE_COUNT;
}

static void reply_delivery(uint32_t request_id, attribute_id_t attribute) {
    const delivery_t* delivery = &peripheral_state()->deliveries[attribute];
    control_reply("%lu OK active=%u stream=%lu sent=%lu completed=%lu reason=%s", (unsigned long)request_id,
                  delivery->active, (unsigned long)delivery->stream_id, (unsigned long)delivery->sent,
                  (unsigned long)delivery->completed, delivery->reason ? delivery->reason : "none");
}

static void reply_value(uint32_t request_id, attribute_id_t attribute, bool last_write) {
    const attribute_t* value = &peripheral_state()->attributes[attribute];
    char hex[FIXTURE_MAX_VALUE_LENGTH * 2 + 1];
    if (last_write) {
        encode_hex(value->last_write, value->last_write_length, hex);
        control_reply("%lu OK len=%u writes=%lu op=%u data=%s", (unsigned long)request_id, value->last_write_length,
                      (unsigned long)value->write_count, value->last_write_operation, hex);
    } else {
        encode_hex(value->data, value->length, hex);
        control_reply("%lu OK len=%u writes=%lu data=%s", (unsigned long)request_id, value->length,
                      (unsigned long)value->write_count, hex);
    }
}

static const char* start_stream(const command_t* command, attribute_id_t attribute) {
    uint32_t stream_id;
    uint32_t length;
    uint32_t count;
    uint32_t interval;
    if (command->count != 7 || !parse_number(command->arguments[3], &stream_id) ||
        !parse_number(command->arguments[4], &length) || !parse_number(command->arguments[5], &count) ||
        !parse_number(command->arguments[6], &interval)) {
        return "syntax";
    }
    if (length < 12 || length > FIXTURE_MAX_SEND_LENGTH || !count || count > 10000 || interval < 20 ||
        interval > 1000 || attribute < ATTRIBUTE_NOTIFY_A || attribute > ATTRIBUTE_INDICATE) {
        return "syntax";
    }
    return peripheral_stream(attribute, stream_id, length, count, interval);
}

static const char* update_attribute(const command_t* command, attribute_id_t attribute) {
    const char* name = command->arguments[1];
    bool set = !strcmp(name, "SET");
    bool send = !strcmp(name, "SEND");
    bool stream = !strcmp(name, "STREAM");
    if (!set && !send && !stream) {
        return "syntax";
    }
    if (peripheral_attribute_busy(attribute)) {
        return "busy";
    }
    if (stream) {
        return start_stream(command, attribute);
    }

    uint8_t data[FIXTURE_MAX_VALUE_LENGTH];
    uint16_t length;
    if (command->count != 4 || !decode_hex(command->arguments[3], data, &length)) {
        return "syntax";
    }
    if (set) {
        return peripheral_set_value(attribute, data, length);
    }
    return peripheral_send(attribute, data, length);
}

static void handle_attribute_command(const command_t* command) {
    if (command->count < 3) {
        reply_result(command->id, "syntax");
        return;
    }

    attribute_id_t attribute = find_attribute(command->arguments[2]);
    if (attribute == ATTRIBUTE_COUNT) {
        reply_result(command->id, "attribute");
        return;
    }

    const char* name = command->arguments[1];
    if (!strcmp(name, "WORK") && command->count == 3) {
        reply_delivery(command->id, attribute);
    } else if (!strcmp(name, "GET") && command->count == 3) {
        reply_value(command->id, attribute, false);
    } else if (!strcmp(name, "GET_WRITE") && command->count == 3) {
        reply_value(command->id, attribute, true);
    } else {
        reply_result(command->id, update_attribute(command, attribute));
    }
}

static void process_command(char* line) {
    command_t command;
    if (!parse_command(line, &command)) {
        reply_result(0, "syntax");
        return;
    }
    if (peripheral_state()->reset_request_id) {
        reply_result(command.id, "resetting");
        return;
    }
    if (!handle_device_command(&command)) {
        handle_attribute_command(&command);
    }
}

void control_init(void) { SEGGER_RTT_Init(); }

void control_poll(void) {
    static char line[CONTROL_LINE_SIZE];
    static unsigned length;
    static bool invalid;
    static bool carriage_return;

    // Leave time for BLE events and scheduled actions when the host sends continuously.
    for (unsigned index = 0; index < RTT_BYTES_PER_POLL; ++index) {
        int character = SEGGER_RTT_GetKey();
        if (character < 0) {
            break;
        }
        if (character == '\n') {
            if (invalid) {
                reply_result(0, "line");
            } else {
                line[length] = '\0';
                process_command(line);
            }
            length = 0;
            invalid = false;
            carriage_return = false;
        } else if (character == '\r' && !carriage_return) {
            carriage_return = true;
        } else {
            if (carriage_return) {
                invalid = true;
            }
            if (character < 32 || character > 126 || length >= sizeof(line) - 1) {
                invalid = true;
            } else if (!invalid) {
                line[length++] = character;
            }
        }
    }
}

void control_clear_input(void) {
    ble_input_length = 0;
    ble_input_invalid = false;
}

bool control_valid_write(const uint8_t* data, uint16_t length) {
    if (length < 2) {
        return false;
    }
    if (data[0] == 0) {
        return true;
    }
    if (data[0] != 1 || length != 3) {
        return false;
    }
    uint16_t offset = data[1] | ((uint16_t)data[2] << 8);
    return offset <= response_length;
}

void control_write(const uint8_t* data, uint16_t length) {
    if (data[0] == 1) {
        response_offset = data[1] | ((uint16_t)data[2] << 8);
        return;
    }

    handling_ble = true;
    for (unsigned index = 1; index < length; ++index) {
        uint8_t character = data[index];
        if (character == '\n') {
            ble_input[ble_input_length] = '\0';
            if (ble_input_invalid) {
                reply_result(0, "line");
            } else {
                process_command(ble_input);
            }
            ble_input_length = 0;
            ble_input_invalid = false;
        } else if (character < 32 || character > 126 || ble_input_length >= sizeof(ble_input) - 1) {
            ble_input_invalid = true;
        } else if (!ble_input_invalid) {
            ble_input[ble_input_length++] = character;
        }
    }
    handling_ble = false;
}

uint16_t control_read_page(uint8_t* page) {
    uint16_t length = response_length - response_offset;
    if (length > RESPONSE_PAGE_PAYLOAD) {
        length = RESPONSE_PAGE_PAYLOAD;
    }

    page[0] = response_length;
    page[1] = response_length >> 8;
    page[2] = response_offset;
    page[3] = response_offset >> 8;
    memcpy(page + 4, ble_response + response_offset, length);
    return length + 4;
}

bool control_final_page_selected(void) {
    return response_length && response_offset + RESPONSE_PAGE_PAYLOAD >= response_length;
}
