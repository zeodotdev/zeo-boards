#pragma once

#include "esp_err.h"

/**
 * Initialize WiFi in STA mode and connect.
 * Blocks until an IP address is obtained or connection fails.
 * Returns ESP_OK on success.
 */
esp_err_t wifi_init_sta(void);
