/*
 * Copyright (c) 2026 Hubble Network, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"

/* ESP-IDF NimBLE includes */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#define BT_NIMBLE_HOST_STACK_SIZE    4096
#define BT_NIMBLE_HOST_TASK_PRIORITY 5

#define CONFIG_BT_DEVICE_NAME        "ESP32-C6"
#define HUBBLE_BLE_UUID_SYNC         0xFCA7
#define HUBBLE_BLE_PKT_CMD           2

enum app_cmd_options {
	APP_BLE_CMD_KEY_SET,
	APP_BLE_CMD_EPOCH_SET = 2,
	APP_BLE_CMD_ORBITAL_INFO_SET,
	APP_BLE_CMD_EPHEMERIS_SET = 7,
};

static const char *BLE_TAG = "ble_sync";

/*
 * 128-bit UUIDs in little-endian byte order for NimBLE.
 * Service: ef2dc9a1-40be-44b6-9dda-8a00fcd61dc0
 * Characteristic: ef2dc9a1-40be-44b6-9dda-8a00fcd61dc1
 */
static const ble_uuid128_t _app_svc_uuid =
	BLE_UUID128_INIT(0xc0, 0x1d, 0xd6, 0xfc, 0x00, 0x8a, 0xda, 0x9d, 0xb6,
			 0x44, 0xbe, 0x40, 0xa1, 0xc9, 0x2d, 0xef);
static const ble_uuid128_t _app_chr_uuid =
	BLE_UUID128_INIT(0xc1, 0x1d, 0xd6, 0xfc, 0x00, 0x8a, 0xda, 0x9d, 0xb6,
			 0x44, 0xbe, 0x40, 0xa1, 0xc9, 0x2d, 0xef);

static SemaphoreHandle_t _hubble_init_sem;
static uint16_t _conn_handle = BLE_HS_CONN_HANDLE_NONE;

static uint64_t _unix_epoch_ms;

/* Library function declarations */
void ble_store_config_init(void);

/* Forward declare */
static void _start_advertising(void);
static int _app_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
			      struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def _gatt_svr_svcs[] = {
	{
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = &_app_svc_uuid.u,
		.characteristics =
			(struct ble_gatt_chr_def[]){
				{
					.uuid = &_app_chr_uuid.u,
					.access_cb = _app_chr_access_cb,
					.flags = BLE_GATT_CHR_F_WRITE |
						 BLE_GATT_CHR_F_WRITE_NO_RSP,
				},
				{0} /* No more characteristics */
			},
	},
	/* No more services */
	{0},
};

static int _app_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
			      struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
		return BLE_ATT_ERR_UNLIKELY;
	}

	struct os_mbuf *om = ctxt->om;
	uint16_t len = OS_MBUF_PKTLEN(om);

	uint8_t header[2];
	if (len < 2 || os_mbuf_copydata(om, 0, 2, header) != 0) {
		return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
	}

	if (header[0] != HUBBLE_BLE_PKT_CMD) {
		return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
	}

	switch (header[1]) {
	case APP_BLE_CMD_EPOCH_SET:
		if (len != (2 + sizeof(uint64_t))) {
			return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
		}

		os_mbuf_copydata(om, 2, sizeof(_unix_epoch_ms), &_unix_epoch_ms);
		break;

	case APP_BLE_CMD_EPHEMERIS_SET:
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
			_conn_handle = event->connect.conn_handle;
			ESP_LOGD(BLE_TAG, "Connected (handle=%d)", _conn_handle);
		} else {
			ESP_LOGW(BLE_TAG, "Connection failed, restarting adv");
			_start_advertising();
		}
		break;

	case BLE_GAP_EVENT_DISCONNECT:
		ESP_LOGD(BLE_TAG, "Disconnected (reason=0x%02x)",
			 event->disconnect.reason);
		_conn_handle = BLE_HS_CONN_HANDLE_NONE;

		if (_unix_epoch_ms != 0xdeadbeef) {
			xSemaphoreGive(_hubble_init_sem);
		} else {
			/* Restart adv */
			_start_advertising();
		}
		break;

	default:
		break;
	}

	return 0;
}

