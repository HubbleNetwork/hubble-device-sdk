/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 **/

/**
 * @file dtm.h
 * @brief Hubble Network DTM (Direct Test Mode) APIs
 *
 * This module provides APIs for Direct Test Mode, which allows testing
 * the radio physical layer by transmitting test packets on
 * a specified channel or hopping.
 **/

#ifndef INCLUDE_HUBBLE_SAT_DTM_H
#define INCLUDE_HUBBLE_SAT_DTM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DTM packet type.
 *
 * Selects the test packet payload size to transmit during DTM.
 * All the payload is random bytes.
 */
enum hubble_sat_dtm_packet_type {
	HUBBLE_SAT_DTM_PACKET_0,
	HUBBLE_SAT_DTM_PACKET_4,
	HUBBLE_SAT_DTM_PACKET_9,
	HUBBLE_SAT_DTM_PACKET_13,
};

/**
 * @brief Send a DTM test packet.
 *
 * Transmits a single DTM packet of the given type on the specified channel.
 *
 * @note channel equals -1 means regular transmission as it is done with
 * @ref hubble_sat_packet_send
 *
 * @param type    Packet payload pattern to use.
 * @param channel RF channel to transmit on. -1 to hop between channels.
 *
 * @retval 0       Success.
 * @retval -EINVAL Invalid channel or packet type.
 */
int hubble_sat_dtm_packet_send(enum hubble_sat_dtm_packet_type type,
			       int8_t channel);

/**
 * @brief Set the transmit power level.
 *
 * Configures the output power used for subsequent DTM transmissions.
 *
 * @param power Desired TX power in dBm.
 *
 * @retval 0       Success.
 * @retval -EINVAL Power level is out of the supported range.
 */
int hubble_sat_dtm_power_set(int8_t power);

/**
 * @brief Start continuous wave (CW) transmission.
 *
 * Begins transmitting an unmodulated carrier on the specified channel.
 * Call @ref hubble_sat_dtm_cw_stop to end the transmission.
 *
 * @param channel RF channel index to transmit on.
 *
 * @retval 0       Success.
 * @retval -EINVAL Invalid channel.
 */
int hubble_sat_dtm_cw_start(uint8_t channel);

/**
 * @brief Stop continuous wave (CW) transmission.
 *
 * Halts the unmodulated carrier started by @ref hubble_sat_dtm_cw_start.
 *
 * @retval 0    Success.
 */
int hubble_sat_dtm_cw_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_HUBBLE_SAT_DTM_H */
