/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file efr32_radio.h
 * @internal
 * @brief Interface between the shared EFR32 radio code and the RTOS glue.
 *
 * The RTOS glue lives in port/rtos/boards/. Internal to the SDK; may change
 * without notice.
 */

#ifndef PORT_BOARDS_SILABS_EFR32_RADIO_H
#define PORT_BOARDS_SILABS_EFR32_RADIO_H

/**
 * @brief Initialize the RAIL library and the EFR32 radio.
 *
 * Implemented by the shared radio code. The RTOS glue calls it once before
 * any transmission.
 *
 * @return 0 on successful initialization, or a negative error code on failure.
 */
int efr32_radio_init(void);

/**
 * @brief Acquire exclusive access to the radio for a packet transmission.
 *
 * Implemented by the RTOS glue. Blocks until the radio is free, then clears
 * any pending symbol signal left from a previous transmission.
 */
void efr32_radio_tx_sem_take(void);

/**
 * @brief Release the radio after a packet transmission.
 *
 * Implemented by the RTOS glue.
 */
void efr32_radio_tx_sem_give(void);

/**
 * @brief Wait for the RAIL timer to signal the end of a symbol on or off period.
 *
 * Implemented by the RTOS glue.
 */
void efr32_radio_symbol_sem_take(void);

/**
 * @brief Signal the end of a symbol on or off period.
 *
 * Implemented by the RTOS glue. Called from the RAIL timer callback in
 * interrupt context, so it must not block.
 */
void efr32_radio_symbol_sem_give_from_isr(void);

#endif /* PORT_BOARDS_SILABS_EFR32_RADIO_H */
