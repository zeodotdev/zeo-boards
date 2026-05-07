#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

#define BTN_NONE  0xFF

void buttons_init(void);

void buttons_configure_for_sleep(void);
void buttons_configure_for_scan(void);

uint8_t buttons_scan_once(void);
uint8_t buttons_wait_press(uint16_t timeout_ms);
void    buttons_wait_release(void);

#endif
