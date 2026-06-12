/*
 * Copyright (c) 2025 Hubble Network
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Host-driven test fixture: exposes the Hubble BLE advertisement API over
 * riglink, with uptime and the sequence counter overridden so the host can
 * make advertisements fully deterministic and verify them by decryption.
 *
 * Transport is riglink's Zephyr shell backend (CONFIG_RIGLINK_BACKEND_SHELL):
 * the shell owns the UART -- so RX is interrupt-driven and won't drop bytes --
 * and dispatches each command below as a subcommand of "rig" (the host targets
 * it with shell_root="rig"). This file therefore implements no rig_putc /
 * rig_getc and no rig_init() / rig_run() pump; the backend brings all of that
 * up via SYS_INIT.
 */
/* The largest str argument is the 64-char (256-bit) master key hex passed to
 * test_init; riglink's default per-arg buffer is 64 bytes, which would
 * truncate it to 63 chars (odd length -> decode failure). Size it to fit the
 * key hex plus the NUL terminator before including riglink.h.
 */
#define RIG_STR_ARG_SIZE 96
#include <riglink.h>

#include <hubble/hubble.h>

#include <zephyr/sys/util.h>

#include <string.h>

/* --- host-controlled SDK port overrides (enabled via *_CUSTOM Kconfig) --- */

static uint64_t test_uptime_ms;
static uint16_t test_seq;

uint64_t hubble_uptime_get(void)
{
	return test_uptime_ms;
}

uint16_t hubble_sequence_counter_get(void)
{
	return test_seq;
}

/* --- helpers --- */

/* hubble_init/hubble_key_set store the key pointer without copying, so the
 * buffer must outlive every advertisement call: keep it static.
 */
static uint8_t master_key[CONFIG_HUBBLE_KEY_SIZE];

/* --- exposed surface --- */

RIG_FN(int, test_init, str, uint64_t)
{
	if (hex2bin(arg0, strlen(arg0), master_key, sizeof(master_key)) !=
	    sizeof(master_key)) {
		return -1;
	}
	test_uptime_ms = 0U;
	test_seq = 0U;
	return hubble_init(arg1, master_key);
}

RIG_FN(void, test_set_uptime, uint64_t)
{
	test_uptime_ms = arg0;
}

RIG_FN(void, test_set_seq, uint16_t)
{
	test_seq = arg0;
}

RIG_FN(int, adv_get, str)
{
	uint8_t input[HUBBLE_BLE_MAX_DATA_LEN + 1];
	uint8_t out[HUBBLE_BLE_ADV_HEADER_SIZE + HUBBLE_BLE_MAX_DATA_LEN + 1];
	char hexout[2U * sizeof(out) + 1U];
	size_t out_len = sizeof(out);
	size_t hlen = strlen(arg0);
	size_t in_len = hex2bin(arg0, hlen, input, sizeof(input));

	/* hex2bin returns 0 for both an empty payload (valid) and a decode
	 * failure; only a non-empty input that decoded to nothing is an error.
	 */
	if (in_len == 0U && hlen != 0U) {
		return -1;
	}

	int status = hubble_ble_advertise_get(input, in_len, out, &out_len);

	if (status != 0) {
		return status;
	}

	bin2hex(out, out_len, hexout, sizeof(hexout));

	rig_emit("adv_hex", hexout);
	rig_emit("adv_len", (int64_t)out_len);
	return 0;
}

RIG_EXPOSE(uint32_t, hubble_ble_advertise_expiration_get);

int main(void)
{
	/* The riglink shell backend runs the link from SYS_INIT and the shell
	 * thread; the main thread has nothing to do.
	 */
	return 0;
}
