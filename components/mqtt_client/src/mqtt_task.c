/**
 * @file    mqtt_task.c
 * @brief   MQTT communication task.
 *
 * Responsibilities:
 *  - Wait for WiFi connectivity (event group).
 *  - Connect to MQTT broker via esp-mqtt component.
 *  - Drain g_mqtt_queue and publish with retry logic.
 *  - Handle broker disconnections transparently.
 *  - Subscribe to task watchdog.
 */

#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_task_wdt.h"
#include "mqtt_client.h"

#include "config.h"
#include "types.h"
#include "rtos_handles.h"
#include "logger_task.h"
#include "mqtt_task.h"

static const char *TAG = "MQTT";

/* ─────────────────────────────────────────────────────────────────────────────
 * Module-level state  (task-private — no mutex needed)
 * ─────────────────────────────────────────────────────────────────────────── */
static esp_mqtt_client_handle_t s_client    = NULL;
static volatile bool             s_connected = false;

/* ─────────────────────────────────────────────────────────────────────────────
 * MQTT event handler (called from esp-mqtt internal task)
 * ─────────────────────────────────────────────────────────────────────────── */
static void mqtt_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)data;

    switch ((esp_mqtt_event_id_t)id) {

    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to broker.");
        s_connected = true;
        xEventGroupSetBits(g_system_events, EVT_MQTT_CONNECTED);
        logger_post(LOG_LEVEL_INFO, TAG, "MQTT broker connected.");
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Disconnected from broker.");
        s_connected = false;
        xEventGroupClearBits(g_system_events, EVT_MQTT_CONNECTED);
        logger_post(LOG_LEVEL_WARNING, TAG, "MQTT broker disconnected.");
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT error type: %d",
                 event->error_handle->error_type);
        logger_post(LOG_LEVEL_ERROR, TAG, "MQTT protocol error.");
        break;

    default:
        break;
    }
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Publish with retry
 * Returns true if published successfully.
 * ─────────────────────────────────────────────────────────────────────────── */
static bool publish_with_retry(const mqtt_message_t *msg)
{
    for (int attempt = 1; attempt <= MQTT_PUBLISH_RETRY; attempt++) {
        if (!s_connected) {
            ESP_LOGW(TAG, "Not connected — waiting for broker...");
            /* Wait up to MQTT_PUBLISH_TIMEOUT_MS for reconnect */
            EventBits_t bits = xEventGroupWaitBits(
                g_system_events, EVT_MQTT_CONNECTED,
                pdFALSE, pdTRUE,
                pdMS_TO_TICKS(MQTT_PUBLISH_TIMEOUT_MS));

            if (!(bits & EVT_MQTT_CONNECTED)) {
                ESP_LOGE(TAG, "Broker still unavailable — attempt %d/%d",
                         attempt, MQTT_PUBLISH_RETRY);
                continue;
            }
        }

        int msg_id = esp_mqtt_client_publish(
            s_client,
            msg->topic,
            msg->payload,
            (int)strlen(msg->payload),
            msg->qos,
            msg->retain ? 1 : 0);

        if (msg_id >= 0) {
            ESP_LOGD(TAG, "Published (id=%d) → %s", msg_id, msg->topic);
            return true;
        }

        ESP_LOGW(TAG, "Publish failed (attempt %d/%d).",
                 attempt, MQTT_PUBLISH_RETRY);
        vTaskDelay(pdMS_TO_TICKS(500 * attempt));   /* back-off */
    }

    logger_post(LOG_LEVEL_ERROR, TAG, "Publish failed after max retries.");
    return false;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Task entry point
 * ─────────────────────────────────────────────────────────────────────────── */
void mqtt_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Task started — waiting for WiFi.");

    /* Wait until WiFi is up before even trying to connect to broker */
    xEventGroupWaitBits(g_system_events, EVT_WIFI_CONNECTED,
                        pdFALSE, pdTRUE, portMAX_DELAY);

    /* Configure and start the MQTT client */
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URL,
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(s_client));

    /* Register with watchdog */
    esp_task_wdt_add(NULL);

    mqtt_message_t msg;

    for (;;) {
        esp_task_wdt_reset();

        if (xQueueReceive(g_mqtt_queue, &msg,
                          pdMS_TO_TICKS(1000)) == pdTRUE) {
            publish_with_retry(&msg);
        }

        /* If WiFi dropped, esp-mqtt will auto-reconnect; just keep looping */
    }
}
