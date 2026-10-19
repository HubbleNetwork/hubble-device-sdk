/*
 * Copyright (c) 2025 Hubble Network
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hubble/hubble.h>
#include <hubble/port/crypto.h>
#include <hubble/port/sys.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/ztest.h>

/* Period offset from the base time counter to move to before the entry,
 * NONCE_SAME_PERIOD to stay.
 */
#define NONCE_SAME_PERIOD UINT8_MAX

struct test_nonce {
	uint8_t period;
	uint16_t nonce;
	bool valid;
};

#define TEST_ADV_BUFFER_SZ 31

/* The nonce check keeps its state across test suites. Suites run in name
 * order, so ble_adv_test has already used BLE_ADV_UNIX_TIME when this suite
 * starts. Its base time is earlier, which makes the first period one that is
 * joined midway (time moved backwards) whether or not another suite ran first.
 */
#define BLE_NONCE_UNIX_TIME 1760037951803ULL
#define BLE_ADV_UNIX_TIME   1760210751803ULL

/* In Unix time mode this suite uses periods 0 and 1 from its base time. */
#define NONCE_PERIOD_OF(_ms)                                                   \
	((_ms) / ((uint64_t)CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC * 1000ULL))

BUILD_ASSERT((NONCE_PERIOD_OF(BLE_NONCE_UNIX_TIME) + 1ULL) <
		     NONCE_PERIOD_OF(BLE_ADV_UNIX_TIME),
	     "nonce suite periods must precede the adv suite period");

static uint8_t ble_nonce_key[CONFIG_HUBBLE_KEY_SIZE] = {};
static uint16_t nonce_idx;
static bool testing_adv;

#ifdef CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME
/* Uptime cannot move backwards, the third period is another step forward. */
#define NONCE_THIRD_PERIOD     2
#define NONCE_THIRD_WRAP_VALID true
#else
/* Time set back to the first period: it is joined midway again. */
#define NONCE_THIRD_PERIOD     0
#define NONCE_THIRD_WRAP_VALID false
#endif

struct test_nonce invalid_nonce_sequence[] = {
	/* First period, joined midway (fresh boot or time moved backwards).
	 * Let start with something != 0 to check wraps.
	 */
	{0, 10, true},
	{NONCE_SAME_PERIOD, 10, false},   /* repeated nonce, should fail */
	{NONCE_SAME_PERIOD, 11, true},    /* Working nonce */
	{NONCE_SAME_PERIOD, 1023, true},  /* Test the boundary */
	{NONCE_SAME_PERIOD, 1024, false}, /* Test the boundary + 1 */
	/* The period was joined midway, lower values may have been used
	 * before: wrapping is not allowed.
	 */
	{NONCE_SAME_PERIOD, 0, false},
	{NONCE_SAME_PERIOD, 8, false},
	/* Next period, its start is observed so wrapping below the first
	 * value is allowed.
	 */
	{1, 20, true},
	{NONCE_SAME_PERIOD, 1023, true},
	{NONCE_SAME_PERIOD, 0, true},   /* Wrap below the reference */
	{NONCE_SAME_PERIOD, 19, true},  /* still below the reference */
	{NONCE_SAME_PERIOD, 20, false}, /* Already used */
	{NONCE_SAME_PERIOD, 5, false},  /* Second wrap */
	/* Third period */
	{NONCE_THIRD_PERIOD, 5, true},
	{NONCE_SAME_PERIOD, 1023, true},
	{NONCE_SAME_PERIOD, 0, NONCE_THIRD_WRAP_VALID},
};

uint16_t hubble_sequence_counter_get(void)
{
	if (!testing_adv) {
		return invalid_nonce_sequence[nonce_idx++].nonce;
	}

	return nonce_idx++ % HUBBLE_MAX_SEQ_COUNTER;
}

/* Move the time counter to the given period offset from the base one. */
static void nonce_period_set(uint8_t period)
{
#ifdef CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME
	zassert_ok(hubble_init(period, ble_nonce_key));
#else
	zassert_ok(hubble_time_set(
		BLE_NONCE_UNIX_TIME +
		((uint64_t)period * CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC *
		 1000ULL)));
#endif
}

