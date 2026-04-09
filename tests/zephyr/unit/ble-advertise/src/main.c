/*
 * Copyright (c) 2025 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hubble/hubble.h>
#include <hubble/ble.h>
#include <hubble/port/sys.h>
#include <hubble/port/crypto.h>

#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/ztest.h>

#include <string.h>

#include "test_vectors.h"

#define TEST_ADV_BUFFER_SZ      31
#define TIMER_COUNTER_FREQUENCY 86400000ULL

/* Test sequence counter override */
static uint16_t test_seq_override;

/* A arbitrarily chosen start time for the test */
static const uint64_t test_unix_time = (uint64_t)20 * TIMER_COUNTER_FREQUENCY;

/* Test uptime override */
static uint64_t test_uptime_ms;

uint16_t hubble_sequence_counter_get(void)
{
	return test_seq_override;
}

uint64_t hubble_uptime_get(void)
{
	return test_uptime_ms;
}

/* Test keys */
static const uint8_t test_key_primary[HUBBLE_KEY_SIZE_BYTES] = {
	0xcd, 0x15, 0xa5, 0xab, 0xc0, 0x60, 0xb6, 0x72, 0x88, 0xa6, 0x1e,
	0x44, 0xe9, 0x95, 0xba, 0x77, 0xd1, 0x40, 0xbd, 0x46, 0x56, 0x4b,
	0x88, 0xde, 0x41, 0xc1, 0x5a, 0x92, 0x73, 0xb0, 0xce, 0x85};

/*===========================================================================*/
/* Test Suite: ble_advertise_test - Core Encryption Tests                   */
/*===========================================================================*/

#ifndef CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME
ZTEST(ble_advertise_test, test_advertise_with_test_vectors)
{
	test_uptime_ms = 0;
	for (size_t i = 0; i < test_vectors_count; i++) {
		const struct ble_adv_test_vector *tv = &test_vectors[i];

		uint64_t unix_time =
			(uint64_t)tv->time_counter * TIMER_COUNTER_FREQUENCY;

		int ret = hubble_init(unix_time, test_key_primary);
		zassert_ok(ret, "hubble_init failed");
		test_seq_override = tv->seq_no;

		uint8_t output[TEST_ADV_BUFFER_SZ];
		size_t output_len = sizeof(output);

		ret = hubble_ble_advertise_get(tv->payload, tv->payload_len,
					       output, &output_len);

		zassert_ok(ret, "Vector %zu (%s) failed with error %d", i,
			   tv->description, ret);

		zassert_equal(
			output_len,
			tv->expected_len, "Vector %zu (%s) length mismatch: got %zu, expected %zu",
			i, tv->description, output_len, tv->expected_len);

		zassert_mem_equal(output, tv->expected, output_len,
				  "Vector %zu (%s) output mismatch", i,
				  tv->description);
	}
}
#endif /* !CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME */

ZTEST(ble_advertise_test, test_advertise_null_input_handling)
{
	int ret = hubble_init(test_unix_time, test_key_primary);
	zassert_ok(ret, "hubble_init failed");
	test_seq_override = 0;

	uint8_t output[TEST_ADV_BUFFER_SZ];
	size_t output_len = sizeof(output);

	/* NULL input with zero length should succeed (empty payload is valid) */
	ret = hubble_ble_advertise_get(NULL, 0, output, &output_len);
	zassert_ok(ret, "NULL payload with length 0 should succeed");

	/* NULL output buffer should fail */
	ret = hubble_ble_advertise_get(NULL, 0, NULL, &output_len);
	zassert_equal(ret, -EINVAL, "NULL output buffer should return -EINVAL");

	/* NULL out_len should fail */
	ret = hubble_ble_advertise_get(NULL, 0, output, NULL);
	zassert_equal(ret, -EINVAL, "NULL out_len should return -EINVAL");

	/* NULL input payload with non-zero length should fail */
	output_len = sizeof(output);
	ret = hubble_ble_advertise_get(NULL, 5, output, &output_len);
	zassert_equal(
		ret, -EINVAL,
		"NULL payload with non-zero length should return -EINVAL");
}

