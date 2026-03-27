/**
 * @file    mqtt_task.h
 * @brief   MQTT communication task — connects, publishes, auto-reconnects.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void mqtt_task(void *pvParameters);

#ifdef __cplusplus
}
#endif
