#include "sensors.h"

#include <string.h>
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sensors";

/* ── I2C addresses ── */
#define BME280_ADDR   0x76
#define LSM6DS3_ADDR  0x6A

/* ── I2C pins (from schematic: IO8=SDA, IO9=SCL) ── */
#define I2C_SDA_PIN   8
#define I2C_SCL_PIN   9

/* ── LSM6DS3 registers ── */
#define LSM6DS3_WHO_AM_I     0x0F
#define LSM6DS3_CTRL1_XL     0x10
#define LSM6DS3_CTRL2_G      0x11
#define LSM6DS3_OUTX_L_G     0x22
#define LSM6DS3_OUTX_L_XL    0x28

/* ── BME280 registers ── */
#define BME280_CHIP_ID_REG   0xD0
#define BME280_CTRL_HUM      0xF2
#define BME280_CTRL_MEAS     0xF4
#define BME280_CONFIG         0xF5
#define BME280_PRESS_MSB     0xF7
#define BME280_CALIB00       0x88
#define BME280_CALIB26       0xE1

/* ── Bus and device handles ── */
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t bme280_dev;
static i2c_master_dev_handle_t lsm6ds3_dev;

/* ── BME280 calibration data ── */
static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t  dig_H1, dig_H3;
static int16_t  dig_H2, dig_H4, dig_H5;
static int8_t   dig_H6;

/* ── I2C helpers ── */
static esp_err_t i2c_read_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, buf, len, 100);
}

static esp_err_t i2c_write_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(dev, buf, 2, 100);
}

/* ── BME280 calibration ── */
static esp_err_t bme280_read_calibration(void)
{
    uint8_t calib0[26];
    esp_err_t ret = i2c_read_reg(bme280_dev, BME280_CALIB00, calib0, 26);
    if (ret != ESP_OK) return ret;

    dig_T1 = (uint16_t)(calib0[1] << 8 | calib0[0]);
    dig_T2 = (int16_t)(calib0[3] << 8 | calib0[2]);
    dig_T3 = (int16_t)(calib0[5] << 8 | calib0[4]);

    dig_P1 = (uint16_t)(calib0[7] << 8 | calib0[6]);
    dig_P2 = (int16_t)(calib0[9] << 8 | calib0[8]);
    dig_P3 = (int16_t)(calib0[11] << 8 | calib0[10]);
    dig_P4 = (int16_t)(calib0[13] << 8 | calib0[12]);
    dig_P5 = (int16_t)(calib0[15] << 8 | calib0[14]);
    dig_P6 = (int16_t)(calib0[17] << 8 | calib0[16]);
    dig_P7 = (int16_t)(calib0[19] << 8 | calib0[18]);
    dig_P8 = (int16_t)(calib0[21] << 8 | calib0[20]);
    dig_P9 = (int16_t)(calib0[23] << 8 | calib0[22]);

    dig_H1 = calib0[25];

    uint8_t calib1[7];
    ret = i2c_read_reg(bme280_dev, BME280_CALIB26, calib1, 7);
    if (ret != ESP_OK) return ret;

    dig_H2 = (int16_t)(calib1[1] << 8 | calib1[0]);
    dig_H3 = calib1[2];
    dig_H4 = (int16_t)((calib1[3] << 4) | (calib1[4] & 0x0F));
    dig_H5 = (int16_t)((calib1[5] << 4) | (calib1[4] >> 4));
    dig_H6 = (int8_t)calib1[6];

    return ESP_OK;
}

/* ── BME280 compensation (from datasheet) ── */
static int32_t t_fine;

static float bme280_compensate_temp(int32_t adc_T)
{
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) *
                      ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) *
                    ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    return (float)((t_fine * 5 + 128) >> 8) / 100.0f;
}

