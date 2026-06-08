/*
 * Copyright (c) 2026 HubbleNetwork
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ti/ble/app_util/framework/bleapputil_api.h"
#include "ti/ble/host/gap/gap_advertiser.h"
#include <ti/ble/host/gatt/gattservapp.h>
#include "ti/drivers/dpl/ClockP.h"
#include "ti/drivers/dpl/SemaphoreP.h"
#include "ti/drivers/dpl/TaskP.h"
#include <ti/log/Log.h>

#include <hubble/hubble.h>
#include <hubble/ble.h>
#include <hubble/sat/pass_prediction.h>

#include "app_ble.h"

#define HUBBLE_BLE_UUID_CONNECTABLE 0xFCA7
#define HUBBLE_BLE_BUFFER_LEN       31U

#define BEACON_ADV_INTERVAL_MIN     3200U /* 2000 ms */
#define BEACON_ADV_INTERVAL_MAX     4000U /* 2500 ms */

/* Period to update adv packets in microseconds (1 hour) */
#define HUBBLE_ADV_PACKET_PERIOD    3600000000UL

#define BLE_ADV_HEADER_SIZE         6U
#define HUBBLE_BLE_ADV_HEADER                                                  \
	0x03, GAP_ADTYPE_16BIT_COMPLETE, LO_UINT16(0xfca6), HI_UINT16(0xfca6), \
		0x01, GAP_ADTYPE_SERVICE_DATA,

/* TODO: replace this by actual command once finalized */
#define HUBBLE_CMD                     0x01
#define HUBBLE_CMD_UNIX_EPOCH          0x02
#define HUBBLE_CMD_ORBITAL_PARAMS      0x03
#define HUBBLE_CMD_DEVICE_LOCATION     0x04
#define HUBBLE_ORBITAL_PARAMS_CMD_SIZE 76

/*
 * 128-bit UUIDs in little-endian byte order
 * Service: 0000fca7-0000-1000-8000-00805f9b34fb
 * Characteristic: 00000005-fca7-4000-8000-00805f9b34fb
 */
static const uint8_t _hubble_svc_uuid[ATT_UUID_SIZE] = {
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x10, 0x00, 0x00, 0xa7, 0xfc, 0x00, 0x00,
};
static const uint8_t _hubble_chr_uuid[ATT_UUID_SIZE] = {
	0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x40, 0xa7, 0xfc, 0x05, 0x00, 0x00, 0x00,
};

static const gattAttrType_t _hubble_svc = {ATT_UUID_SIZE, _hubble_svc_uuid};
static uint8_t _hubble_chr_props = GATT_PROP_WRITE | GATT_PROP_WRITE_NO_RSP;
static uint8_t _hubble_chr_val;

static gattAttribute_t _hubble_attr_tbl[] = {
	GATT_BT_ATT(primaryServiceUUID, GATT_PERMIT_READ, (uint8_t *)&_hubble_svc),
	GATT_BT_ATT(characterUUID, GATT_PERMIT_READ, &_hubble_chr_props),
	GATT_ATT(_hubble_chr_uuid, GATT_PERMIT_WRITE, &_hubble_chr_val),
};

/* Forward declare */
static void _hubble_conn_start(void);
static bStatus_t _write_attr_cb(uint16 conn_handle, gattAttribute_t *attr,
				uint8 *val, uint16 len, uint16 offset,
				uint8 method);

static const gattServiceCBs_t _hubble_svc_callbacks = {
	.pfnReadAttrCB = NULL,
	.pfnWriteAttrCB = _write_attr_cb,
	.pfnAuthorizeAttrCB = NULL};

/* BLE adv specifics */
static uint8_t _conn_adv_handle;
static uint8_t _beacon_adv_handle;
static uint8_t _beacon_adv_data[HUBBLE_BLE_BUFFER_LEN] = {HUBBLE_BLE_ADV_HEADER};

