/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hubble/hubble.h>
#include <hubble/sat/packet.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <stdint.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(main);

/* Replace it properly for real use */
static uint64_t _utc_unused = 0xdeadbeef;
static uint8_t _key_unused[CONFIG_HUBBLE_KEY_SIZE];

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

int hubble_sat_board_packet_send(const struct hubble_sat_packet *packet)
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

	err = hubble_init(_utc_unused, _key_unused);
	if (err != 0) {
		LOG_ERR("Failed to initialize Hubble Sat Network");
		goto end;
	}

	(void)hubble_sat_static_device_id_set(
		CONFIG_HUBBLE_SAT_SAMPLE_NETWORK_ID);

	for (;;) {
		err = hubble_sat_packet_get(&pkt, NULL, 0);
		if (err != 0) {
			LOG_ERR("Failed to get Hubble Sat Network packet");
			goto end;
		}

		err = hubble_sat_packet_send(&pkt, HUBBLE_SAT_RELIABILITY_NORMAL);
		if (err != 0) {
			LOG_ERR("Failed to transmit packet");
			goto end;
		}

		k_sleep(K_SECONDS(2));
	}

end:
	return err;
}
