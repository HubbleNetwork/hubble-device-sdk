/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Standard C Libraries */
#include <errno.h>
#include <stdint.h>

/* FreeRTOS */
#include "FreeRTOS.h"
#include "semphr.h"

/* Hubble */
#include <hubble/port/sat_radio.h>
#include <hubble/sat/packet.h>

/* RAIL */
#include <sl_rail.h>
#include <sl_rail_util_pa_conversions.h>
#include "rail_config.h"

#include "sat_board.h"

/* xG24, xG26 step size = 74.3865966796875 Hz */
#define EFR32_STEP_SCALE(step) (step * 4)
#define MAX_POWER_DDBM         200 /* DBm = 10 * dBm */

/**
 * This semaphore is used to protect a packet transmission and avoid
 * race conditions.
 */
static SemaphoreHandle_t _transmit_sem;

/**
 * This semaphore is used between symbol on/off time.
 */
static SemaphoreHandle_t _symbol_sem;

static sl_rail_handle_t _rail_handle = SL_RAIL_EFR32_HANDLE;
static sl_rail_tx_power_t _power;
static sl_rail_tx_power_t _sat_tx_power = MAX_POWER_DDBM;

/*
 * Generally there is 100 - 150 us between idle to tx,
 * but we set it to 0 for best effort.
 */
static sl_rail_state_timing_t _timing = {
	.idle_to_rx = 0,
};

static int _sl_status_to_errno(sl_rail_status_t status)
{
	/* Let's use EIO as a generic error */
	int ret = -EIO;

	switch (status) {
	case SL_RAIL_STATUS_NO_ERROR:
		ret = 0;
		break;
	case SL_RAIL_STATUS_INVALID_PARAMETER:
		ret = -EINVAL;
		break;
	case SL_RAIL_STATUS_INVALID_STATE:
		ret = -EPERM;
		break;
	case SL_RAIL_STATUS_INVALID_CALL:
		ret = -ENOTSUP;
		break;
	case SL_RAIL_STATUS_SUSPENDED:
		ret = -EBUSY;
		break;
	case SL_RAIL_STATUS_SCHED_ERROR:
		ret = -ECANCELED;
		break;
	case SL_RAIL_STATUS_NO_MORE_RESOURCE:
		ret = -ENOMEM;
		break;
	case SL_RAIL_DMA_INVALID:
		ret = -EFAULT;
		break;
	default:
		break;
	}

	return ret;
}

static void _radio_handler(sl_rail_handle_t rail_handle, sl_rail_events_t events)
{
	if (events & SL_RAIL_EVENT_CAL_NEEDED) {
		(void)sl_rail_calibrate(rail_handle, NULL,
					SL_RAIL_CAL_ALL_PENDING);
	}
}

/* Must never be const */
static sl_rail_config_t _rail_cfg = {
	.events_callback = _radio_handler,
};

static void _timer_cb(sl_rail_handle_t rail_handle)
{
	(void)rail_handle;

	BaseType_t higher_prio_task_woken = pdFALSE;

	xSemaphoreGiveFromISR(_symbol_sem, &higher_prio_task_woken);
	portYIELD_FROM_ISR(higher_prio_task_woken);
}

static int _radio_cw_start(uint8_t channel, uint16_t step, uint32_t abs_start,
			   uint32_t delay, uint32_t duration_us)
{
	sl_rail_status_t status;

	status = sl_rail_set_freq_offset(_rail_handle, EFR32_STEP_SCALE(step));
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	status = sl_rail_set_timer(_rail_handle, abs_start + duration_us,
				   SL_RAIL_TIME_ABSOLUTE, &_timer_cb);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	status = sl_rail_start_tx_stream(_rail_handle, channel,
					 SL_RAIL_STREAM_CARRIER_WAVE,
					 SL_RAIL_TX_OPTIONS_DEFAULT);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	xSemaphoreTake(_symbol_sem, portMAX_DELAY);

	status = sl_rail_stop_tx_stream(_rail_handle);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	status = sl_rail_set_timer(_rail_handle, abs_start + duration_us + delay,
				   SL_RAIL_TIME_ABSOLUTE, &_timer_cb);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	xSemaphoreTake(_symbol_sem, portMAX_DELAY);
	return 0;
}

static int _radio_channel_set(uint8_t channel)
{
	sl_rail_status_t status;

	status = sl_rail_is_valid_channel(_rail_handle, channel);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	status = sl_rail_prepare_channel(_rail_handle, channel);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	return 0;
}

static void rail_on_channel_config(sl_rail_handle_t rail_handle,
				   const sl_rail_channel_config_entry_t *entry)
{
	sl_rail_util_pa_on_channel_config_change(rail_handle, entry);
}

