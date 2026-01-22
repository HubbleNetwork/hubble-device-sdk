/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Counter-based EID test vectors (pool_size=32).
 *
 * Master key: cd:15:a5:ab:c0:60:b6:72:88:a6:1e:44:e9:95:ba:77:
 *             d1:40:bd:46:56:4b:88:de:41:c1:5a:92:73:b0:ce:85
 *
 * Generated with: python tools/ble-adv.py --time-counter <N> --seq-no <S>
 *                        --payload-hex <hex> --print key.bin
 */

#include "test_vectors_counter.h"

/* TV1: counter=0, seq=0, empty payload */
static const uint8_t ctv1_payload[] = {};

static const uint8_t ctv1_expected[] = {0xa6, 0xfc, 0x00, 0x00, 0xb2, 0x12,
					0x55, 0xd9, 0x94, 0x1c, 0x61, 0x36};

/* TV2: counter=0, seq=1, single byte 0xaa */
static const uint8_t ctv2_payload[] = {0xaa};

static const uint8_t ctv2_expected[] = {0xa6, 0xfc, 0x00, 0x01, 0xb2,
					0x12, 0x55, 0xd9, 0x7f, 0x23,
					0x24, 0x09, 0x7b};

/* TV3: counter=1, seq=0, "Hello" */
static const uint8_t ctv3_payload[] = {0x48, 0x65, 0x6c, 0x6c, 0x6f};

static const uint8_t ctv3_expected[] = {0xa6, 0xfc, 0x00, 0x00, 0xc9, 0xf3,
					0x09, 0xbc, 0x76, 0x5a, 0x2f, 0x55,
					0xa7, 0x97, 0x67, 0x62, 0xb7};

/* TV4: counter=5, seq=100, 0xdeadbeef */
static const uint8_t ctv4_payload[] = {0xde, 0xad, 0xbe, 0xef};

static const uint8_t ctv4_expected[] = {0xa6, 0xfc, 0x00, 0x64, 0x11, 0x31,
					0x17, 0x91, 0xb9, 0x9e, 0x2c, 0xf2,
					0xd2, 0x28, 0xdd, 0x8e};

/* TV5: counter=15, seq=0, 8 zero bytes */
static const uint8_t ctv5_payload[] = {0x00, 0x00, 0x00, 0x00,
				       0x00, 0x00, 0x00, 0x00};

static const uint8_t ctv5_expected[] = {
	0xa6, 0xfc, 0x00, 0x00, 0x17, 0x18, 0x8e, 0x9c, 0x0d, 0x42,
	0x9b, 0x21, 0x16, 0x3f, 0xdf, 0xd1, 0x6b, 0xd9, 0xef, 0x9e};

/* TV6: counter=31, seq=1023, 13 bytes (max payload) */
static const uint8_t ctv6_payload[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
				       0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c};

static const uint8_t ctv6_expected[] = {
	0xa6, 0xfc, 0x03, 0xff, 0xcd, 0xef, 0xfb, 0x5a, 0xeb,
	0x90, 0xc4, 0xf5, 0xff, 0x18, 0x87, 0x96, 0x73, 0xa6,
	0x6a, 0x52, 0xc0, 0xeb, 0x64, 0x67, 0xb6};

/* TV7: counter=31, seq=42, "Test123" */
static const uint8_t ctv7_payload[] = {0x54, 0x65, 0x73, 0x74,
				       0x31, 0x32, 0x33};

static const uint8_t ctv7_expected[] = {
	0xa6, 0xfc, 0x00, 0x2a, 0xcd, 0xef, 0xfb, 0x5a, 0x50, 0x17,
	0xfc, 0xf1, 0x2c, 0xd8, 0xb7, 0xd0, 0x64, 0x66, 0x57};

const struct ble_adv_test_vector counter_test_vectors[] = {
	{.description = "Counter=0, empty payload",
	 .time_counter = 0,
	 .seq_no = 0,
	 .payload = ctv1_payload,
	 .payload_len = 0,
	 .expected = ctv1_expected,
	 .expected_len = sizeof(ctv1_expected)},
	{.description = "Counter=0, single byte",
	 .time_counter = 0,
	 .seq_no = 1,
	 .payload = ctv2_payload,
	 .payload_len = sizeof(ctv2_payload),
	 .expected = ctv2_expected,
	 .expected_len = sizeof(ctv2_expected)},
	{.description = "Counter=1, Hello",
	 .time_counter = 1,
	 .seq_no = 0,
	 .payload = ctv3_payload,
	 .payload_len = sizeof(ctv3_payload),
	 .expected = ctv3_expected,
	 .expected_len = sizeof(ctv3_expected)},
	{.description = "Counter=5, deadbeef",
	 .time_counter = 5,
	 .seq_no = 100,
	 .payload = ctv4_payload,
	 .payload_len = sizeof(ctv4_payload),
	 .expected = ctv4_expected,
	 .expected_len = sizeof(ctv4_expected)},
	{.description = "Counter=15, 8 zeros",
	 .time_counter = 15,
	 .seq_no = 0,
	 .payload = ctv5_payload,
	 .payload_len = sizeof(ctv5_payload),
	 .expected = ctv5_expected,
	 .expected_len = sizeof(ctv5_expected)},
	{.description = "Counter=31, max payload",
	 .time_counter = 31,
	 .seq_no = 1023,
	 .payload = ctv6_payload,
	 .payload_len = sizeof(ctv6_payload),
	 .expected = ctv6_expected,
	 .expected_len = sizeof(ctv6_expected)},
	{.description = "Counter=31, Test123",
	 .time_counter = 31,
	 .seq_no = 42,
	 .payload = ctv7_payload,
	 .payload_len = sizeof(ctv7_payload),
	 .expected = ctv7_expected,
	 .expected_len = sizeof(ctv7_expected)},
};

const size_t counter_test_vectors_count =
	sizeof(counter_test_vectors) / sizeof(counter_test_vectors[0]);
