/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "app_assert.h"
#include "app_log.h"
#include "gatt_db.h"
#include "sl_bluetooth.h"
#include "sl_sleeptimer.h"

#include <hubble/hubble.h>
#include <hubble/ble.h>
#include <hubble/sat/pass_prediction.h>

#include "app_ble.h"

#define APP_LOG_PREFIX                  "[ble] "

#define HUBBLE_BLE_UUID_CONNECTABLE     0xFCA7
#define HUBBLE_BLE_BUFFER_LEN           31U

/* TODO: replace this by actual command once finalized */
#define HUBBLE_CMD                      0x01
#define HUBBLE_CMD_UNIX_EPOCH           0x02
#define HUBBLE_CMD_ORBITAL_PARAMS       0x03
#define HUBBLE_CMD_DEVICE_LOCATION      0x04
#define HUBBLE_ORBITAL_PARAMS_CMD_SIZE  76U

/* Converts milliseconds to advertising interval units of 0.625 ms. */
#define MS_TO_ADV_INTERVAL(_ms)         ((_ms) * 1000U / 625U)

#define ADV_INTERVAL_MIN_MS             1000U
#define ADV_INTERVAL_MAX_MS             1200U

/*  Period to update adv packets in milliseconds (1 hour) */
#define HUBBLE_ADV_PACKET_PERIOD_MS     (60U * 60U * 1000U) /* 1 hour */

#define HUBBLE_ADV_PREFIX_LEN           6U
#define HUBBLE_ADV_SERVICE_DATA_LEN_POS 4U

/* Advertising TX power in deci-dBm, 0.1 dBm units (e.g. 40 = 4 dBm) */
#define HUBBLE_ADV_TX_POWER_DDBM        0

/*
 * For connectable adv:
 * Flags (3 bytes) + complete list of 16-bit service UUIDs (4 bytes)
 */
#define CONN_ADV_PREFIX_LEN             7U

#define ADV_REFRESH_TASK_STACK_SIZE     1024U
#define ADV_REFRESH_TASK_PRIORITY       24U /* at osPriorityNormal */

/* Extern variables */
extern struct hubble_sat_device_pos device_pos;
extern struct hubble_sat_orbital_params orb_params[];
extern uint8_t orb_params_count;
extern uint64_t unix_time_ms;

/* Sem to sync time and orbital params */
extern SemaphoreHandle_t sync_sem;

/* Sem to protect the shared bt state */
static SemaphoreHandle_t _bt_sem;
static bool _is_beacon_active;

/* Timer for BLE adv packet refresh */
static sl_sleeptimer_timer_handle_t _ble_timer;

/* Task notification for adv update */
static TaskHandle_t _adv_refresh_task_handle;

/* Handle for each advertising set */
static uint8_t _conn_adv_handle = SL_BT_INVALID_ADVERTISING_SET_HANDLE;
static uint8_t _beacon_adv_handle = SL_BT_INVALID_ADVERTISING_SET_HANDLE;

/* Configure raw data for beacon and connectable advertising */
static uint8_t _beacon_adv_data[HUBBLE_BLE_BUFFER_LEN] = {
	0x03,       /* AD structure length  */
	0x03,       /* AD type: complete list of 16-bit service UUIDs */
	0xA6, 0xFC, /* Hubble UUID */
	0x01,       /* Service data length (filled out at run-time) */
	0x16,       /* Service data AD type */
};

static uint8_t _conn_adv_data[HUBBLE_BLE_BUFFER_LEN] = {
	0x02, /* AD structure length */
	0x01, /* AD type: Flags */
	0x06, /* LE General Discoverable, BR/EDR not supported */
	0x03, /* AD structure length  */
	0x03, /* AD type: complete list of 16-bit service UUIDs */
	HUBBLE_BLE_UUID_CONNECTABLE & 0xFF,
	(HUBBLE_BLE_UUID_CONNECTABLE >> 8) & 0xFF,
};

static void _ble_refresh_timer_cb(sl_sleeptimer_timer_handle_t *handle, void *data)
{
	(void)handle;
	(void)data;

	BaseType_t higher_prio_task_woken = pdFALSE;

	vTaskNotifyGiveFromISR(_adv_refresh_task_handle, &higher_prio_task_woken);
	portYIELD_FROM_ISR(higher_prio_task_woken);
}

