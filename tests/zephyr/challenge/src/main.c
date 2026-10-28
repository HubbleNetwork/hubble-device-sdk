/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hubble/hubble.h>
#include <hubble/challenge.h>

#include <zephyr/ztest.h>

#include <string.h>

#define TEST_UNIX_TIME_MS 1760210751803ULL
#define TEST_PERIOD_MS                                                         \
	((uint64_t)CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC * 1000ULL)
#define TEST_ADV_BUFFER_SZ  31
/* Offset of the device id in the advertisement: UUID (2) + seq_no (2) */
#define TEST_ADV_EID_OFFSET 4
/* Challenge layout: version (1), EID (4), nonce */
#define TEST_EID_OFFSET     1
#define TEST_EID_SIZE       4

BUILD_ASSERT(CONFIG_HUBBLE_KEY_SIZE == 32, "test vectors use a 256-bit key");

/* zRWlq8BgtnKIph5E6ZW6d9FAvUZWS4jeQcFaknOwzoU= */
static const uint8_t test_key[CONFIG_HUBBLE_KEY_SIZE] = {
	0xcd, 0x15, 0xa5, 0xab, 0xc0, 0x60, 0xb6, 0x72, 0x88, 0xa6, 0x1e,
	0x44, 0xe9, 0x95, 0xba, 0x77, 0xd1, 0x40, 0xbd, 0x46, 0x56, 0x4b,
	0x88, 0xde, 0x41, 0xc1, 0x5a, 0x92, 0x73, 0xb0, 0xce, 0x85};

static const uint8_t test_nonce[HUBBLE_CHALLENGE_NONCE_SIZE] = {
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
};

struct test_vector {
	uint8_t challenge[HUBBLE_CHALLENGE_SIZE];
	uint8_t response[HUBBLE_CHALLENGE_RESPONSE_SIZE];
};

/* Challenge and response for test_key and test_nonce in the base time
 * counter period and the next one.
 */
static const struct test_vector test_vectors[] = {
#ifdef CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME
	/* time counter 0 */
	{
		{0x00, 0xb2, 0x12, 0x55, 0xd9, 0xa0, 0xa1,
		 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
		 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf},
		{0x91, 0x37, 0x04, 0xa4, 0x68, 0x53, 0x58, 0x6f, 0x16, 0xfb,
		 0xcf, 0xce, 0x80, 0x5b, 0xe9, 0x24},
	},
	/* time counter 1 */
	{
		{0x00, 0xc9, 0xf3, 0x09, 0xbc, 0xa0, 0xa1,
		 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
		 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf},
		{0x31, 0x97, 0x72, 0xea, 0xd5, 0xc4, 0xa1, 0xbd, 0x64, 0x88,
		 0x6c, 0x16, 0x0f, 0x18, 0xcb, 0xda},
	},
#else
	/* time counter 20372 */
	{
		{0x00, 0xc0, 0x48, 0xb6, 0x33, 0xa0, 0xa1,
		 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
		 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf},
		{0xa6, 0xad, 0x0c, 0xbd, 0x49, 0x19, 0x69, 0x5f, 0x35, 0x85,
		 0x21, 0x03, 0xa3, 0x2c, 0x79, 0x0b},
	},
	/* time counter 20373 */
	{
		{0x00, 0x29, 0xb6, 0xe7, 0x8f, 0xa0, 0xa1,
		 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
		 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf},
		{0x87, 0xa2, 0x15, 0x80, 0xe1, 0x9c, 0xeb, 0xdf, 0xfe, 0x96,
		 0xb5, 0x5d, 0x85, 0x2f, 0xa9, 0x0d},
	},
#endif
};

/* Move the time counter by the given number of periods from the base. */
static void period_set(uint32_t period)
{
#ifdef CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME
	zassert_ok(hubble_init(period, test_key));
#else
	zassert_ok(hubble_time_set(TEST_UNIX_TIME_MS + period * TEST_PERIOD_MS));
#endif
}

static int check(const uint8_t *challenge, const uint8_t *response)
{
	return hubble_challenge_response_check(challenge, HUBBLE_CHALLENGE_SIZE,
					       response,
					       HUBBLE_CHALLENGE_RESPONSE_SIZE);
}

ZTEST(challenge_test, test_vectors)
{
	for (uint32_t i = 0U; i < ARRAY_SIZE(test_vectors); i++) {
		uint8_t challenge[HUBBLE_CHALLENGE_SIZE];
		size_t len = sizeof(challenge);

		period_set(i);

		zassert_ok(hubble_challenge_get(test_nonce, sizeof(test_nonce),
						challenge, &len));
		zassert_equal(len, HUBBLE_CHALLENGE_SIZE);
		zassert_mem_equal(challenge, test_vectors[i].challenge,
				  HUBBLE_CHALLENGE_SIZE, "challenge %u", i);
		zassert_ok(check(test_vectors[i].challenge,
				 test_vectors[i].response),
			   "response %u", i);
	}
}

ZTEST(challenge_test, test_eid_matches_advertisement)
{
	uint8_t challenge[HUBBLE_CHALLENGE_SIZE];
	size_t len = sizeof(challenge);
	uint8_t adv[TEST_ADV_BUFFER_SZ];
	size_t adv_len = sizeof(adv);

	zassert_ok(hubble_challenge_get(test_nonce, sizeof(test_nonce),
					challenge, &len));
	zassert_ok(hubble_ble_advertise_get(NULL, 0, adv, &adv_len));
	zassert_mem_equal(&adv[TEST_ADV_EID_OFFSET],
			  &challenge[TEST_EID_OFFSET], TEST_EID_SIZE);
}

ZTEST(challenge_test, test_wrong_response)
{
	struct test_vector v = test_vectors[0];

	v.response[HUBBLE_CHALLENGE_RESPONSE_SIZE - 1] ^= 0x01;
	zassert_equal(check(v.challenge, v.response), -EACCES);
	v.response[HUBBLE_CHALLENGE_RESPONSE_SIZE - 1] ^= 0x01;

	/* The response covers the nonce */
	v.challenge[HUBBLE_CHALLENGE_SIZE - 1] ^= 0x01;
	zassert_equal(check(v.challenge, v.response), -EACCES);
}

ZTEST(challenge_test, test_period_change)
{
	period_set(1);

	zassert_equal(check(test_vectors[0].challenge, test_vectors[0].response),
		      -ETIMEDOUT);
	zassert_ok(check(test_vectors[1].challenge, test_vectors[1].response));
}

ZTEST(challenge_test, test_invalid_args)
{
	struct test_vector v = test_vectors[0];
	uint8_t challenge[HUBBLE_CHALLENGE_SIZE];
	size_t len = HUBBLE_CHALLENGE_SIZE;
	size_t short_len = HUBBLE_CHALLENGE_SIZE - 1;

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

	zassert_equal(
		hubble_challenge_response_check(NULL, sizeof(v.challenge),
						v.response, sizeof(v.response)),
		-EINVAL);
	zassert_equal(hubble_challenge_response_check(
			      v.challenge, sizeof(v.challenge) - 1, v.response,
			      sizeof(v.response)),
		      -EINVAL);
	zassert_equal(hubble_challenge_response_check(v.challenge,
						      sizeof(v.challenge), NULL,
						      sizeof(v.response)),
		      -EINVAL);
	zassert_equal(hubble_challenge_response_check(
			      v.challenge, sizeof(v.challenge), v.response,
			      sizeof(v.response) - 1),
		      -EINVAL);

	v.challenge[0] = HUBBLE_CHALLENGE_VERSION + 1U;
	zassert_equal(check(v.challenge, v.response), -EINVAL);
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
