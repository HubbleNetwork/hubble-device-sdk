/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Simplicity SDK configuration for the Hubble Satellite Network
 * expressed as CMSIS Configuration Wizard annotations.
 */

#ifndef INCLUDE_PORT_SISDK_SAT_CONFIG_H
#define INCLUDE_PORT_SISDK_SAT_CONFIG_H

// <<< Use Configuration Wizard in Context Menu >>>

// <h> Hubble Satellite Network

// <o SISDK_HUBBLE_SAT_NETWORK_PROTOCOL> Satellite protocol
// <1=> V1
// <i> First version of Satellite protocol. Channel hopping
// <i> within transmissions.
// <d> 1
#define SISDK_HUBBLE_SAT_NETWORK_PROTOCOL   1

// <q SISDK_HUBBLE_SAT_NETWORK_SMALL> Smaller code size for fly-by calculation
// <i> Use polynomial approximations for trigonometric functions in
// <i> satellite pass prediction code.
// <i>
// <i> Reduces code size at the cost of reduced numerical accuracy in
// <i> pass window calculations. Suitable for constrained devices where
// <i> code size is critical.
// <d> 0
#define SISDK_HUBBLE_SAT_NETWORK_SMALL      0

// <o SISDK_HUBBLE_SAT_NETWORK_DEVICE_TDR> Device time drift retry rate in PPM
// <i> Compensates for clock drift by adding retransmission retries
// <i> proportional to time elapsed since the last time sync.
// <i>
// <i> Higher values add more retries for the same drift, increasing
// <i> reliability but also power consumption. Default of 10 PPM is
// <i> suitable for typical crystal oscillators.
// <d> 10
#define SISDK_HUBBLE_SAT_NETWORK_DEVICE_TDR 10

// <q SISDK_HUBBLE_SAT_NETWORK_DTM_MODE> Enable DTM mode
// <i> Enable DTM mode in the SDK. It is intended to test the operation
// <i> of the radio.
// <d> 0
#define SISDK_HUBBLE_SAT_NETWORK_DTM_MODE   0

// </h>

// <<< end of configuration section >>>

/*
 * Derived symbols - do not edit below this line.
 */

#if SISDK_HUBBLE_SAT_NETWORK_PROTOCOL == 1
#define CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1 1
#else
#error "unsupported Hubble Satellite protocol"
#endif

#if SISDK_HUBBLE_SAT_NETWORK_SMALL == 1
#define CONFIG_HUBBLE_SAT_NETWORK_SMALL 1
#endif

#if SISDK_HUBBLE_SAT_NETWORK_DEVICE_TDR < 0
#error "SISDK_HUBBLE_SAT_NETWORK_DEVICE_TDR must not be negative"
#endif
#define CONFIG_HUBBLE_SAT_NETWORK_DEVICE_TDR SISDK_HUBBLE_SAT_NETWORK_DEVICE_TDR

#if SISDK_HUBBLE_SAT_NETWORK_DTM_MODE == 1
#define CONFIG_HUBBLE_SAT_NETWORK_DTM_MODE 1
#endif

#endif /* INCLUDE_PORT_SISDK_SAT_CONFIG_H */