/* Return the len of the Hubble advertising data or negative errno on failure */
static int _get_hubble_adv_data(void)
{
	size_t out_len = sizeof(_beacon_adv_data) - HUBBLE_ADV_PREFIX_LEN;
	int ret;

	memset(&_beacon_adv_data[HUBBLE_ADV_PREFIX_LEN], 0, out_len);

	/* 0 byte payload */
	ret = hubble_ble_advertise_get(
		NULL, 0, &_beacon_adv_data[HUBBLE_ADV_PREFIX_LEN], &out_len);
	if (ret != 0) {
		app_log_error(APP_LOG_PREFIX
			      "Failed to get Hubble adv data, err=%d\n",
			      ret);
		return ret;
	}

	/* +1 because of the service data type */
	_beacon_adv_data[HUBBLE_ADV_SERVICE_DATA_LEN_POS] =
		(uint8_t)(out_len + 1);

	return (int)out_len;
}

static void _adv_refresh_task(void *param)
{
	(void)param;

	int ret;
	for (;;) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		xSemaphoreTake(_bt_sem, portMAX_DELAY);
		if (!_is_beacon_active) {
			xSemaphoreGive(_bt_sem);
			continue;
		}

		ret = _get_hubble_adv_data();
		if (ret < 0) {
			app_log_warning(
				APP_LOG_PREFIX
				"Unable to get new Hubble adv data, err=%d\n",
				ret);
		} else {
			(void)sl_bt_legacy_advertiser_set_data(
				_beacon_adv_handle,
				sl_bt_advertiser_advertising_data_packet,
				HUBBLE_ADV_PREFIX_LEN + ret, _beacon_adv_data);

			app_log_info(APP_LOG_PREFIX
				     "Updated Hubble adv data, len=%d\n",
				     ret);
		}

		xSemaphoreGive(_bt_sem);
	}
}

