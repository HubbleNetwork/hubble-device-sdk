/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include "app_assert.h"
#include "app_log.h"
#include "sl_main_init.h"
#include "sl_sleeptimer.h"
#include "mbedtls/base64.h"

#include <hubble/hubble.h>
#include <hubble/sat/packet.h>
#include <hubble/sat/pass_prediction.h>

#include "app_ble.h"
#include "sat_dual_stack_config.h"

#define APP_LOG_PREFIX      "[main] "

#define APP_TASK_STACK_SIZE 2048
#define APP_TASK_PRIORITY   24U /* at osPriorityNormal */

#define MS_PER_SEC          1000U

/* Device location and sat orbital parameters */
struct hubble_sat_device_pos device_pos;
struct hubble_sat_orbital_params orb_params[HUBBLE_MAX_SAT];
uint8_t orb_params_count;

/* Hubble device key and time */
static uint8_t _hubble_key[CONFIG_HUBBLE_KEY_SIZE];
uint64_t unix_time_ms;

/* Sem for sync time / orb params and wait for sat tx */
SemaphoreHandle_t sync_sem;
static SemaphoreHandle_t _sat_tx_sem;

static sl_sleeptimer_timer_handle_t _sat_timer;
static TaskHandle_t _app_task_handle;

static void _sat_timer_cb(sl_sleeptimer_timer_handle_t *handle, void *data)
{
	(void)handle;
	(void)data;

	BaseType_t higher_prio_task_woken = pdFALSE;

	xSemaphoreGiveFromISR(_sat_tx_sem, &higher_prio_task_woken);
	portYIELD_FROM_ISR(higher_prio_task_woken);
}

static void _app_task(void *params)
{
	(void)params;

	struct hubble_sat_pass_info pass_info = {0};
	struct hubble_sat_packet packet = {0};
	uint64_t now_ms;
	uint64_t sat_wait_ms;
	sl_status_t status;
	int ret;

	ret = ble_init();
	if (ret != 0) {
		app_log_error(
			APP_LOG_PREFIX "Failed to setup BLE provisioning\n");
		goto end;
	}

	app_log_info(APP_LOG_PREFIX "Waiting for provisioning over BLE...\n");
	xSemaphoreTake(sync_sem, portMAX_DELAY);

	/* Init Hubble */
	ret = hubble_init(unix_time_ms, _hubble_key);
	if (ret != 0) {
		app_log_error(APP_LOG_PREFIX "Failed to initialize Hubble\n");
		goto end;
	}

	/* Set sat params */
	ret = hubble_sat_satellites_set(orb_params, orb_params_count);
	if (ret != 0) {
		app_log_error(
			APP_LOG_PREFIX "Failed to set satellite parameters\n");
		goto end;
	}

	app_log_info(APP_LOG_PREFIX "Hubble Network initialized\n");

	for (;;) {
		/* Calculate the next pass time using the device location */
		now_ms = hubble_time_get();
		ret = hubble_sat_next_pass_get(now_ms, &device_pos, &pass_info);
		if (ret != 0) {
			app_log_error(APP_LOG_PREFIX
				      "Failed to get next satellite pass\n");
			goto end;
		}

		/*
		 * If the pass start <= current time, this means we're in the
		 * middle of a pass. We can compute the next one
		 */
		if (pass_info.start <= now_ms) {
			app_log_info(APP_LOG_PREFIX "Pass ongoing or in the "
						    "past, finding next...\n");

			ret = hubble_sat_next_pass_get(
				pass_info.start + pass_info.duration,
				&device_pos, &pass_info);
			if (ret != 0) {
				app_log_error(
					APP_LOG_PREFIX
					"Failed to get next satellite pass\n");
				goto end;
			}
		}

#if HUBBLE_SAMPLE_DEBUG
		sat_wait_ms = 120 * MS_PER_SEC;
		app_log_info(APP_LOG_PREFIX "Next pass in 120 seconds\n");
#else
		app_log_info(APP_LOG_PREFIX
			     "Next pass at: %llu (unix epoch seconds)\n",
			     pass_info.start / MS_PER_SEC);

		/* Calculate how long to wait until the next pass */
		sat_wait_ms = pass_info.start - now_ms;
#endif
		status = sl_sleeptimer_start_timer_ms(
			&_sat_timer, sat_wait_ms, _sat_timer_cb, NULL, 0,
			SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
		if (status != SL_STATUS_OK) {
			app_log_error(APP_LOG_PREFIX
				      "Failed to start satellite timer\n");
			goto end;
		}

		/* Start Hubble BLE advertising */
		ret = ble_adv_start();
		if (ret != 0) {
			app_log_error(
				APP_LOG_PREFIX
				"Failed to start BLE advertising, ret: %d\n",
				ret);
			goto end;
		}

		app_log_info(APP_LOG_PREFIX "Hubble BLE advertising started\n");

		/* Wait for sat pass */
		xSemaphoreTake(_sat_tx_sem, portMAX_DELAY);

		/* Stop BLE and start sat tx */
		ret = ble_adv_stop();
		if (ret != 0) {
			app_log_error(
				APP_LOG_PREFIX
				"Failed to stop BLE advertising, ret: %d\n",
				ret);
			goto end;
		}

		ret = hubble_sat_packet_get(&packet, NULL, 0);
		if (ret != 0) {
			app_log_error(APP_LOG_PREFIX
				      "Failed to get sat packet, ret: %d\n",
				      ret);
			goto end;
		}

		app_log_info(
			APP_LOG_PREFIX "Starting satellite transmission...\n");
		ret = hubble_sat_packet_send(&packet,
					     HUBBLE_SAT_RELIABILITY_NORMAL);
		if (ret != 0) {
			app_log_error(APP_LOG_PREFIX
				      "Failed to send sat packet, ret: %d\n",
				      ret);
			goto end;
		}
	}

end:
	/*
	 * Note: in production code, we should clean up resources here
	 * (sem, timers, deinit stack, etc.) on the failure case. For
	 * simplicity and demonstration purposes, these are a few basic
	 * cleanup, but not an exhaustive one.
	 */
	sl_sleeptimer_stop_timer(&_sat_timer);
	vSemaphoreDelete(sync_sem);
	vSemaphoreDelete(_sat_tx_sem);
	_app_task_handle = NULL;
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

	/* Create sem and application task */
	sync_sem = xSemaphoreCreateBinary();
	app_assert(sync_sem != NULL, "Failed to create sync semaphore");

	_sat_tx_sem = xSemaphoreCreateBinary();
	app_assert(_sat_tx_sem != NULL, "Failed to create sat tx semaphore");

	ret = xTaskCreate(_app_task, "app_task", APP_TASK_STACK_SIZE, NULL,
			  APP_TASK_PRIORITY, &_app_task_handle);
	app_assert(ret == pdPASS, "Failed to create app task");
}
