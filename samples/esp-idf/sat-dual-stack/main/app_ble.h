/*
 * Copyright (c) 2026 HubbleNetwork
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file app_ble.h
 * @brief Bluetooth LE advertising interface.
 *
 * Provides functions to initialize the BLE stack and control advertising.
 */

#ifndef APP_BLE_H
#define APP_BLE_H

#include "esp_err.h"

/**
 * @brief Maximum number of satellites provisioned over GATT.
 */
#define HUBBLE_MAX_SAT 6

/**
 * @brief Initialize the Bluetooth LE subsystem.
 *
 * Registers GATT services, sets up the advertising data, prepares the
 * controller for use, and starts the time / orbital parameters sync over BLE.
 * Must be called once before any other BLE function.
 *
 * @return 0 on success, negative errno on failure.
 */
int ble_init(void);

/**
 * @brief Start Bluetooth LE advertising.
 *
 * Begins broadcasting advertisement packets. @ref ble_init must have been
 * called successfully before invoking this function.
 *
 * @return 0 on success, negative errno on failure.
 */
int ble_adv_start(void);

/**
 * @brief Stop Bluetooth LE advertising.
 *
 * Halts advertisement broadcasting. Safe to call even if advertising is
 * already stopped.
 *
 * @return 0 on success, negative errno on failure.
 */
int ble_adv_stop(void);

#endif /* APP_BLE_H */
