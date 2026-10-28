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


/*
 * Enable the Hubble Satellite network module.
 */
/* #define CONFIG_HUBBLE_SAT_NETWORK 1 */

/*
 * Size of the encryption key in bytes. Valid options are
 * 16 for 128 bits keys or 32 for 256 bits keys. The default
 * option is to use 256 bits key.
 */
#if defined(CONFIG_HUBBLE_NETWORK_KEY_256) &&                                  \
	defined(CONFIG_HUBBLE_NETWORK_KEY_128)
#error "Cannot define both CONFIG_HUBBLE_NETWORK_KEY_256 and CONFIG_HUBBLE_NETWORK_KEY_128"
#elif !defined(CONFIG_HUBBLE_NETWORK_KEY_256) &&                               \
	!defined(CONFIG_HUBBLE_NETWORK_KEY_128)
#define CONFIG_HUBBLE_NETWORK_KEY_256 1
#endif

/*
 * Use the PSA Crypto API (Platform Security Architecture) for cryptographic
 * operations. When enabled, the SDK uses the PSA API (e.g., via Mbed TLS)
 * Enable this when the target platform provides a PSA-compliant suport.
 */
/* #define CONFIG_HUBBLE_NETWORK_CRYPTO_PSA 1 */

/*
 * Enable the challenge-response API (<hubble/challenge.h>).
 */
/* #define CONFIG_HUBBLE_CHALLENGE 1 */

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
#define CONFIG_HUBBLE_SAT_NETWORK_DEVICE_TDR 10
#endif

/* Protocol version
 *
 * - CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1: first version of sat
 * protocol. Channel hopping during transmissions.
 */
#ifndef CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1
#define CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1 1
#endif

/*
 * Initialize the radio via the FreeRTOS Timer/Daemon task startup hook
 * (vApplicationDaemonTaskStartupHook). When enabled, board initialization
 * runs at the start of the timer task — after the scheduler is running but
 * before any timer callbacks fire — which satisfies the RCL driver requirement
 * that RCL_init / RCL_open are called from a task context. Also enables
 * configUSE_DAEMON_TASK_STARTUP_HOOK so FreeRTOS calls the hook.
 *
 * When disabled, the application must call hubble_sat_board_init() from a
 * task context itself before using satellite network or Bluetooth functionality.
 *
 * Default: enabled when CONFIG_HUBBLE_SAT_NETWORK is defined.
 */
#ifndef CONFIG_HUBBLE_FREERTOS_DAEMON_HOOK
#define CONFIG_HUBBLE_FREERTOS_DAEMON_HOOK 1
#define configUSE_DAEMON_TASK_STARTUP_HOOK
#endif

/*
 * DTM Mode
 * Enable DTM (Direct Test Mode) in the SDK, this is intended
 * to test the operation of the radio.
 */
/* #define CONFIG_HUBBLE_SAT_NETWORK_DTM_MODE 1 */

/*
 * Packet sent callback
 * Call hubble_sat_packet_sent_cb() after every successful satellite
 * packet transmission, retransmissions included. The application must
 * define the function.
 */
/* #define CONFIG_HUBBLE_SAT_NETWORK_PACKET_SENT_CB 1 */

#endif /* CONFIG_HUBBLE_SAT_NETWORK */

#endif /* INCLUDE_PORT_FREERTOS_CONFIG_H */
