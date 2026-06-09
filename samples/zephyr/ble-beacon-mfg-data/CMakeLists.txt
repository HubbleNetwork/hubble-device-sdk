# Copyright (c) 2026 Hubble Network, Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

# The Hubble master key is passed on the build command line as base64:
#
#   west build -b <board> . -- -DHUBBLE_KEY="<base64 key>"
#
# It is decoded into a C byte array here, at configure time, so no base64 code
# ships in the firmware. Python3 is already a hard requirement of any Zephyr
# build, so this adds no new dependency and runs automatically during the build.
set(generated_key_c ${CMAKE_CURRENT_BINARY_DIR}/generated_master_key.c)

if(NOT DEFINED HUBBLE_KEY OR "${HUBBLE_KEY}" STREQUAL "")
  # No key passed (e.g. CI build-only). Emit an all-zero placeholder so the
  # sample still compiles; it will not produce valid advertisements until a real
  # key is supplied via -DHUBBLE_KEY.
  message(WARNING
    "HUBBLE_KEY not set; embedding an all-zero placeholder key. Pass your "
    "base64 key with -DHUBBLE_KEY=\"<key>\" before flashing.")
  # Match the format the decode branch emits (0x-prefixed, no trailing comma).
  string(REPEAT "0x0," ${CONFIG_HUBBLE_KEY_SIZE} KEY_BYTES)
  string(REGEX REPLACE ",$" "" KEY_BYTES "${KEY_BYTES}")
else()
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c
      "import base64,sys; print(','.join(hex(b) for b in base64.b64decode(sys.argv[1])))"
      "${HUBBLE_KEY}"
    OUTPUT_VARIABLE KEY_BYTES
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE decode_result)
  if(NOT decode_result EQUAL 0)
    message(FATAL_ERROR
      "Failed to base64-decode HUBBLE_KEY; check that the key is valid base64.")
  endif()
endif()

configure_file(src/master_key.c.in ${generated_key_c} @ONLY)

target_sources(app PRIVATE src/main.c ${generated_key_c})
