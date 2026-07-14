/*
 * Copyright (c) 2026 HubbleNetwork
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/shell/shell.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/iterable_sections.h>

#include <hubble/hubble.h>
#include <hubble/sat.h>
#include <hubble/sat/dtm.h>
#include <hubble/sat/packet.h>

#define THREAD_TRANSMIT_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO
#define STACKSIZE                2048

#define HUBBLE_NUM_CHANNELS      (19)

K_THREAD_STACK_DEFINE(thread_transmit_stack_area, STACKSIZE);

static K_SEM_DEFINE(_transmit_sem, 0, 1);
static K_SEM_DEFINE(_shell_sem, 1, 1);

static enum hubble_sat_dtm_packet_type _packet_type = HUBBLE_SAT_DTM_PACKET_0;
static struct k_thread _thread_transmit;

static bool _is_transmiting = false;
static bool _transmission_in_progress = false;
static uint8_t _channel;

#ifdef CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT

int hubble_sat_board_init(void)
{
	return 0;
}

int hubble_sat_board_enable(void)
{
	return 0;
}

int hubble_sat_board_disable(void)
{
	return 0;
}

int hubble_sat_board_packet_send(const struct hubble_sat_packet_frames *packet)
{
	ARG_UNUSED(packet);
	return 0;
}

int hubble_sat_board_power_set(int8_t power)
{
	ARG_UNUSED(power);
	return 0;
}

int hubble_sat_board_cw_start(uint8_t channel)
{
	ARG_UNUSED(channel);
	return 0;
}

int hubble_sat_board_cw_stop(void)
{
	return 0;
}

#endif /* CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT */

/* Timer for transmit interval */
static void _timer_cb(struct k_timer *timer)
{
	k_sem_give(&_transmit_sem);
}

K_TIMER_DEFINE(_transmit_timer, _timer_cb, NULL);

static void _thread_transmit_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg3);

	bool sweep = (bool)arg1;
	int interval = (int)arg2;
	struct hubble_sat_packet packet;

	if (hubble_sat_packet_get(&packet, NULL, 0) != 0) {
		return;
	}

	k_sem_take(&_shell_sem, K_FOREVER);
	_is_transmiting = true;
	k_sem_give(&_shell_sem);


	k_sem_reset(&_transmit_sem);
	k_sem_give(&_transmit_sem);

	while (true) {
		k_sem_take(&_shell_sem, K_FOREVER);

		if (!_is_transmiting) {
			break;
		}

		k_sem_give(&_shell_sem);

		/* Don't keep both the shell sem and the transmit sem
		 * because the transmit interval could be very long,
		 * and blocking the shell.
		 *
		 * But this turns into a risk if the shell takes too long,
		 * it'll mess up the interval. Unless absolute interval
		 * is needed, this should be fine.
		 */
		k_sem_take(&_transmit_sem, K_FOREVER);
		k_sem_take(&_shell_sem, K_FOREVER);

		/* Just double check once again, bc when tx stops, it
		 * release the transmit sem immdeiately, and we don't
		 * want anymore tx.
		 */
		if (!_is_transmiting) {
			break;
		}

		hubble_sat_dtm_packet_send(_packet_type, sweep ? -1 : _channel);

		/* One shot timer */
		k_timer_start(&_transmit_timer, K_MSEC(interval), K_NO_WAIT);

		k_sem_give(&_shell_sem);
	}

	k_sem_give(&_shell_sem);
}

static int cmd_carrier_wave(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);

	return hubble_sat_dtm_cw_start(_channel);
}

static int cmd_power(const struct shell *sh, size_t argc, char **argv)
{
	int ret;
	int power;

	if (argc != 2) {
		shell_error(sh, "Usage: power <dBm>");
		return -EINVAL;
	}

	power = atoi(argv[1]);

	ret = hubble_sat_dtm_power_set((int8_t)power);
	if (ret < 0) {
		shell_print(sh, "Failed to set power\n");
		return ret;
	}

	return 0;
}

static int cmd_channel(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	int channel;

	if (argc != 2) {
		shell_error(sh, "Usage:  channel <0..%d>",
			    HUBBLE_NUM_CHANNELS - 1);
		return -EINVAL;
	}

	channel = atoi(argv[1]);
	if (channel < 0 || channel >= HUBBLE_NUM_CHANNELS) {
		shell_print(sh, "Invalid channel\n");
		return -EINVAL;
	}

	_channel = (uint8_t)channel;

	return 0;
}

static int cmd_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);

	k_sem_take(&_shell_sem, K_FOREVER);
	if (_is_transmiting) {
		_is_transmiting = false;
		_transmission_in_progress = false;

		/* if interval is too long, the thread blocks on transmit sem,
		 * so release it immediately to exit tx.
		 */
		k_timer_stop(&_transmit_timer);
		k_sem_give(&_transmit_sem);

		k_sem_give(&_shell_sem);
		(void)k_thread_join(&_thread_transmit, K_FOREVER);
	} else {
		hubble_sat_dtm_cw_stop();
		k_sem_give(&_shell_sem);
	}

	return 0;
}

