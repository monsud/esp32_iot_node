/**
 * @file    wifi_manager.c
 * @brief   WiFi STA mode with automatic reconnect logic.
 */

#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"

#include "config.h"
#include "types.h"
#include "rtos_handles.h"
#include "wifi_manager.h"
#include "logger_task.h"

static const char *TAG = "WIFI";

static int  s_retry_count = 0;
static bool s_connected   = false;

/* ─────────────────────────────────────────────────────────────────────────────
 * Event handler
 * ─────────────────────────────────────────────────────────────────────────── */
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT) {
        switch (id) {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;

        case WIFI_EVENT_STA_DISCONNECTED:
            s_connected = false;
            xEventGroupClearBits(g_system_events, EVT_WIFI_CONNECTED);
            logger_post(LOG_LEVEL_WARNING, TAG, "WiFi disconnected.");

            if (s_retry_count < WIFI_MAX_RETRY) {
                s_retry_count++;
                ESP_LOGW(TAG, "Reconnecting (%d/%d)...",
                         s_retry_count, WIFI_MAX_RETRY);
                vTaskDelay(pdMS_TO_TICKS(WIFI_RECONNECT_DELAY_MS));
                esp_wifi_connect();
            } else {
                ESP_LOGE(TAG, "Max retries reached — giving up.");
                logger_post(LOG_LEVEL_ERROR, TAG,
                            "WiFi max retries exceeded.");
            }
            break;

        default:
            break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Got IP: %s", ip_str);

        /* ── Fix DNS per reti hotspot/ICS che non propagano il DNS ── */
        ip_addr_t dns_primary, dns_secondary;
        IP_ADDR4(&dns_primary,   8, 8, 8, 8);   /* Google Primary  */
        IP_ADDR4(&dns_secondary, 8, 8, 4, 4);   /* Google Secondary */
        dns_setserver(0, &dns_primary);
        dns_setserver(1, &dns_secondary);
        ESP_LOGI(TAG, "DNS set to 8.8.8.8 / 8.8.4.4");
        /* ─────────────────────────────────────────────────────────── */

        s_retry_count = 0;
        s_connected   = true;
        xEventGroupSetBits(g_system_events, EVT_WIFI_CONNECTED);
        logger_post(LOG_LEVEL_INFO, TAG, "WiFi connected.");
    }
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Public API
 * ─────────────────────────────────────────────────────────────────────────── */
void wifi_manager_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid     = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    xEventGroupWaitBits(g_system_events, EVT_WIFI_CONNECTED,
                        pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi ready.");
}

bool wifi_manager_is_connected(void)
{
    return s_connected;
}