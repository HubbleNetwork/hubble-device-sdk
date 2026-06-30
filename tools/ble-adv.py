#!/usr/bin/env python3
#
# Copyright (c) 2025 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0


"""
Simple application to advertise data through Hubble BLE Network.
"""

import argparse
import base64

from hubble_ref_crypto import (
    generate_ble_adv,
    get_device_id,
    get_nonce,
    get_encryption_key,
    aes_encrypt,
)


def parse_args() -> argparse.Namespace:
    """
    Advertise data using Hubble BLE Network.

    usage: ble_adv.py [-h] [-b] [--time-counter TC] [--seq-no SEQ]
                      [--payload-hex HEX] [--print] key [payload]
    """

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False)

    parser.add_argument("master_key",
                        help="Path to the device key file")
    parser.add_argument("-b", "--base64",
                        help="The key is encoded in base64",
                        action='store_true', default=False)
    parser.add_argument("payload", nargs="?", default="",
                        help="Data to transmit (string)")
    parser.add_argument("--time-counter", type=int, default=None,
                        help="Override time counter directly "
                             "(default: derive from current date)")
    parser.add_argument("--seq-no", type=int, default=0,
                        help="Sequence number 0-1023 (default: 0)")
    parser.add_argument("--payload-hex", type=str, default=None,
                        help="Payload as hex string (e.g. 'deadbeef'), "
                             "empty string = no payload")
    parser.add_argument("--print", dest="print_mode", action='store_true',
                        default=False,
                        help="Print output as C hex array instead of "
                             "transmitting via BLE")

    return parser.parse_args()


def format_c_hex(data: bytes) -> str:
    """Format bytes as a C hex array string."""
    hex_bytes = ", ".join(f"0x{b:02x}" for b in data)
    return "{" + hex_bytes + "}"


def main() -> None:
    args = parse_args()

    key = None

    with open(args.master_key, "rb") as f:
        key = bytearray(f.read())
        if args.base64:
            key = bytearray(base64.b64decode(key))

    master_key = bytes(key)

    # Determine time counter
    if args.time_counter is not None:
        time_counter = args.time_counter
    else:
        from datetime import datetime
        time_counter = int(datetime.now().timestamp()) // 86400

    # Determine payload
    if args.payload_hex is not None:
        if args.payload_hex == "":
            payload = b""
        else:
            payload = bytes.fromhex(args.payload_hex)
    else:
        payload = args.payload.encode()

    seq_no = args.seq_no

    device_id = get_device_id(master_key, time_counter)
    nonce = get_nonce(master_key, time_counter, seq_no)
    enc_key = get_encryption_key(master_key, time_counter, seq_no)
    encrypted_payload, auth_tag = aes_encrypt(enc_key, nonce, payload)

    ble_adv = generate_ble_adv(device_id, seq_no, auth_tag, encrypted_payload)

    if args.print_mode:
        # Prepend UUID bytes and print as C hex array
        uuid_bytes = bytes([0xa6, 0xfc])
        full_adv = uuid_bytes + ble_adv
        print(format_c_hex(full_adv))
    else:
        from bluezero import broadcaster

        url_beacon = broadcaster.Beacon()
        url_beacon.add_service_data('FCA6', ble_adv)
        url_beacon.start_beacon()


if __name__ == '__main__':
    main()
