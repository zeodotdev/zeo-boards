![../assets/zeodemoboard.JPG]

# ZeoDevBoard

ESP32-S3 development board with onboard BME280 (temperature, humidity, pressure) and LSM6DS3 (6-axis IMU) sensors. This repo contains the designs, ESP-IDF firmware, and a Python plotter for visualization.

## Hardware

- **MCU:** ESP32-S3-WROOM-1 (Wi-Fi + BLE)
- **Sensors:** BME280 (I2C 0x76), LSM6DS3 (I2C 0x6A) on shared bus (GPIO8 SDA, GPIO9 SCL)
- **Power:** USB-C with MCP73871 LiPo charger + AP2112K 3.3V LDO — runs from USB or battery
- **Interface:** USB-C for flashing/debug, BOOT + RESET buttons

## Firmware

Streams sensor data over Wi-Fi as UDP broadcast JSON packets on port 12345.

- **IMU:** 100 Hz — accelerometer (m/s²) + gyroscope (deg/s)
- **BME280:** 1 Hz — temperature (°C), humidity (%RH), pressure (hPa)

### Prerequisites

- [ESP-IDF v5.4](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32s3/get-started/)

### Build and Flash

```bash
# Source ESP-IDF environment
source ~/esp/esp-idf/export.sh

# Enter firmware directory
cd firmware

# Set target and configure Wi-Fi credentials
idf.py set-target esp32s3
idf.py menuconfig  # Navigate to "ZeoDevBoard Configuration" → set SSID and password

# Build
idf.py build

# Flash (use esptool.py directly if idf.py has port permission issues)
esptool.py --chip esp32s3 -p /dev/ttyACM0 -b 460800 \
  --before default_reset --after hard_reset write_flash \
  --flash_mode dio --flash_size 2MB --flash_freq 80m \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/zeodevboard.bin

# Monitor serial output (optional)
idf.py -p /dev/ttyACM0 monitor
```

### Wireless Operation

Once flahsed, the board can run on battery power.

## Plotter

Displays 6 live panels: Accelerometer XYZ, Gyroscope XYZ, Temperature, Humidity, Pressure, and a status panel with packet counts.

### Run

```bash
cd plotter
python3 -m venv venv
./venv/bin/pip install -r requirements.txt
./venv/bin/python plotter.py
```

The plotter listens on UDP port 12345. Make sure your machine is on the same network as the board.

### UDP Packet Format

IMU (100 Hz):
```json
{"type":"imu","ts":1234,"ax":0.12,"ay":-0.05,"az":-9.81,"gx":1.2,"gy":-0.3,"gz":0.1}
```

Environment (1 Hz):
```json
{"type":"env","ts":1234,"temp":25.3,"hum":45.2,"press":1013.4}
```

`ts` is milliseconds since boot.

## Project Structure

```
demo_board/
├── firmware/
│   ├── CMakeLists.txt
│   ├── sdkconfig.defaults
│   └── main/
│       ├── CMakeLists.txt
│       ├── Kconfig.projbuild      # Wi-Fi SSID/password config
│       ├── main.c                 # App entry, FreeRTOS tasks
│       ├── wifi.c / wifi.h        # Wi-Fi STA connection
│       ├── sensors.c / sensors.h  # BME280 + LSM6DS3 I2C drivers
│       └── udp_sender.c / udp_sender.h  # UDP broadcast
├── plotter/
│   ├── plotter.py
│   └── requirements.txt
├── ZeoDevBoard.kicad_sch          # Root schematic
├── mcu.kicad_sch                  # MCU sheet
├── power_supply.kicad_sch         # Power supply sheet
├── sensors.kicad_sch              # Sensors sheet
├── ZeoDevBoard.kicad_pcb          # PCB layout
└── README.md
```
