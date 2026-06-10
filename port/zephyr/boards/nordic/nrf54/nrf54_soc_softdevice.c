/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sat_soc.h>

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/types.h>

#include <hubble/port/sat_radio.h>
#include <hubble/sat/packet.h>

#include <hal/nrf_radio.h>
#include <hal/nrf_dppi.h>
#include <hal/nrf_timer.h>

#include <mpsl_timeslot.h>
#include <mpsl_hwres.h>

#include <errno.h>
#include <stdint.h>

/* From nRF54L15 PS: Time between TXEN -> READY is ~40us with fast ramp-up */
#define WAIT_SYMBOL_OFF_US         (HUBBLE_WAIT_SYMBOL_OFF_US - 40)
#define WAIT_SYMBOL_US             (HUBBLE_WAIT_SYMBOL_US + 40)

#define RADIO_NODE                 DT_NODELABEL(radio)

#define NRF_DPPIC                  NRF_DPPIC10
#define RADIO_ENABLE_TX_ON_CC0_PPI 9U
#define RADIO_DISABLE_ON_CC1_PPI   12U

/*
 * The radio is owned by MPSL when the SoftDevice Controller is enabled, so
 * it can only be touched from within an MPSL timeslot. During a timeslot the
 * application has exclusive access to RADIO and TIMER0, and MPSL forwards the
 * RADIO and TIMER0 interrupts to the timeslot signal callback.
 */
#define SAT_TIMER                  MPSL_TIMER0

#define TIMESLOT_LENGTH_US         MPSL_TIMESLOT_LENGTH_MAX_US
#define TIMESLOT_TIMEOUT_US        (1000000UL)
#define TIMESLOT_EXTEND_US         (WAIT_SYMBOL_OFF_US + WAIT_SYMBOL_US)

#define MPSL_THREAD_STACK_SIZE     (1024U)

/**
 * Signature for APIs provided by the binary library.
 */
int hubble_nrf_lib_frequency_set(uint8_t channel, uint8_t step);
int hubble_nrf_lib_enable(void);
int hubble_nrf_lib_disable(void);

/* TX power applied at the start of every timeslot. */
static nrf_radio_txpower_t _sat_power = RADIO_TXPOWER_TXPOWER_0dBm;

/**
 * This semaphore is used to protect a packet transmission and avoid
 * race conditions.
 */
K_SEM_DEFINE(_transmit_sem, 1, 1);

/**
 * Signalled from the timeslot callback when a packet transmission has completed
 * (or been aborted).
 */
K_SEM_DEFINE(_done_sem, 0, 1);

/* Synchronises the open/close MPSL calls issued on the cooperative thread. */
K_SEM_DEFINE(_call_done_sem, 0, 1);

/* MPSL API calls must be issued from the non-preemptible cooperative thread. */
enum mpsl_call {
	MPSL_CALL_OPEN,
	MPSL_CALL_REQUEST,
	MPSL_CALL_CLOSE,
};

K_MSGQ_DEFINE(_mpsl_msgq, sizeof(enum mpsl_call), 4, 4);

static mpsl_timeslot_session_id_t _session_id = 0xFFu;
static int _call_result;

/* Transmission state, shared between the caller and the timeslot callback. */
static const struct hubble_sat_packet_frames *_tx_packet;
static volatile uint16_t _tx_total;
static volatile uint16_t _tx_index;
static volatile int _tx_result;

static mpsl_timeslot_request_t _ts_request = {
	.request_type = MPSL_TIMESLOT_REQ_TYPE_EARLIEST,
	.params.earliest =
		{
			.hfclk = MPSL_TIMESLOT_HFCLK_CFG_XTAL_GUARANTEED,
			.priority = MPSL_TIMESLOT_PRIORITY_HIGH,
			.length_us = TIMESLOT_LENGTH_US,
			.timeout_us = TIMESLOT_TIMEOUT_US,
		},
};

static mpsl_timeslot_signal_return_param_t _ts_return;

/**
 * Wire TIMER0 EVENTS_COMPARE[0] -> RADIO TASKS_TXEN and
 * TIMER0 EVENTS_COMPARE[1] -> RADIO TASKS_DISABLE.
 */
