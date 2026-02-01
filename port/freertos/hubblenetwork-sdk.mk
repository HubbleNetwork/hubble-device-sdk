# Copyright (c) 2024 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0


THIS_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Define base sources
HUBBLENETWORK_SDK_PORT_DIR := $(THIS_DIR)
HUBBLENETWORK_SDK_SRC_DIR := $(THIS_DIR)/../../src
HUBBLENETWORK_SDK_INCLUDE_DIR := $(THIS_DIR)/../../include

# Extract config variables from config.h
CONFIG_VARS := $(shell sed -nE \
	-e 's/^[[:space:]]*\#define[[:space:]]+(CONFIG_HUBBLE_[A-Z0-9_]*)[[:space:]]+(.*)$$/\1=\2/p' \
	-e 's/^[[:space:]]*\#define[[:space:]]+(CONFIG_HUBBLE_[A-Z0-9_]*)[[:space:]]*$$/\1=1/p' \
	$(HUBBLENETWORK_SDK_PORT_DIR)/config.h)

$(foreach v,$(CONFIG_VARS),$(eval $(v)))

HUBBLENETWORK_SDK_SOURCES = \
	$(HUBBLENETWORK_SDK_PORT_DIR)/hubble_freertos.c \
	$(HUBBLENETWORK_SDK_SRC_DIR)/hubble.c

HUBBLENETWORK_SDK_FLAGS = \
	-I$(HUBBLENETWORK_SDK_INCLUDE_DIR) \
	-I$(HUBBLENETWORK_SDK_SRC_DIR) \
	-imacros $(HUBBLENETWORK_SDK_PORT_DIR)/config.h

ifeq ($(CONFIG_HUBBLE_BLE_NETWORK),1)
HUBBLENETWORK_SDK_SOURCES += \
	$(HUBBLENETWORK_SDK_SRC_DIR)/hubble_ble.c
endif

ifeq ($(CONFIG_HUBBLE_SAT_NETWORK),1)
HUBBLENETWORK_SDK_SOURCES += \
	$(HUBBLENETWORK_SDK_PORT_DIR)/hubble_sat_freertos.c \
	$(HUBBLENETWORK_SDK_SRC_DIR)/hubble_sat.c \
	$(HUBBLENETWORK_SDK_SRC_DIR)/hubble_sat_ephemeris.c \
	$(HUBBLENETWORK_SDK_SRC_DIR)/utils/bitarray.c \
	$(HUBBLENETWORK_SDK_SRC_DIR)/reed_solomon_encoder.c

ifeq ($(CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_V1),1)
HUBBLENETWORK_SDK_SOURCES += $(HUBBLENETWORK_SDK_SRC_DIR)/hubble_sat_packet.c
else ifeq ($(CONFIG_HUBBLE_SAT_NETWORK_PROTOCOL_DEPRECATED),1)
HUBBLENETWORK_SDK_SOURCES += $(HUBBLENETWORK_SDK_SRC_DIR)/hubble_sat_packet_deprecated.c
endif

endif
