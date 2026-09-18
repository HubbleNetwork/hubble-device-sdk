/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Simplicity SDK configuration for the Hubble Device SDK
 * expressed as CMSIS Configuration Wizard annotations.
 */

#ifndef INCLUDE_PORT_SISDK_CONFIG_H
#define INCLUDE_PORT_SISDK_CONFIG_H

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Hubble BLE Network

// <o CONFIG_HUBBLE_KEY_SIZE> Hubble Network key size
// <32=> AES-256
// <16=> AES-128
// <i> Select the master key size for AES encryption and CMAC
// <i> authentication. Used throughout the crypto layer for buffer sizing
// <i> and key operations. 32 selects AES-256, 16 selects AES-128.
// <d> 32
#define CONFIG_HUBBLE_KEY_SIZE                             32

// <o CONFIG_HUBBLE_COUNTER_SOURCE> Counter source
// <0=> Unix time
// <1=> Device uptime
// <i> Selects how the SDK computes the time counter used for EID rotation
// <i> and key derivation.
// <i>
// <i> Unix time derives the counter from Unix epoch time. The counter
// <i> increments once per rotation period. Best for devices with access to
// <i> real-time clocks or network time synchronization.
// <i>
// <i> Device uptime derives the counter from device uptime, with no Unix
// <i> time required. Best for devices without a real-time clock, reliable
// <i> time source, or method to provision time.
// <d> 0
#define CONFIG_HUBBLE_COUNTER_SOURCE                       0

// </h>

// <h> Security

// <q CONFIG_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK> Enforce nonce check
// <i> Validates that the encryption nonce is unique before encrypting.
// <i> Prevents nonce reuse, which would compromise AES-CTR
// <i> confidentiality.
// <i>
// <i> Enabled by default. Disabling removes the runtime check but risks
// <i> catastrophic security failure if nonces are ever repeated. Only
// <i> disable for testing or when the application guarantees uniqueness
// <i> by other means.
// <d> 1
#define CONFIG_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK 1

// </h>

// <h> Advanced

// <q CONFIG_HUBBLE_NETWORK_SEQUENCE_NONCE_CUSTOM> Custom sequence counter
// <i> Override the default auto-incrementing sequence counter with an
// <i> application-provided implementation. Useful for deterministic testing
// <i> or persisting counters across reboots.
// <d> 0
#define CONFIG_HUBBLE_NETWORK_SEQUENCE_NONCE_CUSTOM        0

// <q CONFIG_HUBBLE_UPTIME_CUSTOM> Application-defined uptime implementation
// <i> Override the default platform uptime function with an
// <i> application-provided implementation. Primarily useful for unit
// <i> testing where controlled time values are needed to verify
// <i> time-dependent behavior.
// <d> 0
#define CONFIG_HUBBLE_UPTIME_CUSTOM                        0

// </h>

// <<< end of configuration section >>>

/*
 * Derived symbols - do not edit below this line.
 */

#if !CONFIG_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK
#undef CONFIG_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK
#endif

#if !CONFIG_HUBBLE_NETWORK_SEQUENCE_NONCE_CUSTOM
#undef CONFIG_HUBBLE_NETWORK_SEQUENCE_NONCE_CUSTOM
#endif

#if !CONFIG_HUBBLE_UPTIME_CUSTOM
#undef CONFIG_HUBBLE_UPTIME_CUSTOM
#endif

/* Matching the counter source selection to the right config symbol */
#if CONFIG_HUBBLE_COUNTER_SOURCE
#define CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME 1
#else
#define CONFIG_HUBBLE_COUNTER_SOURCE_UNIX_TIME 1
#endif
#undef CONFIG_HUBBLE_COUNTER_SOURCE

#if CONFIG_HUBBLE_KEY_SIZE == 32
#define CONFIG_HUBBLE_NETWORK_KEY_256 1
#elif CONFIG_HUBBLE_KEY_SIZE == 16
#define CONFIG_HUBBLE_NETWORK_KEY_128 1
#else
#error "CONFIG_HUBBLE_KEY_SIZE must be 16 or 32"
#endif

/* Lock configs for this port */
#define CONFIG_HUBBLE_BLE_NETWORK             1
#define CONFIG_HUBBLE_NETWORK_CRYPTO_PSA      1
#define CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC 86400

#endif /* INCLUDE_PORT_SISDK_CONFIG_H */
