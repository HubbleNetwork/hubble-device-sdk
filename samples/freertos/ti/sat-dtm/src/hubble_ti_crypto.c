/*
 * Copyright (c) 2026 HubbleNetwork
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Dummy crypto implementation.
 *
 * Hubble needs crypto to be implemented, but we don't really need
 * it for the DTM sample, so here is just a dummy implementation.
 */

#include <stdint.h>
#include <string.h>

#include <hubble/port/sys.h>
#include <hubble/port/crypto.h>

void hubble_crypto_zeroize(void *buf, size_t len)
{
	memset(buf, 0, len);
}

int hubble_crypto_cmac(const uint8_t key[CONFIG_HUBBLE_KEY_SIZE],
		       const uint8_t *input, size_t input_len,
		       uint8_t output[HUBBLE_AES_BLOCK_SIZE])
{
	return 0;
}

int hubble_crypto_aes_ctr(const uint8_t key[CONFIG_HUBBLE_KEY_SIZE],
			  uint8_t nonce_counter[HUBBLE_NONCE_BUFFER_SIZE],
			  const uint8_t *data, size_t len, uint8_t *output)
{
	return 0;
}

int hubble_crypto_init(void)
{
	return 0;
}
