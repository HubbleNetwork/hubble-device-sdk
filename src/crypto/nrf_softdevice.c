/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Minimal crypto backend for the legacy nRF5 SDK with a Nordic SoftDevice.
 *
 * AES-CTR and AES-CMAC are built on the AES-128 ECB encrypt primitive the
 * SoftDevice already ships (sd_ecb_blocks_encrypt, FIPS-197 byte order), so
 * no general purpose crypto library (mbedTLS, PSA) has to be linked. The
 * SoftDevice only provides AES-128, hence only 128-bit Hubble keys are
 * supported. nRF52 SoftDevices only.
 *
 * The primitive is not documented to support encrypting in place, so callers
 * below always pass distinct input and output buffers.
 *
 * Every encryption is a SoftDevice SVC call, which cannot be made at or above
 * the SVC priority (application high priority 2 and 3, SoftDevice-reserved
 * levels, or with interrupts masked). Such calls return -EACCES instead of
 * escalating to a HardFault.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <hubble/port/crypto.h>

#include <nrf.h>
#include <nrf_soc.h>

#if CONFIG_HUBBLE_KEY_SIZE != 16
#error "The Nordic SoftDevice crypto backend only supports 128-bit keys. \
Define CONFIG_HUBBLE_NETWORK_KEY_128, or use the mbedTLS or PSA backend \
for devices provisioned with 256-bit keys."
#endif

#if defined(NRF51)
#error "The Nordic SoftDevice crypto backend does not support nRF51 devices"
#endif

/* An SVC issued at or above its own priority escalates to a HardFault, so
 * check the current execution priority before calling into the SoftDevice.
 */
static bool _sd_call_allowed(void)
{
	uint32_t svc_prio = NVIC_GetPriority(SVCall_IRQn);
	uint32_t basepri = __get_BASEPRI() >> (8U - __NVIC_PRIO_BITS);
	uint32_t ipsr = __get_IPSR();

	if (__get_PRIMASK() != 0U) {
		return false;
	}

	if ((basepri != 0U) && (basepri <= svc_prio)) {
		return false;
	}

	/* Thread mode */
	if (ipsr == 0U) {
		return true;
	}

	/* Reset, NMI and HardFault have fixed negative priorities */
	if (ipsr < 4U) {
		return false;
	}

	return NVIC_GetPriority((IRQn_Type)((int32_t)ipsr - 16)) > svc_prio;
}

static int _ecb_encrypt(const uint8_t key[HUBBLE_AES_BLOCK_SIZE],
			const uint8_t in[HUBBLE_AES_BLOCK_SIZE],
			uint8_t out[HUBBLE_AES_BLOCK_SIZE])
{
	/* The multi-block variant takes pointers, so neither the key nor the
	 * data has to be copied into a scratch structure.
	 */
	nrf_ecb_hal_data_block_t block = {
		.p_key = (soc_ecb_key_t const *)key,
		.p_cleartext = (soc_ecb_cleartext_t const *)in,
		.p_ciphertext = (soc_ecb_ciphertext_t *)out,
	};

	if (!_sd_call_allowed()) {
		return -EACCES;
	}

	return (sd_ecb_blocks_encrypt(1, &block) == NRF_SUCCESS) ? 0 : -EIO;
}

static void _xor(uint8_t *dst, const uint8_t *a, const uint8_t *b, size_t len)
{
	for (size_t i = 0; i < len; i++) {
		dst[i] = a[i] ^ b[i];
	}
}

/* Increment the whole block as a big-endian counter */
static void _ctr_inc(uint8_t ctr[HUBBLE_NONCE_BUFFER_SIZE])
{
	for (size_t i = HUBBLE_NONCE_BUFFER_SIZE; i > 0; i--) {
		if (++ctr[i - 1] != 0U) {
			break;
		}
	}
}

void hubble_crypto_zeroize(void *buf, size_t len)
{
	volatile uint8_t *p = buf;

	while (len-- > 0) {
		*p++ = 0U;
	}
}

int hubble_crypto_init(void)
{
	return 0;
}

int hubble_crypto_aes_ctr(const uint8_t key[CONFIG_HUBBLE_KEY_SIZE],
			  uint8_t nonce_counter[HUBBLE_NONCE_BUFFER_SIZE],
			  const uint8_t *data, size_t len, uint8_t *output)
{
	int ret = 0;
	uint8_t stream[HUBBLE_AES_BLOCK_SIZE];

	for (size_t off = 0; off < len; off += HUBBLE_AES_BLOCK_SIZE) {
		size_t n = len - off;

		if (n > HUBBLE_AES_BLOCK_SIZE) {
			n = HUBBLE_AES_BLOCK_SIZE;
		}

		ret = _ecb_encrypt(key, nonce_counter, stream);
		if (ret != 0) {
			break;
		}
		_ctr_inc(nonce_counter);

		_xor(&output[off], &data[off], stream, n);
	}

	hubble_crypto_zeroize(stream, sizeof(stream));

	return ret;
}

/* Multiply by x in GF(2^128), in place (RFC 4493 section 2.3) */
static void _cmac_dbl(uint8_t block[HUBBLE_AES_BLOCK_SIZE])
{
	uint8_t msb = block[0] & 0x80U;

	for (size_t i = 0; i < (HUBBLE_AES_BLOCK_SIZE - 1); i++) {
		block[i] = (uint8_t)((block[i] << 1) | (block[i + 1] >> 7));
	}

	block[HUBBLE_AES_BLOCK_SIZE - 1] <<= 1;
	if (msb != 0U) {
		/* R_128 constant (RFC 4493) */
		block[HUBBLE_AES_BLOCK_SIZE - 1] ^= 0x87U;
	}
}

int hubble_crypto_cmac(const uint8_t key[CONFIG_HUBBLE_KEY_SIZE],
		       const uint8_t *data, size_t len,
		       uint8_t output[HUBBLE_AES_BLOCK_SIZE])
{
	int ret;
	uint8_t state[HUBBLE_AES_BLOCK_SIZE] = {0};
	uint8_t block[HUBBLE_AES_BLOCK_SIZE];
	uint8_t subkey[HUBBLE_AES_BLOCK_SIZE];

	/* L = AES-128(K, 0^128), using the still all-zero state as input */
	ret = _ecb_encrypt(key, state, subkey);
	if (ret != 0) {
		goto exit;
	}

	/* Every block but the last is chained straight through, mixed into a
	 * scratch block so the ECB input and output never overlap.
	 */
	while (len > HUBBLE_AES_BLOCK_SIZE) {
		_xor(block, state, data, HUBBLE_AES_BLOCK_SIZE);

		ret = _ecb_encrypt(key, block, state);
		if (ret != 0) {
			goto exit;
		}

		data += HUBBLE_AES_BLOCK_SIZE;
		len -= HUBBLE_AES_BLOCK_SIZE;
	}

	/* A complete last block uses K1, a padded (or empty) one uses K2 */
	_cmac_dbl(subkey);
	if (len < HUBBLE_AES_BLOCK_SIZE) {
		_cmac_dbl(subkey);
		state[len] ^= 0x80U;
	}

	_xor(state, state, data, len);
	_xor(state, state, subkey, HUBBLE_AES_BLOCK_SIZE);

	ret = _ecb_encrypt(key, state, output);

exit:
	hubble_crypto_zeroize(state, sizeof(state));
	hubble_crypto_zeroize(block, sizeof(block));
	hubble_crypto_zeroize(subkey, sizeof(subkey));

	return ret;
}
