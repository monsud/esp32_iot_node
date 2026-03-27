/**
 * @file    wifi_manager.h
 * @brief   WiFi connection management with auto-reconnect.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialise WiFi in station mode and block until connected.
 *         Sets EVT_WIFI_CONNECTED in g_system_events on success.
 */
void wifi_manager_init(void);

/**
 * @brief  Returns true if WiFi is currently connected.
 */
bool wifi_manager_is_connected(void);

#ifdef __cplusplus
}
#endif
