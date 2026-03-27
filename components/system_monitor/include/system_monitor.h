/**
 * @file    system_monitor.h
 * @brief   System health monitoring task.
 *
 * Periodically checks:
 *   - Free heap
 *   - Stack high-water marks for each task
 *   - WiFi / MQTT connectivity state
 *   - Task watchdog
 *
 * Publishes a health JSON payload to the MQTT broker
 * and logs anomalies via the logger.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void system_monitor_task(void *pvParameters);

#ifdef __cplusplus
}
#endif