static TaskP_Struct _ble_adv_task_struct;
static TaskP_Handle _ble_adv_task_handle;
static uint8_t _ble_adv_task_stack[2048];
static TaskP_Params _ble_adv_task_params = {
	.stackSize = sizeof(_ble_adv_task_stack),
	.stack = _ble_adv_task_stack,
};

/* Timer */
static ClockP_Struct _adv_timer_struct;
static ClockP_Handle _adv_timer_handle;
static ClockP_Params _adv_timer_params;

/* For sync time and update adv data */
static SemaphoreP_Struct _sync_sem_struct;
static SemaphoreP_Handle _sync_sem_handle;
static SemaphoreP_Struct _ble_update_sem_struct;
static SemaphoreP_Handle _ble_update_sem_handle;

/* App specific variables */
extern struct hubble_sat_device_pos device_pos;
extern struct hubble_sat_orbital_params orb_params[];
extern uint8_t orb_params_count;
extern uint64_t unix_time_ms;

/* Adv data */
static GapAdv_params_t _conn_adv_params = {
	.eventProps = GAP_ADV_PROP_CONNECTABLE | GAP_ADV_PROP_SCANNABLE |
		      GAP_ADV_PROP_LEGACY,
	.primIntMin = 160,
	.primIntMax = 160,
	.primChanMap = GAP_ADV_CHAN_ALL,
	.peerAddrType = PEER_ADDRTYPE_RANDOM_OR_RANDOM_ID,
	.filterPolicy = GAP_ADV_AL_POLICY_ANY_REQ,
	.txPower = GAP_ADV_TX_POWER_NO_PREFERENCE,
	.primPhy = GAP_ADV_PRIM_PHY_1_MBPS,
	.secPhy = GAP_ADV_SEC_PHY_1_MBPS,
	.sid = 0,
	.zeroDelay = 0};

static GapAdv_params_t _beacon_adv_params = {
	.eventProps = GAP_ADV_PROP_SCANNABLE | GAP_ADV_PROP_LEGACY,
	.primIntMin = BEACON_ADV_INTERVAL_MIN,
	.primIntMax = BEACON_ADV_INTERVAL_MAX,
	.primChanMap = GAP_ADV_CHAN_ALL,
	.peerAddrType = PEER_ADDRTYPE_RANDOM_OR_RANDOM_ID,
	.filterPolicy = GAP_ADV_AL_POLICY_ANY_REQ,
	.txPower = GAP_ADV_TX_POWER_NO_PREFERENCE,
	.primPhy = GAP_ADV_PRIM_PHY_1_MBPS,
	.secPhy = GAP_ADV_SEC_PHY_1_MBPS,
	.sid = 1,
	.zeroDelay = 0};

static uint8_t _conn_adv_data[] = {
	0x02, /* len = type + 1 */
	GAP_ADTYPE_FLAGS,
	GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,

	0x03, /* len = type + 2 bytes UUID */
	GAP_ADTYPE_16BIT_COMPLETE,
	LO_UINT16(HUBBLE_BLE_UUID_CONNECTABLE),
	HI_UINT16(HUBBLE_BLE_UUID_CONNECTABLE),
};

static uint8_t _sd[] = {
	0x0A, /* len = type + 9 bytes name */
	GAP_ADTYPE_LOCAL_NAME_COMPLETE,
	'H',
	'u',
	'b',
	'b',
	'l',
	'e',
	'-',
	'T',
	'I',
};

static const BLEAppUtil_AdvInit_t _conn_adv_init_set = {
	.advDataLen = sizeof(_conn_adv_data),
	.advData = _conn_adv_data,
	.scanRespDataLen = sizeof(_sd),
	.scanRespData = _sd,
	.advParam = &_conn_adv_params,
};

