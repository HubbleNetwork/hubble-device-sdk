# Copyright (c) 2026 Hubble Network, Inc.
# SPDX-License-Identifier: Apache-2.0

set(SOURCES "main.c")

if(CONFIG_SAMPLE_SYNC_TIME)
    list(APPEND SOURCES "time_sync.c")
endif()

idf_component_register(SRCS ${SOURCES}
		       PRIV_REQUIRES bt nvs_flash mbedtls hubblenetwork-sdk
		       INCLUDE_DIRS ".")
