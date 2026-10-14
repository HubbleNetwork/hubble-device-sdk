/*
 * Copyright (c) 2026 Hubble Network, Inc.
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

/**
 * @brief Maximum number of satellites provisioned over GATT.
 */
#define HUBBLE_MAX_SAT 6

/**
 * @brief Initialize the Bluetooth LE application.
 *
 * The provisioning GATT service is defined in the project's .btconf. The
 * connectable provisioning advertising starts once the Bluetooth stack has
 * booted, and @c sync_sem is given once the time has been received and the
 * peer disconnects.
 *
 * @return 0 on success, negative errno on failure.
 */
int ble_init(void);

/**
 * @brief Start the Hubble terrestrial beacon advertising.
 *
 * Generates the Hubble advertisement payload and begins broadcasting
 * non-connectable beacon packets. The payload is refreshed periodically
 * (hourly). @ref ble_init must have been called successfully before invoking
 * this function.
 *
 * @return 0 on success, negative errno on failure.
 */
int ble_adv_start(void);

/**
 * @brief Stop the Hubble terrestrial beacon advertising.
 *
 * Stops the beacon advertisements and the payload refresh timer. Safe to call
 * even if advertising is already stopped.
 *
 * @return 0 on success, negative errno on failure.
 */
int ble_adv_stop(void);

#endif /* APP_BLE_H */
