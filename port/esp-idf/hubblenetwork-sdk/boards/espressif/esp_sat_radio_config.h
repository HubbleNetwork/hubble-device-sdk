/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORT_ESP_SAT_RADIO_CONFIG_H
#define PORT_ESP_SAT_RADIO_CONFIG_H

/*
 * Compensate for on/off delay
 *
 * TODO: numbers are found experimentally, need to
 * verify with Espressif for the right values.
 */
#if defined(CONFIG_IDF_TARGET_ESP32C6)
#define ESP_RADIO_OFF_DELAY_US 450U
#define ESP_RADIO_ON_DELAY_US  70U

#elif defined(CONFIG_IDF_TARGET_ESP32S31)
#define ESP_RADIO_OFF_DELAY_US 670U
#define ESP_RADIO_ON_DELAY_US  20U

#endif /* CONFIG_IDF_TARGET_ESP32C6 */

/* Step and frequency calculation */
#if defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32S31)
#define ESP_STEP_SCALE(_step)           ((_step) * 4)

/* The center frequency for channel 0 is 2482208625
 * -> f_base = 2482208625 - (32 * 400) = 2482195825
 * Each channel is 25.75 kHz
 * --> offset = 25.75k / 400 ~= 64 steps (round)
 * Base is set at 24822 MHz,
 * (2482195825 - 2.482e9) / 400 = 489 steps
 */
#define HUBBLE_BASE_FREQUENCY           2482U
#define HUBBLE_CHANNEL_OFFSET(_channel) (((_channel) * 64) + 489)

#else
#error "SoC not supported"
#endif

#endif /* PORT_ESP_SAT_RADIO_CONFIG_H */
