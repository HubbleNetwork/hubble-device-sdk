/*
 * Copyright (c) 2025 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <hubble/port/sat_radio.h>
#include <hubble/port/sys.h>
#include <hubble/port/crypto.h>

static uint64_t unix_epoch_synced_ms;
static uint64_t unix_epoch_base_ms;
static const void *master_key;

int hubble_time_set(uint64_t unix_epoch_ms)
{
	if (unix_epoch_ms == 0U) {
		return -EINVAL;
	}

	/* Holds when the device synced Unix epoch time */
	unix_epoch_synced_ms = unix_epoch_ms;

	unix_epoch_base_ms = unix_epoch_ms - hubble_uptime_get();

	return 0;
}

int hubble_key_set(const void *key)
{
	if (key == NULL) {
		return -EINVAL;
	}

	master_key = key;

	return 0;
}

int hubble_init(uint64_t unix_epoch_ms, const void *key)
{
	int ret = hubble_crypto_init();

	if (ret != 0) {
		HUBBLE_LOG_WARNING("Failed to initialize cryptography");
		return ret;
	}

	ret = hubble_time_set(unix_epoch_ms);
	if (ret != 0) {
		HUBBLE_LOG_WARNING("Failed to set Unix epoch time");
		return ret;
	}

	ret = hubble_key_set(key);
	if (ret != 0) {
		HUBBLE_LOG_WARNING("Failed to set key");
		return ret;
	}

#ifdef CONFIG_HUBBLE_SAT_NETWORK
	ret = hubble_sat_port_init();
	if (ret != 0) {
		HUBBLE_LOG_ERROR(
			"Hubble Satellite Network initialization failed");
		return ret;
	}
#endif /* CONFIG_HUBBLE_SAT_NETWORK */

	HUBBLE_LOG_INFO("Hubble Network SDK initialized\n");

	return 0;
}

const void *hubble_internal_key_get(void)
{
	return master_key;
}

uint64_t hubble_internal_time_ms_get(void)
{
	return unix_epoch_base_ms + hubble_uptime_get();
}

uint64_t hubble_internal_time_last_synced_ms_get(void)
{
	return unix_epoch_synced_ms;
}
