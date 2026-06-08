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

#include "ti/ble/stack_util/bcomdef.h"

/**
 * @brief Maximum number of satellites provisioned over GATT.
 */
#define HUBBLE_MAX_SAT 6

/**
 * @brief Initialize the Bluetooth LE subsystem.
 *
 * Registers GATT services, sets up the advertising data, and prepares the
 * controller for use. Must be called once before any other BLE function.
 *
 * @return SUCCESS or error code on failure.
 */
bStatus_t ble_init(void);

/**
 * @brief Start Bluetooth LE advertising.
 *
 * Begins broadcasting advertisement packets. @ref ble_init must have been
 * called successfully before invoking this function.
 *
 * @return SUCCESS or error code on failure.
 */
bStatus_t ble_adv_start(void);

/**
 * @brief Stop Bluetooth LE advertising.
 *
 * Halts advertisement broadcasting. Safe to call even if advertising is
 * already stopped.
 *
 * @return SUCCESS or error code on failure.
 */
bStatus_t ble_adv_stop(void);

/**
 * @brief Synchronize time, location, and orbital parameters data.
 *
 * Synchronizes the current time, location, and orbital parameters data from
 * the Hubble Network Backend over GATT.
 * This starts a connectable advertising session. @ref ble_adv_stop must be
 * called to stop the ongoing broadcasting.
 *
 * @return SUCCESS or error code on failure.
 */
bStatus_t ble_hubble_sync(void);

#endif /* APP_BLE_H */
