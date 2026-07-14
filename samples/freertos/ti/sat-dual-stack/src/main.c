/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <FreeRTOS.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <task.h>

#include <ti/devices/DeviceFamily.h>
#include <ti/drivers/Power.h>
#include "ti/drivers/dpl/ClockP.h"
#include "ti/drivers/dpl/SemaphoreP.h"
#include <ti/log/Log.h>

#include "ti_ble_config.h"
#include "ti/ble/stack_util/icall/app/icall.h"
#include "ti/ble/stack_util/health_toolkit/assert.h"

#ifndef USE_DEFAULT_USER_CFG
#include "ti/ble/app_util/config/ble_user_config.h"
/* BLE user defined configuration */
icall_userCfg_t user0Cfg = BLE_USER_CFG;
#endif /* USE_DEFAULT_USER_CFG */

#include "ti/ble/app_util/framework/bleapputil_api.h"

#include <hubble/hubble.h>
#include <hubble/sat/pass_prediction.h>
#include <hubble/sat/packet.h>

#include "app_ble.h"

#define US_PER_SEC 1000000ULL
#define US_PER_MS  1000ULL

#if defined(HUBBLE_KEY_SET)
#include "key.c"
#else
#warning "Dummy key, replace with actual key by running ./embed_key_time.py -b b64key"
static uint8_t master_key[CONFIG_HUBBLE_KEY_SIZE];
#endif

/* Time */
uint64_t unix_time_ms = 0;

/* Device location and sat orbital paramters */
struct hubble_sat_device_pos device_pos;
struct hubble_sat_orbital_params orb_params[HUBBLE_MAX_SAT];
uint8_t orb_params_count;

/* Timer */
static ClockP_Struct _sat_timer_struct;
static ClockP_Handle _sat_timer_handle;
static ClockP_Params _sat_timer_params;

/* Wait for ble init and sat tx */
static SemaphoreP_Struct _sat_sem_struct;
static SemaphoreP_Handle _sat_sem_handle;

BLEAppUtil_GeneralParams_t appMainParams = {
	.taskPriority = 1,
	.taskStackSize = 4096,
	.profileRole = (BLEAppUtil_Profile_Roles_e)(HOST_CONFIG),
	.addressMode = DEFAULT_ADDRESS_MODE,
	.deviceNameAtt = attDeviceName,
	.pDeviceRandomAddress = pRandomAddress,
};

static BLEAppUtil_PeriCentParams_t appMainPeriCentParams;

static void sat_timer_cb(uintptr_t arg)
{
	SemaphoreP_post(_sat_sem_handle);
}

void criticalErrorHandler(int32 errorCode, void *pInfo)
{
	(void)errorCode;
	(void)pInfo;
}

void App_StackInitDoneHandler(gapDeviceInitDoneEvent_t *deviceInitDoneData)
{
	(void)deviceInitDoneData;
}

