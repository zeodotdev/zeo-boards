# Zeo Compute Module — Design Plan

## 1. Power Rail Definitions (Net Names)

All sheets must use these exact net names. Power symbols (GND, VCC_3V3, etc.) are global. 
Custom rails use **global labels** for cross-sheet connectivity.

### Standard Power Symbols (use `power:` library)
| Net Name | Symbol | Voltage | Notes |
|----------|--------|---------|-------|
| GND | `power:GND` | 0V | Global ground everywhere |
| +5V | `power:+5V` | 5.0V | Input power from B2B connector |
| +3V3 | `power:+3V3` | 3.3V | PMIC BUCK8 → Ethernet, WiFi, general I/O |
| +1V8 | `power:+1V8` | 1.8V | PMIC BUCK10 → General 1.8V, PMU, DDR_VDD1 |

### Custom Power Rails (use global labels)
| Net Name | Voltage | PMIC Source | Consumers |
|----------|---------|-------------|-----------|
| VDD_GPU | 0.7–1.0V | BUCK1 | SoC GPU core (Unit 10) |
| VDD_NPU | 0.7–1.0V | BUCK2 | SoC NPU core (Unit 10) |
| VDD_CPU_LIT | 0.5–1.4V | BUCK3 | SoC A55 cores (Unit 10) |
| VDD_VDENC | 0.7–1.0V | BUCK4 | SoC video encoder (Unit 10) |
| VDD_DDR | 0.75V | BUCK5 | SoC DDR PHY core (Unit 2, 3) |
| DDR_VDDQ | 0.6V | BUCK6 | DDR signaling — SoC PHY + Memory VDDQ |
| DDR_VDD2 | 1.1V | BUCK7 | Memory VDD2 core power |
| DDR_VDD1 | 1.8V | From +1V8 | Memory VDD1 core power |
| VDD_LOGIC | 0.7–1.0V | BUCK9 | SoC logic core (Unit 10) |
| VDD_CPU_BIG0 | 0.5–1.4V | External Buck | SoC A76 cluster 0 (Unit 10) |
| VDD_CPU_BIG1 | 0.5–1.4V | External Buck | SoC A76 cluster 1 (Unit 10) |
| AVDD_0V75 | 0.75V | NLDO1 | Analog 0.75V (HDMI, USB) |
| VCCA_1V8 | 1.8V | PLDO2 | Analog 1.8V (HDMI, MIPI) |
| VCCIO_SD | 1.8/3.3V | PLDO4 | SD card I/O level |

### Usage Rules
- **GND**: Always use `power:GND` symbol. Never use a "GND" local label.
- **+3V3, +1V8, +5V**: Use KiCad power symbols — they're global automatically.
- **Custom rails** (VDD_DDR, DDR_VDDQ, etc.): Use **global labels** so they connect across all sheets.
- **Local labels**: ONLY for intra-sheet signals (e.g., DDR data lines between SoC and memory on same sheet).

---

## 2. Cross-Sheet Signal Analysis

### Signals that need Hierarchical Labels

| Signal Group | From Sheet | To Sheet | Count | Notes |
|-------------|-----------|----------|-------|-------|
| **PMIC SPI** (MOSI, MISO, CLK, CS) | Clock and Boot | Power Supply | 4 | SoC Unit 8 ↔ RK806-1 |
| **PMIC Control** (SLEEP0/1, INT, RESETB, PWRON) | Clock and Boot | Power Supply | ~5 | PMU signals |
| **SDIO** (CLK, CMD, D0–D3) | Storage | WiFi and BT | 6 | SoC Unit 4 → AP6256 |
| **USB2 DP/DM** | USB and PCIe | Board Connectors | 4–8 | USB2.0 pairs to B2B |
| **USB3 SSRX/TX** | USB and PCIe | Board Connectors | 4–8 | SuperSpeed pairs to B2B |
| **PCIe lanes** | USB and PCIe | Board Connectors | 4–8 | PCIe TX/RX pairs to B2B |
| **HDMI TX lanes** | HDMI and Display | Board Connectors | 8–10 | TMDS + CEC + HPD + DDC |
| **MIPI CSI lanes** | Camera and MIPI | Board Connectors | 10–16 | Data + CLK pairs to B2B |
| **MIPI DSI lanes** | Camera and MIPI or HDMI | Board Connectors | 8–12 | Display output to B2B |
| **Ethernet RGMII** | Ethernet and GPIO | Board Connectors | 12–14 | If PHY is on carrier board |
| **Ethernet MDI** | Ethernet and GPIO | Board Connectors | 8 | If PHY is on module → RJ45 on carrier |
| **GPIO** | Ethernet and GPIO | Board Connectors | 10–30 | Exposed GPIOs to B2B |
| **I2C buses** | Various | Board Connectors | 4–8 | I2C0–I2C5 pairs |
| **UART** | Clock and Boot | Board Connectors | 2–4 | Debug UART |
| **SPI** | Various | Board Connectors | 4–6 | Exposed SPI |
| **SARADC** | Clock and Boot | Board Connectors | 1–2 | ADC channels |

