/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sat_soc.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/irq.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/types.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#include <hubble/port/sat_radio.h>
#include <hubble/sat/packet.h>

#include <nrfx_timer.h>
#include <hal/nrf_power.h>
#include <hal/nrf_egu.h>
#include <hal/nrf_radio.h>
#include <hal/nrf_dppi.h>
#include <hal/nrf_timer.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define RADIO_NODE             DT_NODELABEL(radio)

/* From nRF54L15 PS: Time between TXEN -> READY is ~40us with fast ramp-up */
#define WAIT_SYMBOL_OFF_US     (HUBBLE_WAIT_SYMBOL_OFF_US - 40)
#define WAIT_SYMBOL_US         (HUBBLE_WAIT_SYMBOL_US + 40)

/* Max time for semaphore symbol to wait before failing. */
#define WAIT_SYMBOL_TIMEOUT_US K_USEC(2 * (WAIT_SYMBOL_OFF_US + WAIT_SYMBOL_US))

#define NRF_DPPIC              NRF_DPPIC10
#define RADIO_ENABLE_TX_ON_CC0_PPI 9U
#define RADIO_DISABLE_ON_CC1_PPI   12U

/**
 * Signature for APIs provided by the binary library.
 */
int hubble_nrf_lib_frequency_set(uint8_t channel, uint8_t step);
int hubble_nrf_lib_enable(void);
int hubble_nrf_lib_disable(void);

static const struct device *const clock0 = DEVICE_DT_GET_ONE(nordic_nrf_clock);

static nrfx_timer_t _timer0 = NRFX_TIMER_INSTANCE(NRF_TIMER_INST_GET(10));

/* Keep track of power before and in sat tx mode */
static nrf_radio_txpower_t _normal_power = RADIO_TXPOWER_TXPOWER_0dBm;
static nrf_radio_txpower_t _sat_power = RADIO_TXPOWER_TXPOWER_0dBm;

/**
 * This semaphore is used to protect a packet transmission and avoid
 * race conditions.
 */
K_SEM_DEFINE(_transmit_sem, 1, 1);

/**
 * This semaphore is used between symbol transmissions. It allows the current
 * thread to wait for a symbol transmission without needing to poll.
 */
K_SEM_DEFINE(_symbol_sem, 0, 1);

/**
 * Wire EVENT_TIMER EVENTS_COMPARE[0] event to RADIO TASKS_TXEN task and
 * EVENT_TIMER EVENTS_COMPARE[1] event to RADIO TASKS_DISABLE.
 */
static void _dppi_setup(void)
{
	/* TIMER COMPARE[0] -> RADIO TXEN */
	nrf_timer_publish_set(_timer0.p_reg, NRF_TIMER_EVENT_COMPARE0,
			      RADIO_ENABLE_TX_ON_CC0_PPI);
	nrf_radio_subscribe_set(NRF_RADIO, NRF_RADIO_TASK_TXEN,
				RADIO_ENABLE_TX_ON_CC0_PPI);

	/* TIMER COMPARE[1] -> RADIO DISABLE */
	nrf_timer_publish_set(_timer0.p_reg, NRF_TIMER_EVENT_COMPARE1,
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
	nrf_timer_publish_clear(_timer0.p_reg, NRF_TIMER_EVENT_COMPARE0);
	nrf_timer_publish_clear(_timer0.p_reg, NRF_TIMER_EVENT_COMPARE1);
	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_TXEN);
	nrf_radio_subscribe_clear(NRF_RADIO, NRF_RADIO_TASK_DISABLE);
}

static void _timer_enable(uint32_t cc0, uint32_t cc1)
{
	nrf_timer_shorts_enable(_timer0.p_reg,
				NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK |
					NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);

	nrfx_timer_extended_compare(&_timer0, NRF_TIMER_CC_CHANNEL1,
				    nrfx_timer_us_to_ticks(&_timer0, cc1),
				    NRF_TIMER_SHORT_COMPARE1_CLEAR_MASK, false);

	nrfx_timer_extended_compare(&_timer0, NRF_TIMER_CC_CHANNEL0,
				    nrfx_timer_us_to_ticks(&_timer0, cc0), 0,
				    false);

	nrfx_timer_clear(&_timer0);
	nrfx_timer_enable(&_timer0);
}

static void _timer_disable(void)
{
	nrfx_timer_disable(&_timer0);
}

