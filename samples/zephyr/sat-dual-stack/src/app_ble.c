/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <hubble/hubble.h>
#include <hubble/ble.h>
#include <hubble/sat/pass_prediction.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <stdint.h>
#include <string.h>

#include "app_ble.h"

LOG_MODULE_REGISTER(app_ble, CONFIG_APP_LOG_LEVEL);

/*
 * 0xFCA7 is the UUID advertised while the device is connectable and waiting to
 * be provisioned. It distinguishes a device that still needs time / orbital
 * parameters from one that is properly beaconing the Hubble UUID (0xFCA6).
 */
#define HUBBLE_BLE_UUID_PROVISION      0xFCA7

/* Buffer used for the Hubble beacon advertisement payload. */
#define HUBBLE_BLE_BUFFER_LEN          31U

#define HUBBLE_CMD                     0x01
#define HUBBLE_CMD_UNIX_EPOCH          0x02
#define HUBBLE_CMD_ORBITAL_PARAMS      0x03
#define HUBBLE_CMD_DEVICE_LOCATION     0x04
#define HUBBLE_ORBITAL_PARAMS_CMD_SIZE 76U
#define HUBBLE_MAX_SAT                 6U

/* Period to refresh the beacon advertisement payload (1 hour). */
#define HUBBLE_ADV_REFRESH_PERIOD      K_HOURS(1)

/*
 *   Service:        0000fca7-0000-1000-8000-00805f9b34fb
 *   Characteristic: 00000005-fca7-4000-8000-00805f9b34fb
 */
#define HUBBLE_PROV_SVC_UUID                                                   \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x0000fca7, 0x0000, 0x1000,     \
					       0x8000, 0x00805f9b34fb))
#define HUBBLE_PROV_CHR_UUID                                                   \
	BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x00000005, 0xfca7, 0x4000,     \
					       0x8000, 0x00805f9b34fb))

/*
 * Provisioning data lives in main.c
 */
extern struct hubble_sat_orbital_params orb_params[];
extern struct hubble_sat_device_pos device_pos;
extern uint8_t orb_params_count;
extern uint64_t unix_time_ms;

/* Given once provisioning completes (time received and peer disconnected). */
extern struct k_sem sync_sem;

/* Hubble beacon advertisement state. */
static uint16_t _beacon_uuid = HUBBLE_BLE_UUID;
static uint8_t _beacon_buffer[HUBBLE_BLE_BUFFER_LEN];
static struct bt_data _beacon_ad[] = {
	BT_DATA(BT_DATA_UUID16_ALL, &_beacon_uuid, sizeof(_beacon_uuid)),
	/* svc data, set at runtime in _beacon_data_update() */
	BT_DATA(BT_DATA_SVC_DATA16, _beacon_buffer, sizeof(_beacon_buffer)),
};

/* Connectable provisioning advertisement. */
static const struct bt_data _prov_ad[] = {
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(HUBBLE_BLE_UUID_PROVISION)),
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

/* Fetch a fresh Hubble beacon payload into the advertisement data set. */
static int _beacon_data_update(void)
{
	size_t out_len = sizeof(_beacon_buffer);
	int err;

	memset(_beacon_buffer, 0, sizeof(_beacon_buffer));
	err = hubble_ble_advertise_get(NULL, 0, _beacon_buffer, &out_len);
	if (err != 0) {
		LOG_ERR("Failed to get Hubble adv data (err=%d)", err);
		return err;
	}

	_beacon_ad[1].data_len = out_len;

	return 0;
}

static void _adv_refresh_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	if (_beacon_data_update() != 0) {
		return;
	}

	(void)bt_le_adv_update_data(_beacon_ad, ARRAY_SIZE(_beacon_ad), NULL, 0);
}

static K_WORK_DEFINE(_adv_refresh_work, _adv_refresh_work_handler);

static void _adv_refresh_timer_cb(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	k_work_submit(&_adv_refresh_work);
}

static K_TIMER_DEFINE(_adv_refresh_timer, _adv_refresh_timer_cb, NULL);

