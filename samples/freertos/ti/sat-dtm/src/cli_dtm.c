/*
 * Copyright (c) 2026 HubbleNetwork
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/* POSIX Header files */
#include <pthread.h>

#include "ti/drivers/dpl/ClockP.h"
#include "ti/drivers/dpl/SemaphoreP.h"
#include <ti/drivers/UART2.h>

/* Driver configuration */
#include "ti_drivers_config.h"

/* Hubbble */
#include <hubble/hubble.h>
#include <hubble/sat/dtm.h>
#include <hubble/sat/packet.h>

#include "cli_driver.h"

#define MAX_COMMAND_LEN        64

#define HUBBLE_NUM_CHANNELS    19U
#define HUBBLE_DEFAULT_CHANNEL 0U

#define TX_THREAD_STACK_SIZE   2048

/* UART handle and parameters */
UART2_Handle uart;
UART2_Params uartParams;

/* Rolling buffer for storing input characters */
static uint8_t input_cli[MAX_COMMAND_LEN] = {0};
static size_t input_index = 0;

/* Transmit thread */
static pthread_t _tx_thread;
static pthread_attr_t attrs;
static struct sched_param priority_params;

/* Timer for tx interval */
static ClockP_Struct _tx_timer_struct;
static ClockP_Handle _tx_timer_handle;
static ClockP_Params _tx_timer_params;

/* Sem for tx interval, shell, and uart read sync */
static SemaphoreP_Struct _tx_sem_struct;
static SemaphoreP_Handle _tx_sem_handle;
static SemaphoreP_Struct _shell_sem_struct;
static SemaphoreP_Handle _shell_sem_handle;
static SemaphoreP_Struct _uart_sem_struct;
static SemaphoreP_Handle _uart_sem_handle;

/* Application / DTM specific variables */
static enum hubble_sat_dtm_packet_type _packet_type = HUBBLE_SAT_DTM_PACKET_0;
static uint32_t _interval_ms = 1000; /* Default to 1s interval */
static uint8_t _channel = HUBBLE_DEFAULT_CHANNEL;
static bool _sweep = false;
static bool _is_transmitting = false;
static bool _transmission_in_progress = false;

static void _uart_cb(UART2_Handle handle, void *buffer, size_t count,
		     void *user_arg, int_fast16_t status)
{
	if (status != UART2_STATUS_SUCCESS) {
		while (1) {
			/* TODO: add error handling */
		}
	}

	/* Data ready signal */
	SemaphoreP_post(_uart_sem_handle);
}

/* Timer for transmit interval */
static void _timer_cb(uintptr_t arg)
{
	SemaphoreP_post(_tx_sem_handle);
}

static void *_transmit_task_entry(void *arg)
{
	(void)arg;
	uint32_t ticks;

	SemaphoreP_pend(_shell_sem_handle, SemaphoreP_WAIT_FOREVER);
	_is_transmitting = true;
	SemaphoreP_post(_shell_sem_handle);

	/* Re-init count to 1 by 1st reset the sem and give it */
	SemaphoreP_pend(_tx_sem_handle, SemaphoreP_NO_WAIT);
	SemaphoreP_post(_tx_sem_handle);

	while (true) {
		SemaphoreP_pend(_shell_sem_handle, SemaphoreP_WAIT_FOREVER);
		if (!_is_transmitting) {
			break;
		}

		SemaphoreP_post(_shell_sem_handle);

		/* Don't keep both the shell sem and the transmit sem
		 * because the transmit interval could be very long,
		 * and blocking the shell.
		 *
		 * But this turns into a risk if the shell takes too long,
		 * it'll mess up the interval. Unless absolute interval
		 * is needed, this should be fine.
		 */
		SemaphoreP_pend(_tx_sem_handle, SemaphoreP_WAIT_FOREVER);
		SemaphoreP_pend(_shell_sem_handle, SemaphoreP_WAIT_FOREVER);

		/* Just double check once again, bc when tx stops, it
		 * release the transmit sem immdeiately, and we don't
		 * want anymore tx.
		 */
		if (!_is_transmitting) {
			break;
		}

		if (hubble_sat_dtm_packet_send(_packet_type,
					       _sweep ? -1 : _channel) != 0) {
			CLI_print("Failed to send packet\r\n");
		}

		/* Start the 1 shot timer */
		ticks = (_interval_ms * 1000U) / ClockP_getSystemTickPeriod();
		ClockP_setTimeout(_tx_timer_handle, ticks);
		ClockP_start(_tx_timer_handle);

		SemaphoreP_post(_shell_sem_handle);
	}

	SemaphoreP_post(_shell_sem_handle);
	return NULL;
}

static void _cmd_carrier_wave(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	if (hubble_sat_dtm_cw_start(_channel) != 0) {
		CLI_print("Failed to start carrier wave\r\n");
	}
}

