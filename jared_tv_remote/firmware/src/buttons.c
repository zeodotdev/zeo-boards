#include "buttons.h"
#include "board.h"
#include <util/delay.h>

static const uint8_t COL_BITS[3] = { COL0_BIT, COL1_BIT, COL2_BIT };
static const uint8_t ROW_BITS[5] = { ROW0_BIT, ROW1_BIT, ROW2_BIT, ROW3_BIT, ROW4_BIT };

void buttons_init(void) {
    buttons_configure_for_sleep();
}

void buttons_configure_for_sleep(void) {
    /* All columns driven LOW so any button press pulls a row low.
       Rows are inputs with pull-ups, idle high. INT0..3 wake on low level. */
    COL_DDR  |=  COL_MASK;
    COL_PORT &= ~COL_MASK;

    ROW_DDR  &= ~ROW_MASK;
    ROW_PORT |=  ROW_MASK;
}

void buttons_configure_for_scan(void) {
    /* Same row config; columns will be walked one-low-at-a-time during scan. */
    COL_DDR  |= COL_MASK;
    COL_PORT |= COL_MASK;

    ROW_DDR  &= ~ROW_MASK;
    ROW_PORT |=  ROW_MASK;
}

static uint8_t scan_raw(void) {
    for (uint8_t c = 0; c < 3; c++) {
        COL_PORT |=  COL_MASK;
        COL_PORT &= ~BIT(COL_BITS[c]);
        _delay_us(20);

        uint8_t pin = ROW_PIN;
        for (uint8_t r = 0; r < 5; r++) {
            if (!(pin & BIT(ROW_BITS[r]))) {
                COL_PORT |= COL_MASK;
                return r * 3 + c;
            }
        }
    }
    COL_PORT |= COL_MASK;
    return BTN_NONE;
}

uint8_t buttons_scan_once(void) {
    uint8_t a = scan_raw();
    if (a == BTN_NONE) return BTN_NONE;
    _delay_ms(8);
    uint8_t b = scan_raw();
    return (a == b) ? a : BTN_NONE;
}

uint8_t buttons_wait_press(uint16_t timeout_ms) {
    /* Poll at ~5ms; debounce by requiring two consecutive equal samples. */
    uint16_t elapsed = 0;
    while (timeout_ms == 0 || elapsed < timeout_ms) {
        uint8_t b = buttons_scan_once();
        if (b != BTN_NONE) return b;
        _delay_ms(5);
        elapsed += 5;
    }
    return BTN_NONE;
}

void buttons_wait_release(void) {
    uint8_t stable = 0;
    while (stable < 3) {
        if (scan_raw() == BTN_NONE) stable++;
        else stable = 0;
        _delay_ms(5);
    }
}
