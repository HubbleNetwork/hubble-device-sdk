/*
 * Copyright (c) 2025 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 **/

/**
 * @file packet.h
 * @brief Hubble Network packet APIs
 **/

#ifndef INCLUDE_HUBBLE_SAT_PACKET_H
#define INCLUDE_HUBBLE_SAT_PACKET_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Hubble Sat Network Packet APIs
 * @defgroup hubble_sat_packet_api Satellite Network packet APIs
 * @ingroup hubble_sat_api
 * @{
 */

/* @brief Max number of symbols that packet can have */
#define HUBBLE_PACKET_MAX_SIZE               52

/* @brief Max number of bytes for data payload */
#define HUBBLE_SAT_PAYLOAD_MAX               13

/* @brief Max number of symbols per frame */
#define HUBBLE_PACKET_FRAME_PAYLOAD_MAX_SIZE 16U

/* @brief Max number of frames per packet */
#define HUBBLE_PACKET_FRAME_MAX_SIZE         4U

/**
 * @brief Structure representing a Hubble packet.
 *
 * This structure is used to represent a Hubble packet, the data
 * is a set symbols that represents the frequency in a specific channel.
 *
 * Since the preamble is a fixed pattern that is formed by the reference
 * frequency and the complete absence of tranmission for a certain period,
 * it is NOT included in this structure.
 *
 * @note The preamble is NOT included in this struct.
 */
struct hubble_sat_packet {
	/**
	 * @brief Data of the packet.
	 */
	uint8_t data[HUBBLE_PACKET_MAX_SIZE];
	/**
	 * @brief Number of symbols in the packet.
	 */
	size_t length;
};

/**
 * @brief Structure representing a set of Hubble packet frames.
 *
 * A single Hubble packet may be split into multiple frames for transmission.
 * Each frame carries a chunk of the encoded packet payload along with the
 * channel on which it should be transmitted.
 *
 * @note The preamble is included in this struct.
 */
struct hubble_sat_packet_frames {
	/** @brief Individual frames derived from a Hubble packet. */
	struct {
		/** @brief Encoded payload data for this frame. */
		uint8_t data[HUBBLE_PACKET_FRAME_PAYLOAD_MAX_SIZE];
		/** @brief Channel on which this frame should be transmitted. */
		uint8_t channel;
	} frame[HUBBLE_PACKET_FRAME_MAX_SIZE];

	/** @brief Total number of symbols used. The number of frame is
	 *  (int)ceil(total_number_of_symbols / HUBBLE_PACKET_FRAME_PAYLOAD_MAX_SIZE)
	 */
	size_t total_number_of_symbols;
};

/**
 * @brief Build a Hubble satellite packet from a payload.
 *
 * This function constructs a Hubble satellite packet by encoding the provided
 * payload data into the packet structure.
 *
 * @param  packet  Pointer to the packet structure to be populated.
 * @param  payload Pointer to the payload data to be included in the packet.
 * @param  length  Length of the payload data in bytes.
 *
 * @retval 0       On success.
 * @retval -EINVAL If any of the input parameters are invalid.
 * @retval -ENOMEM If the payload length exceeds the maximum allowed size.
 */
int hubble_sat_packet_get(struct hubble_sat_packet *packet, const void *payload,
			  size_t length);

/**
 * @brief Split a Hubble satellite packet into transmission frames.
 *
 * This function divides the encoded data of a Hubble packet into one or more
 * frames suitable for transmission. Each frame carries a portion of the packet
 * payload along with its associated channel.
 *
 * @param  packet Pointer to the source packet to split.
 * @param  frames Pointer to the frames structure to be populated.
 *
 * @retval 0       On success.
 * @retval -EINVAL If any of the input parameters are invalid.
 */
int hubble_sat_packet_frames_get(const struct hubble_sat_packet *packet,
				 struct hubble_sat_packet_frames *frames);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_HUBBLE_SAT_PACKET_H */
