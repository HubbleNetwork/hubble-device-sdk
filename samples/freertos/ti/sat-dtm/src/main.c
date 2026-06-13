/*
 * Copyright (c) 2026 HubbleNetwork
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <FreeRTOS.h>
#include <stdint.h>
#include <task.h>

/* POSIX Header files */
#include <pthread.h>

/* TI Drivers */
#include <ti/drivers/Board.h>

/* Hubble */
#include <hubble/hubble.h>

extern void *main_thread(void *arg0);

/* Stack size in bytes */
#define THREADSTACKSIZE 1024

int main(void)
{
	uint8_t _hubble_key[CONFIG_HUBBLE_KEY_SIZE] = {0};
	pthread_t thread;
	pthread_attr_t attrs;
	struct sched_param priParam;
	int retc;

	Board_init();

	/*
	 * We don't need a real key since we don't have any requirement
	 * for the payload contents.
	 */
	retc = hubble_init(1U, _hubble_key);
	if (retc != 0) {
		while (1) {
			/* TODO: add error handling */
		}
	}

	/* Initialize the attributes structure with default values */
	pthread_attr_init(&attrs);

	/* Set priority, detach state, and stack size attributes */
	priParam.sched_priority = 1;
	retc = pthread_attr_setschedparam(&attrs, &priParam);
	retc |= pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
	retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
	if (retc != 0) {
		while (1) {
			/* TODO: add error handling */
		}
	}

	retc = pthread_create(&thread, &attrs, main_thread, NULL);
	if (retc != 0) {
		/* pthread_create() failed */
		while (1) {
			/* TODO: add error handling */
		}
	}

	/* Start the FreeRTOS scheduler */
	vTaskStartScheduler();

	return (0);
}
