/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

/* ESP-IDF NimBLE includes */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include <hubble/hubble.h>
#include <hubble/ble.h>
#include <hubble/sat/pass_prediction.h>

#include "app_ble.h"

static const char *BLE_TAG = "ble";

#define CONFIG_BT_DEVICE_NAME          "Hubble-ESP"

#define HUBBLE_BLE_UUID_CONNECTABLE    0xFCA7
#define HUBBLE_BLE_BUFFER_LEN          31U

#define ADV_INTERVAL_MIN_MS            1000
#define ADV_INTERVAL_MAX_MS            1200

/* Period to update adv packets in microseconds (1 hour) */
#define HUBBLE_ADV_PACKET_PERIOD       3600000000UL

/* TODO: replace this by actual command once finalized */
#define HUBBLE_CMD                     0x01
#define HUBBLE_CMD_UNIX_EPOCH          0x02
#define HUBBLE_CMD_ORBITAL_PARAMS      0x03
#define HUBBLE_CMD_DEVICE_LOCATION     0x04
#define HUBBLE_ORBITAL_PARAMS_CMD_SIZE 76

/*
 * NOTE: If the caller is running in the same task as the NimBLE host, or if it
 * is running in a higher priority task than that of the host, care must be
 * taken when restarting advertising.  Under these conditions, the following is
 * *not* a reliable method to restart advertising:
 *     ble_gap_adv_stop()
 *     ble_gap_adv_start()
 *
 * Since the refresh task will restart advertising, the caller runs on a lower
 * priority than the NimBLE host.
 */
#define NIMBLE_HOST_TASK_STACK_SIZE    4096
#define NIMBLE_HOST_TASK_PRIORITY      5
#define ADV_REFRESH_TASK_STACK_SIZE    2048
#define ADV_REFRESH_TASK_PRIORITY      4

/*
 * 128-bit UUIDs in little-endian byte order
 * Service: 0000fca7-0000-1000-8000-00805f9b34fb
 * Characteristic: 00000005-fca7-4000-8000-00805f9b34fb
 */
static const ble_uuid128_t _svc_uuid =
	BLE_UUID128_INIT(0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00,
			 0x10, 0x00, 0x00, 0xa7, 0xfc, 0x00, 0x00);
static const ble_uuid128_t _chr_uuid =
	BLE_UUID128_INIT(0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00,
			 0x40, 0xa7, 0xfc, 0x05, 0x00, 0x00, 0x00);

/* Extern variables */
extern struct hubble_sat_device_pos device_pos;
extern struct hubble_sat_orbital_params orb_params[];
extern uint8_t orb_params_count;
extern uint64_t unix_time_ms;

/* Sem to sync time and orbital params */
extern SemaphoreHandle_t sync_sem;

/* Timers */
static esp_timer_handle_t _ble_timer;

/* Task notification for adv update */
static TaskHandle_t _adv_refresh_task_handle;

/* BLE adv specifics */
static uint16_t _conn_adv_handle = BLE_HS_CONN_HANDLE_NONE;
static uint8_t _beacon_adv_buffer[HUBBLE_BLE_BUFFER_LEN];

/* Library function declarations */
void ble_store_config_init(void);

/* Forward declarations */
static void _nimble_host_task(void *param);
static void _start_connectable_adv(void);
static int _chr_write_cb(uint16_t conn_handle, uint16_t attr_handle,
			 struct ble_gatt_access_ctxt *ctxt, void *arg);

/* GATT service (for conn adv) */
static const struct ble_gatt_svc_def _gatt_svr_svcs[] = {
	{
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = &_svc_uuid.u,
		.characteristics =
			(struct ble_gatt_chr_def[]){
				{
					.uuid = &_chr_uuid.u,
					.access_cb = _chr_write_cb,
					.flags = BLE_GATT_CHR_F_WRITE |
						 BLE_GATT_CHR_F_WRITE_NO_RSP,
				},
				{0}, /* No more characteristics */
			},
	},
	/* No more services */
	{0},
};

