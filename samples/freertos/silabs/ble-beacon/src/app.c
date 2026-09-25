/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "sl_main_init.h"
#include "sl_bluetooth.h"
#include "sl_sleeptimer.h"
#include "app_assert.h"
#include "app_log.h"

#include <hubble/hubble.h>

#include "key.c"
#include "time.c"

#define APP_LOG_PREFIX                 "[ble_beacon] "

#define BLE_ADV_LEN                    31U
#define ADV_PREFIX_LEN                 6U
#define ADV_SERVICE_DATA_LEN_POS       4U

#define ADV_INTERVAL_MIN               1600U /* 1s * 1.6 */
#define ADV_INTERVAL_MAX               2400U /* 1.5s * 1.6 */

/*  Period to update adv packets in milliseconds (1 hour) */
#define ADV_PACKET_REFRESH_INTERVAL_MS (60U * 60U * 1000U) /* 1 hour */

#define BLE_ADV_STACK_SIZE             1024U
#define BLE_ADV_PRIORITY               24U /* at osPriorityNormal */

/* Adv set handle allocated from the BT stack */
static uint8_t _adv_set_handle = SL_BT_INVALID_ADVERTISING_SET_HANDLE;

/* Configure raw data for advertising packet */
static uint8_t _adv_data[BLE_ADV_LEN] = {
	0x03,       /* AD structure length  */
	0x03,       /* AD type: complete list of 16-bit service UUIDs */
	0xA6, 0xFC, /* Hubble UUID */
	0x01,       /* Service data length (filled out at run-time) */
	0x16,       /* Service data AD type */
};
static sl_sleeptimer_timer_handle_t _adv_refresh_timer;
static TaskHandle_t _adv_refresh_task_handle;

static sl_status_t _update_hubble_adv_data(void)
{
	size_t out_len = sizeof(_adv_data) - ADV_PREFIX_LEN;
	int ret;

	memset(&_adv_data[ADV_PREFIX_LEN], 0, out_len);

	/* 0 byte payload */
	ret = hubble_ble_advertise_get(NULL, 0, &_adv_data[ADV_PREFIX_LEN],
				       &out_len);
	if (ret != 0) {
		app_log_error(APP_LOG_PREFIX
			      "Failed to get Hubble adv data, err=%d\n",
			      ret);
		return SL_STATUS_FAIL;
	}

	/* +1 because of the service data type */
	_adv_data[ADV_SERVICE_DATA_LEN_POS] = (uint8_t)(out_len + 1);

	return sl_bt_legacy_advertiser_set_data(
		_adv_set_handle, sl_bt_advertiser_advertising_data_packet,
		ADV_PREFIX_LEN + out_len, _adv_data);
}

static void _adv_refresh_timer_cb(sl_sleeptimer_timer_handle_t *handle, void *data)
{
	(void)handle;
	(void)data;

	BaseType_t higher_prio_task_woken = pdFALSE;

	vTaskNotifyGiveFromISR(_adv_refresh_task_handle, &higher_prio_task_woken);
	portYIELD_FROM_ISR(higher_prio_task_woken);
}

/* Bluetooth stack event handler */
void sl_bt_on_event(sl_bt_msg_t *evt)
{
	sl_status_t status;

	if (SL_BT_MSG_ID(evt->header) != sl_bt_evt_system_boot_id) {
		return;
	}

	/*
	 * Note: sl_bt_evt_system_boot_id event indicates the device
	 * has started and the radio is ready. Do not call any stack
	 * command before receiving this boot event!
	 */

	/* Create the advertising set and config to NRPA */
	status = sl_bt_advertiser_create_set(&_adv_set_handle);
	app_assert(status == SL_STATUS_OK, "Failed to create advertising set");

	status = sl_bt_advertiser_configure(
		_adv_set_handle, SL_BT_ADVERTISER_USE_NONRESOLVABLE_ADDRESS);
	app_assert(status == SL_STATUS_OK,
		   "Failed to configure advertising set");

	status = sl_bt_advertiser_set_timing(_adv_set_handle, ADV_INTERVAL_MIN,
					     ADV_INTERVAL_MAX, 0, 0);
	app_assert(status == SL_STATUS_OK, "Failed to set advertising timing");

	/* Start advertising and timer */
	status = sl_sleeptimer_start_periodic_timer_ms(
		&_adv_refresh_timer, ADV_PACKET_REFRESH_INTERVAL_MS,
		_adv_refresh_timer_cb, NULL, 0,
		SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
	app_assert(status == SL_STATUS_OK, "Failed to start adv refresh timer");

	status = sl_bt_legacy_advertiser_start(
		_adv_set_handle, sl_bt_legacy_advertiser_non_connectable);
	app_assert(status == SL_STATUS_OK, "Failed to start advertising");

	xTaskNotifyGive(_adv_refresh_task_handle);
}

static void _adv_refresh_task(void *param)
{
	(void)param;

	for (;;) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		sl_status_t status = _update_hubble_adv_data();
		if (status != SL_STATUS_OK) {
			app_log_error(
				APP_LOG_PREFIX
				"Failed to update Hubble adv data, err=%d\n",
				(int)status);
			break;
		}

		app_log_info(APP_LOG_PREFIX "Hubble adv data updated\n");
	}

	/* Clean up resources (timer, bt) */
	(void)sl_sleeptimer_stop_timer(&_adv_refresh_timer);
	(void)sl_bt_advertiser_stop(_adv_set_handle);
	(void)sl_bt_advertiser_delete_set(_adv_set_handle);

	_adv_refresh_task_handle = NULL;
	vTaskDelete(NULL);
}

void app_init(void)
{
	BaseType_t ret;
	int err;

	err = hubble_init(unix_time, master_key);
	app_assert(err == 0, "Failed to initialize Hubble, err=%d", err);

	ret = xTaskCreate(_adv_refresh_task, "adv_refresh", BLE_ADV_STACK_SIZE,
			  NULL, BLE_ADV_PRIORITY, &_adv_refresh_task_handle);
	app_assert(ret == pdPASS, "Failed to create adv refresh task");

	app_log_info(APP_LOG_PREFIX "Hubble Network initialized\n");
}
