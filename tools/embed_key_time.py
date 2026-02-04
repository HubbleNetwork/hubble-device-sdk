#!/usr/bin/env python3
#
# Copyright (c) 2025 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import base64
import time


KEY_TEMPLATE = """
/*
 * This file contents was automatically generated.
 */
#define HUBBLE_KEY_SET 1

static uint8_t master_key[CONFIG_HUBBLE_KEY_SIZE] = {key};
"""

UNIX_TIME_TEMPLATE = """
/*
 * This file contents was automatically generated.
 */
#define HUBBLE_UNIX_EPOCH_SET 1

static uint64_t unix_epoch_ms = {epoch_ms};
"""

def provision_data(key: str, encoded: bool, path: str, dry: bool) -> None:
    with open(key, "rb") as f:
        key_data = bytearray(f.read())
        if encoded:
            key_data = bytearray(base64.b64decode(key_data))

    key_hex = "{" +", ".join([hex(x) for x in key_data]) + "}"
    unix_epoch_ms =  str(int(time.time() * 1000))

    if dry:
        print(f"static uint8_t master_key[CONFIG_HUBBLE_KEY_SIZE] = {key_hex}")
        print(f"static uint64_t unix_epoch_ms = {unix_epoch_ms}")
        return

    with open(path + "/key.c", "w") as f:
        f.write(KEY_TEMPLATE.format(key=key_hex))

    with open(path + "/epoch.c", "w") as f:
        f.write(UNIX_TIME_TEMPLATE.format(epoch_ms=unix_epoch_ms))


def parse_args() -> None:
    """
    Embed key & Unix epoch time into the fw.

    This is a simple script to provision a key and Unix epoch time into a device
    for test purpose.

    usage: embed_key_time.py [-h] [-b] [-o OUTPUT_DIR] [-d] key
    """

    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument("key",
                        help="The key to provision")
    parser.add_argument("-b", "--base64",
                        help="The key is encoded in base64", action='store_true', default=False)
    parser.add_argument("-o", "--output-dir",
                        help="Path where epoch.c and key.c will be generated", default=".")
    parser.add_argument("-d", "--dry-run",
                        help="Just print the data into console", action='store_true', default=False)
    args = parser.parse_args()


def main():
    parse_args()

    provision_data(args.key, args.base64, args.output_dir, args.dry_run)


if __name__ == '__main__':
    main()