static void _ble_refresh_timer_cb(void *arg)
{
	xTaskNotifyGive(_adv_refresh_task_handle);
}

static void _adv_refresh_task(void *param)
{
	for (;;) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if (!ble_gap_adv_active()) {
			continue;
		}

		ble_adv_stop();
		ble_adv_start();
	}
}

/* BLE Beacon */
int ble_adv_start(void)
{
	int rc;
	esp_err_t ret;
	size_t out_len = sizeof(_beacon_adv_buffer);

	struct ble_hs_adv_fields adv_fields = {0};
	struct ble_gap_adv_params adv_params = {
		.conn_mode = BLE_GAP_CONN_MODE_NON,
		.disc_mode = BLE_GAP_DISC_MODE_GEN,
		.itvl_min = BLE_GAP_ADV_ITVL_MS(ADV_INTERVAL_MIN_MS),
		.itvl_max = BLE_GAP_ADV_ITVL_MS(ADV_INTERVAL_MAX_MS),
	};

	/* Start BLE adv refresh timer */
	ret = esp_timer_start_once(_ble_timer,
				   (uint64_t)HUBBLE_ADV_PACKET_PERIOD);
	if (ret != ESP_OK) {
		ESP_LOGE(BLE_TAG,
			 "Failed to start BLE adv refresh timer (ret=%d)", ret);
		return -EINVAL;
	}

	/* Prepare the adv data */
	memset(_beacon_adv_buffer, 0, sizeof(_beacon_adv_buffer));
	rc = hubble_ble_advertise_get(NULL, 0, _beacon_adv_buffer, &out_len);
	if (rc != 0) {
		ESP_LOGE(BLE_TAG, "Failed to get Hubble adv data (rc=%d)", rc);
		return rc;
	}

	adv_fields.uuids16 = (ble_uuid16_t[]){BLE_UUID16_INIT(HUBBLE_BLE_UUID)};
	adv_fields.num_uuids16 = 1;
	adv_fields.uuids16_is_complete = 1;

	adv_fields.svc_data_uuid16 = _beacon_adv_buffer;
	adv_fields.svc_data_uuid16_len = out_len;

	adv_fields.name = (uint8_t *)CONFIG_BT_DEVICE_NAME;
	adv_fields.name_len = strlen(CONFIG_BT_DEVICE_NAME);
	adv_fields.name_is_complete = 1;

	rc = ble_gap_adv_set_fields(&adv_fields);
	if (rc != 0) {
		ESP_LOGE(BLE_TAG, "Failed to set adv fields (rc=%d)", rc);
		return rc;
	}

	rc = ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER,
			       &adv_params, NULL, NULL);
	if (rc != 0 && rc != BLE_HS_EALREADY) {
		ESP_LOGE(BLE_TAG, "Failed to start beacon adv (rc=%d)", rc);
		return rc;
	}

	return 0;
}

int ble_adv_stop(void)
{
	/* if already stop, do not call again */
	if (!ble_gap_adv_active()) {
		return 0;
	}

	(void)esp_timer_stop(_ble_timer);
	return ble_gap_adv_stop();
}

