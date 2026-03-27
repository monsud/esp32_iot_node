/**
 * @file    types.h
 * @brief   Shared data structures exchanged between tasks via queues.
 *
 * No dynamic memory allocation.  All structures have fixed size so they
 * can be copied by value into/out of FreeRTOS queues.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─────────────────────────────────────────────────────────────────────────────
 * Sensor payload
 * ─────────────────────────────────────────────────────────────────────────── */
typedef struct {
    float    temperature_c;   /**< °C  */
    float    humidity_pct;    /**< %RH */
    float    pressure_hpa;    /**< hPa */
    uint32_t timestamp_s;     /**< Unix epoch (seconds) */
    bool     valid;           /**< false → data must be discarded */
} sensor_data_t;

/* ─────────────────────────────────────────────────────────────────────────────
 * MQTT message (ready for publish)
 * ─────────────────────────────────────────────────────────────────────────── */
#define MQTT_TOPIC_MAX_LEN   64
#define MQTT_PAYLOAD_MAX_LEN 256

typedef struct {
    char     topic[MQTT_TOPIC_MAX_LEN];
    char     payload[MQTT_PAYLOAD_MAX_LEN];
    uint8_t  qos;
    bool     retain;
} mqtt_message_t;

/* ─────────────────────────────────────────────────────────────────────────────
 * Log entry
 * ─────────────────────────────────────────────────────────────────────────── */
typedef enum {
    LOG_LEVEL_DEBUG   = 0,
    LOG_LEVEL_INFO    = 1,
    LOG_LEVEL_WARNING = 2,
    LOG_LEVEL_ERROR   = 3,
} log_level_t;

#define LOG_SOURCE_MAX_LEN  24
#define LOG_MSG_MAX_LEN    128

typedef struct {
    log_level_t level;
    char        source[LOG_SOURCE_MAX_LEN];
    char        message[LOG_MSG_MAX_LEN];
    uint32_t    timestamp_s;
} log_entry_t;

/* ─────────────────────────────────────────────────────────────────────────────
 * System event (used by monitor task)
 * ─────────────────────────────────────────────────────────────────────────── */
typedef enum {
    SYS_EVENT_WIFI_CONNECTED,
    SYS_EVENT_WIFI_DISCONNECTED,
    SYS_EVENT_MQTT_CONNECTED,
    SYS_EVENT_MQTT_DISCONNECTED,
    SYS_EVENT_SENSOR_ERROR,
    SYS_EVENT_TASK_WATCHDOG,
} sys_event_t;

#ifdef __cplusplus
}
#endif
