/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "sl_main_init.h"
#include "app_assert.h"
#include "app_log.h"
#include "mbedtls/base64.h"

#include <hubble/hubble.h>
#include <hubble/sat/packet.h>

#include "sat_cont_config.h"

#define APP_LOG_PREFIX     "[sat_continuous] "

#define SAT_ADV_STACK_SIZE 1024U
#define SAT_ADV_PRIORITY   24U /* at osPriorityNormal */

#define SAT_TX_SLEEP_MS    10000

static uint8_t _hubble_key[CONFIG_HUBBLE_KEY_SIZE];

static TaskHandle_t _sat_tx_task_handle;

static void _sat_tx_task(void *param)
{
	(void)param;

	int err;
	struct hubble_sat_packet pkt;

	app_log_info(APP_LOG_PREFIX "Starting Sat Transmission...\n");

	for (;;) {
		err = hubble_sat_packet_get(&pkt, NULL, 0);
		if (err != 0) {
			app_log_error(APP_LOG_PREFIX
				      "Failed to get Hubble Sat Network "
				      "packet, error: %d",
				      err);
			break;
		}

		/*
		 * Set reliability to NONE. This will trigger a single
		 * transmission instead of a sequence. This is only for testing
		 * purposes and not recommended for production.
		 */
		err = hubble_sat_packet_send(&pkt, HUBBLE_SAT_RELIABILITY_NONE);
		if (err != 0) {
			app_log_error(APP_LOG_PREFIX
				      "Failed to transmit packet, error: %d",
				      err);
			break;
		}

		vTaskDelay(pdMS_TO_TICKS(SAT_TX_SLEEP_MS));
	}

	_sat_tx_task_handle = NULL;
	vTaskDelete(NULL);
}

void app_init(void)
{
	BaseType_t ret;
	int err;

	/* Decode device key */
	if (strlen(HUBBLE_DEVICE_KEY) != 0) {
		size_t outlen = 0;

		err = mbedtls_base64_decode(
			_hubble_key, sizeof(_hubble_key), &outlen,
			(const unsigned char *)HUBBLE_DEVICE_KEY,
			strlen(HUBBLE_DEVICE_KEY));

		app_assert(err == 0 && outlen == sizeof(_hubble_key),
			   "Invalid key provided!\n");
	}

	/*
	 * Use the device uptime counter source so the device does not need
	 * Unix time provisioned. The EID counter starts from the value passed
	 * to hubble_init() and advances with uptime.
	 */
	err = hubble_init(0, _hubble_key);
	app_assert(err == 0, "Failed to initialize Hubble, err=%d", err);

	ret = xTaskCreate(_sat_tx_task, "sat_tx", SAT_ADV_STACK_SIZE, NULL,
			  SAT_ADV_PRIORITY, &_sat_tx_task_handle);
	app_assert(ret == pdPASS, "Failed to create sat tx task");

	app_log_info(APP_LOG_PREFIX "Hubble Network initialized\n");
}
