/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Sync time with Hubble Connect App (BLE).
 *
 * @param epoch_ms Pointer to store the synced epoch time in milliseconds.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ble_sync_time(uint64_t *epoch_ms);

#endif /* TIME_SYNC_H */