static void _dppi_setup(void)
{
	/* TIMER COMPARE[0] -> RADIO TXEN */
	nrf_timer_publish_set(SAT_TIMER, NRF_TIMER_EVENT_COMPARE0,
			      RADIO_ENABLE_TX_ON_CC0_PPI);
	nrf_radio_subscribe_set(NRF_RADIO, NRF_RADIO_TASK_TXEN,
				RADIO_ENABLE_TX_ON_CC0_PPI);

	/* TIMER COMPARE[1] -> RADIO DISABLE */
	nrf_timer_publish_set(SAT_TIMER, NRF_TIMER_EVENT_COMPARE1,
			      RADIO_DISABLE_ON_CC1_PPI);
	nrf_radio_subscribe_set(NRF_RADIO, NRF_RADIO_TASK_DISABLE,
				RADIO_DISABLE_ON_CC1_PPI);
}

static void _dppi_enable(void)
{
	nrf_dppi_channels_enable(NRF_DPPIC,
				 BIT(RADIO_ENABLE_TX_ON_CC0_PPI) |
					 BIT(RADIO_DISABLE_ON_CC1_PPI));
}

static void _dppi_disable(void)
{
	nrf_dppi_channels_disable(NRF_DPPIC,
				  BIT(RADIO_ENABLE_TX_ON_CC0_PPI) |
					  BIT(RADIO_DISABLE_ON_CC1_PPI));
}

static void _dppi_clear(void)
{
	nrf_timer_publish_clear(SAT_TIMER, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_publish_clear(SAT_TIMER, NRF_TIMER_EVENT_COMPARE1);
	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_TXEN);
	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_DISABLE);
}

static void _timer_start(uint32_t cc0, uint32_t cc1)
{
	nrf_timer_shorts_set(SAT_TIMER, NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK);
	nrf_timer_cc_set(SAT_TIMER, NRF_TIMER_CC_CHANNEL0, cc0);
	nrf_timer_cc_set(SAT_TIMER, NRF_TIMER_CC_CHANNEL1, cc1);
	nrf_timer_task_trigger(SAT_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_task_trigger(SAT_TIMER, NRF_TIMER_TASK_START);
}

static void _radio_symbol_set(uint16_t symbol)
{
	uint8_t frame = symbol / HUBBLE_PACKET_FRAME_PAYLOAD_MAX_SIZE;
	uint8_t pos = symbol % HUBBLE_PACKET_FRAME_PAYLOAD_MAX_SIZE;

	hubble_nrf_lib_frequency_set(_tx_packet->frame[frame].channel,
				     _tx_packet->frame[frame].data[pos]);
}

/* Configure the radio and start transmitting the first symbol. Runs in the
 * timeslot callback context, with exclusive access to RADIO and TIMER0.
 */
static void _radio_tx_start(void)
{
	_tx_index = 0;

	(void)hubble_nrf_lib_enable();

	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_RXEN);

	nrf_radio_fast_ramp_up_enable_set(NRF_RADIO, true);
	nrf_radio_mode_set(NRF_RADIO, NRF_RADIO_MODE_BLE_1MBIT);
	nrf_radio_txpower_set(NRF_RADIO, _sat_power);

	nrf_radio_shorts_set(NRF_RADIO, NRF_RADIO_SHORT_READY_START_MASK);

	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);
	nrf_radio_int_enable(NRF_RADIO, NRF_RADIO_INT_DISABLED_MASK);

	/*
	 * MPSL owns the RADIO IRQ handler and forwards the interrupt as
	 * MPSL_TIMESLOT_SIGNAL_RADIO, but it does not enable the RADIO IRQ line.
	 * We must enable it itself, otherwise the DISABLED interrupt
	 * never reaches the CPU and SIGNAL_RADIO never fires.
	 */
	NVIC_ClearPendingIRQ(DT_IRQN(RADIO_NODE));
	irq_enable(DT_IRQN(RADIO_NODE));

	_dppi_setup();
	_dppi_enable();

	/* Program the first symbol before the timer triggers TXEN. */
	_radio_symbol_set(0);

	_timer_start(WAIT_SYMBOL_OFF_US, (WAIT_SYMBOL_OFF_US + WAIT_SYMBOL_US));
}

/* Tear down the radio at the end of (or on a failure during) a transmission.
 * Runs in the timeslot callback context, before the timeslot is ended.
 */