ZTEST(ble_advertise_test, test_advertise_buffer_too_small)
{
	uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};

	int ret = hubble_init(test_unix_time, test_key_primary);
	zassert_ok(ret, "hubble_init failed");
	test_seq_override = 0;

	uint8_t output[8];
	size_t output_len = sizeof(output);

	/* Buffer too small for output */
	ret = hubble_ble_advertise_get(payload, sizeof(payload), output,
				       &output_len);
	zassert_equal(ret, -EINVAL, "Too small buffer should return -EINVAL");
}

ZTEST(ble_advertise_test, test_advertise_payload_size_limits)
{
	int ret = hubble_init(test_unix_time, test_key_primary);
	zassert_ok(ret, "hubble_init failed");
	test_seq_override = 0;

	uint8_t output[TEST_ADV_BUFFER_SZ];
	size_t output_len = sizeof(output);

	/* 13-byte payload should succeed (max = HUBBLE_BLE_MAX_DATA_LEN) */
	uint8_t max_payload[HUBBLE_BLE_MAX_DATA_LEN] = {0};
	ret = hubble_ble_advertise_get(max_payload, sizeof(max_payload), output,
				       &output_len);
	zassert_ok(ret, "Max payload (13 bytes) should succeed");

	/* 14-byte payload should fail */
	uint8_t too_large_payload[HUBBLE_BLE_MAX_DATA_LEN + 1] = {0};
	output_len = sizeof(output);
	ret = hubble_ble_advertise_get(too_large_payload,
				       sizeof(too_large_payload), output,
				       &output_len);
	zassert_equal(ret, -EINVAL, "Payload > 13 bytes should return -EINVAL");
}

ZTEST(ble_advertise_test, test_advertise_deterministic)
{
	uint8_t payload[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};

	int ret = hubble_init(test_unix_time, test_key_primary);
	zassert_ok(ret, "hubble_init failed");
	test_seq_override = 100;

	uint8_t output1[TEST_ADV_BUFFER_SZ];
	size_t output_len1 = sizeof(output1);

	ret = hubble_ble_advertise_get(payload, sizeof(payload), output1,
				       &output_len1);
	zassert_ok(ret);

	/* Call again with identical inputs */
	uint8_t output2[TEST_ADV_BUFFER_SZ];
	size_t output_len2 = sizeof(output2);

	ret = hubble_ble_advertise_get(payload, sizeof(payload), output2,
				       &output_len2);
	zassert_ok(ret);

	/* Verify both outputs are byte-identical */
	zassert_equal(output_len1, output_len2,
		      "Output lengths should be identical");
	zassert_mem_equal(output1, output2, output_len1,
			  "Outputs should be byte-identical (deterministic)");
}

ZTEST(ble_advertise_test, test_advertise_time_dependent_output)
{
	uint8_t payload[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};

	/* Initialize with uptime at 0 */
	test_uptime_ms = 0;
	int ret = hubble_init(test_unix_time, test_key_primary);
	zassert_ok(ret, "hubble_init failed");
	test_seq_override = 100;

	uint8_t output1[TEST_ADV_BUFFER_SZ];
	size_t output_len1 = sizeof(output1);

	ret = hubble_ble_advertise_get(payload, sizeof(payload), output1,
				       &output_len1);
	zassert_ok(ret, "First call failed");

	/* Advance time by one day (TIMER_COUNTER_FREQUENCY ms)
	 * This should change the time_counter and produce different output
	 */
	test_uptime_ms = TIMER_COUNTER_FREQUENCY;

	uint8_t output2[TEST_ADV_BUFFER_SZ];
	size_t output_len2 = sizeof(output2);

	ret = hubble_ble_advertise_get(payload, sizeof(payload), output2,
				       &output_len2);
	zassert_ok(ret, "Second call failed");

	/* Outputs should have the same length but different content
	 * due to different time_counter values
	 */
	zassert_equal(output_len1, output_len2, "Output lengths should match");
	zassert_true(memcmp(output1, output2, output_len1) != 0,
		     "Outputs should differ due to time progression");
}

