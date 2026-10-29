/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hubble/hubble.h>
#include <hubble/challenge.h>
#include <hubble/port/crypto.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include <stdio.h>
#include <string.h>

#define TEST_UNIX_TIME_MS 1760210751803ULL
#define TEST_PERIOD_MS                                                         \
	((uint64_t)CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC * 1000ULL)
#define TEST_ADV_BUFFER_SZ  31
/* Offset of the device id in the advertisement: UUID (2) + seq_no (2) */
#define TEST_ADV_EID_OFFSET 4
/* Challenge layout: version (1), EID (4), nonce */
#define TEST_EID_SIZE       4

static uint8_t test_key[CONFIG_HUBBLE_KEY_SIZE] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
#if CONFIG_HUBBLE_KEY_SIZE == 32
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
#endif
};

/* SP 800-108 counter mode KDF with AES-CMAC, written from the description in
 * <hubble/challenge.h> so the test does not share code with the SDK.
 */
static void kdf(const uint8_t *key, const char *label, const uint8_t *context,
		size_t context_len, uint8_t *out, size_t out_len)
{
	uint8_t msg[64];
	uint8_t block[HUBBLE_AES_BLOCK_SIZE];
	size_t label_len = strlen(label);
	size_t msg_len = 4 + label_len + 1 + context_len + 4;
	uint32_t i = 1U;

	zassert_true(msg_len <= sizeof(msg));

	memcpy(&msg[4], label, label_len);
	msg[4 + label_len] = 0x00;
	memcpy(&msg[4 + label_len + 1], context, context_len);
	sys_put_be32(out_len * 8U, &msg[msg_len - 4]);

	for (size_t done = 0U; done < out_len; i++) {
		size_t n = MIN(out_len - done, sizeof(block));

		sys_put_be32(i, msg);
		zassert_ok(hubble_crypto_cmac(key, msg, msg_len, block));
		memcpy(&out[done], block, n);
		done += n;
	}
}

static void expected_response(uint32_t counter, const uint8_t *challenge,
			      uint8_t response[HUBBLE_CHALLENGE_RESPONSE_SIZE])
{
	char digits[11];
	uint8_t challenge_key[CONFIG_HUBBLE_KEY_SIZE];
	int len = snprintf(digits, sizeof(digits), "%u", counter);

	kdf(test_key, "ChallengeKey", (const uint8_t *)digits, len,
	    challenge_key, sizeof(challenge_key));
	kdf(challenge_key, "Response", challenge, HUBBLE_CHALLENGE_SIZE,
	    response, HUBBLE_CHALLENGE_RESPONSE_SIZE);
}

/* Move the time counter by the given number of periods from the base. */
static void period_set(uint32_t period)
{
#ifdef CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME
	zassert_ok(hubble_init(period, test_key));
#else
	zassert_ok(hubble_time_set(TEST_UNIX_TIME_MS + period * TEST_PERIOD_MS));
#endif
}

static const uint8_t test_nonce[HUBBLE_CHALLENGE_NONCE_SIZE] = {
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
};

static int check(const uint8_t *challenge, const uint8_t *response)
{
	return hubble_challenge_response_check(challenge, HUBBLE_CHALLENGE_SIZE,
					       response,
					       HUBBLE_CHALLENGE_RESPONSE_SIZE);
}

static void challenge_get(uint8_t challenge[HUBBLE_CHALLENGE_SIZE],
			  uint32_t *counter)
{
	size_t len = HUBBLE_CHALLENGE_SIZE;

	zassert_ok(hubble_challenge_get(test_nonce, sizeof(test_nonce),
					challenge, &len));
	zassert_equal(len, HUBBLE_CHALLENGE_SIZE);
	zassert_ok(hubble_counter_get(counter));
}

ZTEST(challenge_test, test_format)
{
	uint8_t challenge[HUBBLE_CHALLENGE_SIZE];
	uint8_t adv[TEST_ADV_BUFFER_SZ];
	size_t adv_len = sizeof(adv);
	uint32_t counter;

	challenge_get(challenge, &counter);

	zassert_equal(challenge[0], HUBBLE_CHALLENGE_VERSION);
	zassert_mem_equal(&challenge[1 + TEST_EID_SIZE], test_nonce,
			  sizeof(test_nonce));

	/* The EID is the one the device advertises */
	zassert_ok(hubble_ble_advertise_get(NULL, 0, adv, &adv_len));
	zassert_mem_equal(&adv[TEST_ADV_EID_OFFSET], &challenge[1],
			  TEST_EID_SIZE);
}

