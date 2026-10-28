/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_HUBBLE_CHALLENGE_H
#define INCLUDE_HUBBLE_CHALLENGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Hubble Challenge-Response APIs
 *
 * Lets the device authenticate a peer that holds its master key, for
 * example the Hubble Cloud or a service acting on its behalf.
 *
 * The application picks a nonce and builds a challenge from it with
 * @ref hubble_challenge_get, then sends the challenge to the peer over any
 * transport. The challenge carries the device's current EID in plain bytes,
 * so the peer can look up the device and its key the same way it resolves
 * an advertisement. The peer computes the response and sends it back, and
 * the application checks it with @ref hubble_challenge_response_check.
 *
 * Challenge layout (@ref HUBBLE_CHALLENGE_SIZE bytes):
 *
 * @code
 * | offset | size | field                                       |
 * |--------+------+---------------------------------------------|
 * | 0      | 1    | version (HUBBLE_CHALLENGE_VERSION)          |
 * | 1      | 4    | EID, same bytes as the BLE advertisement    |
 * | 5      | 16   | nonce supplied by the application           |
 * @endcode
 *
 * The response (@ref HUBBLE_CHALLENGE_RESPONSE_SIZE bytes) is derived with
 * the NIST SP 800-108 counter mode KDF, AES-CMAC as PRF, the same function
 * the SDK uses for its other derived keys and values:
 *
 * @code
 * challenge_key = KDF(master_key, "ChallengeKey",
 *                     decimal(time_counter), key_len)
 * response      = KDF(challenge_key, "Response", challenge, 16)
 * @endcode
 *
 * where time_counter is the counter the EID was derived from.
 *
 * The SDK keeps no state between the two calls. Replay protection is the
 * application's job: the nonce must be unpredictable, and a response must
 * be accepted at most once for a given nonce.
 *
 * @defgroup hubble_challenge_api Challenge-Response APIs
 * @{
 */

/** @brief Version of the challenge format. */
#define HUBBLE_CHALLENGE_VERSION       0x00U

/** @brief Size of the nonce carried in the challenge in bytes. */
#define HUBBLE_CHALLENGE_NONCE_SIZE    16U

/** @brief Size of a challenge in bytes. */
#define HUBBLE_CHALLENGE_SIZE          21U

/** @brief Size of a challenge response in bytes. */
#define HUBBLE_CHALLENGE_RESPONSE_SIZE 16U

/**
 * @brief Build a challenge from a nonce.
 *
 * Writes the challenge for the current time counter: version, current EID
 * and @p nonce.
 *
 * @param nonce     Nonce chosen by the application, it should come from a
 *                  cryptographically secure random number generator.
 * @param nonce_len Length of @p nonce in bytes, must be
 *                  @ref HUBBLE_CHALLENGE_NONCE_SIZE.
 * @param challenge Output buffer of at least @ref HUBBLE_CHALLENGE_SIZE bytes.
 * @param len       in: Size of @p challenge in bytes.
 *                  out: Number of bytes written.
 *
 * @retval 0       Challenge built.
 * @retval -EINVAL A pointer is NULL, @p nonce_len is wrong, @p len is too
 *                 small or no key is set.
 * @retval <0      Negative errno from the time counter or the crypto
 *                 provider.
 */
int hubble_challenge_get(const uint8_t *nonce, size_t nonce_len,
			 uint8_t *challenge, size_t *len);

/**
 * @brief Check the response to a challenge.
 *
 * Compares @p response with the value expected for @p challenge. The
 * challenge must have been built by @ref hubble_challenge_get in
 * the current EID rotation period.
 *
 * @param challenge     Challenge sent to the peer.
 * @param challenge_len Length of @p challenge in bytes, must be
 *                      @ref HUBBLE_CHALLENGE_SIZE.
 * @param response      Response received from the peer.
 * @param response_len  Length of @p response in bytes, must be
 *                      @ref HUBBLE_CHALLENGE_RESPONSE_SIZE.
 *
 * @retval 0       The response matches, the peer holds the device key.
 * @retval -EINVAL A pointer is NULL, a length is wrong, the challenge
 *                 version is unknown or no key is set.
 * @retval -ESTALE The EID rotated since the challenge was built, build a
 *                 new one.
 * @retval -EACCES The response does not match.
 * @retval <0      Negative errno from the time counter or the crypto
 *                 provider.
 */
int hubble_challenge_response_check(
	const uint8_t *challenge, size_t challenge_len, const uint8_t *response,
	size_t response_len);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_HUBBLE_CHALLENGE_H */
