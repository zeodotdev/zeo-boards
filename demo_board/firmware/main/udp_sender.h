#pragma once

#include "esp_err.h"
#include "sensors.h"

/** Create the broadcast UDP socket. Call after WiFi is connected. */
esp_err_t udp_sender_init(void);

/** Broadcast an IMU sample as JSON. */
esp_err_t udp_send_imu(const imu_data_t *data, int64_t timestamp_ms);

/** Broadcast an environment sample as JSON. */
esp_err_t udp_send_env(const env_data_t *data, int64_t timestamp_ms);