void *main_thread_entry(void *arg0)
{
	(void)arg0;

	struct hubble_sat_pass_info pass_info;
	struct hubble_sat_packet packet;
	uint64_t now_ms;
	uint64_t sat_wait_us;
	bStatus_t status;
	int ret;

	if (ble_init() != SUCCESS) {
		Log_printf(Log_Dual_Stack, Log_ERROR, "Failed to initialize BLE");
	}

	/* Wait for time and orbital params sync */
	status = ble_hubble_sync();
	if (status != SUCCESS) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to sync time and orbital params, err: %d",
			   status);
		return NULL;
	}

	ret = hubble_init(unix_time_ms, 0, master_key);
	if (ret != 0) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to init Hubble SDK");
		return NULL;
	}

	ret = hubble_sat_satellites_set(orb_params, orb_params_count);
	if (ret != 0) {
		Log_printf(
			Log_Dual_Stack, Log_ERROR,
			"Failed to set satellite orbital params data, err: %d",
			ret);
		return NULL;
	}

	for (;;) {
		/* Calculate the next pass time */
		now_ms = hubble_time_get();
		ret = hubble_sat_next_pass_get(now_ms, &device_pos, &pass_info);
		if (ret != 0) {
			Log_printf(Log_Dual_Stack, Log_ERROR,
				   "Failed to get next pass info, err: %d", ret);
			return NULL;
		}

		/*
		 * If the pass start < current time, this means we're in a
		 * middle of a pass. We can compute the next one
		 */
		if (pass_info.start <= now_ms) {
			Log_printf(
				Log_Dual_Stack,
				Log_INFO, "Current pass is ongoing or in the past, search for next pass...");

			ret = hubble_sat_next_pass_get(
				pass_info.start + pass_info.duration,
				&device_pos, &pass_info);
			if (ret != 0) {
				Log_printf(Log_Dual_Stack, Log_ERROR,
					   "Failed to get next pass info: %d",
					   ret);
				return NULL;
			}
		}

#ifdef CONFIG_DEBUG
		sat_wait_us = 120 * US_PER_SEC;
		Log_printf(Log_Dual_Stack, Log_INFO, "Next pass in 120 seconds");
#else
		Log_printf(
			Log_Dual_Stack,
			Log_INFO, "Next pass at: %u (unix epoch seconds), max elevation angle: %.2f",
			(uint32_t)(pass_info.start / 1000U),
			pass_info.max_elevation_angle);

		/* Schedule the pass */
		sat_wait_us = (pass_info.start - now_ms) * US_PER_MS;
#endif

		/*
		 * Note that it is crucial to check configTICK_RATE_HZ to make
		 * sure sleep time does not exceed maximum time supported.
		 */
		ClockP_setTimeout(
			_sat_timer_handle,
			(uint32_t)(sat_wait_us / ClockP_getSystemTickPeriod()));
		ClockP_start(_sat_timer_handle);

		/* Start BLE */
		status = ble_adv_start();
		if (status != SUCCESS) {
			Log_printf(Log_Dual_Stack, Log_ERROR,
				   "Failed to start BLE advertising, err: %d",
				   status);
			return NULL;
		}

		Log_printf(Log_Dual_Stack, Log_INFO,
			   "Starting BLE advertising...");

		/* Wait for sat pass */
		(void)SemaphoreP_pend(_sat_sem_handle, SemaphoreP_WAIT_FOREVER);

		/* Stop BLE and start sat tx */
		status = ble_adv_stop();
		if (status != SUCCESS) {
			Log_printf(Log_Dual_Stack, Log_ERROR,
				   "Failed to stop BLE advertising, err: %d",
				   status);
			return NULL;
		}

		ret = hubble_sat_packet_get(&packet, NULL, 0);
		if (ret != 0) {
			Log_printf(Log_Dual_Stack, Log_ERROR,
				   "Failed to get satellite packet, err: %d",
				   ret);
			return NULL;
		}

		Log_printf(Log_Dual_Stack, Log_INFO, "Starting Sat TX...");

		ret = hubble_sat_packet_send(&packet,
					     HUBBLE_SAT_RELIABILITY_NORMAL);
		if (ret != 0) {
			Log_printf(Log_Dual_Stack, Log_ERROR,
				   "Failed to send satellite packet, err: %d",
				   ret);
			return NULL;
		}
	}

	return NULL;
}

int main()
{
	pthread_t thread;
	pthread_attr_t attrs;
	struct sched_param priParam;
	int ret;

	Board_init();

	_sat_sem_handle = SemaphoreP_constructBinary(&_sat_sem_struct, 0);
	if (_sat_sem_handle == NULL) {
		return -ENOMEM;
	}

	ClockP_Params_init(&_sat_timer_params);
	_sat_timer_handle = ClockP_construct(&_sat_timer_struct, sat_timer_cb,
					     0, &_sat_timer_params);
	if (_sat_timer_handle == NULL) {
		return -ENODEV;
	}

	/* Update User Configuration of the stack */
	user0Cfg.appServiceInfo->timerTickPeriod = ICall_getTickPeriod();
	user0Cfg.appServiceInfo->timerMaxMillisecond = ICall_getMaxMSecs();

	BLEAppUtil_init(&criticalErrorHandler, &App_StackInitDoneHandler,
			&appMainParams, &appMainPeriCentParams);

	/* Initialize the attributes structure with default values */
	ret = pthread_attr_init(&attrs);
	if (ret != 0) {
		return ret;
	}

	/* Set priority, detach state, and stack size attributes */
	priParam.sched_priority = 1;
	ret = pthread_attr_setschedparam(&attrs, &priParam);
	ret |= pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
	ret |= pthread_attr_setstacksize(&attrs, 2048);
	if (ret != 0) {
		/* failed to set attributes */
		while (1) {
		}
	}

	ret = pthread_create(&thread, &attrs, main_thread_entry, NULL);
	if (ret != 0) {
		Log_printf(Log_Dual_Stack, Log_ERROR,
			   "Failed to create main thread");
		return ret;
	}

	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();

	return 0;
}