int ble_adv_start(void)
{
	int err;

	if (!bt_is_ready()) {
		err = bt_enable(NULL);
		if (err != 0) {
			return err;
		}
	}

	err = _beacon_data_update();
	if (err != 0) {
		return err;
	}
	LOG_DBG("Beacon advertisement payload: %d bytes", _beacon_ad[1].data_len);

	err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NRPA,
					      BT_GAP_ADV_FAST_INT_MIN_2,
					      BT_GAP_ADV_FAST_INT_MAX_2, NULL),
			      _beacon_ad, ARRAY_SIZE(_beacon_ad), NULL, 0);
	if (err != 0) {
		LOG_ERR("Failed to start beacon adv (err=%d)", err);
		return err;
	}

	k_timer_start(&_adv_refresh_timer, HUBBLE_ADV_REFRESH_PERIOD,
		      HUBBLE_ADV_REFRESH_PERIOD);

	return 0;
}

int ble_adv_stop(void)
{
	k_timer_stop(&_adv_refresh_timer);
	bt_le_adv_stop();
	return bt_disable();
}

static ssize_t _prov_write(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr, const void *buf,
			   uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *data = buf;

	ARG_UNUSED(conn);
	ARG_UNUSED(attr);
	ARG_UNUSED(flags);

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len < 2 || data[0] != HUBBLE_CMD) {
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	switch (data[1]) {
	case HUBBLE_CMD_UNIX_EPOCH:
		if (len != (2 + sizeof(uint64_t))) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		memcpy(&unix_time_ms, &data[2], sizeof(unix_time_ms));
		LOG_INF("Received unix time: %llu", unix_time_ms);
		break;

	case HUBBLE_CMD_ORBITAL_PARAMS:
		struct hubble_sat_orbital_params *dst;
		const uint8_t *p = &data[2];

		if (len != (2 + HUBBLE_ORBITAL_PARAMS_CMD_SIZE)) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		if (orb_params_count >= HUBBLE_MAX_SAT) {
			LOG_WRN("Max satellite count reached, ignoring params");
			break;
		}

		dst = &orb_params[orb_params_count];

		/* Copy field by field to avoid alignment issues. */
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
		LOG_INF("Received orbital params for satellite ID %u",
			dst->satellite_id);
		break;

	case HUBBLE_CMD_DEVICE_LOCATION:
		if (len != (2 + 2 * sizeof(double))) {
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}
		memcpy(&device_pos.lat, &data[2], sizeof(double));
		memcpy(&device_pos.lon, &data[2] + sizeof(double),
		       sizeof(double));
		break;
	default:
		return BT_GATT_ERR(BT_ATT_ERR_NOT_SUPPORTED);
	}

	return len;
}

/* Provisioning GATT service. */
BT_GATT_SERVICE_DEFINE(
	hubble_prov_svc, BT_GATT_PRIMARY_SERVICE(HUBBLE_PROV_SVC_UUID),
	BT_GATT_CHARACTERISTIC(HUBBLE_PROV_CHR_UUID,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE, NULL, _prov_write, NULL), );

static int _start_provisioning_adv(void)
{
	int err;

	err = bt_le_adv_start(
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_NRPA | BT_LE_ADV_OPT_CONN,
				BT_GAP_ADV_FAST_INT_MIN_2,
				BT_GAP_ADV_FAST_INT_MAX_2, NULL),
		_prov_ad, ARRAY_SIZE(_prov_ad), NULL, 0);
	if (err != 0) {
		LOG_ERR("Failed to start provisioning adv (err=%d)", err);
	}

	return err;
}

static void _prov_restart_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	(void)_start_provisioning_adv();
}

static K_WORK_DEFINE(_prov_restart_work, _prov_restart_work_handler);

static void _connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	if (err != 0) {
		LOG_WRN("Connection failed (err 0x%02x), restarting adv", err);
		k_work_submit(&_prov_restart_work);
		return;
	}

	LOG_DBG("Connected, waiting for provisioning data");
}

static void _disconnected(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(conn);

	LOG_DBG("Disconnected (reason 0x%02x)", reason);

	if (unix_time_ms != 0U) {
		k_sem_give(&sync_sem);
	} else {
		k_work_submit(&_prov_restart_work);
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = _connected,
	.disconnected = _disconnected,
};

int ble_init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	return _start_provisioning_adv();
}
