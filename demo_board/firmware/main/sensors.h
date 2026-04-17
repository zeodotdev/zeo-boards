#pragma once

#include "esp_err.h"

typedef struct {
    float ax, ay, az;  // m/s²
    float gx, gy, gz;  // deg/s
} imu_data_t;

typedef struct {
    float temperature;  // °C
    float humidity;     // %RH
    float pressure;     // hPa
} env_data_t;

/**
 * Initialize I2C bus and both sensors.
 * I2C0: SDA=GPIO8, SCL=GPIO9, 400kHz
 */
esp_err_t sensors_init(void);

/** Read accelerometer + gyroscope from LSM6DS3 (addr 0x6A). */
esp_err_t imu_read(imu_data_t *data);

/** Trigger a measurement and read from BME280 (addr 0x76). */
esp_err_t env_read(env_data_t *data);
