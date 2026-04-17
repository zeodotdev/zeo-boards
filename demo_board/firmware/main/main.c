#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wifi.h"
#include "sensors.h"
#include "udp_sender.h"

static const char *TAG = "main";

static void imu_task(void *arg)
{
    imu_data_t data;
    const TickType_t period = pdMS_TO_TICKS(10);  /* 100 Hz */
    TickType_t last_wake = xTaskGetTickCount();
    int count = 0;

    while (1) {
        if (imu_read(&data) == ESP_OK) {
            int64_t ts = esp_timer_get_time() / 1000;  /* ms since boot */
            udp_send_imu(&data, ts);
            if (++count >= 100) {  /* log once per second */
                ESP_LOGI(TAG, "IMU: ax=%.2f ay=%.2f az=%.2f  gx=%.1f gy=%.1f gz=%.1f",
                         data.ax, data.ay, data.az, data.gx, data.gy, data.gz);
                count = 0;
            }
        }
        vTaskDelayUntil(&last_wake, period);
    }
}

static void env_task(void *arg)
{
    env_data_t data;

    while (1) {
        if (env_read(&data) == ESP_OK) {
            int64_t ts = esp_timer_get_time() / 1000;
            udp_send_env(&data, ts);
            ESP_LOGI(TAG, "ENV: %.1f°C  %.1f%%RH  %.1f hPa",
                     data.temperature, data.humidity, data.pressure);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));  /* 1 Hz */
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ZeoDevBoard starting...");

    /* Connect to WiFi (blocks until connected) */
    ESP_ERROR_CHECK(wifi_init_sta());

    /* Initialize sensors */
    ESP_ERROR_CHECK(sensors_init());

    /* Initialize UDP broadcaster */
    ESP_ERROR_CHECK(udp_sender_init());

    /* Launch sensor tasks */
    xTaskCreatePinnedToCore(imu_task, "imu_task", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(env_task, "env_task", 4096, NULL, 4, NULL, 0);

    ESP_LOGI(TAG, "Sensor streaming started");
}