static void _radio_tx_stop(void)
{
	nrf_timer_task_trigger(SAT_TIMER, NRF_TIMER_TASK_STOP);
	nrf_timer_shorts_set(SAT_TIMER, 0);

	_dppi_disable();
	_dppi_clear();

	nrf_radio_int_disable(NRF_RADIO, ~0U);
	nrf_radio_shorts_set(NRF_RADIO, 0);
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

	irq_disable(DT_IRQN(RADIO_NODE));

	(void)hubble_nrf_lib_disable();
}

static mpsl_timeslot_signal_return_param_t *_timeslot_callback(
	mpsl_timeslot_session_id_t session_id, uint32_t signal)
{
	ARG_UNUSED(session_id);

	_ts_return.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_NONE;

	switch (signal) {
	case MPSL_TIMESLOT_SIGNAL_START:
		_radio_tx_start();
		break;

	case MPSL_TIMESLOT_SIGNAL_RADIO:
		if (!nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
			break;
		}
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

		_tx_index++;
		if (_tx_index < _tx_total) {
			/* Program the next symbol and keep the slot alive. */
			_radio_symbol_set(_tx_index);
			_ts_return.params.extend.length_us = TIMESLOT_EXTEND_US;
			_ts_return.callback_action =
				MPSL_TIMESLOT_SIGNAL_ACTION_EXTEND;
		} else {
			/* Whole packet transmitted. */
			_radio_tx_stop();
			_tx_result = 0;
			_ts_return.callback_action =
				MPSL_TIMESLOT_SIGNAL_ACTION_END;
		}
		break;

	case MPSL_TIMESLOT_SIGNAL_EXTEND_FAILED:
		/* Could not keep the radio for the rest of the packet. */
		_radio_tx_stop();
		_tx_result = -EIO;
		_ts_return.callback_action = MPSL_TIMESLOT_SIGNAL_ACTION_END;
		break;

	case MPSL_TIMESLOT_SIGNAL_EXTEND_SUCCEEDED:
		break;

	case MPSL_TIMESLOT_SIGNAL_BLOCKED:
	case MPSL_TIMESLOT_SIGNAL_CANCELLED:
		_tx_result = -EAGAIN;
		k_sem_give(&_done_sem);
		break;

	case MPSL_TIMESLOT_SIGNAL_SESSION_IDLE:
		k_sem_give(&_done_sem);
		break;

	case MPSL_TIMESLOT_SIGNAL_SESSION_CLOSED:
	default:
		break;
	}

	return &_ts_return;
}

/* MPSL APIs must be called from a single non-preemptible cooperative thread. */
static void _mpsl_thread(void *p1, void *p2, void *p3)
{
	enum mpsl_call call;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (k_msgq_get(&_mpsl_msgq, &call, K_FOREVER) == 0) {
		switch (call) {
		case MPSL_CALL_OPEN:
			_call_result = mpsl_timeslot_session_open(
				_timeslot_callback, &_session_id);
			k_sem_give(&_call_done_sem);
			break;
		case MPSL_CALL_REQUEST:
			_call_result =
				mpsl_timeslot_request(_session_id, &_ts_request);
			if (_call_result != 0) {
				_tx_result = _call_result;
				k_sem_give(&_done_sem);
			}
			break;
		case MPSL_CALL_CLOSE:
			_call_result = mpsl_timeslot_session_close(_session_id);
			k_sem_give(&_call_done_sem);
			break;
		}
	}
}

K_THREAD_DEFINE(_mpsl_thread_id, MPSL_THREAD_STACK_SIZE, _mpsl_thread, NULL,
		NULL, NULL, K_PRIO_COOP(CONFIG_MPSL_THREAD_COOP_PRIO), 0, 0);

static int _mpsl_call_sync(enum mpsl_call call)
{
	int err = k_msgq_put(&_mpsl_msgq, &call, K_FOREVER);

	if (err != 0) {
		return err;
	}

	k_sem_take(&_call_done_sem, K_FOREVER);

	return _call_result;
}

int hubble_sat_soc_enable(void)
{
	if (_mpsl_call_sync(MPSL_CALL_OPEN) != 0) {
		return -EIO;
	}

	return 0;
}

int hubble_sat_soc_disable(void)
{
	(void)_mpsl_call_sync(MPSL_CALL_CLOSE);
	_session_id = 0xFFu;

	return 0;
}

