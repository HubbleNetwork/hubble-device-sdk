/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Hubble Network BLE Beacon + Manufacturer Data sample.
 *
 * This sample shows how to emit a SINGLE BLE advertisement that contains both
 * the standard Hubble beacon payload AND an application-owned
 * "manufacturer specific data" field. The manufacturer field carries a 4-byte
 * counter that is incremented every time the advertisement is refreshed
 * (every CONFIG_HUBBLE_MFG_DATA_SAMPLE_UPDATE_PERIOD seconds, 5 minutes by
 * default).
 *
 * The advertisement is built from three AD (Advertising Data) structures:
 *
 *   [1] BT_DATA_UUID16_ALL      -> 0xFCA6, the Hubble service UUID. This is how
 *                                  Hubble gateways recognize the beacon.
 *   [2] BT_DATA_SVC_DATA16      -> the encrypted Hubble payload produced by
 *                                  hubble_ble_advertise_get(). The buffer
 *                                  already starts with the 0xFCA6 service UUID.
 *   [3] BT_DATA_MANUFACTURER_DATA -> application data. By BLE convention this
 *                                  field starts with a 2-byte company
 *                                  identifier; here we reuse Hubble's 0xFCA6
 *                                  identifier (little-endian) followed by a
 *                                  4-byte little-endian counter.
 *
 * Advertisement size note: legacy BLE advertising allows 31 payload bytes.
 * This sample passes no Hubble user payload (NULL, 0), so the Hubble service
 * data is 12 bytes and the whole advertisement is ~26 bytes -- well under the
 * limit. If you add Hubble user payload (up to 13 bytes) it shares the same
 * 31-byte budget with the manufacturer field, so they cannot both be maxed.
 *
 * Key provisioning: the Hubble master key is passed on the build command line
 * as base64 (west build ... -- -DHUBBLE_KEY="<key>") and decoded to a byte
 * array at build time (see CMakeLists.txt), so no base64 code runs on the
 * device. No pre-build script is required.
 */

#include <hubble/hubble.h>
#include <hubble/ble.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>

#include <stdint.h>

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

/*
 * Master key, decoded from the -DHUBBLE_KEY base64 value at build time and
 * defined in the generated source (see CMakeLists.txt). The SDK stores this
 * pointer directly, so the array has static storage duration and outlives all
 * SDK calls.
 */
extern const uint8_t master_key[];

/*
 * Buffer that receives the encrypted Hubble advertisement payload.
 * 31 bytes is the maximum a legacy advertisement can hold.
 */
#define HUBBLE_USER_BUFFER_LEN 31
static uint8_t hubble_buffer[HUBBLE_USER_BUFFER_LEN];

/*
 * Manufacturer-specific data: a 2-byte identifier prefix followed by a 4-byte
 * counter. We reuse Hubble's 0xFCA6 identifier as the prefix. Both fields are
 * little-endian, matching how BLE company identifiers are transmitted.
 */
#define MFG_DATA_COMPANY_ID HUBBLE_BLE_UUID /* 0xFCA6 */

static struct {
	uint16_t company_id;
	uint32_t counter;
} __packed mfg_data = {
	.company_id = MFG_DATA_COMPANY_ID,
	.counter = 0U,
};

/* The Hubble service UUID advertised in the UUID16 list. */
static uint16_t adv_uuid = HUBBLE_BLE_UUID;

/*
 * The three AD structures that make up the advertisement. AD[1] (the Hubble
 * service data) is filled in at runtime once we have generated the payload.
 */
static struct bt_data adv_data[] = {
	BT_DATA(BT_DATA_UUID16_ALL, &adv_uuid, sizeof(adv_uuid)),
	{0},
	BT_DATA(BT_DATA_MANUFACTURER_DATA, &mfg_data, sizeof(mfg_data)),
};

/* Timer that wakes the main loop to refresh the advertisement. */
K_SEM_DEFINE(timer_sem, 0, 1);

static void timer_cb(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	k_sem_give(&timer_sem);
}

K_TIMER_DEFINE(refresh_timer, timer_cb, NULL);

int main(void)
{
	int err;
	size_t out_len;

	LOG_DBG("Hubble Network BLE Beacon + Manufacturer Data sample started");

	/* Bring up the Bluetooth subsystem. */
	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	/*
	 * Initialize the Hubble SDK with the provisioned master key. This sample
	 * uses the device uptime counter source
	 * (CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME), so no UTC time is needed:
	 * we pass 0 as the initial EID counter value and it advances with uptime.
	 */
	err = hubble_init(0, master_key);
	if (err != 0) {
		LOG_ERR("Failed to initialize Hubble BLE Network (err %d)", err);
		goto end;
	}

	/* Refresh the advertisement on a fixed period (default 5 minutes). */
	k_timer_start(&refresh_timer,
		      K_SECONDS(CONFIG_HUBBLE_MFG_DATA_SAMPLE_UPDATE_PERIOD),
		      K_SECONDS(CONFIG_HUBBLE_MFG_DATA_SAMPLE_UPDATE_PERIOD));

	for (;;) {
		/*
		 * Generate the Hubble payload. We send no application payload
		 * of our own through Hubble (NULL, 0); our custom data lives in
		 * the manufacturer-data field instead.
		 */
		out_len = HUBBLE_USER_BUFFER_LEN;
		err = hubble_ble_advertise_get(NULL, 0, hubble_buffer, &out_len);
		if (err != 0) {
			LOG_ERR("Failed to get advertisement data (err=%d)", err);
			goto end;
		}

		/* Point AD[1] at the freshly generated Hubble service data. */
		adv_data[1].type = BT_DATA_SVC_DATA16;
		adv_data[1].data = hubble_buffer;
		adv_data[1].data_len = out_len;

		LOG_DBG("Hubble payload: %u bytes, counter: %u",
			(unsigned int)out_len, mfg_data.counter);

		/*
		 * Start a non-connectable beacon using a Non-Resolvable Private
		 * Address (NRPA); the actual address is derived by the Hubble
		 * payload, so the advertised address rotates with the EID.
		 */
		err = bt_le_adv_start(
			BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NRPA,
					BT_GAP_ADV_FAST_INT_MIN_2,
					BT_GAP_ADV_FAST_INT_MAX_2, NULL),
			adv_data, ARRAY_SIZE(adv_data), NULL, 0);
		if (err != 0) {
			LOG_ERR("Advertisement start failed (err %d)", err);
			goto end;
		}

		/* Wait until the timer fires. */
		k_sem_take(&timer_sem, K_FOREVER);

		err = bt_le_adv_stop();
		if (err != 0) {
			LOG_ERR("Advertisement stop failed (err %d)", err);
			goto end;
		}

		/* Increment the counter for the next advertisement. */
		mfg_data.counter++;
	}

end:
	(void)bt_disable();
	return err;
}
