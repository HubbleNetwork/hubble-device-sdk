/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hubble/hubble.h>
#include <hubble/sat/packet.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/base64.h>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

LOG_MODULE_REGISTER(main);

/*
 * Decoded master key. The SDK stores this pointer directly, so the buffer has
 * static storage duration and outlives all SDK calls. Zero-initialised, which
 * also serves as the all-zero placeholder when no key is provided.
 */
static uint8_t master_key[CONFIG_HUBBLE_KEY_SIZE];

/*
 * Decode the base64 master key (CONFIG_SAMPLE_HUBBLE_KEY, provisioned at build
 * time) into master_key, validating its length.
 */
static int master_key_provision(void)
{
	const char *key = CONFIG_SAMPLE_HUBBLE_KEY;
	size_t olen;
	int err;

	if (strlen(key) == 0) {
		LOG_WRN("CONFIG_SAMPLE_HUBBLE_KEY not set; using an all-zero "
			"placeholder key. Pass your base64 key with "
			"-DCONFIG_SAMPLE_HUBBLE_KEY=\"<key>\".");
		return 0;
	}

	err = base64_decode(master_key, sizeof(master_key), &olen, key,
			    strlen(key));
	if (err != 0 || olen != sizeof(master_key)) {
		LOG_ERR("Invalid CONFIG_SAMPLE_HUBBLE_KEY: expected a base64-"
			"encoded %u-byte key (matching CONFIG_HUBBLE_KEY_SIZE)",
			(unsigned int)sizeof(master_key));
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT

int hubble_sat_board_init(void)
{
	return 0;
}

int hubble_sat_board_enable(void)
{
	return 0;
}

int hubble_sat_board_disable(void)
{
	return 0;
}

int hubble_sat_board_packet_send(const struct hubble_sat_packet_frames *packet)
{
	ARG_UNUSED(packet);

	return 0;
}

#endif /* CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT */


int main(void)
{
	int err = 0;
	struct hubble_sat_packet pkt;

	LOG_DBG("Hubble Network Sat Sample started");

	err = master_key_provision();
	if (err != 0) {
		goto end;
	}

	/*
	 * This sample uses the device uptime counter source
	 * (CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME), so no UTC time is needed:
	 * we pass 0 as the initial EID counter value and it advances with uptime.
	 */
	err = hubble_init(0, 0, master_key);
	if (err != 0) {
		LOG_ERR("Failed to initialize Hubble Sat Network");
		goto end;
	}

	for (;;) {
		err = hubble_sat_packet_get(&pkt, NULL, 0);
		if (err != 0) {
			LOG_ERR("Failed to get Hubble Sat Network packet");
			goto end;
		}

		/* Set reliability to NONE - this will trigger a single
		 * transmission instead of a sequence. This is only for testing
		 * purposes and not recommended for production.
		 */
		err = hubble_sat_packet_send(&pkt, HUBBLE_SAT_RELIABILITY_NONE);
		if (err != 0) {
			LOG_ERR("Failed to transmit packet");
			goto end;
		}

		k_sleep(K_SECONDS(CONFIG_SAMPLE_SAT_TX_INTERVAL_SECONDS));
	}

end:
	return err;
}
