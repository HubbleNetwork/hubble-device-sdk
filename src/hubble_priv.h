/*
 * Copyright (c) 2025 Hubble Network, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SRC_HUBBLE_PRIV_H
#define SRC_HUBBLE_PRIV_H

const void *hubble_internal_key_get(void);

uint64_t hubble_internal_time_ms_get(void);

/* Returns the last time Unix epoch time was synced (in milliseconds).
 * Used to accommodate clock drifts.
 */
uint64_t hubble_internal_time_last_synced_ms_get(void);

#endif /* SRC_HUBBLE_PRIV_H */
