# Copyright (c) 2026 Hubble Network, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

# The Hubble master key is provisioned at build time via the
# CONFIG_SAMPLE_HUBBLE_KEY Kconfig option (a base64 string), e.g.
#
#   west build -b <board> . -- -DCONFIG_SAMPLE_HUBBLE_KEY="<base64 key>"
#
# It is decoded into bytes at runtime (Zephyr provides base64_decode). If it is
# omitted (e.g. a CI build-only run) the firmware falls back to an all-zero
# placeholder. See src/main.c and Kconfig.

target_sources(app PRIVATE src/main.c)
