/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_ESP_IDF_BUILD
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#else
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#endif

#include <errno.h>

#include <hubble/port/sat_radio.h>
#include <hubble/port/sys.h>

#include "sat_board.h"
#include "utils/macros.h"

#define MSEC_PER_SEC 1000

static SemaphoreHandle_t _transmit_sem;

static inline int16_t _time_offset_get_ms(void)
{
	/* Rand is anything in [0-255] (uint8_t). Let's split it into
	 * few offsets in a 2s range.
	 */
	int16_t offset_values[] = {-1000, -500, 0, 500, 1000};
	uint8_t rand_value;

	hubble_rand_get(&rand_value, sizeof(rand_value));

	return offset_values[rand_value / 52];
}

int hubble_sat_port_packet_send(const struct hubble_sat_packet *packet,
				uint8_t retries, uint8_t interval_s)
{
	int ret;

	/* TODO: Should we add a parameter in the API instead of wait forever? */
	xSemaphoreTake(_transmit_sem, portMAX_DELAY);

	ret = hubble_sat_board_enable();
	if (ret != 0) {
		goto enable_error;
	}

	while (retries-- > 0) {
		ret = hubble_sat_board_packet_send(packet);
		if (ret != 0) {
			goto end;
		}

		if (retries > 0) {
			uint32_t sleep_ms =
				HUBBLE_MAX(0, (interval_s * MSEC_PER_SEC) +
						      _time_offset_get_ms());
			vTaskDelay(pdMS_TO_TICKS(sleep_ms));
		}
	}

end:
	/* Let's preserve a possible earlier error */
	if (ret == 0) {
		ret = hubble_sat_board_disable();
	} else {
		(void)hubble_sat_board_disable();
	}

enable_error:
	(void)xSemaphoreGive(_transmit_sem);
	return ret;
}

int hubble_sat_port_init(void)
{
	_transmit_sem = xSemaphoreCreateBinary();
	if (_transmit_sem == NULL) {
		return -ENOMEM;
	}

	if (xSemaphoreGive(_transmit_sem) != pdTRUE) {
		vSemaphoreDelete(_transmit_sem);
		_transmit_sem = NULL;
		return -EAGAIN;
	}

	return hubble_sat_board_init();
}
