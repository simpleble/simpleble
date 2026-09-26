#pragma once

#include "peripheral.h"

void gatt_init(peripheral_state_t* state);
void gatt_restore_values(void);
void gatt_set_value(attribute_id_t attribute, const uint8_t* data, uint16_t length);
