#include "ir_tx.h"
#include "board.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/delay_basic.h>

/* Timer1 CTC on OC1A drives PD5 as a 38kHz square wave.
   Half-period at 16MHz: OCR1A = 16e6 / (2 * 38e3) - 1 = ~210. */
#define CARRIER_TOP 210

static inline void carrier_on(void) {
    /* Toggle OC1A on compare match -> 38kHz square wave on PD5. */
    TCCR1A = (TCCR1A & ~(_BV(COM1A1) | _BV(COM1A0))) | _BV(COM1A0);
}

static inline void carrier_off(void) {
    TCCR1A &= ~(_BV(COM1A1) | _BV(COM1A0));
    IR_TX_PORT &= ~_BV(IR_TX_BIT);
}

static void busy_us_per_tick(uint8_t ticks, uint8_t tick_us) {
    /* _delay_loop_2(n) burns 4n cycles. At 16MHz, n = us * 4 per tick. */
    const uint16_t loops = (uint16_t)tick_us * (F_CPU / 4000000UL);
    while (ticks--) _delay_loop_2(loops);
}

void ir_tx_init(void) {
    IR_TX_DDR  |= _BV(IR_TX_BIT);
    IR_TX_PORT &= ~_BV(IR_TX_BIT);

    /* CTC mode 4: WGM13:0 = 0100, TOP = OCR1A. Prescaler /1. */
    TCCR1A = 0;
    TCCR1B = _BV(WGM12) | _BV(CS10);
    OCR1A  = CARRIER_TOP;
    TCNT1  = 0;
}

void ir_tx_send(const ir_code_t *code) {
    if (!code || code->edge_count == 0) return;

    uint8_t prev_sreg = SREG;
    cli();

    /* Edges alternate: edge[0] = mark, edge[1] = space, ... */
    for (uint8_t i = 0; i < code->edge_count; i++) {
        if ((i & 1) == 0) carrier_on();
        else              carrier_off();
        busy_us_per_tick(code->edges[i], code->tick_us);
    }
    carrier_off();

    SREG = prev_sreg;
}
