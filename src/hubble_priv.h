/*
 * Copyright (c) 2025 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_HUBBLE_PRIV_H
#define SRC_HUBBLE_PRIV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint64_t hubble_internal_utc_time_get(void);

/* Returns the last time UTC was synced. It
 * is used to accommodate clock drifts.
 */
uint64_t hubble_internal_utc_time_last_synced_get(void);

/**
 * @brief Get the master encryption key.
 *
 * Returns a pointer to the master key that was previously set via
 * hubble_key_set(). This key is used as the root for deriving
 * encryption keys, nonces, and device identifiers.
 *
 * @return Pointer to the master key, or NULL if no key has been set.
 */
const void *hubble_internal_key_get(void);

/**
 * @brief Get the current time counter value.
 *
 * Computes a time counter by dividing the current unix epoch time by
 * a daily period (86400000 ms). This counter is used for key
 * derivation and rotates once per day, providing time-based
 * cryptographic isolation.
 *
 * @return Current time counter value representing the number of days
 *         since unix epoch.
 */
uint32_t hubble_internal_time_counter_get(void);

/**
 * @brief Check if nonce values are safe to use for encryption.
 *
 * This function validates that the provided time counter and sequence number
 * combination will not result in nonce reuse.
 *
 * When the time counter changes, any sequence number is accepted and becomes
 * the new daily reference. Within the same time counter period, the function
 * ensures sequence numbers are not reused, including after wrap-around.
 *
 * @param time_counter Time-based counter value.
 * @param seq_no       Sequence number to validate.
 *
 * @return true if the nonce values are safe to use, false if using them
 *         would result in nonce reuse.
 */
bool hubble_internal_nonce_values_check(uint32_t time_counter, uint16_t seq_no);

/**
 * @brief Get a derived device ID based on the time counter.
 *
 * This function generates a device identifier by deriving it from an
 * internal device key and the provided time counter.
 *
 * @param device_id     Pointer to the output buffer for the device ID.
 * @param device_id_len Length of the device ID buffer in bytes.
 * @param counter       Time-based counter used in the derivation.
 *
 * @return 0 on success, negative error code on failure.
 */
int hubble_internal_device_id_get(uint8_t *device_id, size_t device_id_len,
				  uint32_t counter);

/**
 * @brief Encrypt data using AES-CTR and generate a CMAC authentication tag.
 *
 * This function encrypts the input data using AES-CTR mode and
 * generates a CMAC authentication tag over the ciphertext for
 * integrity verification.
 *
 * The encryption key and nonce are derived from the provided counter and
 * sequence number.
 *
 * @param counter   Time-based counter used to derive the encryption key and nonce.
 * @param seq_no    Sequence number used to derive the encryption key and nonce.
 * @param input     Pointer to the plaintext data to encrypt.
 * @param input_len Length of the input data in bytes.
 * @param out       Pointer to the output buffer for the ciphertext. Must be at
 *                  least @p input_len bytes.
 * @param tag       Pointer to the output buffer for the authentication tag.
 * @param tag_len   Length of the authentication tag to copy (truncated from
 *                  the full CMAC output).
 *
 * @return 0 on success, negative error code on failure.
 */
int hubble_internal_data_encrypt(uint32_t counter, uint16_t seq_no,
				 const uint8_t *input, size_t input_len,
				 uint8_t *out, uint8_t *tag, size_t tag_len);

#endif /* SRC_HUBBLE_PRIV_H */