/* Connectable Adv */
static int _chr_write_cb(uint16_t conn_handle, uint16_t attr_handle,
			 struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
		return BLE_ATT_ERR_UNLIKELY;
	}

	struct os_mbuf *rx_mbuf = ctxt->om;
	uint16_t len = OS_MBUF_PKTLEN(rx_mbuf);

	uint8_t header[2];
	if (len < 2 || os_mbuf_copydata(rx_mbuf, 0, 2, header) != 0) {
		return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
	}

	if (header[0] != HUBBLE_CMD) {
		return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
	}

	switch (header[1]) {
	case HUBBLE_CMD_UNIX_EPOCH:
		if (len != (2 + sizeof(uint64_t))) {
			return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
		}

		os_mbuf_copydata(rx_mbuf, 2, sizeof(unix_time_ms), &unix_time_ms);

		ESP_LOGI(BLE_TAG, "Received unix time: %llu", unix_time_ms);
		break;

	case HUBBLE_CMD_ORBITAL_PARAMS: {
		if (len != (2 + HUBBLE_ORBITAL_PARAMS_CMD_SIZE)) {
			return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
		}

		if (orb_params_count >= HUBBLE_MAX_SAT) {
			ESP_LOGW(BLE_TAG, "Max satellite count reached, cannot "
					  "add more orbital params");
			break;
		}

		struct hubble_sat_orbital_params *dst =
			&orb_params[orb_params_count];
		uint8_t orb_params_data[HUBBLE_ORBITAL_PARAMS_CMD_SIZE];
		os_mbuf_copydata(rx_mbuf, 2, HUBBLE_ORBITAL_PARAMS_CMD_SIZE,
				 orb_params_data);

		/* Let's copy field by field to avoid alignment issues */
		memcpy(&dst->t0, orb_params_data, sizeof(uint64_t));
		memcpy(&dst->n0, orb_params_data + 8, sizeof(double));
		memcpy(&dst->ndot, orb_params_data + 16, sizeof(double));
		memcpy(&dst->raan0, orb_params_data + 24, sizeof(double));
		memcpy(&dst->raandot, orb_params_data + 32, sizeof(double));
		memcpy(&dst->aop0, orb_params_data + 40, sizeof(double));
		memcpy(&dst->aopdot, orb_params_data + 48, sizeof(double));
		memcpy(&dst->inclination, orb_params_data + 56, sizeof(double));
		memcpy(&dst->eccentricity, orb_params_data + 64, sizeof(double));
		memcpy(&dst->satellite_id, orb_params_data + 72,
		       sizeof(uint32_t));

		++orb_params_count;

		ESP_LOGI(BLE_TAG, "Received orbital params for satellite ID %u",
			 dst->satellite_id);
		break;
	}

	case HUBBLE_CMD_DEVICE_LOCATION:
		if (len != (2 + 2 * sizeof(double))) {
			return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
		}

		os_mbuf_copydata(rx_mbuf, 2, sizeof(double), &device_pos.lat);
		os_mbuf_copydata(rx_mbuf, 2 + sizeof(double), sizeof(double),
				 &device_pos.lon);

		ESP_LOGI(BLE_TAG, "Received device location: lat=%f, lon=%f",
			 device_pos.lat, device_pos.lon);
		break;

	default:
		return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
	}

	return 0;
}

static int _gap_event_handler(struct ble_gap_event *event, void *arg)
{
	switch (event->type) {
	case BLE_GAP_EVENT_CONNECT:
		if (event->connect.status == 0) {
			_conn_adv_handle = event->connect.conn_handle;
			ESP_LOGD(BLE_TAG, "Connected (handle=%d)",
				 _conn_adv_handle);

		} else {
			ESP_LOGW(BLE_TAG, "Connection failed, restarting adv");
			_start_connectable_adv();
		}
		break;

	case BLE_GAP_EVENT_DISCONNECT:
		ESP_LOGD(BLE_TAG, "Disconnected (reason=0x%02x)",
			 event->disconnect.reason);
		_conn_adv_handle = BLE_HS_CONN_HANDLE_NONE;

		if (unix_time_ms != 0) {
			xSemaphoreGive(sync_sem);
		} else {
			_start_connectable_adv();
		}
		break;

	default:
		break;
	}

	return 0;
}