static void _timer_setup(void)
{
	nrfx_timer_config_t timer_cfg = {
		.frequency = NRF_TIMER_FREQ_1MHz,
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
	};

	nrfx_timer_init(&_timer0, &timer_cfg, NULL);
}

static void _radio_isr(const void *arg)
{
	if (nrf_radio_event_check(NRF_RADIO, NRF_RADIO_EVENT_DISABLED)) {
		nrf_radio_event_clear(NRF_RADIO, NRF_RADIO_EVENT_DISABLED);
		k_sem_give(&_symbol_sem);
	}
}

int hubble_sat_soc_disable(void)
{
	nrf_radio_shorts_set(NRF_RADIO, 0);
	nrf_radio_int_disable(NRF_RADIO, ~0);

	_dppi_clear();

	irq_disable(DT_IRQN(RADIO_NODE));

#ifdef CONFIG_DYNAMIC_INTERRUPTS
	irq_disconnect_dynamic(DT_IRQN(RADIO_NODE), DT_IRQ(RADIO_NODE, priority),
			       _radio_isr, NULL, 0);
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

	(void)hubble_nrf_lib_disable();

	nrf_radio_txpower_set(NRF_RADIO, _normal_power);

	return 0;
}

int hubble_sat_soc_enable(void)
{
	int ret;

	if (!device_is_ready(clock0)) {
		return -ENODEV;
	}

	ret = clock_control_on(clock0, CLOCK_CONTROL_NRF_SUBSYS_HF);
	if ((ret < 0) && (ret != -EALREADY)) {
		return ret;
	}

	(void)hubble_nrf_lib_enable();

	NRF_RADIO->SUBSCRIBE_RXEN = 0;

	nrf_radio_fast_ramp_up_enable_set(NRF_RADIO, true);
	nrf_radio_mode_set(NRF_RADIO, NRF_RADIO_MODE_BLE_1MBIT);
	nrf_radio_shorts_enable(NRF_RADIO, NRF_RADIO_SHORT_READY_START_MASK);
	nrf_radio_int_enable(NRF_RADIO, NRF_RADIO_INT_DISABLED_MASK);

	_timer_setup();
	_dppi_setup();

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#ifdef CONFIG_DYNAMIC_DIRECT_INTERRUPTS
	ARM_IRQ_DIRECT_DYNAMIC_CONNECT(
		DT_IRQN(RADIO_NODE), DT_IRQ(RADIO_NODE, priority), 0, reschedule);
#endif /* CONFIG_DYNAMIC_DIRECT_INTERRUPTS */
	irq_connect_dynamic(DT_IRQN(RADIO_NODE), DT_IRQ(RADIO_NODE, priority),
			    _radio_isr, NULL, 0);
#else  /* !CONFIG_DYNAMIC_INTERRUPTS */
	IRQ_CONNECT(DT_IRQN(RADIO_NODE), DT_IRQ(RADIO_NODE, priority),
		    _radio_isr, NULL, 0);
#endif /* CONFIG_DYNAMIC_INTERRUPTS */

	irq_enable(DT_IRQN(RADIO_NODE));

	_normal_power = nrf_radio_txpower_get(NRF_RADIO);
	nrf_radio_txpower_set(NRF_RADIO, _sat_power);

	return 0;
}

int hubble_sat_soc_packet_send(const struct hubble_sat_packet_frames *packet)
{
	int ret;
	int8_t frame = -1;

	ret = k_sem_take(&_transmit_sem,
			 K_SECONDS(HUBBLE_SAT_TRANSMISSION_TIMEOUT_S));
	if (ret != 0) {
		return ret;
	}

	k_sem_reset(&_symbol_sem);

	_dppi_enable();
	_timer_enable(WAIT_SYMBOL_OFF_US, WAIT_SYMBOL_OFF_US + WAIT_SYMBOL_US);

	for (uint8_t i = 0; i < packet->total_number_of_symbols; i++) {
		uint8_t data_pos = i % HUBBLE_PACKET_FRAME_PAYLOAD_MAX_SIZE;

		if (data_pos == 0) {
			frame++;
		}

		hubble_nrf_lib_frequency_set(packet->frame[frame].channel,
					     packet->frame[frame].data[data_pos]);
		ret = k_sem_take(&_symbol_sem, WAIT_SYMBOL_TIMEOUT_US);
		if (ret != 0) {
			break;
		}
	}

	_dppi_disable();
	_timer_disable();

	k_sem_give(&_transmit_sem);

	return ret;
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