static void _start_advertising(void)
{
	struct ble_hs_adv_fields adv_fields = {0};
	struct ble_gap_adv_params adv_params = {
		.conn_mode = BLE_GAP_CONN_MODE_UND,
		.disc_mode = BLE_GAP_DISC_MODE_GEN,
		.itvl_min = BLE_GAP_ADV_FAST_INTERVAL2_MIN,
		.itvl_max = BLE_GAP_ADV_FAST_INTERVAL2_MAX,
	};
	static uint8_t svc_data[] = {
		HUBBLE_BLE_UUID_SYNC & 0xFF,
		HUBBLE_BLE_UUID_SYNC >> 8,
	};

	adv_fields.uuids16 =
		(ble_uuid16_t[]){BLE_UUID16_INIT(HUBBLE_BLE_UUID_SYNC)};
	adv_fields.num_uuids16 = 1;
	adv_fields.uuids16_is_complete = 1;

	adv_fields.svc_data_uuid16 = svc_data;
	adv_fields.svc_data_uuid16_len = sizeof(svc_data);

	adv_fields.name = (uint8_t *)CONFIG_BT_DEVICE_NAME;
	adv_fields.name_len = strlen(CONFIG_BT_DEVICE_NAME);
	adv_fields.name_is_complete = 1;

	int rc = ble_gap_adv_set_fields(&adv_fields);
	if (rc != 0) {
		ESP_LOGE(BLE_TAG, "Failed to set adv fields (rc=%d)", rc);
		return;
	}

	/* NRPA */
	rc = ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER,
			       &adv_params, _gap_event_handler, NULL);
	if (rc != 0) {
		ESP_LOGE(BLE_TAG, "Failed to start advertising (rc=%d)", rc);
		return;
	}
}

static void _nimble_host_task(void *param)
{
	/* This function won't return until nimble_port_stop() is executed */
	nimble_port_run();

	/* Clean up at exit */
	vTaskDelete(NULL);
}

static void _on_nimble_stack_reset(int reason)
{
	ESP_LOGW(BLE_TAG, "nimble stack reset, reset reason: %d", reason);
}

static void _on_nimble_stack_sync(void)
{
	/* 1 = random address */
	int rc = ble_hs_util_ensure_addr(1);
	if (rc != 0) {
		ESP_LOGE(BLE_TAG, "Failed to ensure address (rc=%d)", rc);
		return;
	}
	_start_advertising();
}

/*
 * @brief Sync time with Hubble Connect App (BLE).
 *
 * @param epoch_ms Pointer to store the synced epoch time in milliseconds.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t ble_sync_time(uint64_t *epoch_ms)
{
	esp_err_t err;

	/* Init the sem */
	_hubble_init_sem = xSemaphoreCreateBinary();
	if (_hubble_init_sem == NULL) {
		ESP_LOGE(BLE_TAG, "Failed to create semaphore");
		return ESP_ERR_NO_MEM;
	}

	/* Nimble stack init */
	err = nimble_port_init();
	if (err != ESP_OK) {
		ESP_LOGE(BLE_TAG,
			 "Failed to initialize NimBLE stack, error: %d", err);
		return err;
	}

	/* Register GATT services */
	ble_svc_gap_init();
	ble_svc_gatt_init();

	/* Update GATT service counter and add the service */
	err = ble_gatts_count_cfg(_gatt_svr_svcs);
	if (err != ESP_OK) {
		ESP_LOGE(BLE_TAG, "Failed to count GATT services, error: %d",
			 err);

		/*
		 * In production, clean up / deinit needs to happen before return.
		 * Since in this sample, the caller will exit main after this
		 * function failed, we can skip the cleanup to avoid code complexity.
		 */
		return err;
	}

	err = ble_gatts_add_svcs(_gatt_svr_svcs);
	if (err != ESP_OK) {
		ESP_LOGE(BLE_TAG, "Failed to add GATT services, error: %d", err);
		return err;
	}

	/* Set host callbacks */
	ble_hs_cfg.reset_cb = _on_nimble_stack_reset;
	ble_hs_cfg.sync_cb = _on_nimble_stack_sync;
	ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

	ble_store_config_init();
	xTaskCreate(_nimble_host_task, "NimBLE Host", BT_NIMBLE_HOST_STACK_SIZE,
		    NULL, BT_NIMBLE_HOST_TASK_PRIORITY, NULL);

	ESP_LOGI(BLE_TAG, "Time sync started...");
	xSemaphoreTake(_hubble_init_sem, portMAX_DELAY);

	ESP_LOGI(BLE_TAG, "Time sync completed, epoch_ms: %llu", _unix_epoch_ms);
	*epoch_ms = _unix_epoch_ms;

	/* Disable bluetooth */
	err = ble_gap_adv_stop();
	if (err != ESP_OK && err != BLE_HS_EALREADY) {
		ESP_LOGE(BLE_TAG, "Failed to stop advertising, error: %d", err);
		return err;
	}

	err = nimble_port_stop();
	if (err != ESP_OK) {
		ESP_LOGE(BLE_TAG, "Failed to stop NimBLE port, error: %d", err);
		return err;
	}

	err = nimble_port_deinit();
	if (err != ESP_OK) {
		ESP_LOGE(BLE_TAG, "Failed to deinit NimBLE port, error: %d", err);
		return err;
	}

	return ESP_OK;
}
