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

from bitstring import BitArray

from Crypto.Cipher import AES
from Crypto.Hash import CMAC
from Crypto.Protocol.KDF import SP800_108_Counter

HUBBLE_AES_KEY_SIZE = 32
HUBBLE_AES_NONCE_SIZE = 12
HUBBLE_DEVICE_ID_SIZE = 4
HUBBLE_AES_TAG_SIZE = 4


def generate_kdf_key(key: bytes, key_size: int, label: str,
                     context: int) -> bytes:
    label = label.encode()
    context = str(context).encode()

    return SP800_108_Counter(
        key,
        key_size,
        lambda session_key, data: CMAC.new(session_key, data, AES).digest(),
        label=label,
        context=context,
    )


def get_device_id(master_key: bytes, time_counter: int) -> int:
    device_key = generate_kdf_key(master_key, HUBBLE_AES_KEY_SIZE,
                                  'DeviceKey', time_counter)
    device_id = generate_kdf_key(device_key, HUBBLE_DEVICE_ID_SIZE,
                                 'DeviceID', 0)

    return int.from_bytes(device_id, byteorder='big')


def get_nonce(master_key: bytes, time_counter: int, counter: int) -> bytes:
    nonce_key = generate_kdf_key(
        master_key, HUBBLE_AES_KEY_SIZE, "NonceKey", time_counter
    )

    return generate_kdf_key(nonce_key, HUBBLE_AES_NONCE_SIZE, "Nonce", counter)


def get_encryption_key(master_key: bytes, time_counter: int,
                       counter: int) -> bytes:
    encryption_key = generate_kdf_key(
        master_key, HUBBLE_AES_KEY_SIZE, "EncryptionKey", time_counter
    )

    return generate_kdf_key(encryption_key, HUBBLE_AES_KEY_SIZE,
                            'Key', counter)


def get_auth_tag(key: bytes, ciphertext: bytes) -> bytes:
    computed_cmac = CMAC.new(key, ciphertext, AES).digest()

    return computed_cmac[:HUBBLE_AES_TAG_SIZE]


def aes_encrypt(key: bytes, nonce_session: bytes, data: bytes) -> bytes:
    ciphertext = AES.new(key, AES.MODE_CTR, nonce=nonce_session).encrypt(data)
    tag = get_auth_tag(key, ciphertext)

    return ciphertext, tag


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


def generate_ble_adv(device_id, seq_no, auth_tag, encrypted_payload) -> bytes:
    ble_adv = BitArray()

    if len(encrypted_payload) > 13:
        raise ValueError('Encrypted Payload is too long.')

    protocol_version = 0b000000

    ble_adv.append(f'uint:6={protocol_version}')
    ble_adv.append(f'uint:10={seq_no}')
    ble_adv.append(f'uint:32={device_id}')

    ble_adv.append(auth_tag)
    ble_adv.append(encrypted_payload)

    return ble_adv.tobytes()


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