### Signals that stay LOCAL (no hierarchical labels)
| Signal Group | Sheet | Notes |
|-------------|-------|-------|
| DDR data/addr/ctrl/clk | DDR CH0 / DDR CH1 | SoC ↔ Memory on same sheet |
| eMMC data/cmd/clk | Storage | SoC Unit 4 ↔ eMMC on same sheet |
| SPI Flash (FSPI) | Clock and Boot | SoC Unit 8 ↔ W25Q32 on same sheet |
| Crystal oscillator | Clock and Boot | SoC Unit 8/9 ↔ crystal on same sheet |
| Ethernet RGMII | Ethernet and GPIO | SoC Unit 4 ↔ RTL8211F on same sheet |
| SoC power pins | Power Supply | SoC Units 10–14 → PMIC on same sheet |

---

## 3. Hierarchy Plan

### Power Distribution
Power rails are distributed via **power symbols** (GND, +3V3, +1V8) and **global labels** (VDD_DDR, DDR_VDDQ, etc.). No hierarchical labels needed for power — global labels and power symbols handle it.

### Signal Hierarchy
Each sub-sheet that exports signals uses **hierarchical labels** inside the sheet, which appear as **hierarchical pins** on the sheet symbol in root.

**Sheets needing hierarchical pins:**
1. **Board Connectors** — MOST pins are hierarchical (USB, HDMI, MIPI, GPIO, I2C, UART, PCIe, etc.)
2. **USB and PCIe** — USB2/3 + PCIe signals to Board Connectors
3. **HDMI and Display** — HDMI/eDP signals to Board Connectors
4. **Camera and MIPI** — MIPI CSI/DSI signals to Board Connectors
5. **Ethernet and GPIO** — GPIO + Ethernet MDI to Board Connectors
6. **Clock and Boot** — PMIC SPI + UART debug + boot config to Board Connectors + Power Supply
7. **Storage** — SDIO signals to WiFi and BT
8. **WiFi and Bluetooth** — SDIO from Storage + antenna to Board Connectors
9. **Power Supply** — PMIC control from Clock and Boot

**Sheets with NO hierarchical pins:**
- **DDR CH0** — fully self-contained
- **DDR CH1** — fully self-contained

---

## 4. Sheet Block Sizing (Root Sheet)

Recommended sizes based on hierarchical pin count:

| Sheet | Est. Hier. Pins | Recommended Size (W×H mm) |
|-------|----------------|--------------------------|
| Power Supply | ~10 | 40 × 30 |
| DDR CH0 | 0 | 30 × 15 |
| DDR CH1 | 0 | 30 × 15 |
| Storage | ~10 (SDIO out) | 35 × 25 |
| Ethernet and GPIO | ~40 (GPIO + ETH) | 50 × 60 |
| HDMI and Display | ~15 | 40 × 35 |
| Camera and MIPI | ~20 | 40 × 40 |
| USB and PCIe | ~25 | 45 × 45 |
| Clock and Boot | ~15 | 40 × 35 |
| WiFi and Bluetooth | ~10 | 35 × 25 |
| Board Connectors | ~150+ | 60 × 100 |

---

## 5. Decoupling Cap Strategy

| Rail | Cap Values | Per-IC Count | Notes |
|------|-----------|-------------|-------|
| DDR_VDDQ (0.6V) | 100nF + 10µF | 3×100nF + 1×10µF per channel | Close to memory BGA |
| DDR_VDD2 (1.1V) | 100nF + 10µF | 3×100nF + 1×10µF per channel | Close to memory BGA |
| DDR_VDD1 (1.8V) | 100nF + 10µF | 2×100nF + 1×10µF per channel | Close to memory BGA |
| VDD_DDR (0.75V) | 100nF | Per SoC DDR unit | SoC PHY power |
| +3V3 | 100nF + 10µF | Per IC | Ethernet PHY, WiFi, etc. |
| +1V8 | 100nF + 10µF | Per IC | General |
| VDD_LOGIC etc. | 100nF | Per SoC group | On Power Supply sheet |

---

## 6. ZQ / ODT / Termination Summary

| Component | Connection | Value | Per Datasheet |
|-----------|-----------|-------|---------------|
| Memory ZQ0 | ZQ0 → DDR_VDDQ via 240Ω ±1% | 240R | Samsung LPDDR4X spec |
| Memory ZQ1 | ZQ1 → DDR_VDDQ via 240Ω ±1% | 240R | Samsung LPDDR4X spec |
| Memory ODT_CA_a | Tied directly to GND | 0R/wire | LPDDR4X: "connect to VDD2 or VSS" |
| Memory ODT_CA_b | Tied directly to GND | 0R/wire | LPDDR4X: "connect to VDD2 or VSS" |
| SoC ZQ_A | ZQ → GND via 240Ω ±1% | 240R | RK3588 reference design |
| SoC ZQ_B | ZQ → GND via 240Ω ±1% | 240R | RK3588 reference design |
