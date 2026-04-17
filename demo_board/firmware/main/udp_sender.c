#include "udp_sender.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "lwip/sockets.h"

static const char *TAG = "udp";
static int sock = -1;
static struct sockaddr_in dest_addr;

esp_err_t udp_sender_init(void)
{
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        return ESP_FAIL;
    }

    /* Enable broadcast */
    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(CONFIG_UDP_BROADCAST_PORT);

    ESP_LOGI(TAG, "UDP broadcast socket ready on port %d", CONFIG_UDP_BROADCAST_PORT);
    return ESP_OK;
}

esp_err_t udp_send_imu(const imu_data_t *data, int64_t timestamp_ms)
{
    char buf[256];
    int len = snprintf(buf, sizeof(buf),
        "{\"type\":\"imu\",\"ts\":%lld,"
        "\"ax\":%.4f,\"ay\":%.4f,\"az\":%.4f,"
        "\"gx\":%.3f,\"gy\":%.3f,\"gz\":%.3f}",
        (long long)timestamp_ms,
        data->ax, data->ay, data->az,
        data->gx, data->gy, data->gz);

    int err = sendto(sock, buf, len, 0,
                     (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "sendto failed: errno %d", errno);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t udp_send_env(const env_data_t *data, int64_t timestamp_ms)
{
    char buf[192];
    int len = snprintf(buf, sizeof(buf),
        "{\"type\":\"env\",\"ts\":%lld,"
        "\"temp\":%.2f,\"hum\":%.2f,\"press\":%.2f}",
        (long long)timestamp_ms,
        data->temperature, data->humidity, data->pressure);

    int err = sendto(sock, buf, len, 0,
                     (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        ESP_LOGE(TAG, "sendto failed: errno %d", errno);
        return ESP_FAIL;
    }
    return ESP_OK;
}
