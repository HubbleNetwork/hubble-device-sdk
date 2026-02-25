/*
 * Copyright (c) 2013-2022, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <pthread.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>

/* TI Drivers */
#include <ti/drivers/Board.h>

/* Hubble Network */
#include <hubble/hubble.h>
#include <hubble/sat/packet.h>

/* Stack size in bytes */
#define THREADSTACKSIZE  2048

#define HUBBLE_DEVICE_ID 1337
#define SLEEP_PERIOD_MS  4000

static uint64_t _utc_unused = 0xdeadbeef;
static uint8_t _key_unused[CONFIG_HUBBLE_KEY_SIZE];

void *mainThread(void *arg0)
{
	struct hubble_sat_packet packet;
	int ret;

	ret = hubble_init(_utc_unused, _key_unused);
	if (ret != 0) {
		/* TODO: Call Error Handler */
		return NULL;
	}

	ret = hubble_sat_packet_get(&packet, NULL, 0);
	if (ret != 0) {
		/* TODO: Call Error Handler */
		return NULL;
	}

	for (;;) {
		ret = hubble_sat_packet_send(&packet, HUBBLE_SAT_RELIABILITY_NONE);
		if (ret != 0) {
			/* TODO: Call Error Handler */
			return NULL;
		}
		vTaskDelay(pdMS_TO_TICKS(SLEEP_PERIOD_MS));
	}

	return NULL;
}

int main(void)
{
	pthread_t thread;
	pthread_attr_t attrs;
	struct sched_param priParam;
	int retc;

	/* initialize the system locks */
	Board_init();

	/* Initialize the attributes structure with default values */
	pthread_attr_init(&attrs);

	/* Set priority, detach state, and stack size attributes */
	priParam.sched_priority = 1;
	retc = pthread_attr_setschedparam(&attrs, &priParam);
	retc |= pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
	retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
	if (retc != 0) {
		/* failed to set attributes */
		while (1) {
		}
	}

	retc = pthread_create(&thread, &attrs, mainThread, NULL);
	if (retc != 0) {
		/* pthread_create() failed */
		while (1) {
		}
	}

	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();

	return (0);
}
