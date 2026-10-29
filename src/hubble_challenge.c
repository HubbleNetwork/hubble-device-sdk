/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <hubble/hubble.h>
#include <hubble/challenge.h>
#include <hubble/port/crypto.h>

#include "hubble_priv.h"

#define _CHALLENGE_VERSION_OFFSET 0U
#define _CHALLENGE_EID_OFFSET     1U
#define _CHALLENGE_NONCE_OFFSET   (_CHALLENGE_EID_OFFSET + sizeof(uint32_t))

int hubble_challenge_get(const uint8_t *nonce, size_t nonce_len,
			 uint8_t *challenge, size_t *len)
{
	int err;
	uint32_t counter;

	if ((nonce == NULL) || (nonce_len != HUBBLE_CHALLENGE_NONCE_SIZE) ||
	    (challenge == NULL) || (len == NULL) ||
	    (*len < HUBBLE_CHALLENGE_SIZE)) {
		return -EINVAL;
	}

	if (hubble_internal_key_get() == NULL) {
		return -EINVAL;
	}

	err = hubble_counter_get(&counter);
	if (err != 0) {
		return err;
	}

	err = hubble_internal_device_id_get(&challenge[_CHALLENGE_EID_OFFSET],
					    sizeof(uint32_t), counter);
	if (err != 0) {
		return err;
	}

	challenge[_CHALLENGE_VERSION_OFFSET] = HUBBLE_CHALLENGE_VERSION;
	memcpy(&challenge[_CHALLENGE_NONCE_OFFSET], nonce, nonce_len);
	*len = HUBBLE_CHALLENGE_SIZE;

	return 0;
}

int hubble_challenge_response_check(const uint8_t *challenge,
				    size_t challenge_len,
				    const uint8_t *response, size_t response_len)
{
	int err;
	uint32_t counter;
	uint32_t eid;
	uint8_t expected[HUBBLE_CHALLENGE_RESPONSE_SIZE];

	if ((challenge == NULL) || (challenge_len != HUBBLE_CHALLENGE_SIZE) ||
	    (response == NULL) ||
	    (response_len != HUBBLE_CHALLENGE_RESPONSE_SIZE)) {
		return -EINVAL;
	}

	if ((challenge[_CHALLENGE_VERSION_OFFSET] != HUBBLE_CHALLENGE_VERSION) ||
	    (hubble_internal_key_get() == NULL)) {
		return -EINVAL;
	}

	err = hubble_counter_get(&counter);
	if (err != 0) {
		return err;
	}

	/* The response is bound to the counter the EID was derived from */
	err = hubble_internal_device_id_get((uint8_t *)&eid, sizeof(eid),
					    counter);
	if (err != 0) {
		return err;
	}

	if (memcmp(&eid, &challenge[_CHALLENGE_EID_OFFSET], sizeof(eid)) != 0) {
		return -ESTALE;
	}

	err = hubble_internal_challenge_response_get(
		counter, challenge, challenge_len, expected, sizeof(expected));
	if (err != 0) {
		goto exit;
	}

	if (memcmp(expected, response, sizeof(expected)) != 0) {
		err = -EACCES;
	}

exit:
	hubble_crypto_zeroize(expected, sizeof(expected));

	return err;
}