static int cmd_payload(const struct shell *sh, size_t argc, char **argv)
{
	int32_t len;

	if (argc != 2) {
		goto error;
	}

	len = strtol(argv[1], NULL, 10);

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

	return 0;

error:
	shell_print(sh, "Failed to set payload len. Valid options are: "
			"-1, 0, 4, 9 or 13\n");
	return -EINVAL;
}

static int cmd_transmit(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return hubble_sat_dtm_packet_send(_packet_type, _channel);
}

static bool _spawn_thread(bool sweep, int interval)
{
	bool ret = false;
	k_tid_t tid;

	k_sem_take(&_shell_sem, K_FOREVER);

	if (_transmission_in_progress) {
		goto end;
	}

	tid = k_thread_create(&_thread_transmit, thread_transmit_stack_area,
			      K_THREAD_STACK_SIZEOF(thread_transmit_stack_area),
			      _thread_transmit_entry, (void *)sweep,
			      (void *)interval, NULL, THREAD_TRANSMIT_PRIORITY,
			      0, K_FOREVER);
	if (tid == NULL) {
		goto end;
	}

	_transmission_in_progress = true;
	ret = true;
	k_thread_start(tid);

end:
	k_sem_give(&_shell_sem);
	return ret;
}


static int cmd_transmit_continuously(const struct shell *sh, size_t argc,
				     char **argv)
{
	int interval;

	if (argc < 2) {
		shell_error(sh, "Usage: transmit_continuously <interval_ms>");
		return -EINVAL;
	}

	interval = atoi(argv[1]); /* in ms */

	if (interval < 0) {
		shell_print(sh, "Invalid interval\n");
		return -EINVAL;

	} else if (interval == 0) {
		/* Interval 0 = transmit back to back,
		 * so just set it a number < packet tx time
		 */
		interval = 100;
	}

	if (!_spawn_thread(false, interval)) {
		shell_print(sh, "Another transmission in progress. Use stop to "
				"finish it first.\n");
		return -EALREADY;
	}

	return 0;
}

static int cmd_transmit_sweep(const struct shell *sh, size_t argc, char **argv)
{
	int interval;

	if (argc < 2) {
		shell_error(sh, "Usage: transmit_sweep <interval_ms>");
		return -EINVAL;
	}

	interval = atoi(argv[1]); /* in ms */

	if (interval < 0) {
		shell_print(sh, "Invalid interval\n");
		return -EINVAL;

	} else if (interval == 0) {
		/* Interval 0 = transmit back to back,
		 * so just set it a number < packet tx time
		 */
		interval = 100;
	}

	if (!_spawn_thread(true, interval)) {
		shell_print(sh, "Another transmission in progress. Use stop to "
				"finish it first.\n");
		return -EALREADY;
	}

	return 0;
}

static int cmd_toggle_log(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#ifdef CONFIG_LOG
	const char *tmp_name;
	static bool enabled = false;

	STRUCT_SECTION_FOREACH(log_backend, backend) {
		for (int i = 0U; i < log_src_cnt_get(Z_LOG_LOCAL_DOMAIN_ID); i++) {
			tmp_name = log_source_name_get(Z_LOG_LOCAL_DOMAIN_ID, i);

			if (strncmp(tmp_name, "radio", 64) == 0) {
				log_filter_set(backend, Z_LOG_LOCAL_DOMAIN_ID, i,
					       enabled ? LOG_LEVEL_INF
						       : LOG_LEVEL_NONE);
			}
		}
	}

	enabled = !enabled;
#endif

	return 0;
}

SHELL_CMD_REGISTER(power, NULL, "Set transmission power in dBm", cmd_power);
SHELL_CMD_REGISTER(channel, NULL, "Set frequency channel", cmd_channel);
SHELL_CMD_REGISTER(wave, NULL, "Set a carrier wave", cmd_carrier_wave);
SHELL_CMD_REGISTER(stop, NULL, "Stop ongoing operation", cmd_stop);
SHELL_CMD_REGISTER(payload, NULL, "Set payload len", cmd_payload);
SHELL_CMD_REGISTER(transmit, NULL,
		   "Transmit one single packet on the current channel",
		   cmd_transmit);
SHELL_CMD_REGISTER(transmit_continuously, NULL,
		   "Transmit hubble packets continuously",
		   cmd_transmit_continuously);
SHELL_CMD_REGISTER(
	transmit_sweep, NULL,
	"Transmit hubble packets continuously alternating between channels",
	cmd_transmit_sweep);
SHELL_CMD_REGISTER(toggle_log, NULL, "Toggle log messages", cmd_toggle_log);

static int _hubble_init(void)
{
	/*
	 * We don't need a real key since we don't have any requirement
	 * for the payload contents.
	 */
	static uint8_t _hubble_key[CONFIG_HUBBLE_KEY_SIZE];

	/* Same for time. It is not needed. */
	return hubble_init(0U, 0U, _hubble_key);
}

SYS_INIT(_hubble_init, APPLICATION, 0);