static const BLEAppUtil_AdvInit_t _beacon_adv_init_set = {
	.advDataLen = sizeof(_beacon_adv_data),
	.advData = _beacon_adv_data,
	.scanRespDataLen = sizeof(_sd),
	.scanRespData = _sd,
	.advParam = &_beacon_adv_params,
};

const BLEAppUtil_AdvStart_t _hubble_start_adv_set = {
	/* Use the maximum possible value. This is the spec-defined maximum for */
	/* directed advertising and infinite advertising for all other types */
	.enableOptions = GAP_ADV_ENABLE_OPTIONS_USE_MAX,
	.durationOrMaxEvents = 0,
};

/* GATT cb */
static bStatus_t _write_attr_cb(uint16 conn_handle, gattAttribute_t *attr,
				uint8 *val, uint16 len, uint16 offset,
				uint8 method)
{
	/* Match by 128-bit UUID */
	if (attr->type.len != ATT_UUID_SIZE ||
	    memcmp(attr->type.uuid, _hubble_chr_uuid, ATT_UUID_SIZE) != 0) {
		return ATT_ERR_ATTR_NOT_FOUND;
	}

	/* No partial / blob writes */
	if (offset != 0) {
		return ATT_ERR_ATTR_NOT_LONG;
	}

	/* Need at least the 2 bytes cmd */
	if (len < 2) {
		return ATT_ERR_INVALID_VALUE_SIZE;
	}

	if (val[0] != HUBBLE_CMD) {
		return ATT_ERR_UNSUPPORTED_REQ;
	}

	switch (val[1]) {
	case HUBBLE_CMD_UNIX_EPOCH:
		if (len != 2 + sizeof(uint64_t)) {
			return ATT_ERR_INVALID_VALUE_SIZE;
		}

		memcpy(&unix_time_ms, &val[2], sizeof(unix_time_ms));

		/* Work around because Log cast to uintptr_t (32-bit) */
		Log_printf(Log_Dual_Stack, Log_DEBUG,
			   "Received UNIX time 0x%08x%08x ms",
			   (uint32_t)(unix_time_ms >> 32),
			   (uint32_t)unix_time_ms);
		break;

	case HUBBLE_CMD_ORBITAL_PARAMS: {
		if (len != (2 + HUBBLE_ORBITAL_PARAMS_CMD_SIZE)) {
			Log_printf(Log_Dual_Stack, Log_WARNING,
				   "Invalid orbital params data length: %d", len);
			return ATT_ERR_INVALID_VALUE_SIZE;
		}

		if (orb_params_count >= HUBBLE_MAX_SAT) {
			Log_printf(
				Log_Dual_Stack,
				Log_WARNING, "Received orbital params data but max satellite count reached");
			break;
		}

		const uint8_t *data_p = &val[2];
		struct hubble_sat_orbital_params *dst =
			&orb_params[orb_params_count];

		/* Let's copy field by field to avoid alignment issues */
		memcpy(&dst->t0, data_p, sizeof(uint64_t));
		memcpy(&dst->n0, data_p + 8, sizeof(double));
		memcpy(&dst->ndot, data_p + 16, sizeof(double));
		memcpy(&dst->raan0, data_p + 24, sizeof(double));
		memcpy(&dst->raandot, data_p + 32, sizeof(double));
		memcpy(&dst->aop0, data_p + 40, sizeof(double));
		memcpy(&dst->aopdot, data_p + 48, sizeof(double));
		memcpy(&dst->inclination, data_p + 56, sizeof(double));
		memcpy(&dst->eccentricity, data_p + 64, sizeof(double));
		memcpy(&dst->satellite_id, data_p + 72, sizeof(uint32_t));

		++orb_params_count;

		Log_printf(Log_Dual_Stack, Log_DEBUG,
			   "Received orbital params data for satellite ID %u",
			   dst->satellite_id);
		break;
	}

	case HUBBLE_CMD_DEVICE_LOCATION:
		if (len != 2 + 2 * sizeof(double)) {
			return ATT_ERR_INVALID_VALUE_SIZE;
		}

		memcpy(&device_pos.lat, &val[2], sizeof(double));
		memcpy(&device_pos.lon, &val[2] + sizeof(double), sizeof(double));
		break;

	default:
		return ATT_ERR_UNSUPPORTED_REQ;
	}

	return SUCCESS;
}

