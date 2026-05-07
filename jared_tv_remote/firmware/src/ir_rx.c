#include "ir_rx.h"
#include "board.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <string.h>

/* Sample at 100us. Timer3 CTC, prescaler /8 -> 0.5us per timer tick.
   OCR3A = 199 -> compare match every 200 ticks = 100us. */
#define SAMPLE_TICK_US  100
#define IDLE_END_TICKS  250  /* 25ms of space => end of frame */

enum { RX_IDLE, RX_WAIT_START, RX_CAPTURING, RX_DONE };

static volatile uint8_t  rx_state = RX_IDLE;
static volatile uint8_t  rx_count;
static volatile uint8_t  rx_buf[IR_MAX_EDGES];
static volatile uint8_t  rx_prev;        /* 0 = mark, 1 = space */
static volatile uint16_t rx_dur;         /* 100us ticks of current level */

ISR(TIMER3_COMPA_vect) {
    uint8_t cur = ir_rx_low() ? 0 : 1;

    if (rx_state == RX_WAIT_START) {
        if (cur == 0) {
            rx_state = RX_CAPTURING;
            rx_prev  = 0;
            rx_dur   = 1;
            rx_count = 0;
        }
        return;
    }
    if (rx_state != RX_CAPTURING) return;

    if (cur != rx_prev) {
        uint8_t ticks = (rx_dur > 255) ? 255 : (uint8_t)rx_dur;
        if (rx_count < IR_MAX_EDGES) rx_buf[rx_count++] = ticks;
        else                         rx_state = RX_DONE;     /* buffer full */
        rx_prev = cur;
        rx_dur  = 1;
    } else {
        if (rx_dur < 0xFFFF) rx_dur++;
        if (cur == 1 && rx_dur > IDLE_END_TICKS) {
            rx_state = RX_DONE;
        }
    }
}

void ir_rx_init(void) {
    IR_RX_DDR  &= ~_BV(IR_RX_BIT);
    IR_RX_PORT |=  _BV(IR_RX_BIT);   /* internal pull-up */

    /* Timer3 CTC, prescaler /8 (CS3 = 010), TOP = OCR3A. */
    TCCR3A = 0;
    TCCR3B = _BV(WGM32) | _BV(CS31);
    OCR3A  = 199;
    TCNT3  = 0;
    TIMSK3 = 0;                       /* IRQ disabled until capture starts */
}

bool ir_rx_capture(ir_code_t *out, uint16_t timeout_ms) {
    if (!out) return false;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        rx_state = RX_WAIT_START;
        rx_count = 0;
        TCNT3    = 0;
        TIFR3    = _BV(OCF3A);
        TIMSK3   = _BV(OCIE3A);
    }

    /* Spin-wait with millisecond ticks. */
    uint16_t waited = 0;
    while (waited < timeout_ms) {
        uint8_t s;
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { s = rx_state; }
        if (s == RX_DONE) break;
        _delay_ms(1);
        waited++;
    }

    /* Stop timer ISR, snapshot buffer. */
    uint8_t  count;
    uint8_t  state;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        TIMSK3 = 0;
        state  = rx_state;
        count  = rx_count;
        rx_state = RX_IDLE;
    }

    if (state != RX_DONE || count < 2) return false;

    /* Drop trailing space if odd count: keep edges in mark/space pairs ending on a mark. */
    if (count & 1) count--;

    out->edge_count = count;
    out->tick_us    = SAMPLE_TICK_US;
    /* rx_buf is volatile; cast away for memcpy of an idle ISR-stopped buffer. */
    memcpy(out->edges, (const void *)rx_buf, count);
    if (count < IR_MAX_EDGES) {
        memset(out->edges + count, 0, IR_MAX_EDGES - count);
    }
    return true;
}