ZTEST(ble_nonce_test, test_ble_nonce_invalid)
{
	uint16_t idx;

	for (idx = 0; idx < ARRAY_SIZE(invalid_nonce_sequence); idx++) {
		uint8_t buf[TEST_ADV_BUFFER_SZ];
		size_t out_len = sizeof(buf);
		int status;

		if (invalid_nonce_sequence[idx].period != NONCE_SAME_PERIOD) {
			nonce_period_set(invalid_nonce_sequence[idx].period);
		}

		status = hubble_ble_advertise_get(NULL, 0, buf, &out_len);

		if (invalid_nonce_sequence[idx].valid) {
			zassert_ok(status, "entry %u rejected", idx);
		} else {
			zassert_not_ok(status, "entry %u accepted", idx);
		}
	}
}

static void *ble_nonce_test_setup(void)
{
#ifdef CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME
	(void)hubble_init(0, ble_nonce_key);
#else
	(void)hubble_init(BLE_NONCE_UNIX_TIME, ble_nonce_key);
#endif

	testing_adv = false;
	nonce_idx = 0U;

	return NULL;
}

ZTEST_SUITE(ble_nonce_test, NULL, ble_nonce_test_setup, NULL, NULL, NULL);

/* Adv test section */

/* zRWlq8BgtnKIph5E6ZW6d9FAvUZWS4jeQcFaknOwzoU= */
static uint8_t ble_adv_key[CONFIG_HUBBLE_KEY_SIZE] = {
	0xcd, 0x15, 0xa5, 0xab, 0xc0, 0x60, 0xb6, 0x72, 0x88, 0xa6, 0x1e,
	0x44, 0xe9, 0x95, 0xba, 0x77, 0xd1, 0x40, 0xbd, 0x46, 0x56, 0x4b,
	0x88, 0xde, 0x41, 0xc1, 0x5a, 0x92, 0x73, 0xb0, 0xce, 0x85};

#define ADV_TEST_BUFFER_SIZE 32

struct test_adv {
	uint8_t input[ADV_TEST_BUFFER_SIZE];
	uint8_t input_len;
	uint8_t output[ADV_TEST_BUFFER_SIZE];
};

struct test_adv test_adv_data[] = {
	{{},
	 0,
	 {0xa6, 0xfc, 0x00, 0x00, 0xc0, 0x48, 0xb6, 0x33, 0x7f, 0x4f, 0x35, 0xbb}},
	{{0xde, 0xad, 0xbe, 0xef},
	 4,
	 {0xa6, 0xfc, 0x00, 0x01, 0xc0, 0x48, 0xb6, 0x33, 0x45, 0xa8, 0xae,
	  0xc6, 0xc0, 0x2e, 0xac, 0xf0}},
};

ZTEST(ble_adv_test, test_ble_adv)
{
#ifdef CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME
	/* Skip byte-exact comparison in counter mode - protocol version differs.
	 * Counter mode advertisement generation is tested in ble_counter_test.
	 */
	ztest_test_skip();
#else
	uint16_t idx;

	for (idx = 0; idx < ARRAY_SIZE(test_adv_data); idx++) {
		uint8_t buf[TEST_ADV_BUFFER_SZ];
		size_t out_len = sizeof(buf);
		int status = hubble_ble_advertise_get(
			test_adv_data[idx].input, test_adv_data[idx].input_len,
			buf, &out_len);
		zassert_ok(status);
		zassert_mem_equal(buf, test_adv_data[idx].output, out_len);
	}
#endif
}

ZTEST(ble_adv_test, test_adv_get_overflow)
{
	uint8_t buf[TEST_ADV_BUFFER_SZ];
	uint8_t in[HUBBLE_BLE_MAX_DATA_LEN + 1] = {};
	size_t out_len = sizeof(buf);
	int status = hubble_ble_advertise_get(in, HUBBLE_BLE_MAX_DATA_LEN + 1,
					      buf, &out_len);
	zassert_not_ok(status);
}

