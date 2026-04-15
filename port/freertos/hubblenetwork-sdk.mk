# Copyright (c) 2024 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0


THIS_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Define base sources
HUBBLENETWORK_SDK_PORT_DIR := $(THIS_DIR)
HUBBLENETWORK_SDK_SRC_DIR := $(THIS_DIR)/../../src
HUBBLENETWORK_SDK_INCLUDE_DIR := $(THIS_DIR)/../../include

# Allow application to supply its own config file.
# If not set, fall back to the SDK default.
HUBBLENETWORK_SDK_CONFIG ?= $(HUBBLENETWORK_SDK_PORT_DIR)/config.h

# Extract config variables from config file
CONFIG_VARS := $(shell sed -nE \
	-e 's/^[[:space:]]*\#define[[:space:]]+(CONFIG_HUBBLE_[A-Z0-9_]*)[[:space:]]+(.*)$$/\1=\2/p' \
	-e 's/^[[:space:]]*\#define[[:space:]]+(CONFIG_HUBBLE_[A-Z0-9_]*)[[:space:]]*$$/\1=1/p' \
	$(HUBBLENETWORK_SDK_CONFIG))

$(foreach v,$(CONFIG_VARS),$(eval $(v)))

HUBBLENETWORK_SDK_SOURCES = \
	$(HUBBLENETWORK_SDK_PORT_DIR)/hubble_freertos.c \
	$(HUBBLENETWORK_SDK_SRC_DIR)/hubble.c \
	$(HUBBLENETWORK_SDK_SRC_DIR)/hubble_crypto.c

HUBBLENETWORK_SDK_FLAGS = \
	-I$(HUBBLENETWORK_SDK_INCLUDE_DIR) \
	-I$(HUBBLENETWORK_SDK_SRC_DIR) \
	-imacros $(HUBBLENETWORK_SDK_CONFIG)

ifeq ($(CONFIG_HUBBLE_NETWORK_KEY_256),1)
HUBBLENETWORK_SDK_FLAGS += -DCONFIG_HUBBLE_KEY_SIZE=32
endif

ifeq ($(CONFIG_HUBBLE_NETWORK_KEY_128),1)
HUBBLENETWORK_SDK_FLAGS += -DCONFIG_HUBBLE_KEY_SIZE=16
endif

ifeq ($(CONFIG_HUBBLE_NETWORK_CRYPTO_PSA),1)
HUBBLENETWORK_SDK_SOURCES += \
	$(HUBBLENETWORK_SDK_SRC_DIR)/crypto/psa.c
endif

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
endif

endif