static void _cmd_power(int argc, char **argv)
{
	int power;

	if (argc != 1) {
		CLI_print("Usage: power <dBm>\r\n");
		return;
	}

	power = atoi(argv[0]);

	if (hubble_sat_dtm_power_set((int8_t)power) != 0) {
		CLI_print("Failed to set power\r\n");
	}
}

static void _cmd_channel(int argc, char **argv)
{
	int channel;

	if (argc != 1) {
		CLI_print("Usage: channel <0..18>\r\n");
		return;
	}

	channel = atoi(argv[0]);
	if (channel < 0 || channel >= HUBBLE_NUM_CHANNELS) {
		CLI_print("Invalid channel\r\n");
		return;
	}
	_channel = (uint8_t)channel;
}

static void _cmd_stop(int argc, char **argv)
{
	(void)argv;

	(void)SemaphoreP_pend(_shell_sem_handle, SemaphoreP_WAIT_FOREVER);

	/* If not transmit, we can return */
	if (!_is_transmitting) {
		SemaphoreP_post(_shell_sem_handle);
		return;
	}

	_is_transmitting = false;
	_transmission_in_progress = false;

	/*
	 * If interval is too long, the thread blocks on transmit sem,
	 * so release it immediately to exit tx.
	 */
	ClockP_stop(_tx_timer_handle);
	SemaphoreP_post(_tx_sem_handle);

	SemaphoreP_post(_shell_sem_handle);
	(void)pthread_join(_tx_thread, NULL);
}

static void _cmd_payload(int argc, char **argv)
{

	int32_t len;

	if (argc != 1) {
		goto error;
	}

	len = strtol(argv[0], NULL, 10);

	switch (len) {
	case -1:
		_packet_type = HUBBLE_SAT_DTM_PACKET_SINGLE_FRAME;
		break;
	case 0:
		_packet_type = HUBBLE_SAT_DTM_PACKET_0;
		break;
	case 4:
		_packet_type = HUBBLE_SAT_DTM_PACKET_4;
		break;
	case 9:
		_packet_type = HUBBLE_SAT_DTM_PACKET_9;
		break;
	case 13:
		_packet_type = HUBBLE_SAT_DTM_PACKET_13;
		break;
	default:
		goto error;
		break;
	}

	return;

error:
	CLI_print("Invalid payload length. Valid options are: -1, 0, 4, 9 or "
		  "13\r\n");
}

static void _cmd_transmit(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	if (hubble_sat_dtm_packet_send(_packet_type, _channel) != 0) {
		CLI_print("Failed to send packet\r\n");
	}
}

static bool _spawn_thread(void)
{
	int err;

	/* Initialize the attributes structure with default values */
	pthread_attr_init(&attrs);
	priority_params.sched_priority = 1; /* Lowest prio */
	err = pthread_attr_setschedparam(&attrs, &priority_params);
	if (err != 0) {
		CLI_print("Failed to set thread priority\r\n");
		return false;
	}

	err = pthread_attr_setstacksize(&attrs, TX_THREAD_STACK_SIZE);
	if (err != 0) {
		CLI_print("Failed to set thread stack size\r\n");
		return false;
	}

	err = pthread_create(&_tx_thread, &attrs, _transmit_task_entry, NULL);
	if (err != 0) {
		CLI_print("Failed to create transmit thread\r\n");
		return false;
	}

	_transmission_in_progress = true;
	return true;
}

/* Helper function for tx continuous and sweep */
static void _cmd_transmit_repeating(int argc, char **argv, bool sweep,
				    const char *usage)
{
	int interval;

	if (argc != 1) {
		CLI_print("Usage: ");
		CLI_print(usage);
		CLI_print(" <interval_ms>\r\n");
		return;
	}

	interval = atoi(argv[0]); /* in ms */

	if (interval < 0) {
		CLI_print("Invalid interval\r\n");
		return;
	}

	/*
	 * Interval 0 = transmit back to back,
	 * so just set it a number < packet tx time
	 */
	if (interval == 0) {
		interval = 100;
	}

	SemaphoreP_pend(_shell_sem_handle, SemaphoreP_WAIT_FOREVER);

	/* Check first before set the shared variables */
	if (_transmission_in_progress) {
		CLI_print("Transmission already in progress, use stop to "
			  "finish it first\r\n");
		SemaphoreP_post(_shell_sem_handle);
		return;
	}

	_interval_ms = (uint32_t)interval;
	_sweep = sweep;

	if (!_spawn_thread()) {
		CLI_print("Failed to start transmission\r\n");
	}

	SemaphoreP_post(_shell_sem_handle);
}

static void _cmd_transmit_continuously(int argc, char **argv)
{
	_cmd_transmit_repeating(argc, argv, false, "transmit_continuously");
}

