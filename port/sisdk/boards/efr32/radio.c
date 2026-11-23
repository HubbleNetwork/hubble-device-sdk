/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Standard C Libraries */
#include <errno.h>

/* FreeRTOS */
#include "FreeRTOS.h"
#include "semphr.h"

#include "efr32_radio.h"
#include "sat_board.h"

/**
 * This semaphore is used to protect a packet transmission and avoid
 * race conditions.
 */
static SemaphoreHandle_t _transmit_sem;

/**
 * This semaphore is used between symbol on/off time.
 */
static SemaphoreHandle_t _symbol_sem;

void efr32_radio_tx_sem_take(void)
{
	xSemaphoreTake(_transmit_sem, portMAX_DELAY);

	/* Need to reset symbol sem to prepare for transmit */
	xSemaphoreTake(_symbol_sem, 0);
}

void efr32_radio_tx_sem_give(void)
{
	xSemaphoreGive(_transmit_sem);
}

void efr32_radio_symbol_sem_take(void)
{
	xSemaphoreTake(_symbol_sem, portMAX_DELAY);
}

void efr32_radio_symbol_sem_give_from_isr(void)
{
	BaseType_t higher_prio_task_woken = pdFALSE;

	xSemaphoreGiveFromISR(_symbol_sem, &higher_prio_task_woken);
	portYIELD_FROM_ISR(higher_prio_task_woken);
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

	return efr32_radio_init();
}