static float bme280_compensate_press(int32_t adc_P)
{
    int64_t var1 = ((int64_t)t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
    if (var1 == 0) return 0;
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (float)p / 25600.0f;  /* hPa */
}

static float bme280_compensate_hum(int32_t adc_H)
{
    int32_t v_x1_u32r = t_fine - ((int32_t)76800);
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) -
                    (((int32_t)dig_H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)dig_H6)) >> 10) *
                      (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) +
                       ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) *
                   ((int32_t)dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                                ((int32_t)dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    return (float)(v_x1_u32r >> 12) / 1024.0f;
}

/* ── Init ── */
esp_err_t sensors_init(void)
{
    /* Configure I2C bus */
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,  /* external 4.7k on board */
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    /* Add BME280 device */
    i2c_device_config_t bme_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BME280_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &bme_cfg, &bme280_dev));

    /* Add LSM6DS3 device */
    i2c_device_config_t lsm_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LSM6DS3_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &lsm_cfg, &lsm6ds3_dev));

    /* Verify BME280 */
    uint8_t chip_id;
    ESP_ERROR_CHECK(i2c_read_reg(bme280_dev, BME280_CHIP_ID_REG, &chip_id, 1));
    ESP_LOGI(TAG, "BME280 chip ID: 0x%02X (expected 0x60)", chip_id);

    /* Read BME280 calibration */
    ESP_ERROR_CHECK(bme280_read_calibration());

    /* Configure BME280: oversampling x1 for T/P/H, forced mode triggered per read */
    ESP_ERROR_CHECK(i2c_write_reg(bme280_dev, BME280_CTRL_HUM, 0x01));   /* hum x1 */
    ESP_ERROR_CHECK(i2c_write_reg(bme280_dev, BME280_CONFIG, 0x00));      /* no filter, no standby */
    ESP_ERROR_CHECK(i2c_write_reg(bme280_dev, BME280_CTRL_MEAS, 0x25));   /* temp x1, press x1, forced */

    /* Verify LSM6DS3 */
    uint8_t who_am_i;
    ESP_ERROR_CHECK(i2c_read_reg(lsm6ds3_dev, LSM6DS3_WHO_AM_I, &who_am_i, 1));
    ESP_LOGI(TAG, "LSM6DS3 WHO_AM_I: 0x%02X (expected 0x69 or 0x6A)", who_am_i);

    /* Configure LSM6DS3: 104Hz ODR, ±4g accel, ±500dps gyro */
    ESP_ERROR_CHECK(i2c_write_reg(lsm6ds3_dev, LSM6DS3_CTRL1_XL, 0x48)); /* 104Hz, ±4g */
    ESP_ERROR_CHECK(i2c_write_reg(lsm6ds3_dev, LSM6DS3_CTRL2_G, 0x44));  /* 104Hz, ±500dps */

    ESP_LOGI(TAG, "Sensors initialized");
    return ESP_OK;
}

/* ── IMU read ── */
esp_err_t imu_read(imu_data_t *data)
{
    uint8_t buf[12];

    /* Read gyro (6 bytes from 0x22) */
    esp_err_t ret = i2c_read_reg(lsm6ds3_dev, LSM6DS3_OUTX_L_G, buf, 6);
    if (ret != ESP_OK) return ret;

    /* Read accel (6 bytes from 0x28) */
    ret = i2c_read_reg(lsm6ds3_dev, LSM6DS3_OUTX_L_XL, buf + 6, 6);
    if (ret != ESP_OK) return ret;

    /* Gyro: sensitivity for ±500dps = 17.50 mdps/LSB */
    int16_t gx_raw = (int16_t)(buf[1] << 8 | buf[0]);
    int16_t gy_raw = (int16_t)(buf[3] << 8 | buf[2]);
    int16_t gz_raw = (int16_t)(buf[5] << 8 | buf[4]);
    data->gx = gx_raw * 17.50f / 1000.0f;
    data->gy = gy_raw * 17.50f / 1000.0f;
    data->gz = gz_raw * 17.50f / 1000.0f;

    /* Accel: sensitivity for ±4g = 0.122 mg/LSB → m/s² */
    int16_t ax_raw = (int16_t)(buf[7] << 8 | buf[6]);
    int16_t ay_raw = (int16_t)(buf[9] << 8 | buf[8]);
    int16_t az_raw = (int16_t)(buf[11] << 8 | buf[10]);
    data->ax = ax_raw * 0.122f / 1000.0f * 9.80665f;
    data->ay = ay_raw * 0.122f / 1000.0f * 9.80665f;
    data->az = az_raw * 0.122f / 1000.0f * 9.80665f;

    return ESP_OK;
}

/* ── BME280 read ── */
esp_err_t env_read(env_data_t *data)
{
    /* Trigger forced measurement */
    esp_err_t ret = i2c_write_reg(bme280_dev, BME280_CTRL_MEAS, 0x25);
    if (ret != ESP_OK) return ret;

    /* Wait for measurement to complete (~10ms max) */
    vTaskDelay(pdMS_TO_TICKS(12));

    /* Read 8 bytes: press[3] temp[3] hum[2] starting at 0xF7 */
    uint8_t buf[8];
    ret = i2c_read_reg(bme280_dev, BME280_PRESS_MSB, buf, 8);
    if (ret != ESP_OK) return ret;

    int32_t adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | (buf[2] >> 4);
    int32_t adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | (buf[5] >> 4);
    int32_t adc_H = ((int32_t)buf[6] << 8) | buf[7];

    /* Compensate (must do temp first to set t_fine) */
    data->temperature = bme280_compensate_temp(adc_T);
    data->pressure = bme280_compensate_press(adc_P);
    data->humidity = bme280_compensate_hum(adc_H);

    return ESP_OK;
}