int hubble_sat_soc_packet_send(const struct hubble_sat_packet_frames *packet)
{
	enum mpsl_call call = MPSL_CALL_REQUEST;
	int err;

	if (packet->total_number_of_symbols == 0U) {
		return 0;
	}

	k_sem_take(&_transmit_sem, K_FOREVER);

	_tx_packet = packet;
	_tx_total = packet->total_number_of_symbols;
	_tx_index = 0;
	_tx_result = -EIO;

	k_sem_reset(&_done_sem);

	err = k_msgq_put(&_mpsl_msgq, &call, K_FOREVER);
	if (err == 0) {
		k_sem_take(&_done_sem, K_FOREVER);
		err = _tx_result;
	}

	k_sem_give(&_transmit_sem);

	return err;
}

#ifdef CONFIG_HUBBLE_SAT_NETWORK_DTM_MODE

int hubble_sat_soc_power_set(int8_t power)
{
	switch (power) {
	case 8:
		_sat_power = NRF_RADIO_TXPOWER_POS8DBM;
		break;
	case 7:
		_sat_power = NRF_RADIO_TXPOWER_POS7DBM;
		break;
	case 6:
		_sat_power = NRF_RADIO_TXPOWER_POS6DBM;
		break;
	case 5:
		_sat_power = NRF_RADIO_TXPOWER_POS5DBM;
		break;
	case 4:
		_sat_power = NRF_RADIO_TXPOWER_POS4DBM;
		break;
	case 3:
		_sat_power = NRF_RADIO_TXPOWER_POS3DBM;
		break;
	case 2:
		_sat_power = NRF_RADIO_TXPOWER_POS2DBM;
		break;
	case 1:
		_sat_power = NRF_RADIO_TXPOWER_POS1DBM;
		break;
	case 0:
		_sat_power = NRF_RADIO_TXPOWER_0DBM;
		break;
	case -1:
		_sat_power = NRF_RADIO_TXPOWER_NEG1DBM;
		break;
	case -2:
		_sat_power = NRF_RADIO_TXPOWER_NEG2DBM;
		break;
	case -3:
		_sat_power = NRF_RADIO_TXPOWER_NEG3DBM;
		break;
	case -4:
		_sat_power = NRF_RADIO_TXPOWER_NEG4DBM;
		break;
	case -5:
		_sat_power = NRF_RADIO_TXPOWER_NEG5DBM;
		break;
	case -6:
		_sat_power = NRF_RADIO_TXPOWER_NEG6DBM;
		break;
	case -7:
		_sat_power = NRF_RADIO_TXPOWER_NEG7DBM;
		break;
	case -8:
		_sat_power = NRF_RADIO_TXPOWER_NEG8DBM;
		break;
	case -9:
		_sat_power = NRF_RADIO_TXPOWER_NEG9DBM;
		break;
	case -10:
		_sat_power = NRF_RADIO_TXPOWER_NEG10DBM;
		break;
	case -12:
		_sat_power = NRF_RADIO_TXPOWER_NEG12DBM;
		break;
	case -14:
		_sat_power = NRF_RADIO_TXPOWER_NEG14DBM;
		break;
	case -16:
		_sat_power = NRF_RADIO_TXPOWER_NEG16DBM;
		break;
	case -20:
		_sat_power = NRF_RADIO_TXPOWER_NEG20DBM;
		break;
	case -40:
		_sat_power = NRF_RADIO_TXPOWER_NEG40DBM;
		break;
	default:
		return -EINVAL;
	}

	nrf_radio_txpower_set(NRF_RADIO, _sat_power);
	return 0;
}

int hubble_sat_soc_cw_start(uint8_t channel)
{
	int ret = hubble_nrf_lib_frequency_set(channel, 32);

	if (ret != 0) {
		return ret;
	}

	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_TXEN);
	while (!nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_TXREADY)) {
		/* Do nothing */
	}

	return 0;
}

int hubble_sat_soc_cw_stop(void)
{
	nrf_radio_task_trigger(NRF_RADIO, NRF_RADIO_TASK_DISABLE);

	while (!nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
		/* Do nothing */
	}
	nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);

	return 0;
}

#endif /* CONFIG_HUBBLE_SAT_NETWORK_DTM_MODE */
