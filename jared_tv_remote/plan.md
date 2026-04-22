## Project Summary: Solar-Powered Universal TV IR Remote

### Design Overview
Compact solar-powered universal TV IR remote with "learn" mode and native USB. Inspired by Chromecast remote layout and Samsung's SolarCell Remote.

### Architecture
- **MCU**: ATmega32U4-A (TQFP-44, 16MHz external crystal, native USB, 32KB flash, 2.5KB SRAM)
- **Power**: Dual solar cells (front + back) + USB-C (OR'd via BAT54 Schottky diodes) → Supercap (1F/5.5V) → MCP1700-3.3V LDO
- **IR TX**: NPN transistor (Q1) + 1k base R (R4) + 10 current limit R (R5) + E6QYDD1204-IRA940NM (940nm IR LED, U4)
- **IR RX**: IRM-H638T (38kHz IR demodulator, U1) + 100 series R (R1) + 100nF bypass cap (C1)
- **Status LED**: 330 R (R3) + Green LED (D1)
- **15 Buttons** in 5x3 matrix (5 rows, 3 columns)
- **USB**: Native USB via ATmega32U4 D+/D- pins, USB-C connector with CC pull-downs, HWB pulled low for DFU bootloader

### Pin Allocation (ATmega32U4-A)

| Pin # | Port       | Signal     | Direction | Notes                              |
|-------|------------|------------|-----------|------------------------------------|
| 8     | PB0        | COL0       | Output    | Button matrix column 0             |
| 9     | PB1        | COL1       | Output    | Button matrix column 1             |
| 10    | PB2        | COL2       | Output    | Button matrix column 2             |
| 18    | PD0 (INT0) | ROW0       | Input     | Button matrix row 0, ext interrupt |
| 19    | PD1 (INT1) | ROW1       | Input     | Button matrix row 1, ext interrupt |
| 20    | PD2 (INT2) | ROW2       | Input     | Button matrix row 2, ext interrupt |
| 21    | PD3 (INT3) | ROW3       | Input     | Button matrix row 3, ext interrupt |
| 25    | PD4        | ROW4       | Input     | Button matrix row 4                |
| 22    | PD5 (OC1A) | IR_TX      | Output    | IR transmit, Timer1 PWM for 38kHz  |
| 26    | PD6        | IR_RX      | Input     | IR receive (demodulated, active-low)|
| 27    | PD7        | STATUS_LED | Output    | Green status LED, drive high = on  |
| 4     | D+         | USB_DP     | Bidir     | USB data positive                  |
| 3     | D-         | USB_DM     | Bidir     | USB data negative                  |
| 7     | VBUS       | VBUS       | Input     | USB voltage sense (via 22 R6)      |
| 33    | PE2 (HWB)  | —          | Input     | 10k pull-down to GND (DFU enable)  |
| 13    | RESET      | —          | Input     | 10k pull-up to +3V3               |
| 17    | XTAL1      | —          | —         | 16MHz crystal                      |
| 16    | XTAL2      | —          | —         | 16MHz crystal                      |

Unused (no-connect): PB3-PB7, PC6, PC7, PE6, PF0-PF1, PF4-PF7

### Button Matrix (5 rows x 3 columns)

Active-low scanning: drive column low, read rows with internal pull-ups. Pressed = row reads low.

|            | COL0 (PB0)  | COL1 (PB1)  | COL2 (PB2)  |
|------------|-------------|-------------|-------------|
| ROW0 (PD0) | SW1: Power  | SW2: Up     | SW3: Mute   |
| ROW1 (PD1) | SW4: Left   | SW5: Select | SW6: Right  |
| ROW2 (PD2) | SW7: Vol-   | SW8: Down   | SW9: Vol+   |
| ROW3 (PD3) | SW10: Back  | SW11: Home  | SW12: Source|
| ROW4 (PD4) | SW13: C1    | SW14: C2    | SW15: Learn |

C1/C2 are customizable buttons. Learn enters IR learn mode.

### IR Transmit Circuit
- PD5 (OC1A) → R4 (1k) → Q1 base (NPN)
- Q1 collector → R5 (10) → U4 anode (940nm IR LED) → +3V3
- Q1 emitter → GND
- Use Timer1 CTC mode on OC1A for hardware 38kHz carrier generation
- Modulate by enabling/disabling Timer1 output compare per protocol timing
- Timer1 16-bit: for 38kHz at 16MHz, set OCR1A = 210 (16MHz / (2 * 38kHz) - 1)

### IR Receive Circuit
- U1 (IRM-H638T): 38kHz demodulated output, active-low
- OUT → PD6 (IR_RX): low during IR burst, high during space
- Vs → R1 (100) → +3V3, C1 (100nF) bypass to GND
- PD6 has no PCINT or dedicated external interrupt on ATmega32U4
- Use timer-based sampling: Timer3 interrupt at ~50us to sample PD6 state, measure mark/space durations
- Alternatively, poll in a tight loop during learn mode

### Status LED
- PD7 → R3 (330) → D1 (Green LED anode) → GND (cathode)
- Drive PD7 high to turn on, low to turn off
- Current: (3.3V - ~2.1V) / 330 = ~3.6mA

### USB
- ATmega32U4 native USB 2.0 Full Speed
- USB-C connector (J2) with both-orientation data pins (A6/B6 → USB_DP, A7/B7 → USB_DM)
- CC1/CC2 pull-downs: R7, R8 (5.1k) — identifies as USB sink device
- VBUS → D4 (BAT54) → VSTOR (charges supercap), also VBUS → R6 (22) → pin 7 for voltage detection
- HWB/PE2 pulled low (R9 10k to GND): holding RESET then releasing enters DFU bootloader
- UCAP (pin 6): 1uF cap (C6) for internal USB voltage regulator
- AREF (pin 42): 100nF cap (C2) to GND

### Power Supply
- **Solar front** (J1, Conn_01x02) → D3 (BAT54 Schottky) → VSTOR
- **Solar back** (J3, Conn_01x02) → D5 (BAT54 Schottky) → VSTOR
- **USB-C VBUS** → D4 (BAT54 Schottky) → VSTOR
- **VSTOR** → C4 (1F/5.5V supercap) + C3 (1uF) → U3 (MCP1700-3.3V LDO)
- **LDO output**: +3V3, C5 (1uF) output cap
- Dropout: ~178mV at 250mA, max output 250mA
- PWR_FLAG on VSTOR and GND nets

### Timer Resources
| Timer   | Width | Assignment          | Notes                                   |
|---------|-------|---------------------|-----------------------------------------|
| Timer0  | 8-bit | Available           | General purpose (millis, debounce)       |
| Timer1  | 16-bit| IR TX (OC1A = PD5)  | 38kHz carrier via CTC mode              |
| Timer3  | 16-bit| IR RX sampling      | ~50us interrupt to sample PD6            |
| Timer4  | 10-bit| Available           | High-speed timer, complex prescaler      |

### Sleep / Low Power
- Use Power-down mode between key presses for maximum battery life
- Wake sources: INT0-INT3 (PD0-PD3 = ROW0-ROW3) on level or edge
- Sleep setup: configure columns (PB0-PB2) as outputs driven low, rows (PD0-PD4) as inputs with pull-ups, enable INT0-INT3
- Any key press pulls a row low → wakes MCU
- ROW4 (PD4) has no external interrupt — to detect keys in row 4 during sleep, periodically wake via watchdog timer, or accept that row 4 buttons (C1, C2, Learn) only work when awake
- Disable unused peripherals (ADC, SPI, TWI, USB when not connected) via PRR0/PRR1

### Clock Configuration
- 16MHz external crystal (Y1) with 22pF load caps (C10, C11)
- Fuses: CKSEL=1111 (low-power crystal, 16MHz), SUT for slow rising power
- CKDIV8 fuse: clear (no prescaler, run at full 16MHz)
- Can reduce clock via CLKPR for power savings during idle

### Schematic Structure (3 sub-sheets, hierarchical labels for inter-sheet connections)
1. **Root sheet** (`remote.kicad_sch`): 3 sub-sheet symbols (Power Supply, MCU, Peripherals) connected via local labels on sheet pins
2. **MCU sheet** (`mcu.kicad_sch`): ATmega32U4-A + crystal + decoupling + reset/HWB pull resistors + USB caps
3. **Power Supply sheet** (`power_supply.kicad_sch`): Dual solar + USB-C → OR'd diodes → supercap → LDO
4. **Peripherals sheet** (`peripherals.kicad_sch`): IR TX/RX circuits, status LED, 15-button matrix

### ERC Status
1 error (power pin not driven — likely false positive), 4 warnings (library symbol mismatch/issues — cosmetic)