ZTEST(ble_advertise_test, test_advertise_same_time_different_sequences)
{
	uint8_t payload[] = {0xAA, 0xBB};

	int ret = hubble_init(test_unix_time, test_key_primary);
	zassert_ok(ret, "hubble_init failed");

	/* Get output with sequence number 0 */
	test_seq_override = 0;

	uint8_t output1[TEST_ADV_BUFFER_SZ];
	size_t output_len1 = sizeof(output1);

	ret = hubble_ble_advertise_get(payload, sizeof(payload), output1,
				       &output_len1);
	zassert_ok(ret, "First call failed");

	/* Get output with sequence number 1, same time */
	test_seq_override = 1;

	uint8_t output2[TEST_ADV_BUFFER_SZ];
	size_t output_len2 = sizeof(output2);

	ret = hubble_ble_advertise_get(payload, sizeof(payload), output2,
				       &output_len2);
	zassert_ok(ret, "Second call failed");

	/* Outputs should differ due to different sequence numbers */
	zassert_equal(output_len1, output_len2, "Output lengths should match");
	zassert_true(memcmp(output1, output2, output_len1) != 0,
		     "Outputs should differ due to different sequence numbers");
}

static void *ble_advertise_test_setup(void)
{
	test_seq_override = 0;
	test_uptime_ms = 0;
	return NULL;
}

ZTEST_SUITE(ble_advertise_test, NULL, ble_advertise_test_setup, NULL, NULL, NULL);

/*===========================================================================*/
/* Test Suite: ble_advertise_format_test - Output Format Validation         */
/*===========================================================================*/

ZTEST(ble_advertise_format_test, test_service_uuid_present)
{
	uint8_t payload[] = {0xAA, 0xBB, 0xCC};

	int ret = hubble_init(test_unix_time, test_key_primary);
	zassert_ok(ret, "hubble_init failed");
	test_seq_override = 0;

	uint8_t output[TEST_ADV_BUFFER_SZ];
	size_t output_len = sizeof(output);

	ret = hubble_ble_advertise_get(payload, sizeof(payload), output,
				       &output_len);
	zassert_ok(ret);

	/* Service UUID should be first 2 bytes: 0xA6 0xFC (little-endian) */
	zassert_equal(output[0], 0xA6,
		      "First byte should be 0xA6 (UUID low byte)");
	zassert_equal(output[1], 0xFC,
		      "Second byte should be 0xFC (UUID high byte)");
}

ZTEST(ble_advertise_format_test, test_output_length_calculation)
{
	int ret = hubble_init(test_unix_time, test_key_primary);
	zassert_ok(ret, "hubble_init failed");

	/* Test different payload lengths */
	size_t test_payload_lens[] = {0, 1, 5, 13};

	/*
	 * Output length = 2 (UUID) + 2 (ver/seq) + 4 (tag) + 4 (EID)
	 * payload_len = 12 + payload_len
	 */
	const size_t HUBBLE_PAYLOAD_OVERHEAD = 12;

	for (size_t i = 0; i < ARRAY_SIZE(test_payload_lens); i++) {
		size_t payload_len = test_payload_lens[i];
		uint8_t payload[13] = {0};

		test_seq_override = (uint16_t)i;

		uint8_t output[TEST_ADV_BUFFER_SZ];
		size_t output_len = sizeof(output);

		ret = hubble_ble_advertise_get(payload, payload_len, output,
					       &output_len);
		zassert_ok(ret, "Failed for payload length %zu", payload_len);

		size_t expected_len = HUBBLE_PAYLOAD_OVERHEAD + payload_len;
		zassert_equal(
			output_len,
			expected_len, "Length mismatch for payload_len=%zu: got %zu, expected %zu",
			payload_len, output_len, expected_len);
	}
}

