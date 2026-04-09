/*
 * Copyright (c) 2025 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_PORT_FREERTOS_CONFIG_H
#define INCLUDE_PORT_FREERTOS_CONFIG_H

/*
 * Enable the Hubble BLE network module.
 */
#ifndef CONFIG_HUBBLE_BLE_NETWORK
#define CONFIG_HUBBLE_BLE_NETWORK 1
#endif

#ifndef CONFIG_HUBBLE_SAT_NETWORK
#define CONFIG_HUBBLE_SAT_NETWORK 0
#endif

/*
 * Key size selection. Define ONE of:
 *   CONFIG_HUBBLE_NETWORK_KEY_128 - 128-bit keys (default)
 *   CONFIG_HUBBLE_NETWORK_KEY_256 - 256-bit keys
 */
#if !defined(CONFIG_HUBBLE_NETWORK_KEY_128) &&                                 \
	!defined(CONFIG_HUBBLE_NETWORK_KEY_256)
#define CONFIG_HUBBLE_NETWORK_KEY_128
#endif

#if defined(CONFIG_HUBBLE_NETWORK_KEY_128) &&                                  \
	defined(CONFIG_HUBBLE_NETWORK_KEY_256)
#error "Cannot define both CONFIG_HUBBLE_NETWORK_KEY_128 and CONFIG_HUBBLE_NETWORK_KEY_256"
#endif

/*
 * Use the PSA Crypto API (Platform Security Architecture) for cryptographic
 * operations. When enabled, the SDK uses the PSA API (e.g., via Mbed TLS)
 * Enable this when the target platform provides a PSA-compliant suport.
 */
/* #define CONFIG_HUBBLE_NETWORK_CRYPTO_PSA 1 */

/*
 * Counter Source
 *
 * Unix time mode is used by default. To use device uptime mode instead,
 * define CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME.
 */
/* #define CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME  1 */

#if defined(CONFIG_HUBBLE_COUNTER_SOURCE_UNIX_TIME) &&                         \
	defined(CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME)
#error "Cannot define both CONFIG_HUBBLE_COUNTER_SOURCE_UNIX_TIME and CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME"
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
 * last time the device had time synced.
 */
#ifndef CONFIG_HUBBLE_SAT_NETWORK_DEVICE_TDR
#define CONFIG_HUBBLE_SAT_NETWORK_DEVICE_TDR  500
#endif

/* Protocol version
 *
 * - CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1: first version of sat
 * protocol. Channel hopping during transmissions.
 */
#ifndef CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1
#define CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1 1
#endif

#endif /* CONFIG_HUBBLE_SAT_NETWORK */

#endif /* INCLUDE_PORT_FREERTOS_CONFIG_H */