/* Conn handlers */
static void _conn_event_handler(uint32 event, BLEAppUtil_msgHdr_t *pMsg)
{
	switch (event) {
	case BLEAPPUTIL_LINK_ESTABLISHED_EVENT:
		Log_printf(Log_Dual_Stack, Log_DEBUG, "Client connected");

		break;

	case BLEAPPUTIL_LINK_TERMINATED_EVENT:
		Log_printf(Log_Dual_Stack, Log_DEBUG, "Disconnected");

		if (unix_time_ms != 0) {
			(void)hubble_time_set(unix_time_ms);
			SemaphoreP_post(_sync_sem_handle);

		} else {
			_hubble_conn_start();
		}
		break;
	}
}

static BLEAppUtil_EventHandler_t _conn_handler = {
	.handlerType = BLEAPPUTIL_GAP_CONN_TYPE,
	.pEventHandler = _conn_event_handler,
	.eventMask = BLEAPPUTIL_LINK_ESTABLISHED_EVENT |
		     BLEAPPUTIL_LINK_TERMINATED_EVENT,
};

/* Timers cbs and tasks function */
static void _ble_timer_cb(uintptr_t arg)
{
	SemaphoreP_post(_ble_update_sem_handle);
}

static void _ble_adv_update(void *arg)
{
	(void)arg;

	size_t len = HUBBLE_BLE_BUFFER_LEN - BLE_ADV_HEADER_SIZE;
	int status = hubble_ble_advertise_get(
		NULL, 0, &_beacon_adv_data[BLE_ADV_HEADER_SIZE], &len);

	if (status) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to get advertise data, err: %d", status);
		return;
	}

	/* output len + BLE service data type */
	_beacon_adv_data[4] = len + 1;

	if (GapAdv_prepareLoadByHandle(_beacon_adv_handle,
				       GAP_ADV_FREE_OPTION_DONT_FREE) != SUCCESS) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to prepare adv data load");
		return;
	}

	if (GapAdv_loadByHandle(_beacon_adv_handle, GAP_ADV_DATA_TYPE_ADV,
				sizeof(_beacon_adv_data),
				_beacon_adv_data) != SUCCESS) {
		Log_printf(Log_Dual_Stack, Log_ERROR, "Failed to load adv data");
		return;
	}

	Log_printf(Log_Dual_Stack, Log_INFO, "Advertise data updated");
}

static void _adv_refresh_task_entry(void *arg)
{
	while (true) {
		(void)SemaphoreP_pend(_ble_update_sem_handle,
				      SemaphoreP_WAIT_FOREVER);

		/* Start / stop adv must be done from BLEAppUtil module context */
		if (BLEAppUtil_invokeFunction(
			    (InvokeFromBLEAppUtilContext_t)_ble_adv_update,
			    NULL) != SUCCESS) {
			Log_printf(Log_Dual_Stack, Log_ERROR,
				   "Failed to invoke BLEAppUtil function");
			break;
		}
	}
}

static void _beacon_stop(void)
{
	if (BLEAppUtil_advStop(_beacon_adv_handle) != SUCCESS) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to stop beacon advertising");
	}

	ClockP_stop(_adv_timer_handle);
}

static void _beacon_start(void)
{
	size_t len = HUBBLE_BLE_BUFFER_LEN - BLE_ADV_HEADER_SIZE;
	int status = hubble_ble_advertise_get(
		NULL, 0, &_beacon_adv_data[BLE_ADV_HEADER_SIZE], &len);

	if (status) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to get advertise data, err: %d", status);
		return;
	}

	/* output len + BLE service data type */
	_beacon_adv_data[4] = len + 1;

	if (BLEAppUtil_advStart(_beacon_adv_handle, &_hubble_start_adv_set) !=
	    SUCCESS) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to start advertising");
		return;
	}

	ClockP_start(_adv_timer_handle);
}