ZTEST(challenge_test, test_valid_response)
{
	uint8_t challenge[HUBBLE_CHALLENGE_SIZE];
	uint8_t response[HUBBLE_CHALLENGE_RESPONSE_SIZE];
	uint32_t counter;

	challenge_get(challenge, &counter);
	expected_response(counter, challenge, response);

	zassert_ok(check(challenge, response));
}

ZTEST(challenge_test, test_wrong_response)
{
	uint8_t challenge[HUBBLE_CHALLENGE_SIZE];
	uint8_t response[HUBBLE_CHALLENGE_RESPONSE_SIZE];
	uint32_t counter;

	challenge_get(challenge, &counter);
	expected_response(counter, challenge, response);

	response[HUBBLE_CHALLENGE_RESPONSE_SIZE - 1] ^= 0x01;
	zassert_equal(check(challenge, response), -EACCES);
	response[HUBBLE_CHALLENGE_RESPONSE_SIZE - 1] ^= 0x01;

	/* The response covers the nonce */
	challenge[HUBBLE_CHALLENGE_SIZE - 1] ^= 0x01;
	zassert_equal(check(challenge, response), -EACCES);
}

ZTEST(challenge_test, test_period_change)
{
	uint8_t challenge[HUBBLE_CHALLENGE_SIZE];
	uint8_t response[HUBBLE_CHALLENGE_RESPONSE_SIZE];
	uint8_t next[HUBBLE_CHALLENGE_SIZE];
	uint32_t counter;
	uint32_t next_counter;

	challenge_get(challenge, &counter);
	expected_response(counter, challenge, response);

	period_set(1);
	zassert_equal(check(challenge, response), -ESTALE);

	challenge_get(next, &next_counter);
	zassert_not_equal(counter, next_counter);
	zassert_true(memcmp(&challenge[1], &next[1], TEST_EID_SIZE) != 0,
		     "EID did not rotate");

	expected_response(next_counter, next, response);
	zassert_ok(check(next, response));
}

ZTEST(challenge_test, test_invalid_args)
{
	uint8_t challenge[HUBBLE_CHALLENGE_SIZE];
	uint8_t response[HUBBLE_CHALLENGE_RESPONSE_SIZE];
	size_t len = HUBBLE_CHALLENGE_SIZE;
	size_t short_len = HUBBLE_CHALLENGE_SIZE - 1;
	uint32_t counter;

	zassert_equal(
		hubble_challenge_get(NULL, sizeof(test_nonce), challenge, &len),
		-EINVAL);
	zassert_equal(hubble_challenge_get(test_nonce, sizeof(test_nonce) - 1,
					   challenge, &len),
		      -EINVAL);
	zassert_equal(
		hubble_challenge_get(test_nonce, sizeof(test_nonce), NULL, &len),
		-EINVAL);
	zassert_equal(hubble_challenge_get(test_nonce, sizeof(test_nonce),
					   challenge, NULL),
		      -EINVAL);
	zassert_equal(hubble_challenge_get(test_nonce, sizeof(test_nonce),
					   challenge, &short_len),
		      -EINVAL);

	challenge_get(challenge, &counter);
	expected_response(counter, challenge, response);

	zassert_equal(hubble_challenge_response_check(NULL, sizeof(challenge),
						      response, sizeof(response)),
		      -EINVAL);
	zassert_equal(
		hubble_challenge_response_check(challenge, sizeof(challenge) - 1,
						response, sizeof(response)),
		-EINVAL);
	zassert_equal(
		hubble_challenge_response_check(challenge, sizeof(challenge),
						NULL, sizeof(response)),
		-EINVAL);
	zassert_equal(
		hubble_challenge_response_check(challenge, sizeof(challenge),
						response, sizeof(response) - 1),
		-EINVAL);

	challenge[0] = HUBBLE_CHALLENGE_VERSION + 1U;
	zassert_equal(check(challenge, response), -EINVAL);
}

static void challenge_test_before(void *fixture)
{
	ARG_UNUSED(fixture);

#ifdef CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME
	zassert_ok(hubble_init(0, test_key));
#else
	zassert_ok(hubble_init(TEST_UNIX_TIME_MS, test_key));
#endif
}

ZTEST_SUITE(challenge_test, NULL, NULL, challenge_test_before, NULL, NULL);
