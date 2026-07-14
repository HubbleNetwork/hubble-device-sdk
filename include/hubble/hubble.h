/*
 * Copyright (c) 2025 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_HUBBLE_HUBBLE_H
#define INCLUDE_HUBBLE_HUBBLE_H

#include <stdint.h>

#ifdef CONFIG_HUBBLE_SAT_NETWORK
#include <hubble/sat.h>
#endif /* CONFIG_HUBBLE_SAT_NETWORK */

#ifdef CONFIG_HUBBLE_BLE_NETWORK
#include <hubble/ble.h>
#endif /* CONFIG_HUBBLE_BLE_NETWORK */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Hubble Device SDK APIs
 * @defgroup hubble_api Hubble Device SDK APIs
 * @{
 */

/**
 * @brief Initializes the Hubble Device SDK.
 *
 * Calling this function is essential before using any other SDK APIs.
 *
 * `unix_time` and `initial_counter` are independent parameters; how each is
 * used depends on the configured counter source:
 *
 * **Unix time mode (CONFIG_HUBBLE_COUNTER_SOURCE_UNIX_TIME):**
 *   - `unix_time` drives the EID counter and must be non-zero for
 *     hubble_counter_get() to succeed.
 *   - `initial_counter` is a no-op in this mode, reserved for future use /
 *     device uptime mode only.
 *
 * **Device uptime mode (CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME):**
 *   - `initial_counter` seeds the EID counter. A value of 0 starts the
 *     counter at epoch 0 (valid); a saved counter resumes from a known state
 *     after reboot.
 *   - `unix_time` is optional here: when non-zero it seeds the SDK clock so
 *     hubble_time_get(), satellite pass prediction, and BLE advertisement
 *     timestamps work.
 *
 * @code
 * // Unix time mode example
 * uint64_t unix_time_ms = 1705881600000; // Current Unix Epoch time
 * int ret = hubble_init(unix_time_ms, 0, master_key);
 *
 * // Device uptime mode example (start at 0, no time available)
 * int ret = hubble_init(0, 0, master_key);
 *
 * // Device uptime mode example (resume from saved counter, time known)
 * uint32_t saved_counter = load_from_flash();
 * int ret = hubble_init(unix_time_ms, saved_counter, master_key);
 * @endcode
 *
 * @param unix_time Unix time in milliseconds since epoch. Applied via
 *                  hubble_time_set() whenever non-zero, in both counter-source
 *                  modes. 0 means "time unknown" and is skipped without error.
 * @param initial_counter Initial EID counter value (0 = start at epoch 0).
 *                        Only meaningful in device uptime mode; ignored
 *                        (reserved for future use) in Unix time mode.
 * @param key An opaque pointer to the master key buffer. The SDK
 *            stores this pointer directly and does not copy the key
 *            data. The caller must ensure the buffer remains valid
 *            for the lifetime of SDK usage (do not use stack or
 *            temporary buffers). The key size must match
 *            CONFIG_HUBBLE_NETWORK_KEY_256 or CONFIG_HUBBLE_NETWORK_KEY_128.
 *            If NULL, must be set with hubble_key_set before getting advertisements.
 *
 * @return
 *          - 0 on success.
 *          - Non-zero on failure.
 */
int hubble_init(uint64_t unix_time, uint32_t initial_counter, const void *key);

/**
 * @brief Sets the current Unix time in the Hubble Device SDK.
 *
 * @param unix_time The Unix Epoch time in milliseconds since the Unix epoch
 *                  (January 1, 1970).
 *
 * @return
 *          - 0 on success.
 *          - Non-zero on failure.
 */
int hubble_time_set(uint64_t unix_time);

/**
 * @brief Gets the current time.
 *
 * Gets the time in milliseconds since the Unix epoch (January 1, 1970).
 *
 * @return
 *          - 0 if time is not set.
 *          - current time since the Unix epoch.
 */
uint64_t hubble_time_get(void);

/**
 * @brief Sets the encryption key for advertisement data creation.
 *
 * @param key An opaque pointer to the master key buffer. The SDK
 *            stores this pointer directly and does not copy the key
 *            data. The caller must ensure the buffer remains valid
 *            for the lifetime of SDK usage. The key size must match
 *            CONFIG_HUBBLE_NETWORK_KEY_256 or CONFIG_HUBBLE_NETWORK_KEY_128.
 *
 * @return
 *         - 0 on success.
 *         - Non-zero on failure.
 */
int hubble_key_set(const void *key);

/**
 * @brief Get the current time counter value.
 *
 * Returns the time counter based on the configured counter source:
 * - Device uptime: Uses uptime-derived counter with initial offset, wrapping
 *                  at HUBBLE_EID_POOL_SIZE (128) to produce values in [0, pool_size-1]
 * - Unix time: Uses Unix time divided by rotation period (no wrapping)
 *
 * @param counter Pointer to store the time counter value
 * @return 0 on success, negative error code on failure (Unix time mode only, if time not set)
 */
int hubble_counter_get(uint32_t *counter);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_HUBBLE_HUBBLE_H */
