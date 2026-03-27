/**
 * @file    config.h
 * @brief   Global system configuration — single source of truth
 *
 * All tunable parameters, priorities, timeouts and queue depths are
 * centralised here so that nothing is hard-coded in source files.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * WiFi
 * ─────────────────────────────────────────────────────────────────────────── */
#define WIFI_SSID              CONFIG_ESP_WIFI_SSID
#define WIFI_PASSWORD          CONFIG_ESP_WIFI_PASSWORD
#define WIFI_MAX_RETRY         5
#define WIFI_RECONNECT_DELAY_MS 5000

/* ─────────────────────────────────────────────────────────────────────────────
 * MQTT
 * ─────────────────────────────────────────────────────────────────────────── */
#define MQTT_BROKER_URL        CONFIG_BROKER_URL
#define MQTT_TOPIC_PREFIX      CONFIG_MQTT_TOPIC_PREFIX
#define MQTT_QOS               1
#define MQTT_RECONNECT_DELAY_MS 3000
#define MQTT_PUBLISH_RETRY     3
#define MQTT_PUBLISH_TIMEOUT_MS 5000

/* ─────────────────────────────────────────────────────────────────────────────
 * RTOS Task Priorities  (higher number = higher priority)
 * ─────────────────────────────────────────────────────────────────────────── */
#define PRIORITY_MQTT          (configMAX_PRIORITIES - 2)  /* 23 */
#define PRIORITY_SENSOR        (configMAX_PRIORITIES - 3)  /* 22 */
#define PRIORITY_MONITOR       (configMAX_PRIORITIES - 4)  /* 21 */
#define PRIORITY_PROCESSING    (configMAX_PRIORITIES - 5)  /* 20 */
#define PRIORITY_LOGGER        (tskIDLE_PRIORITY + 2)      /*  2 */

/* ─────────────────────────────────────────────────────────────────────────────
 * RTOS Task Stack Sizes (words, not bytes)
 * ─────────────────────────────────────────────────────────────────────────── */
#define STACK_SENSOR           (4096 / sizeof(StackType_t))
#define STACK_PROCESSING       (4096 / sizeof(StackType_t))
#define STACK_MQTT             (8192 / sizeof(StackType_t))
#define STACK_LOGGER           (4096 / sizeof(StackType_t))
#define STACK_MONITOR          (4096 / sizeof(StackType_t))

/* ─────────────────────────────────────────────────────────────────────────────
 * Inter-Task Queue Depths
 * ─────────────────────────────────────────────────────────────────────────── */
#define QUEUE_SENSOR_DEPTH     10
#define QUEUE_MQTT_DEPTH       20
#define QUEUE_LOG_DEPTH        30

/* ─────────────────────────────────────────────────────────────────────────────
 * Sensor
 * ─────────────────────────────────────────────────────────────────────────── */
#define SENSOR_POLL_INTERVAL_MS  2000
#define SENSOR_TEMP_MIN_C       -40.0f
#define SENSOR_TEMP_MAX_C        85.0f
#define SENSOR_HUM_MIN_PCT        0.0f
#define SENSOR_HUM_MAX_PCT      100.0f

/* ─────────────────────────────────────────────────────────────────────────────
 * Logger / SPIFFS
 * ─────────────────────────────────────────────────────────────────────────── */
#define LOG_FILE_PATH          "/spiffs/system.log"
#define LOG_FILE_MAX_SIZE_BYTES (256 * 1024)   /* 256 KB */
#define LOG_FLUSH_INTERVAL_MS  10000

/* ─────────────────────────────────────────────────────────────────────────────
 * System Monitor / Watchdog
 * ─────────────────────────────────────────────────────────────────────────── */
#define MONITOR_CHECK_INTERVAL_MS  5000
#define WATCHDOG_TIMEOUT_S         30

#ifdef __cplusplus
}
#endif