/* BLE Beacon */
int ble_adv_start(void)
{
	int16_t tx_power_out;
	sl_status_t status;
	int ret;

	/* Get the hubble adv data */
	ret = _get_hubble_adv_data();
	if (ret < 0) {
		app_log_error(APP_LOG_PREFIX
			      "Failed to get Hubble adv data, err=%d\n",
			      ret);
		return ret;
	}

	/* Set data and power */
	status = sl_bt_legacy_advertiser_set_data(
		_beacon_adv_handle, sl_bt_advertiser_advertising_data_packet,
		HUBBLE_ADV_PREFIX_LEN + ret, _beacon_adv_data);
	if (status != SL_STATUS_OK) {
		app_log_error(APP_LOG_PREFIX
			      "Failed to set Hubble adv data, err=%lu\n",
			      status);
		return -EIO;
	}

	status = sl_bt_advertiser_set_tx_power(
		_beacon_adv_handle, HUBBLE_ADV_TX_POWER_DDBM, &tx_power_out);
	if (status != SL_STATUS_OK) {
		app_log_error(APP_LOG_PREFIX
			      "Failed to set TX power, err=%lu\n",
			      status);
		return -EIO;
	}

	/* Start the periodic BLE adv refresh timer */
	status = sl_sleeptimer_start_periodic_timer_ms(
		&_ble_timer, HUBBLE_ADV_PACKET_PERIOD_MS, _ble_refresh_timer_cb,
		NULL, 0, SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
	if (status != SL_STATUS_OK) {
		app_log_error(
			APP_LOG_PREFIX
			"Failed to start BLE adv refresh timer, err=%lu\n",
			status);
		return -EIO;
	}

	/* Finally, start the BLE advertising */
	status = sl_bt_legacy_advertiser_start(
		_beacon_adv_handle, sl_bt_legacy_advertiser_non_connectable);
	if (status != SL_STATUS_OK) {
		app_log_error(APP_LOG_PREFIX
			      "Failed to start BLE advertising, err=%lu\n",
			      status);
		ret = -EAGAIN;
		goto err_stop_timer;
	}

	xSemaphoreTake(_bt_sem, portMAX_DELAY);
	_is_beacon_active = true;
	xSemaphoreGive(_bt_sem);

	return 0;

err_stop_timer:
	(void)sl_sleeptimer_stop_timer(&_ble_timer);
	return ret;
}

int ble_adv_stop(void)
{

	sl_status_t status;

	xSemaphoreTake(_bt_sem, portMAX_DELAY);

	/* If already stopped, do nothing */
	if (!_is_beacon_active) {
		xSemaphoreGive(_bt_sem);
		return 0;
	}

	/* Stop the timer and BLE advertising */
	(void)sl_sleeptimer_stop_timer(&_ble_timer);
	status = sl_bt_advertiser_stop(_beacon_adv_handle);

	_is_beacon_active = false;
	xSemaphoreGive(_bt_sem);

	return (status == SL_STATUS_OK) ? 0 : -EIO;
}

/* Connectable Adv */
static sl_status_t _start_connectable_adv(void)
{
	size_t name_len = 0;
	sl_status_t status;

	/*
	 * Complete local name, taken from the GATT device name.
	 *
	 * - sizeof(_conn_adv_data) - CONN_ADV_PREFIX_LEN - 2: the maximum
	 * device name len we want to read before it overflow the adv buffer.
	 * - _conn_adv_data[CONN_ADV_PREFIX_LEN + 2]: because we need to
	 * account for the len byte and type byte.
	 */
	(void)sl_bt_gatt_server_read_attribute_value(
		gattdb_device_name, 0,
		sizeof(_conn_adv_data) - CONN_ADV_PREFIX_LEN - 2, &name_len,
		&_conn_adv_data[CONN_ADV_PREFIX_LEN + 2]);

	_conn_adv_data[CONN_ADV_PREFIX_LEN] = (uint8_t)(name_len + 1);
	_conn_adv_data[CONN_ADV_PREFIX_LEN + 1] = 0x09; /* Complete Local Name */

	status = sl_bt_legacy_advertiser_set_data(
		_conn_adv_handle, sl_bt_advertiser_advertising_data_packet,
		/* Prefix + len byte + type byte + data (aka the device name) */
		CONN_ADV_PREFIX_LEN + name_len + 2, _conn_adv_data);

	if (status != SL_STATUS_OK) {
		app_log_error(
			APP_LOG_PREFIX
			"Failed to set connectable advertising data, err=%lu\n",
			status);
		return status;
	}

	status = sl_bt_legacy_advertiser_start(
		_conn_adv_handle, sl_bt_legacy_advertiser_connectable);
	if (status != SL_STATUS_OK) {
		app_log_error(
			APP_LOG_PREFIX
			"Failed to start connectable advertising, err=%lu\n",
			status);
		return status;
	}

	return SL_STATUS_OK;
}

static sl_status_t _chr_write_cb(const uint8_t *data, size_t len)
{
	if (data == NULL || len < 2 || data[0] != HUBBLE_CMD) {
		return SL_STATUS_BT_ATT_REQUEST_NOT_SUPPORTED;
	}

	switch (data[1]) {
	case HUBBLE_CMD_UNIX_EPOCH:
		if (len != (2 + sizeof(uint64_t))) {
			return SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
		}

		memcpy(&unix_time_ms, &data[2], sizeof(uint64_t));
		break;

	case HUBBLE_CMD_ORBITAL_PARAMS: {
		if (len != (2 + HUBBLE_ORBITAL_PARAMS_CMD_SIZE)) {
			return SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
		}

		if (orb_params_count >= HUBBLE_MAX_SAT) {
			app_log_warning(
				APP_LOG_PREFIX "Max satellite count reached, "
					       "ignore orbital params\n");
			break;
		}

		struct hubble_sat_orbital_params *dst =
			&orb_params[orb_params_count];
		const uint8_t *p = &data[2];

		/* Copy field by field to avoid alignment issues */
		memcpy(&dst->t0, p, sizeof(uint64_t));
		memcpy(&dst->n0, p + 8, sizeof(double));
		memcpy(&dst->ndot, p + 16, sizeof(double));
		memcpy(&dst->raan0, p + 24, sizeof(double));
		memcpy(&dst->raandot, p + 32, sizeof(double));
		memcpy(&dst->aop0, p + 40, sizeof(double));
		memcpy(&dst->aopdot, p + 48, sizeof(double));
		memcpy(&dst->inclination, p + 56, sizeof(double));
		memcpy(&dst->eccentricity, p + 64, sizeof(double));
		memcpy(&dst->satellite_id, p + 72, sizeof(uint32_t));

		orb_params_count++;
		app_log_info(APP_LOG_PREFIX
			     "Received orbital params for satellite ID %lu\n",
			     dst->satellite_id);
		break;
	}

	case HUBBLE_CMD_DEVICE_LOCATION:
		if (len != (2 + 2 * sizeof(double))) {
			return SL_STATUS_BT_ATT_INVALID_ATT_LENGTH;
		}

		memcpy(&device_pos.lat, &data[2], sizeof(double));
		memcpy(&device_pos.lon, &data[2 + sizeof(double)],
		       sizeof(double));
		break;

	default:
		return SL_STATUS_BT_ATT_REQUEST_NOT_SUPPORTED;
	}

	/* SL_STATUS_OK = 0, which corresponds BT ATT success */
	return SL_STATUS_OK;
}

/* Bluetooth stack event handler */
void sl_bt_on_event(sl_bt_msg_t *evt)
{
	sl_status_t status;

	switch (SL_BT_MSG_ID(evt->header)) {
	/*
	 * Note: sl_bt_evt_system_boot_id event indicates the device
	 * has started and the radio is ready. Do not call any stack
	 * command before receiving this boot event!
	 */
	case sl_bt_evt_system_boot_id:
		/* Create conn and non-conn adv sets */
		status = sl_bt_advertiser_create_set(&_conn_adv_handle);
		app_assert(status == SL_STATUS_OK,
			   "Failed to create connectable adv set");

		status = sl_bt_advertiser_create_set(&_beacon_adv_handle);
		app_assert(status == SL_STATUS_OK,
			   "Failed to create beacon adv set");

		status = sl_bt_advertiser_configure(
			_beacon_adv_handle,
			SL_BT_ADVERTISER_USE_NONRESOLVABLE_ADDRESS);
		app_assert(status == SL_STATUS_OK,
			   "Failed to configure beacon adv set to NRPA");

		status = sl_bt_advertiser_set_timing(
			_beacon_adv_handle,
			MS_TO_ADV_INTERVAL(ADV_INTERVAL_MIN_MS),
			MS_TO_ADV_INTERVAL(ADV_INTERVAL_MAX_MS), 0, 0);
		app_assert(status == SL_STATUS_OK,
			   "Failed to set timing for beacon adv set");

		status = _start_connectable_adv();
		app_assert(status == SL_STATUS_OK,
			   "Failed to start connectable adv");
		break;

	case sl_bt_evt_connection_opened_id:
		app_log_debug(APP_LOG_PREFIX "Connected to a device\n");
		break;

	case sl_bt_evt_connection_closed_id:
		app_log_debug(APP_LOG_PREFIX "Disconnected (reason=0x%04x)\n",
			      evt->data.evt_connection_closed.reason);

		if (unix_time_ms != 0) {
			xSemaphoreGive(sync_sem);
		} else {
			_start_connectable_adv();
		}
		break;

	case sl_bt_evt_gatt_server_user_write_request_id: {
		sl_bt_evt_gatt_server_user_write_request_t *req =
			&evt->data.evt_gatt_server_user_write_request;
		sl_status_t att_err = SL_STATUS_BT_ATT_REQUEST_NOT_SUPPORTED;

		/*
		 * The gattdb_hubble_prov provisioning characteristic
		 * is defined in the project's .btconf
		 */
		if (req->characteristic == gattdb_hubble_prov) {
			att_err = _chr_write_cb(req->value.data, req->value.len);
		}

		/* If this is a write with response, then send a reply */
		if (req->att_opcode == sl_bt_gatt_write_request) {
			/*
			 * The low byte of SL_STATUS_BT_ATT_* is the 1-byte ATT
			 * error code from the Bluetooth spec
			 */
			(void)sl_bt_gatt_server_send_user_write_response(
				req->connection, req->characteristic,
				(uint8_t)att_err);
		}
		break;
	}

	default:
		break;
	}
}

int ble_init(void)
{
	BaseType_t ret;

	_bt_sem = xSemaphoreCreateBinary();
	if (_bt_sem == NULL) {
		return -ENOMEM;
	}

	/* Make the count to 1 initially */
	(void)xSemaphoreGive(_bt_sem);

	ret = xTaskCreate(_adv_refresh_task, "ble_adv_refresh",
			  ADV_REFRESH_TASK_STACK_SIZE, NULL,
			  ADV_REFRESH_TASK_PRIORITY, &_adv_refresh_task_handle);
	if (ret != pdPASS) {
		vSemaphoreDelete(_bt_sem);
		return -ENOMEM;
	}

	/*
	 * The GATT services and adv sets is handled by the
	 * Bluetooth stack in sl_bt_on_event()
	 */
	return 0;
}
