/*
 * Copyright (c) 2025 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file sat_soc.h
 * @internal
 * @brief Hubble Network Nordic SOC-specific satellite communication APIs
 */

#ifndef PORT_ZEPHYR_BOARDS_NORDIC_SAT_SOC_H
#define PORT_ZEPHYR_BOARDS_NORDIC_SAT_SOC_H

#include <stdint.h>

#include <hubble/sat/packet.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enables the SOC radio hardware for satellite transmission.
 *
 * This function initializes and enables the SoC radio hardware
 * components required for satellite packet transmission. It configures the
 * high-frequency clock, saves the current radio register states, sets up the
 * radio base frequency, configures timers and PPI channels, and enables radio
 * interrupts.
 *
 * @return 0 on success, or a negative error code on failure.
 *
 * @note This function must be called before transmitting packets.
 *       Call hubble_sat_soc_disable() after transmission is complete to restore
 *       the radio state.
 */
int hubble_sat_soc_enable(void);

/**
 * @brief Disables the SOC radio hardware and restores previous state.
 *
 * This function disables the SOC radio hardware and restores the radio register
 * states that were saved during hubble_sat_soc_enable(). This ensures the radio
 * returns to its previous configuration.
 *
 * @return 0 on success, or a negative error code on failure.
 *
 * @note This function should be called after transmission operations
 *       are complete to properly restore the radio state.
 */
int hubble_sat_soc_disable(void);

/**
 * @brief Sends a packet over the SOC radio hardware.
 *
 * This function transmits a packet over the satellite communication channel
 * using the SOC radio hardware. It sends a fixed preamble pattern followed by
 * the packet payload symbols. Each symbol is transmitted as a frequency step
 * offset from the base channel frequency.
 *
 * @param packet A pointer to the hubble_sat_packet_frames structure containing
 *               the data to be transmitted.
 *
 * @return 0 on successful transmission, or a negative error code on failure.
 *
 * @warning This function blocks until the entire packet is transmitted.
 *          It does not perform any validation on the packet structure.
 *          It is the caller's responsibility to ensure the packet is
 *          correctly formatted.
 */
int hubble_sat_soc_packet_send(const struct hubble_sat_packet_frames *packet);

#ifdef __cplusplus
}
#endif

#endif /* PORT_ZEPHYR_BOARDS_NORDIC_SAT_SOC_H */