static void _hubble_conn_start(void)
{
	if (BLEAppUtil_advStart(_conn_adv_handle, &_hubble_start_adv_set) !=
	    SUCCESS) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to start connectable advertising");
	}
}

static void _adv_init(void)
{
	bStatus_t status;

	/* register GATT service and conn handler */
	status = GATTServApp_RegisterService(
		_hubble_attr_tbl, GATT_NUM_ATTRS(_hubble_attr_tbl),
		GATT_MAX_ENCRYPT_KEY_SIZE, &_hubble_svc_callbacks);
	if (status != SUCCESS) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to register GATT service");
		return;
	}

	status = BLEAppUtil_registerEventHandler(&_conn_handler);
	if (status != SUCCESS) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to register connection event handler");
		return;
	}

	/* Init both adv sets */
	status = BLEAppUtil_initAdvSet(&_beacon_adv_handle,
				       &_beacon_adv_init_set);
	if (status != SUCCESS) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to initialize advertising set, err: %d",
			   status);
		return;
	}

	status = BLEAppUtil_initAdvSet(&_conn_adv_handle, &_conn_adv_init_set);
	if (status != SUCCESS) {
		Log_printf(
			Log_Dual_Stack,
			Log_ERROR, "Failed to initialize connectable advertising set, err: %d",
			status);
		return;
	}
}

/* Public API */
bStatus_t ble_init(void)
{
	/* Create sems */
	_sync_sem_handle = SemaphoreP_constructBinary(&_sync_sem_struct, 0);
	_ble_update_sem_handle =
		SemaphoreP_constructBinary(&_ble_update_sem_struct, 0);

	if (_sync_sem_handle == NULL || _ble_update_sem_handle == NULL) {
		return (FAILURE);
	}

	/* Create timer */
	ClockP_Params_init(&_adv_timer_params);
	_adv_timer_params.period =
		HUBBLE_ADV_PACKET_PERIOD / ClockP_getSystemTickPeriod();
	_adv_timer_handle =
		ClockP_construct(&_adv_timer_struct, _ble_timer_cb,
				 _adv_timer_params.period, &_adv_timer_params);
	if (_adv_timer_handle == NULL) {
		return (FAILURE);
	}

	/* Create task */
	_ble_adv_task_handle =
		TaskP_construct(&_ble_adv_task_struct, _adv_refresh_task_entry,
				&_ble_adv_task_params);

	if (_ble_adv_task_handle == NULL) {
		return (FAILURE);
	}

	return BLEAppUtil_invokeFunction(
		(InvokeFromBLEAppUtilContext_t)_adv_init, NULL);
}

bStatus_t ble_adv_start(void)
{
	return BLEAppUtil_invokeFunction(
		(InvokeFromBLEAppUtilContext_t)_beacon_start, NULL);
}

bStatus_t ble_adv_stop(void)
{
	return BLEAppUtil_invokeFunction(
		(InvokeFromBLEAppUtilContext_t)_beacon_stop, NULL);
}

bStatus_t ble_hubble_sync(void)
{
	if (BLEAppUtil_invokeFunction(
		    (InvokeFromBLEAppUtilContext_t)_hubble_conn_start, NULL) !=
	    SUCCESS) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to start connectable advertising");
		return (FAILURE);
	}

	Log_printf(Log_Dual_Stack, Log_INFO,
		   "Waiting for time and orbital params sync...");

	/* Wait for sync to complete */
	(void)SemaphoreP_pend(_sync_sem_handle, SemaphoreP_WAIT_FOREVER);

	return (SUCCESS);
}