static int _rail_radio_init(void)
{
	sl_rail_status_t status;
	sl_rail_tx_pa_mode_t pa_mode = SL_RAIL_TX_PA_MODE_2P4_GHZ;
	sl_rail_state_transitions_t transitions = {
		.success = SL_RAIL_RF_STATE_IDLE,
		.error = SL_RAIL_RF_STATE_IDLE,
	};

	/* Init the RAIL lib */
	status = sl_rail_init(&_rail_handle, &_rail_cfg, NULL);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	sl_rail_util_pa_init();
	sl_rail_util_pa_post_init(_rail_handle, pa_mode);

	/* Config calibration settings */
	status = sl_rail_config_cal(_rail_handle, SL_RAIL_CAL_ALL);
	if (status) {
		return _sl_status_to_errno(status);
	}

	/* Config chanel according to the generated settings */
	status = sl_rail_config_channels(
		_rail_handle, (const sl_rail_channel_config_t *)channelConfigs[0],
		&rail_on_channel_config);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	/* Let's prepare the first channel at init */
	status = sl_rail_prepare_channel(_rail_handle, 0);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	status = sl_rail_config_events(_rail_handle, SL_RAIL_EVENTS_ALL,
				       SL_RAIL_EVENT_CAL_NEEDED);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	status = sl_rail_set_tx_transitions(_rail_handle, &transitions);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	status = sl_rail_set_rx_transitions(_rail_handle, &transitions);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	status = sl_rail_set_state_timing(_rail_handle, &_timing);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	return _sl_status_to_errno(status);
}

int hubble_sat_board_init(void)
{
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

	/* Make tx count to 1 initially */
	if (xSemaphoreGive(_transmit_sem) != pdTRUE) {
		vSemaphoreDelete(_transmit_sem);
		vSemaphoreDelete(_symbol_sem);

		_transmit_sem = NULL;
		_symbol_sem = NULL;

		return -EAGAIN;
	}

	return _rail_radio_init();
}

int hubble_sat_board_enable(void)
{
	_power = sl_rail_get_tx_power_dbm(_rail_handle);
	return _sl_status_to_errno(
		sl_rail_set_tx_power_dbm(_rail_handle, _sat_tx_power));
}

int hubble_sat_board_disable(void)
{
	/* Make sure to stop tx first before idling the radio */
	(void)sl_rail_stop_tx_stream(_rail_handle);
	(void)sl_rail_set_tx_power_dbm(_rail_handle, _power);
	return _sl_status_to_errno(
		sl_rail_idle(_rail_handle, SL_RAIL_IDLE, true));
}

int hubble_sat_board_packet_send(const struct hubble_sat_packet_frames *packet)
{
	int ret = 0;
	int8_t frame = -1;
	sl_rail_time_t symbol_start;

	xSemaphoreTake(_transmit_sem, portMAX_DELAY);
	xSemaphoreTake(_symbol_sem, 0); /* Reset semaphore */

	symbol_start = sl_rail_get_time(_rail_handle);

	for (size_t i = 0; i < packet->total_number_of_symbols; i++) {
		uint8_t data_pos = i % HUBBLE_PACKET_FRAME_PAYLOAD_MAX_SIZE;

		if (data_pos == 0) {
			frame++;
			ret = _radio_channel_set(packet->frame[frame].channel);
			if (ret != 0) {
				goto end;
			}
		}

		ret = _radio_cw_start(packet->frame[frame].channel,
				      packet->frame[frame].data[data_pos],
				      symbol_start, HUBBLE_WAIT_SYMBOL_OFF_US,
				      HUBBLE_WAIT_SYMBOL_US);
		if (ret != 0) {
			goto end;
		}

		symbol_start += HUBBLE_WAIT_SYMBOL_US + HUBBLE_WAIT_SYMBOL_OFF_US;
	}

end:
	xSemaphoreGive(_transmit_sem);
	return ret;
}

#ifdef CONFIG_HUBBLE_SAT_NETWORK_DTM_MODE

int hubble_sat_board_power_set(int8_t power)
{
	sl_rail_status_t status =
		sl_rail_set_tx_power_dbm(_rail_handle, power * 10);

	if (status == SL_RAIL_STATUS_NO_ERROR) {
		_sat_tx_power = power * 10;
	}

	return _sl_status_to_errno(status);
}

int hubble_sat_board_cw_start(uint8_t channel)
{
	sl_rail_status_t status;

	status = sl_rail_prepare_channel(_rail_handle, channel);
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	/* CW at the channel's center frequency */
	status = sl_rail_set_freq_offset(_rail_handle, EFR32_STEP_SCALE(32));
	if (status != SL_RAIL_STATUS_NO_ERROR) {
		return _sl_status_to_errno(status);
	}

	return _sl_status_to_errno(sl_rail_start_tx_stream(
		_rail_handle, channel, SL_RAIL_STREAM_CARRIER_WAVE,
		SL_RAIL_TX_OPTIONS_DEFAULT));
}

int hubble_sat_board_cw_stop(void)
{
	return _sl_status_to_errno(sl_rail_stop_tx_stream(_rail_handle));
}

#endif /* CONFIG_HUBBLE_SAT_NETWORK_DTM_MODE */