static void _start_connectable_adv(void)
{
	int rc;
	struct ble_hs_adv_fields adv_fields = {0};
	struct ble_gap_adv_params adv_params = {
		.conn_mode = BLE_GAP_CONN_MODE_UND,
		.disc_mode = BLE_GAP_DISC_MODE_GEN,
		.itvl_min = BLE_GAP_ADV_FAST_INTERVAL2_MIN,
		.itvl_max = BLE_GAP_ADV_FAST_INTERVAL2_MAX,
	};

	static uint8_t svc_data[] = {
		HUBBLE_BLE_UUID_CONNECTABLE & 0xFF,
		HUBBLE_BLE_UUID_CONNECTABLE >> 8,
	};

	adv_fields.uuids16 =
		(ble_uuid16_t[]){BLE_UUID16_INIT(HUBBLE_BLE_UUID_CONNECTABLE)};
	adv_fields.num_uuids16 = 1;
	adv_fields.uuids16_is_complete = 1;

	adv_fields.svc_data_uuid16 = svc_data;
	adv_fields.svc_data_uuid16_len = sizeof(svc_data);

	adv_fields.name = (uint8_t *)CONFIG_BT_DEVICE_NAME;
	adv_fields.name_len = strlen(CONFIG_BT_DEVICE_NAME);
	adv_fields.name_is_complete = 1;

	rc = ble_gap_adv_set_fields(&adv_fields);
	if (rc != 0) {
		ESP_LOGE(BLE_TAG,
			 "Failed to set connectable adv fields (rc=%d)", rc);
		return;
	}

	rc = ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER,
			       &adv_params, _gap_event_handler, NULL);
	if (rc != 0) {
		ESP_LOGE(BLE_TAG, "Failed to start connectable adv (rc=%d)", rc);
	}
}

/* NimBLE host stack */
static void _nimble_host_task(void *param)
{
	/* This function won't return until nimble_port_stop() is executed */
	nimble_port_run();

	/* Clean up at exit */
	vTaskDelete(NULL);
}

static void _on_nimble_stack_reset(int reason)
{
	ESP_LOGW(BLE_TAG, "NimBLE stack reset (reason=%d)", reason);
}

static void _on_nimble_stack_sync(void)
{
	/* 1 = random address */
	int rc = ble_hs_util_ensure_addr(1);
	if (rc != 0) {
		ESP_LOGE(BLE_TAG, "Failed to ensure address (rc=%d)", rc);
		return;
	}

	_start_connectable_adv();
}

int ble_init(void)
{
	int rc;
	esp_err_t ret;

	/* Create / Init timers and sems */
	esp_timer_create_args_t ble_timer_args = {
		.callback = _ble_refresh_timer_cb,
		.name = "ble_adv_refresh",
	};

	ret = esp_timer_create(&ble_timer_args, &_ble_timer);
	if (ret != ESP_OK) {
		ESP_LOGE(BLE_TAG, "Failed to create BLE timer (rc=%d)", ret);
		return -EINVAL;
	}

	ret = nimble_port_init();
	if (ret != ESP_OK) {
		ESP_LOGE(BLE_TAG, "Failed to init NimBLE (rc=%d)", ret);
		return -EAGAIN;
	}

	/* Register GATT services */
	ble_svc_gap_init();
	ble_svc_gatt_init();

	/* Update GATT service couner and add the service */
	rc = ble_gatts_count_cfg(_gatt_svr_svcs);
	if (rc != 0) {
		ESP_LOGE(BLE_TAG, "Failed to count GATT services (rc=%d)", rc);
		goto exit;
	}

	rc = ble_gatts_add_svcs(_gatt_svr_svcs);
	if (rc != 0) {
		ESP_LOGE(BLE_TAG, "Failed to add GATT services (rc=%d)", rc);
		goto exit;
	}

	/* Host callbacks */
	ble_hs_cfg.reset_cb = _on_nimble_stack_reset;
	ble_hs_cfg.sync_cb = _on_nimble_stack_sync;
	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

	ble_store_config_init();
	xTaskCreate(_nimble_host_task, "NimBLE_Host", NIMBLE_HOST_TASK_STACK_SIZE,
		    NULL, NIMBLE_HOST_TASK_PRIORITY, NULL);

	/* adv refresh task */
	xTaskCreate(_adv_refresh_task, "adv_refresh", ADV_REFRESH_TASK_STACK_SIZE,
		    NULL, ADV_REFRESH_TASK_PRIORITY, &_adv_refresh_task_handle);

	return 0;

exit:
	/* Clean up resources */
	ble_svc_gap_deinit();
	ble_svc_gatt_deinit();
	(void)esp_timer_delete(_ble_timer);
	(void)nimble_port_deinit();

	return rc;
}