ZTEST(ble_advertise_format_test, test_sequence_number_encoding)
{
	int ret = hubble_init(test_unix_time, test_key_primary);
	zassert_ok(ret, "hubble_init failed");

	/* Test various sequence numbers */
	uint16_t test_seq_nos[] = {0, 1, 255, 256, 512, 1023};

	for (size_t i = 0; i < ARRAY_SIZE(test_seq_nos); i++) {
		test_seq_override = test_seq_nos[i];

		uint8_t output[TEST_ADV_BUFFER_SZ];
		size_t output_len = sizeof(output);

		ret = hubble_ble_advertise_get(NULL, 0, output, &output_len);
		zassert_ok(ret);

		/*
		 * Parse sequence number from address field:
		 * Byte 2-3 contain protocol_version (6 bits) + seq_no (10 bits)
		 * Skip first 2 bytes (UUID), then parse bits
		 */
		uint16_t parsed_seq_no = ((output[2] & 0x03) << 8) | output[3];

		zassert_equal(parsed_seq_no, test_seq_nos[i],
			      "Seq_no mismatch: got %u, expected %u",
			      parsed_seq_no, test_seq_nos[i]);
	}
}

static void *ble_advertise_format_test_setup(void)
{
	test_seq_override = 0;
	test_uptime_ms = 0;
	return NULL;
}

ZTEST_SUITE(ble_advertise_format_test, NULL, ble_advertise_format_test_setup,
	    NULL, NULL, NULL);

/*===========================================================================*/
/* Test Suite: ble_advertise_counter_test - Counter-Based EID Tests          */
/*===========================================================================*/

#ifdef CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME

#include "test_vectors_counter.h"

#define TEST_COUNTER_ROTATION_MS                                               \
	((uint64_t)CONFIG_HUBBLE_EID_ROTATION_PERIOD_SEC * 1000ULL)
/* Once pool size is configurable, do not hard code */
#define TEST_COUNTER_POOL_SIZE 128

ZTEST(ble_advertise_counter_test, test_counter_advertise_with_test_vectors)
{
	for (size_t i = 0; i < counter_test_vectors_count; i++) {
		const struct ble_adv_test_vector *tv = &counter_test_vectors[i];

		int ret = hubble_init(tv->time_counter, test_key_primary);
		zassert_ok(ret, "hubble_init failed for vector %zu", i);

		test_uptime_ms = 0;
		test_seq_override = tv->seq_no;

		uint8_t output[TEST_ADV_BUFFER_SZ];
		size_t output_len = sizeof(output);

		ret = hubble_ble_advertise_get(tv->payload, tv->payload_len,
					       output, &output_len);

		zassert_ok(ret, "Vector %zu (%s) failed with error %d", i,
			   tv->description, ret);

		zassert_equal(
			output_len,
			tv->expected_len, "Vector %zu (%s) length mismatch: got %zu, expected %zu",
			i, tv->description, output_len, tv->expected_len);

		zassert_mem_equal(output, tv->expected, output_len,
				  "Vector %zu (%s) output mismatch", i,
				  tv->description);
	}
}

ZTEST(ble_advertise_counter_test, test_counter_init_zero_valid)
{
	/* In counter mode, 0 is a valid initial counter (unlike Unix Epoch mode) */
	int ret = hubble_init(0, test_key_primary);
	zassert_ok(ret, "hubble_init with counter=0 should succeed");
}

