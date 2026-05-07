#include "sleep.h"
#include "board.h"
#include "buttons.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

/* Empty wake ISRs: low-level wake from power-down via INT0..INT3.
   The ISR body just needs to return; we mask the INT in main before
   scanning so it doesn't refire while a row is held low. */
ISR(INT0_vect) { EIMSK &= ~_BV(INT0); }
ISR(INT1_vect) { EIMSK &= ~_BV(INT1); }
ISR(INT2_vect) { EIMSK &= ~_BV(INT2); }
ISR(INT3_vect) { EIMSK &= ~_BV(INT3); }

void power_init(void) {
    /* Disable watchdog (set by bootloader sometimes). */
    MCUSR &= ~_BV(WDRF);
    wdt_disable();

    /* Disable analog comparator and ADC. */
    ACSR  |= _BV(ACD);
    ADCSRA &= ~_BV(ADEN);

    /* Disable USB module entirely (we only use the on-chip DFU bootloader). */
    USBCON &= ~_BV(USBE);
    UDIEN   = 0;
    UHWCON  = 0;

    /* Power-Reduction: shut down peripherals we don't use in app code. */
    PRR0 = _BV(PRTWI) | _BV(PRTIM0) | _BV(PRSPI) | _BV(PRADC);
    PRR1 = _BV(PRTIM4) | _BV(PRUSART1) | _BV(PRUSB);
    /* PRTIM1 (IR TX) and PRTIM3 (IR RX) intentionally left enabled. */

    /* Drive all unused pins explicitly to avoid floating inputs:
       PB3..PB7, PC6, PC7, PE6, PF0..PF1, PF4..PF7. Set as outputs low. */
    DDRB  |= 0xF8;  PORTB &= ~0xF8;     /* PB3..PB7 */
    DDRC  |= 0xC0;  PORTC &= ~0xC0;     /* PC6, PC7 */
    DDRE  |= _BV(6); PORTE &= ~_BV(6);  /* PE6 */
    DDRF  |= 0xF3;  PORTF &= ~0xF3;     /* PF0,PF1,PF4..PF7 */

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void deep_sleep_until_press(void) {
    buttons_configure_for_sleep();

    /* Configure INT0..INT3 to trigger on LOW level (the only mode that wakes
       from power-down). Bits ISCn1:ISCn0 = 00 -> low level. */
    EICRA &= ~(_BV(ISC31) | _BV(ISC30) |
               _BV(ISC21) | _BV(ISC20) |
               _BV(ISC11) | _BV(ISC10) |
               _BV(ISC01) | _BV(ISC00));

    /* Clear pending and arm INT0..INT3. */
    EIFR  = _BV(INTF3) | _BV(INTF2) | _BV(INTF1) | _BV(INTF0);
    EIMSK |= _BV(INT3) | _BV(INT2) | _BV(INT1) | _BV(INT0);

    cli();
    sleep_enable();
#if defined(sleep_bod_disable)
    sleep_bod_disable();
#endif
    sei();
    sleep_cpu();
    sleep_disable();

    /* Mask all wake INTs so a held button doesn't keep firing. */
    EIMSK &= ~(_BV(INT3) | _BV(INT2) | _BV(INT1) | _BV(INT0));
}
