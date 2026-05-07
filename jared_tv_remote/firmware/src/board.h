#ifndef BOARD_H
#define BOARD_H

#include <avr/io.h>

#define BIT(n) (1U << (n))

#define COL_DDR   DDRB
#define COL_PORT  PORTB
#define COL0_BIT  0
#define COL1_BIT  1
#define COL2_BIT  2
#define COL_MASK  (BIT(COL0_BIT) | BIT(COL1_BIT) | BIT(COL2_BIT))

#define ROW_DDR   DDRD
#define ROW_PORT  PORTD
#define ROW_PIN   PIND
#define ROW0_BIT  0
#define ROW1_BIT  1
#define ROW2_BIT  2
#define ROW3_BIT  3
#define ROW4_BIT  4
#define ROW_MASK  (BIT(ROW0_BIT) | BIT(ROW1_BIT) | BIT(ROW2_BIT) | BIT(ROW3_BIT) | BIT(ROW4_BIT))
#define ROW_INT_MASK (BIT(ROW0_BIT) | BIT(ROW1_BIT) | BIT(ROW2_BIT) | BIT(ROW3_BIT))

#define IR_TX_DDR   DDRD
#define IR_TX_PORT  PORTD
#define IR_TX_BIT   5

#define IR_RX_DDR   DDRD
#define IR_RX_PORT  PORTD
#define IR_RX_PIN   PIND
#define IR_RX_BIT   6
#define ir_rx_low() (!(IR_RX_PIN & BIT(IR_RX_BIT)))

#define LED_DDR   DDRD
#define LED_PORT  PORTD
#define LED_BIT   7
#define led_on()    (LED_PORT |=  BIT(LED_BIT))
#define led_off()   (LED_PORT &= ~BIT(LED_BIT))
#define led_toggle() (LED_PORT ^= BIT(LED_BIT))

#define NUM_BUTTONS  15
#define NUM_CODES    14
#define BTN_LEARN    14

#endif
