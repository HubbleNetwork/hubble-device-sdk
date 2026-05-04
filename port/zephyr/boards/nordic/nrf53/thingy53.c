/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <hal/nrf_radio.h>

#include <hubble/sat.h>
#include <hubble/sat/packet.h>
#include <sat_soc.h>

#include "fem.h"


static nrf_radio_txpower_t _power = RADIO_TXPOWER_TXPOWER_0dBm;

int hubble_sat_board_init(void)
{
	hubble_board_fem_setup();

	_power = nrf_radio_txpower_get(NRF_RADIO);

	return 0;
}

int hubble_sat_board_enable(void)
{
	nrf_radio_txpower_set(NRF_RADIO, RADIO_TXPOWER_TXPOWER_0dBm);

	return hubble_sat_soc_enable();
}

int hubble_sat_board_disable(void)
{
	hubble_sat_soc_disable();
	nrf_radio_txpower_set(NRF_RADIO, _power);

	return 0;
}

int hubble_sat_board_packet_send(const struct hubble_sat_packet_frames *packet)
{
	hubble_board_fem_enable();
	hubble_sat_soc_packet_send(packet);
	hubble_board_fem_sleep();

	return 0;
}
