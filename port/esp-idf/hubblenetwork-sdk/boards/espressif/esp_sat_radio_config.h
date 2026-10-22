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

#elif defined(CONFIG_IDF_TARGET_ESP32S3)
#define ESP_RADIO_OFF_DELAY_US 740U
#define ESP_RADIO_ON_DELAY_US  30U

#elif defined(CONFIG_IDF_TARGET_ESP32S31)
#define ESP_RADIO_OFF_DELAY_US 670U
#define ESP_RADIO_ON_DELAY_US  20U

#else
#error "SoC not supported"
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
#define HUBBLE_CHANNEL_OFFSET(_channel) (((_channel) * 64) + 489)

#elif defined(CONFIG_IDF_TARGET_ESP32S3)
/*
 * From experiment, phy_set_freq() does not move in 100 Hz units, but snap
 * to a grid = synth_res (~457.763671875 Hz).
 *
 * Let u = # of 100 Hz units.
 * Let n = # of grid points.
 * Let phi = 0.75 (experimentally dertemined offset)
 * We have:
 * n = floor(u * 100 / 457.763671875 + phi)
 * -> (n - phi) * 457.763671875 / 100 <= u <= (n + 1 - phi) * 457.763671875 / 100
 *
 * Let u be the middle of that range:
 * u = (n + 0.5 - phi) * 4.57763671875
 * u = (n - 0.25) * 4.57763671875
 *
 * 4.57763671875 = 9375 / 2048
 * -> u = (n - 0.25) * 9375 / 2048
 *
 * Since phy_set_freq() only takes integer:
 * u = round((n - 0.25) * 9375 / 2048)
 *
 * Rewrite using integer only:
 * u = round((4 / 4) * (n - 0.25) * 9375 / 2048)
 * u = round((4n - 1) * 9375 / 8192)
 *
 * To round integer division, add half the denom before dividing:
 * u = ((4n - 1) * 9375 + 4096) / 8192)
 */
#define ESP_STEP_SCALE(_n)                                                     \
	((((4U * (uint32_t)(_n)) - 1U) * 9375U + 4096U) / 8192U)

/*
 * From experiment, the synthesizer frequency is ~457.763671875 Hz.
 * The center frequency for channel 0 is 2482208625
 * -> f_base = 2482208625 - (32 * 457.763671875) = 2482193976
 * Each channel is 25.75 kHz
 * --> offset = 25.75k / 457.763671875 ~= 56 steps (round)
 * Base is set at 24822 MHz,
 * (2482193976 - 2.482e9) / 457.763671875 = 424 steps
 */
#define HUBBLE_CHANNEL_OFFSET(_channel) (((_channel) * 56U) + 424U)

#else
#error "SoC not supported"
#endif

#endif /* PORT_ESP_SAT_RADIO_CONFIG_H */
