/*
 * Copyright (c) 2025 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_PORT_FREERTOS_CONFIG_H
#define INCLUDE_PORT_FREERTOS_CONFIG_H

/*
 * Enable the Hubble BLE and Satellite network modules.
 */
#define CONFIG_HUBBLE_BLE_NETWORK 1
#define CONFIG_HUBBLE_SAT_NETWORK 1

/*
 * Size of the encryption key in bytes. Valid options are
 * 16 for 128 bits keys or 32 for 256 bits keys.
 */
#define CONFIG_HUBBLE_KEY_SIZE    16

#ifdef CONFIG_HUBBLE_BLE_NETWORK

/*
 * EID Generation Mode
 *
 * UTC-based mode is used by default. To use counter-based mode instead,
 * define CONFIG_HUBBLE_EID_COUNTER_BASED (and CONFIG_HUBBLE_EID_POOL_SIZE).
 */
/* #define CONFIG_HUBBLE_EID_COUNTER_BASED  1 */

#if defined(CONFIG_HUBBLE_EID_UTC_BASED) &&                                    \
	defined(CONFIG_HUBBLE_EID_COUNTER_BASED)
#error "Cannot define both CONFIG_HUBBLE_EID_UTC_BASED and CONFIG_HUBBLE_EID_COUNTER_BASED"
#endif

/*
 * EID rotation period in seconds.
 * Currently only daily rotation (86400) is supported for backend compatibility.
 * Future versions may support 900-86400 (15 minutes to 24 hours).
 * Default: 86400 (24 hours / daily)
 */
#ifndef CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC
#define CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC 86400
#endif

/*
 * EID pool size (counter-based mode only).
 * Range: 16 to 2048 unique EID epochs.
 *
 * Examples with default rotation period (86400 sec / daily):
 *   16:   ~16 days of unique EIDs
 *   64:   ~64 days of unique EIDs
 *   256:  ~256 days of unique EIDs
 *   2048: ~5.6 years of unique EIDs
 *
 * Must be defined when CONFIG_HUBBLE_EID_COUNTER_BASED is enabled.
 */
/* #define CONFIG_HUBBLE_EID_POOL_SIZE 64 */

/*
 * DEPRECATED: Use CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC instead.
 * Kept for backward compatibility.
 */
#define CONFIG_HUBBLE_BLE_NETWORK_TIMER_COUNTER_DAILY

#endif /* CONFIG_HUBBLE_BLE_NETWORK */

#ifdef CONFIG_HUBBLE_SAT_NETWORK

/*
 * Use for fly by calculation. Enable this option
 * reduces code using polynomial approximation
 * for trigonometric functions
 */
/* #define CONFIG_HUBBLE_SAT_NETWORK_SMALL */

/*
 * Device time drift retry rate in parts per million (PPM).
 * Additional retries is added proportional to time since
 * last time the device had utc time synced.
 */
#define CONFIG_HUBBLE_SAT_NETWORK_DEVICE_TDR  500

/* Protocol version
 *
 * Select only one of the following options:
 * - CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1: first version of sat
 * protocol. Channel hopping during transmissions.
 * - CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_DEPRECATED: deprecated version
 * of sat protocol. No channel hopping during transmissions.
 */
#define CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1 1
#if defined(CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1) &&                          \
	defined(CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_DEPRECATED)
#error "Only one protocol can be selected"
#endif

#endif /* CONFIG_HUBBLE_SAT_NETWORK */

#endif /* INCLUDE_PORT_FREERTOS_CONFIG_H */