static void _cmd_transmit_sweep(int argc, char **argv)
{
	_cmd_transmit_repeating(argc, argv, true, "transmit_sweep");
}

void *main_thread(void *arg0)
{
	(void)arg0;

	uint32_t uart_status = UART2_STATUS_SUCCESS;

	/* Variable to store the current input character */
	char input;

	/* Initialize the CLI driver */
	CLI_init();

	/* Register commands with the CLI */
	CLI_ADD_COMMAND(
		wave, _cmd_carrier_wave, "\r\nCarrier Wave Finished\r\n",
		"Start transmitting carrier wave on the current channel", 0);
	CLI_ADD_COMMAND(power, _cmd_power, "\r\nPower Set\r\n",
			"Set radio power in dBm", 1);
	CLI_ADD_COMMAND(channel, _cmd_channel, "\r\nChannel Set\r\n",
			"Set channel (0..18)", 1);
	CLI_ADD_COMMAND(stop, _cmd_stop, "\r\nTransmission Stopped\r\n",
			"Stop ongoing packet transmission", 0);
	CLI_ADD_COMMAND(
		payload, _cmd_payload,
		"\r\nPayload Length Set\r\n", "Set payload length. Valid options are: -1 (single frame), 0, 4, 9 or 13 bytes",
		1);
	CLI_ADD_COMMAND(transmit, _cmd_transmit, "\r\nPacket Transmitted\r\n",
			"Transmit a single packet with the current settings", 0);
	CLI_ADD_COMMAND(transmit_continuously, _cmd_transmit_continuously,
			"\r\nContinuous Transmission Started\r\n",
			"Start transmitting packets continuously ", 1);
	CLI_ADD_COMMAND(transmit_sweep, _cmd_transmit_sweep,
			"\r\nSweep Transmission Started\r\n",
			"Start transmitting packets with hopping channels", 1);


	/* Initialize timer and sems */
	_tx_sem_handle = SemaphoreP_constructBinary(&_tx_sem_struct, 0);
	_shell_sem_handle = SemaphoreP_constructBinary(&_shell_sem_struct, 1);
	_uart_sem_handle = SemaphoreP_constructBinary(&_uart_sem_struct, 0);

	if (_tx_sem_handle == NULL || _shell_sem_handle == NULL ||
	    _uart_sem_handle == NULL) {
		while (1) {
			/* TODO: add error handling */
		}
	}

	ClockP_Params_init(&_tx_timer_params);
	_tx_timer_params.period = 0; /* 1 shot timer */
	_tx_timer_handle = ClockP_construct(&_tx_timer_struct, _timer_cb, 0,
					    &_tx_timer_params);
	if (_tx_timer_handle == NULL) {
		while (1) {
			/* TODO: add error handling */
		}
	}

	/* Initialize UART in callback read mode */
	UART2_Params_init(&uartParams);
	uartParams.readMode = UART2_Mode_CALLBACK;
	uartParams.readCallback = _uart_cb;
	uartParams.baudRate = 115200;

	uart = UART2_open(CONFIG_UART2_0, &uartParams);
	if (uart == NULL) {
		while (1) {
			/* TODO: add error handling */
		}
	}

	/* Print initialization message */
	CLI_print("DTM Application Initialized:\r\n");

	/* Main loop to handle and process commands */
	while (true) {
		uart_status = UART2_read(uart, &input, 1, NULL);
		if (uart_status != UART2_STATUS_SUCCESS) {
			while (1) {
				/* TODO: add error handling */
			}
		}

		/* Pend until a character is received */
		SemaphoreP_pend(_uart_sem_handle, SemaphoreP_WAIT_FOREVER);

		/* Handle backspace or delete key */
		if (input == '\b' || input == 127) {
			if (input_index > 0) {
				input_index--; /* Remove the last character from the buffer */
				UART2_write(
					uart, "\b \b", 3,
					NULL); /* Erase character on terminal */
			}
			continue;
		}

		/* Handle line endings ("\r", "\n", "\r\n") */
		if (input == '\r' || input == '\n') {
			/* If the previous character was '\r' and the current is '\n', skip processing */
			if (input == '\n' && input_index > 0 &&
			    input_cli[input_index - 1] == '\r') {
				continue;
			}

			/* Move to a new line on terminal */
			UART2_write(uart, "\r\n", 2, NULL);

			/* Process the input buffer as a command */
			CLI_processInput(input_cli, input_index, MAX_COMMAND_LEN);

			/* Clear the buffer for the next command */
			memset(input_cli, 0, MAX_COMMAND_LEN);
			input_index = 0;
			continue;
		}

		/* Add input to the buffer if there's space */
		if (input_index < MAX_COMMAND_LEN - 1) {
			input_cli[input_index++] = input;

			/* Echo the input character back to the terminal */
			UART2_write(uart, &input, 1, NULL);
		}
	}
}
