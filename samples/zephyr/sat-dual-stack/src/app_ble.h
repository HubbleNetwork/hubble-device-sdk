/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_BLE_H
#define APP_BLE_H

/**
 * @brief Initialize the Bluetooth LE subsystem.
 *
 * Enables the Bluetooth stack, and starts a connectable
 * advertising session that exposes the Hubble provisioning GATT service.
 * Must be called once before any other BLE function.
 *
 * Provisioning data (UTC time and satellite orbital parameters) received over
 * GATT is stored in the @c unix_time_ms / @c orb_params globals owned by the
 * application. Once a peer has written the time and disconnects, @c sync_sem
 * is given so the application can proceed.
 *
 * @return 0 on success, negative errno on failure.
 */
int ble_init(void);

/**
 * @brief Start the Hubble terrestrial beacon advertising.
 *
 * Begins broadcasting non-connectable Hubble beacon packets. The payload is
 * refreshed periodically (hourly). @ref ble_init must have been called
 * successfully before invoking this function.
 *
 * @return 0 on success, negative errno on failure.
 */
int ble_adv_start(void);

/**
 * @brief Stop the Hubble terrestrial beacon advertising.
 *
 * Stop advertisements and the payload refresh timer. Safe to call even
 * if advertising is already stopped.
 *
 * @return 0 on success, negative errno on failure.
 */
int ble_adv_stop(void);

#endif /* APP_BLE_H */
