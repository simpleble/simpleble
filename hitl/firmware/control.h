#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CONTROL_LINE_SIZE 1536
#define CONTROL_PAGE_SIZE 20

void control_init(void);
void control_poll(void);
void control_reply(const char* format, ...);
void control_event(const char* format, ...);

// The peripheral owns ATT authorization and the GATT response value.
void control_clear_input(void);
bool control_valid_write(const uint8_t* data, uint16_t length);
void control_write(const uint8_t* data, uint16_t length);
uint16_t control_read_page(uint8_t* page);
bool control_final_page_selected(void);
