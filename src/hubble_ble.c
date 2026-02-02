/*
 * Copyright (c) 2024 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <hubble/ble.h>
#include <hubble/port/sys.h>
#include <hubble/port/crypto.h>

#include "hubble_priv.h"
#include "utils/macros.h"

#define HUBBLE_BLE_ADVERTISE_PREFIX 2
#define HUBBLE_BLE_PROTOCOL_VERSION 0b000000
#define HUBBLE_BLE_ADDR_SIZE        6
#define HUBBLE_BLE_AUTH_TAG_SIZE    4U
#define HUBBLE_BLE_ADV_FIELDS_SIZE                                             \
	(HUBBLE_BLE_ADVERTISE_PREFIX + HUBBLE_BLE_ADDR_SIZE +                  \
	 HUBBLE_BLE_AUTH_TAG_SIZE)

/* Define some helpers for payload offsets */
#define _PAYLOAD_SERVICE_UUID_LO(buf) (buf + 0)
#define _PAYLOAD_SERVICE_UUID_HI(buf) (buf + 1)
#define _PAYLOAD_ADDR(buf)            (buf + HUBBLE_BLE_ADVERTISE_PREFIX)
#define _PAYLOAD_AUTH_TAG(buf)        ((_PAYLOAD_ADDR(buf)) + HUBBLE_BLE_ADDR_SIZE)
#define _PAYLOAD_DATA(buf)            ((_PAYLOAD_AUTH_TAG(buf)) + HUBBLE_BLE_AUTH_TAG_SIZE)

static void _addr_set(uint8_t *addr, uint16_t seq_no, uint32_t device_id)
{
	uint8_t seq_no_first_2bits = (seq_no >> 8) & 0x03;
	uint8_t seq_no_last_8bits = seq_no & 0xFF;

	addr[0] = HUBBLE_BLE_PROTOCOL_VERSION | seq_no_first_2bits;
	addr[1] = seq_no_last_8bits;

	memcpy((addr + 2), &device_id, sizeof(device_id));
}

int hubble_ble_advertise_get(const uint8_t *input, size_t input_len,
			     uint8_t *out, size_t *out_len)
{
	int err;
	uint32_t device_id;
	uint16_t seq_no;
	const void *master_key = hubble_internal_key_get();
	uint32_t time_counter = hubble_internal_time_counter_get();

	if ((master_key == NULL) || (out == NULL) || (out_len == NULL)) {
		return -EINVAL;
	}

	if ((input == NULL) && (input_len > 0)) {
		return -EINVAL;
	}

	if (input_len > HUBBLE_BLE_MAX_DATA_LEN) {
		return -EINVAL;
	}

	if (input_len + HUBBLE_BLE_ADV_FIELDS_SIZE > *out_len) {
		return -EINVAL;
	}

	seq_no = hubble_sequence_counter_get();

	if (!hubble_internal_nonce_values_check(time_counter, seq_no)) {
		HUBBLE_LOG_WARNING("Re-using same nonce is insecure !");
		return -EPERM;
	}

	// Set the constant data
	*_PAYLOAD_SERVICE_UUID_LO(out) = HUBBLE_LO_UINT16(HUBBLE_BLE_UUID);
	*_PAYLOAD_SERVICE_UUID_HI(out) = HUBBLE_HI_UINT16(HUBBLE_BLE_UUID);

	err = hubble_internal_device_id_get((uint8_t *)&device_id,
					    sizeof(device_id), time_counter);
	if (err) {
		return err;
	}
	_addr_set(_PAYLOAD_ADDR(out), seq_no, device_id);

	err = hubble_internal_data_encrypt(
		time_counter, seq_no, input, input_len, _PAYLOAD_DATA(out),
		_PAYLOAD_AUTH_TAG(out), HUBBLE_BLE_AUTH_TAG_SIZE);
	if (err) {
		return err;
	}

	*out_len = HUBBLE_BLE_ADVERTISE_PREFIX + HUBBLE_BLE_ADDR_SIZE +
		   HUBBLE_BLE_AUTH_TAG_SIZE + input_len;

	return err;
}
