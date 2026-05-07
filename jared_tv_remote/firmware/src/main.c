#include "board.h"
#include "buttons.h"
#include "ir_tx.h"
#include "ir_rx.h"
#include "storage.h"
#include "sleep.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define LEARN_BUTTON_TIMEOUT_MS  15000
#define LEARN_IR_TIMEOUT_MS      10000
#define WAKE_DEBOUNCE_MS         60
#define POST_TX_FLASH_MS         40

static void led_init(void) {
    LED_DDR |= _BV(LED_BIT);
    led_off();
}

static void blink(uint8_t count, uint16_t on_ms, uint16_t off_ms) {
    while (count--) {
        led_on();
        for (uint16_t i = 0; i < on_ms; i++) _delay_ms(1);
        led_off();
        if (count) for (uint16_t i = 0; i < off_ms; i++) _delay_ms(1);
    }
}

static void blip_tx(void)        { blink(1, POST_TX_FLASH_MS, 0); }
static void blip_no_code(void)   { blink(3, 50, 80); }
static void blip_learn_ok(void)  { blink(2, 120, 120); }
static void blip_learn_fail(void){ blink(6, 60, 60); }

/* Held LED indicates "ready to receive an IR code from another remote". */
static void learn_indicator_on(void)  { led_on(); }
static void learn_indicator_off(void) { led_off(); }

static void enter_learn_mode(void) {
    /* Phase 1: ask which button to teach. Slow blink while waiting. */
    uint16_t waited = 0;
    uint8_t target = BTN_NONE;
    while (waited < LEARN_BUTTON_TIMEOUT_MS) {
        led_on();  _delay_ms(80);
        led_off(); _delay_ms(420);
        waited += 500;

        uint8_t b = buttons_scan_once();
        if (b != BTN_NONE) { target = b; break; }
    }

    if (target == BTN_NONE || target == BTN_LEARN || target >= NUM_CODES) {
        blip_learn_fail();
        buttons_wait_release();
        return;
    }
    buttons_wait_release();

    /* Phase 2: capture IR. Solid LED while listening. */
    learn_indicator_on();
    ir_code_t code;
    bool ok = ir_rx_capture(&code, LEARN_IR_TIMEOUT_MS);
    learn_indicator_off();

    if (!ok || !storage_save(target, &code)) {
        blip_learn_fail();
        return;
    }
    blip_learn_ok();
}

static void handle_button(uint8_t btn) {
    if (btn == BTN_LEARN) {
        buttons_wait_release();
        enter_learn_mode();
        return;
    }
    if (btn >= NUM_CODES) return;

    ir_code_t code;
    if (storage_load(btn, &code)) {
        ir_tx_send(&code);
        blip_tx();
    } else {
        blip_no_code();
    }
    buttons_wait_release();
}

int main(void) {
    cli();
    power_init();
    led_init();
    storage_init();
    ir_tx_init();
    ir_rx_init();
    buttons_init();
    sei();

    /* Boot indicator. */
    blink(2, 30, 80);

    for (;;) {
        deep_sleep_until_press();

        /* Brief debounce after wake; the row that woke us must still read low. */
        buttons_configure_for_scan();
        uint8_t btn = buttons_wait_press(WAKE_DEBOUNCE_MS);
        if (btn == BTN_NONE) {
            /* Glitch / released too fast. Back to sleep. */
            continue;
        }
        handle_button(btn);
    }
}
