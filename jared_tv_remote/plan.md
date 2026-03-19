## Project Summary: Solar-Powered Universal TV IR Remote

### Design Overview
Building a compact solar-powered universal TV IR remote with "learn" mode, inspired by Chromecast remote layout and Samsung's SolarCell Remote (confirmed feasible via web search).

### Architecture
- **MCU**: ATtiny84A-SS (SOIC-14, 14-pin, 12 I/O, 8MHz internal oscillator, low power)
- **Power**: Solar cell + USB-C (OR'd via dual Schottky diodes) → Supercap (1F/5.5V) → MCP1700-3.3V LDO
- **IR TX**: NPN transistor (2N2222) + 1kΩ base R + 10Ω current limit R + 940nm IR LED
- **IR RX**: TSOP321xx (38kHz IR receiver) + 100Ω filter R + 100nF bypass cap (for learn mode)
- **Status LED**: 330Ω R + Green LED
- **15 Buttons** in 4×4 matrix (1 spare slot): Power, Vol+, Vol-, Mute, Up, Down, Left, Right, Select, Source, Home, Back, App1, App2, Learn

### Pin Allocation
- PA0-PA3: Matrix columns (COL0-COL3)
- PA4-PA7: Matrix rows (ROW0-ROW3)  
- PB0: IR_TX (38kHz PWM)
- PB1: IR_RX (TSOP input)
- PB2: STATUS_LED
- PB3: RESET (10k pull-up to +3V3)

### Schematic Structure (3 sub-sheets, global labels for inter-sheet connections)
1. **Root sheet**: Clean overview with 3 sub-sheet symbols only (Power Supply, MCU, Peripherals)
2. **MCU sheet** (`mcu.kicad_sch`): ATtiny84A-SS + 100nF decoupling cap + 10k reset pull-up + +3V3/GND power symbols + global labels on all I/O
3. **Power Supply sheet** (`power_supply.kicad_sch`): 
   - Solar panel (Conn_01x02) → Schottky D1 (BAT54, 180° rotation) → VSTOR net
   - USB-C (USB_C_Receptacle_USB2.0_14P) → Schottky D2 (BAT54, 180°) → VSTOR net
   - USB-C: CC1/CC2 have 5.1k pull-downs to GND, D+/D-/B6/B7 have no-connect flags
   - VSTOR → Supercap (C_Polarized, 1F/5.5V) → MCP1700-3.3V LDO
   - LDO: 1µF input cap, 1µF output cap, PWR_FLAG on +3V3 and GND
4. **Peripherals sheet** (`peripherals.kicad_sch`):
   - IR TX: Q1 (NPN) with R3 (1k base), R4 (10Ω collector), D2 (IR LED) chain to +3V3, emitter to GND
   - IR RX: U1 (TSOP321xx, value "TSOP38238") with R5 (100Ω), R6 (0Ω series on OUT), C3 (100nF bypass)
   - Status LED: R1 (330Ω) → D1 (Green LED) → GND, connected to STATUS_LED label
   - 15 switches (SW1-SW15) each with ROW label on pin 1 and COL label on pin 2

### Current State & Issues (from last ERC run: 48 errors, 17 warnings)
1. **Hierarchical label mismatch (11 errors)**: The Peripherals sheet still has old hierarchical labels creating orphaned sheet pins. Need to verify and delete any remaining hierarchical labels inside Peripherals sheet since we switched to global labels.
2. **Duplicate reference (5 errors)**: Multiple sheets have components with same refs (U1 on MCU and Peripherals sheets, D1, D2, R1, etc.). Need to run `sch_annotate` to fix.
3. **Pin not connected (19 errors)**: Likely USB-C pins and some other unconnected pins. Some may be real issues.
4. **Label not connected (11 errors)**: The global labels on the Peripherals sheet may not be properly connected to the local labels on switches (global and local labels with same name create "Same local/global label" warnings).
5. **Same local/global label (11 warnings)**: Expected - we have both local labels (on switch pins) and global labels (for inter-sheet connection) with same names. This is technically correct but KiCad warns about it.
6. **Power pin not driven (2 errors)**: Common false positive, likely on +3V3 or GND.
7. **Unconnected wire endpoints (4 warnings)**: Need to check and fix dangling wires.

### Next Steps
1. **Fix Peripherals sheet**: Delete remaining hierarchical labels, verify global labels connect properly to local switch labels
2. **Run `sch_annotate`** to fix duplicate references across sheets
3. **Fix dangling wires** on any sheet
4. **Re-run ERC** and address remaining real errors
5. **Take screenshots** of each sheet for visual verification
6. **Clean up root sheet layout** - rearrange the 3 sub-sheet symbols neatly
