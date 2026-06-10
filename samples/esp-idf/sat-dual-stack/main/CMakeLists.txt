# Copyright (c) 2026 Hubble Network, Inc.
# SPDX-License-Identifier: Apache-2.0

set(SOURCES
	"main.c"
	"app_ble.c"
)

idf_component_register(SRCS ${SOURCES}
		       PRIV_REQUIRES bt console nvs_flash esp_timer mbedtls hubblenetwork-sdk
		       INCLUDE_DIRS ".")