static void *ble_adv_test_setup(void)
{
	(void)hubble_init(BLE_ADV_UNIX_TIME, ble_adv_key);

	testing_adv = true;
	nonce_idx = 0U;

	return NULL;
}

ZTEST_SUITE(ble_adv_test, NULL, ble_adv_test_setup, NULL, NULL, NULL);

/* Counter-based EID test section */

#ifdef CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME

static uint8_t counter_test_key[CONFIG_HUBBLE_KEY_SIZE] = {
	0xcd, 0x15, 0xa5, 0xab, 0xc0, 0x60, 0xb6, 0x72, 0x88, 0xa6, 0x1e,
	0x44, 0xe9, 0x95, 0xba, 0x77, 0xd1, 0x40, 0xbd, 0x46, 0x56, 0x4b,
	0x88, 0xde, 0x41, 0xc1, 0x5a, 0x92, 0x73, 0xb0, 0xce, 0x85};

ZTEST(ble_counter_test, test_counter_init_zero)
{
	/* In counter mode, 0 is a valid initial counter value */
	int ret = hubble_init(0, counter_test_key);

	zassert_ok(ret, "hubble_init with counter=0 should succeed");
}

ZTEST(ble_counter_test, test_counter_init_nonzero)
{
	/* In counter mode, non-zero initial counter should work */
	int ret = hubble_init(100, counter_test_key);

	zassert_ok(ret, "hubble_init with counter=100 should succeed");
}

ZTEST(ble_counter_test, test_eid_counter_get)
{
	/* Initialize with counter=0 */
	int ret = hubble_init(0, counter_test_key);

	zassert_ok(ret, "hubble_init should succeed");

	/* At startup with counter=0, the counter should be 0 */
	uint32_t counter;
	ret = hubble_counter_get(&counter);

	zassert_ok(ret, "hubble_counter_get should succeed");

	zassert_equal(counter, 0, "Counter should be 0 at startup");
}

ZTEST(ble_counter_test, test_eid_counter_get_with_initial)
{
	/* Initialize with counter=50 */
	const uint64_t initial_counter = 50;
	int ret = hubble_init(initial_counter, counter_test_key);

	zassert_ok(ret, "hubble_init should succeed");

	uint32_t counter;
	ret = hubble_counter_get(&counter);

	zassert_ok(ret, "hubble_counter_get should succeed");

	zassert_equal(initial_counter, counter,
		      "Counter value should match initial counter value");
}

ZTEST(ble_counter_test, test_time_to_next_rotation)
{
	int ret = hubble_init(0, counter_test_key);

	zassert_ok(ret, "hubble_init should succeed");

	uint32_t time_to_rotation = hubble_ble_advertise_expiration_get();
	uint32_t rotation_period_ms =
		CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC * 1000U;

	/* Time to next rotation should be <= rotation period */
	zassert_true(time_to_rotation <= rotation_period_ms,
		     "Time to rotation %u should be <= %u", time_to_rotation,
		     rotation_period_ms);

	/* Time to next rotation should be > 0 (unless exactly at boundary) */
	zassert_true(
		time_to_rotation > 0 || time_to_rotation == rotation_period_ms,
		"Time to rotation should be valid");
}

ZTEST(ble_counter_test, test_counter_adv_generation)
{
	int ret = hubble_init(0, counter_test_key);

	zassert_ok(ret, "hubble_init should succeed");

	testing_adv = true;
	nonce_idx = 0U;

	/* Advertisement generation should work in counter mode */
	uint8_t buf[TEST_ADV_BUFFER_SZ];
	size_t out_len = sizeof(buf);
	int status = hubble_ble_advertise_get(NULL, 0, buf, &out_len);

	zassert_ok(status, "Advertisement generation should succeed");
	zassert_true(out_len > 0, "Output length should be > 0");
}

static void *ble_counter_test_setup(void)
{
	testing_adv = false;
	nonce_idx = 0U;

	return NULL;
}

ZTEST_SUITE(ble_counter_test, NULL, ble_counter_test_setup, NULL, NULL, NULL);

#endif /* CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME */
