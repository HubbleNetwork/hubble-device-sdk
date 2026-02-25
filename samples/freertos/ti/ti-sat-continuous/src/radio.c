/*
 * Copyright (c) 2025 HubbleNetwork
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Standard C Libraries */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* TODO: check this config */
/* TI Drivers */
#include <ti/drivers/timer/LGPTimerLPF3.h>
#include <ti/drivers/rcl/RCL.h>
#include <ti/drivers/rcl/RCL_Scheduler.h>
#include <ti/drivers/rcl/commands/generic.h>

/* Hubble */
#include <hubble/port/sat_radio.h>
#include <hubble/sat/packet.h>

/* DMM */
#if defined(USE_DMM_OVRDE)
#include <dmm_scheduler.h>
#include <dmm_policy.h>
#include <ti_dmm_application_policy.h>
#include "dmm_priority_ble_custom.h"
#endif

/* SysConfig Generated */
#include "ti_drivers_config.h"
#include "ti_radio_config.h"

#include "pa.h"
#include "sat_board.h"

#define DEFAULT_TX_POWER_DBM TI_PA_MAX_DBM
#define TI_STEP_SIZE_HZ      338U
#define TI_RADIO_RAMP_OFFSET 100U /* TODO: find a better name for it */
#define TIMER_INSTANCE       0
#define TICKS_PER_US         6    /* 48 MHz prescale by 8 = 6 MHz */

#if defined(CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_DEPRECATED)
#define HUBBLE_BASE_FREQUENCY           2483000000U
#define HUBBLE_CHANNEL_OFFSET(_channel) ((_channel) * 64)
#elif defined(CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1)
/* The base frequency for channel 0 is 2.482509510937500.
 * Each channel is 25.5 kHz
 * --> offset = 25.5k / 373 ~= 68 step
 * Base is set at 2482.5 MHz,
 * (2.482509510937500e9 - 2.4825e9) / 373 = ~25 step
 */
#define HUBBLE_BASE_FREQUENCY           2482500000U
#define HUBBLE_CHANNEL_OFFSET(_channel) (((_channel) * 68) + 25)
#endif

static int8_t _preamble[8] = HUBBLE_SAT_PREAMBLE_SEQUENCE;

/**
 * This semaphore is used to protect a packet transmission and avoid
 * race conditions.
 */
static SemaphoreHandle_t _transmit_sem;

/**
 * This semaphore is used between symbol tranmissions. It allows the current
 * thread to wait a symbol transmission without need to poll for a condition.
 */
static SemaphoreHandle_t _symbol_sem;

/**
 * General Purpose Timer for symbol timing.
 * This must be enable in TI SysConfig with Timer 0 instance.
 */
static LGPTimerLPF3_Handle _timer_handle;

/* RCL */
#if defined(USE_DMM_OVRDE)
/* BLE RCL Client Object */
extern RCL_Client rfClient;
extern RCL_CmdGenericTxTest rclPacketTxCmdGenericTxTest_ble_gen_3;
#define rclPacketTxCmdGenericTxTest_ble_custom                                 \
	rclPacketTxCmdGenericTxTest_ble_gen_3
#else
extern RCL_CmdGenericTxTest rclPacketTxCmdGenericTxTest_ble_gen_0;
#define rclPacketTxCmdGenericTxTest_ble_custom                                 \
	rclPacketTxCmdGenericTxTest_ble_gen_0
#endif

static RCL_Client rcl_client;
static RCL_Handle rcl_handle;

static void _timer_cb(LGPTimerLPF3_Handle lgpt_handle,
		      LGPTimerLPF3_IntMask interrupt_mask)
{
	if (interrupt_mask & LGPTimerLPF3_INT_TGT) {
		BaseType_t higher_prio_task_woken = pdFALSE;
		xSemaphoreGiveFromISR(_symbol_sem, &higher_prio_task_woken);
		portYIELD_FROM_ISR(higher_prio_task_woken);
	}
}

static void _timer_start(uint32_t period_us)
{
	uint32_t ticks = TICKS_PER_US * period_us;
	uint32_t counter_target =
		ticks - 1U; /* TI requires -1 from the actual value */

	LGPTimerLPF3_setInitialCounterTarget(_timer_handle, counter_target, true);
	LGPTimerLPF3_start(_timer_handle, LGPTimerLPF3_CTL_MODE_UP_ONCE);
}

static int _timer_enable(void)
{
	LGPTimerLPF3_Params params;

	LGPTimerLPF3_Params_init(&params);
	params.hwiCallbackFxn = _timer_cb;
	params.prescalerDiv = 7; /* Index 7 = prescale of 8 */
	params.intPhaseLate = true;

	_timer_handle = LGPTimerLPF3_open(TIMER_INSTANCE, &params);
	if (_timer_handle == NULL) {
		return -EIO;
	}

	/* Enable interrupt */
	LGPTimerLPF3_enableInterrupt(_timer_handle, LGPTimerLPF3_INT_TGT);
	return 0;
}

static void _timer_disable(void)
{
	LGPTimerLPF3_stop(_timer_handle);
	LGPTimerLPF3_close(_timer_handle);
}

static void _radio_cw_start(uint16_t step, uint32_t delay, uint32_t duration_us)
{
	/* On time */
	rclPacketTxCmdGenericTxTest_ble_custom.rfFrequency =
		HUBBLE_BASE_FREQUENCY + (step * TI_STEP_SIZE_HZ);
	rclPacketTxCmdGenericTxTest_ble_custom.common.timing.relHardStopTime =
		RCL_SCHEDULER_SYSTIM_US(duration_us);

	/* Submit command & pend on completion */
#if defined(USE_DMM_OVRDE)
	DMMSch_RCL_Command_submit(rcl_handle,
				  &rclPacketTxCmdGenericTxTest_ble_custom);
	DMMSch_RCL_Command_pend(&rclPacketTxCmdGenericTxTest_ble_custom);
#else
	RCL_Command_submit(rcl_handle, &rclPacketTxCmdGenericTxTest_ble_custom);
	RCL_Command_pend(&rclPacketTxCmdGenericTxTest_ble_custom);
#endif

	/* Off time */
	_timer_start(delay);
}

#if defined(USE_DMM_OVRDE)
static int hubble_dmm_init(void)
{
	/* Initialize DMM Scheduler */
	DMMPolicy_Params dmm_policy_params;
	DMMPolicy_Status policy_ret;

	/* Initial Policy Manager for tests that require priority handling */
	DMMPolicy_init();
	DMMPolicy_Params_init(&dmm_policy_params);

	dmm_policy_params.numPolicyTableEntries = DMMPolicy_ApplicationPolicySize;
	dmm_policy_params.policyTable = DMMPolicy_ApplicationPolicyTable;
	dmm_policy_params.globalPriorityTable = globalPriorityTable_bleCustom;

	policy_ret = DMMPolicy_open(&dmm_policy_params);
	if (policy_ret != DMMPolicy_StatusSuccess) {
		return -EAGAIN;
	}

	/* Initialize DMM scheduler */
	if (!DMMSch_init()) {
		return -EAGAIN;
	}

	return ((DMMSch_registerClient(&rcl_client, DMMPolicy_StackRole_Custom1,
				       DMMPolicy_Id_Custom1) == true) &&
		(DMMSch_registerClient(&rfClient, DMMPolicy_StackRole_BlePeripheral,
				       DMMPolicy_Id_Ble) == true))
		       ? 0
		       : -EAGAIN;
}
#endif

/* TODO: make sure that the init is only called once during a lifetime of the application */
int hubble_sat_board_init(void)
{
	int ret = 0;

	_symbol_sem = xSemaphoreCreateBinary();
	if (_symbol_sem == NULL) {
		return -ENOMEM;
	}

	_transmit_sem = xSemaphoreCreateBinary();
	if (_transmit_sem == NULL) {
		vSemaphoreDelete(_symbol_sem);
		_symbol_sem = NULL;
		return -ENOMEM;
	}

	/* Make count to 1 initially */
	if (xSemaphoreGive(_transmit_sem) != pdTRUE) {
		vSemaphoreDelete(_transmit_sem);
		vSemaphoreDelete(_symbol_sem);

		_transmit_sem = NULL;
		_symbol_sem = NULL;

		return -EAGAIN;
	}

#if defined(USE_DMM_OVRDE)
	DMMSch_RCL_init();
	ret = hubble_dmm_init();
	if (ret != 0) {
		return ret;
	}

	rcl_handle = DMMSch_RCL_open(&rcl_client, &LRF_config_ble_gen_3);
#else
	RCL_init();
	rcl_handle = RCL_open(&rcl_client, &LRF_config_ble_gen_0);
#endif

	if (rcl_handle == NULL) {
		/* TODO: clean up the sem */
		return -EIO;
	}

	/* Set RF frequency */
	rclPacketTxCmdGenericTxTest_ble_custom.rfFrequency =
		HUBBLE_BASE_FREQUENCY;
	rclPacketTxCmdGenericTxTest_ble_custom.txPower.dBm = DEFAULT_TX_POWER_DBM;

	/* Start command as soon as possible */
	rclPacketTxCmdGenericTxTest_ble_custom.common.scheduling =
		RCL_Schedule_Now;
	rclPacketTxCmdGenericTxTest_ble_custom.common.status =
		RCL_CommandStatus_Idle;

	/* No whitening, on repeated word, freq synth off after cmd, enable cw */
	rclPacketTxCmdGenericTxTest_ble_custom.config.whitenMode = 0U;
	rclPacketTxCmdGenericTxTest_ble_custom.config.txWord = 0U;
	rclPacketTxCmdGenericTxTest_ble_custom.config.fsOff = 1U;
	rclPacketTxCmdGenericTxTest_ble_custom.config.sendCw = 1U;

	return ret;
}

int hubble_sat_board_enable(void)
{
#if defined(USE_DMM_OVRDE)
	DMMSch_setBlockModeOn(DMMPolicy_StackRole_BlePeripheral);
	DMMSch_setBlockModeOff(DMMPolicy_StackRole_Custom1);

	/* Block wait for completion */
	while (DMMSch_getBlockModeStatus(DMMPolicy_StackRole_Custom1)) {
	};
#endif

	return _timer_enable();
}

int hubble_sat_board_disable(void)
{
	_timer_disable();
#if defined(USE_DMM_OVRDE)
	DMMSch_setBlockModeOn(DMMPolicy_StackRole_Custom1);
	DMMSch_setBlockModeOff(DMMPolicy_StackRole_BlePeripheral);

	/* Block wait for completion */
	while (DMMSch_getBlockModeStatus(DMMPolicy_StackRole_BlePeripheral)) {
	};
#endif

	/* TODO: should we close and open rcl every time or just once during init? */

	return 0;
}

int hubble_sat_board_packet_send(const struct hubble_sat_packet *packet)
{
	uint16_t step;
	uint8_t channel = packet->channel;

#ifdef CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1
	uint8_t symbols_counter = 0U;
#endif

	xSemaphoreTake(_transmit_sem, portMAX_DELAY);
	xSemaphoreTake(_symbol_sem, 0); /* Reset semaphore */

	/* Send preamble */
	for (uint8_t i = 0; i < sizeof(_preamble); i++) {
		if (_preamble[i] == -1) {
			_timer_start(HUBBLE_WAIT_PREAMBLE_US);
			xSemaphoreTake(_symbol_sem, portMAX_DELAY);
		} else {
			step = _preamble[i] + HUBBLE_CHANNEL_OFFSET(channel);
			_radio_cw_start(step, HUBBLE_WAIT_SYMBOL_OFF_US,
					HUBBLE_WAIT_SYMBOL_US);
			xSemaphoreTake(_symbol_sem, portMAX_DELAY);
		}
#ifdef CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1
		symbols_counter++;
#endif
	}

	/* Send payload */
	for (uint8_t i = 0; i < packet->length; i++) {
		step = packet->data[i] + HUBBLE_CHANNEL_OFFSET(channel);
		_radio_cw_start(step, HUBBLE_WAIT_SYMBOL_OFF_US,
				HUBBLE_WAIT_SYMBOL_US);
		xSemaphoreTake(_symbol_sem, portMAX_DELAY);
#ifdef CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1
		symbols_counter++;
		if (symbols_counter == HUBBLE_SAT_SYMBOLS_FRAME_MAX) {
			symbols_counter = 0U;
			hubble_sat_channel_next_hop_get(
				packet->hopping_sequence, channel, &channel);
		}
#endif
	}

	xSemaphoreGive(_transmit_sem);
	return 0;
}