ZTEST(ble_advertise_counter_test, test_counter_wraps_at_pool_size)
{
	/* Init at pool_size - 1 (counter=127 for pool_size=128) */
	int ret = hubble_init(TEST_COUNTER_POOL_SIZE - 1, test_key_primary);
	zassert_ok(ret, "hubble_init failed");

	/* Advance one epoch: counter should wrap to 0 */
	test_uptime_ms = TEST_COUNTER_ROTATION_MS;
	test_seq_override = 0;

	uint8_t output_wrapped[TEST_ADV_BUFFER_SZ];
	size_t output_wrapped_len = sizeof(output_wrapped);

	ret = hubble_ble_advertise_get(NULL, 0, output_wrapped,
				       &output_wrapped_len);
	zassert_ok(ret, "advertise_get after wrap failed");

	/* Compare against counter=0 reference (TV1: counter=0, seq=0, empty) */
	ret = hubble_init(0, test_key_primary);
	zassert_ok(ret, "hubble_init for reference failed");

	test_uptime_ms = 0;
	test_seq_override = 0;

	uint8_t output_ref[TEST_ADV_BUFFER_SZ];
	size_t output_ref_len = sizeof(output_ref);

	ret = hubble_ble_advertise_get(NULL, 0, output_ref, &output_ref_len);
	zassert_ok(ret, "advertise_get for reference failed");

	zassert_equal(output_wrapped_len, output_ref_len,
		      "Wrapped output length should match reference");
	zassert_mem_equal(output_wrapped, output_ref, output_wrapped_len,
			  "Wrapped counter should produce same output as "
			  "counter=0");
}

ZTEST(ble_advertise_counter_test, test_counter_advances_with_uptime)
{
	int ret = hubble_init(0, test_key_primary);
	zassert_ok(ret, "hubble_init failed");
	test_seq_override = 0;

	/* Capture output at epoch 0 */
	test_uptime_ms = 0;

	uint8_t output1[TEST_ADV_BUFFER_SZ];
	size_t output_len1 = sizeof(output1);

	ret = hubble_ble_advertise_get(NULL, 0, output1, &output_len1);
	zassert_ok(ret, "First advertise_get failed");

	/* Advance one epoch */
	test_uptime_ms = TEST_COUNTER_ROTATION_MS;

	uint8_t output2[TEST_ADV_BUFFER_SZ];
	size_t output_len2 = sizeof(output2);

	ret = hubble_ble_advertise_get(NULL, 0, output2, &output_len2);
	zassert_ok(ret, "Second advertise_get failed");

	/* Different epochs should produce different output */
	zassert_true(
		memcmp(output1, output2, output_len1) != 0,
		"Different counter epochs should produce different output");
}

ZTEST(ble_advertise_counter_test, test_counter_full_wrap)
{
	/* Init at counter=0, advance by exactly pool_size epochs */
	int ret = hubble_init(0, test_key_primary);
	zassert_ok(ret, "hubble_init failed");
	test_seq_override = 0;

	/* Output at epoch 0 */
	test_uptime_ms = 0;

	uint8_t output_start[TEST_ADV_BUFFER_SZ];
	size_t output_start_len = sizeof(output_start);

	ret = hubble_ble_advertise_get(NULL, 0, output_start, &output_start_len);
	zassert_ok(ret, "advertise_get at start failed");

	/* Advance by full pool size: (0 + 32) % 32 = 0 */
	test_uptime_ms =
		(uint64_t)TEST_COUNTER_POOL_SIZE * TEST_COUNTER_ROTATION_MS;

	uint8_t output_full_wrap[TEST_ADV_BUFFER_SZ];
	size_t output_full_wrap_len = sizeof(output_full_wrap);

	ret = hubble_ble_advertise_get(NULL, 0, output_full_wrap,
				       &output_full_wrap_len);
	zassert_ok(ret, "advertise_get after full wrap failed");

	/* Should match the starting output */
	zassert_equal(output_start_len, output_full_wrap_len,
		      "Full wrap output length should match start");
	zassert_mem_equal(output_start, output_full_wrap, output_start_len,
			  "Full pool wrap should produce same output as start");
}

static void *ble_advertise_counter_test_setup(void)
{
	test_seq_override = 0;
	test_uptime_ms = 0;
	return NULL;
}

ZTEST_SUITE(ble_advertise_counter_test, NULL, ble_advertise_counter_test_setup,
	    NULL, NULL, NULL);

#endif /* CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME */
