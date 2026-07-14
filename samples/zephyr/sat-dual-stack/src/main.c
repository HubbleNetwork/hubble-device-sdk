/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hubble/hubble.h>
#include <hubble/sat/packet.h>
#include <hubble/sat/pass_prediction.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/base64.h>

#include <stdint.h>
#include <string.h>

#include "app_ble.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

#define HUBBLE_MAX_SAT 6U

struct hubble_sat_orbital_params orb_params[HUBBLE_MAX_SAT];
struct hubble_sat_device_pos device_pos;
uint8_t orb_params_count;
uint64_t unix_time_ms;

/* Hubble device key, decoded from CONFIG_HUBBLE_DEVICE_KEY. */
static uint8_t _hubble_key[CONFIG_HUBBLE_KEY_SIZE];

/* Given by app_ble.c once provisioning (time + orbital params) completes. */
K_SEM_DEFINE(sync_sem, 0, 1);

/* Given by the satellite-pass timer when it is time to transmit. */
static K_SEM_DEFINE(sat_tx_sem, 0, 1);

static void _sat_timer_cb(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	k_sem_give(&sat_tx_sem);
}

static K_TIMER_DEFINE(sat_timer, _sat_timer_cb, NULL);

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
	struct hubble_sat_pass_info pass_info = {0};
	struct hubble_sat_packet packet = {0};
	uint64_t now_ms;
	int err;

	LOG_INF("Hubble Network dual-stack sample started");

	/* Decode the device key, if one was provided at build time. */
	if (strlen(CONFIG_HUBBLE_DEVICE_KEY) != 0) {
		size_t olen;

		err = base64_decode(_hubble_key, sizeof(_hubble_key), &olen,
				    CONFIG_HUBBLE_DEVICE_KEY,
				    strlen(CONFIG_HUBBLE_DEVICE_KEY));
		if (err != 0) {
			LOG_ERR("Invalid key provided!");
			return -EINVAL;
		}
	}

	/*
	 * Enable BLE and start connectable advertising.
	 */
	err = ble_init();
	if (err != 0) {
		LOG_ERR("Failed to init BLE (err %d)", err);
		return err;
	}

	LOG_INF("Waiting for provisioning over BLE...");
	k_sem_take(&sync_sem, K_FOREVER);

	err = hubble_init(unix_time_ms, 0, _hubble_key);
	if (err != 0) {
		LOG_ERR("Failed to initialize Hubble Network (err %d)", err);
		return err;
	}

	err = hubble_sat_satellites_set(orb_params, orb_params_count);
	if (err != 0) {
		LOG_ERR("Failed to set satellite orbital params (err %d)", err);
		return err;
	}

	LOG_INF("Hubble Network initialized with %u satellite(s)",
		orb_params_count);

	for (;;) {
		/* Compute the next satellite pass over the device location. */
		now_ms = hubble_time_get();

		err = hubble_sat_next_pass_get(now_ms, &device_pos, &pass_info);
		if (err != 0) {
			LOG_ERR("Failed to get next pass info (err %d)", err);
			return err;
		}

		/*
		 * A pass start in the past means we are in the middle of a
		 * pass; compute the next one.
		 */
		if (pass_info.start <= now_ms) {
			LOG_INF("Pass ongoing or in the past, finding next...");

			err = hubble_sat_next_pass_get(
				pass_info.start + pass_info.duration,
				&device_pos, &pass_info);
			if (err != 0) {
				LOG_ERR("Failed to get next pass info (err %d)",
					err);
				return err;
			}
		}

#ifdef CONFIG_HUBBLE_SAMPLE_DEBUG
		LOG_INF("Debug mode: next pass in 120 seconds");
		k_timer_start(&sat_timer, K_SECONDS(120), K_NO_WAIT);
#else
		LOG_INF("Next pass at %llu (unix epoch seconds)",
			pass_info.start / MSEC_PER_SEC);
		k_timer_start(&sat_timer, K_MSEC(pass_info.start - now_ms),
			      K_NO_WAIT);
#endif

		err = ble_adv_start();
		if (err != 0) {
			LOG_ERR("Failed to start beacon adv (err %d)", err);
			return err;
		}
		LOG_INF("Beaconing until next pass...");

		k_sem_take(&sat_tx_sem, K_FOREVER);

		/* Stop the beacon and transmit to the satellite. */
		err = ble_adv_stop();
		if (err != 0) {
			LOG_ERR("Failed to stop beacon adv (err %d)", err);
			return err;
		}

		err = hubble_sat_packet_get(&packet, NULL, 0);
		if (err != 0) {
			LOG_ERR("Failed to get sat packet (err %d)", err);
			return err;
		}

		LOG_INF("Transmitting to satellite...");
		err = hubble_sat_packet_send(&packet,
					     HUBBLE_SAT_RELIABILITY_NORMAL);
		if (err != 0) {
			LOG_ERR("Failed to send sat packet (err %d)", err);
			return err;
		}
	}

	return 0;
}
