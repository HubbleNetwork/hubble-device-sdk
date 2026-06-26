/*
 * Minimal stub for the TI UART2 driver — host-test use only.
 */
#pragma once
#include <stddef.h>

typedef void *UART2_Handle;

static inline int UART2_write(UART2_Handle h, const void *buf, size_t n,
			      void *p)
{
	(void)h;
	(void)buf;
	(void)n;
	(void)p;
	return 0;
}
