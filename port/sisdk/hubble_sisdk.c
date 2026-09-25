/*
 * Copyright (c) 2026 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdarg.h>

#include "app_log.h"
#include "sl_iostream.h"

#include <hubble/port/sys.h>

int hubble_log(enum hubble_log_level level, const char *format, ...)
{
#if defined(APP_LOG_ENABLE) && APP_LOG_ENABLE
	static const uint8_t app_level[HUBBLE_LOG_COUNT] = {
		[HUBBLE_LOG_DEBUG] = APP_LOG_LEVEL_DEBUG,
		[HUBBLE_LOG_INFO] = APP_LOG_LEVEL_INFO,
		[HUBBLE_LOG_WARNING] = APP_LOG_LEVEL_WARNING,
		[HUBBLE_LOG_ERROR] = APP_LOG_LEVEL_ERROR,
	};

	va_list args;
	sl_status_t status;

	if (!app_log_check_level(app_level[level])) {
		return 0;
	}

	va_start(args, format);
	status = sl_iostream_vprintf(app_log_iostream, format, args);
	va_end(args);

	return (status == SL_STATUS_OK) ? 0 : -EIO;
#else
	(void)level;
	(void)format;

	return 0;
#endif /* defined(APP_LOG_ENABLE) && APP_LOG_ENABLE */
}
