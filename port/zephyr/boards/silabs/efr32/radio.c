/*
 * Copyright (c) 2025 HubbleNetwork
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "efr32_radio.h"
#include "sat_board.h"

/**
 * This semaphore is used to protect a packet transmission and avoid
 * race conditions.
 */
K_SEM_DEFINE(_transmit_sem, 1, 1);

/**
 * This semaphore is used between symbol on/off time.
 */
K_SEM_DEFINE(_symbol_sem, 0, 1);

void efr32_radio_tx_sem_take(void)
{
	k_sem_take(&_transmit_sem, K_FOREVER);

	/* Need to reset symbol sem to prepare for transmit */
	k_sem_reset(&_symbol_sem);
}

void efr32_radio_tx_sem_give(void)
{
	k_sem_give(&_transmit_sem);
}

void efr32_radio_symbol_sem_take(void)
{
	k_sem_take(&_symbol_sem, K_FOREVER);
}

void efr32_radio_symbol_sem_give_from_isr(void)
{
	k_sem_give(&_symbol_sem);
}

/* TODO: this is just a work around for silabs for now because
 * when in dual stack, we need to init the radio before enable
 * bluetooth. Else it'll silently fail.
 */
int hubble_sat_board_init(void)
{
	return 0;
}

SYS_INIT(efr32_radio_init, POST_KERNEL, 0);
