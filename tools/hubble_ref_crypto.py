#!/usr/bin/env python3
#
# Copyright (c) 2025 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""Reference implementation of the Hubble BLE advertisement crypto.

This mirrors the on-wire format produced by hubble_ble_advertise_get(). It is
the single source of truth shared by tools/ble-adv.py and the riglink test
harness.
"""

from bitstring import BitArray

from Crypto.Cipher import AES
from Crypto.Hash import CMAC
from Crypto.Protocol.KDF import SP800_108_Counter

HUBBLE_AES_KEY_SIZE = 32
HUBBLE_AES_NONCE_SIZE = 12
HUBBLE_DEVICE_ID_SIZE = 4
HUBBLE_AES_TAG_SIZE = 4
HUBBLE_UUID_PREFIX = bytes([0xA6, 0xFC])
HUBBLE_ROTATION_PERIOD_SEC = 86400


def generate_kdf_key(key: bytes, key_size: int, label: str,
                     context: int) -> bytes:
    return SP800_108_Counter(
        key,
        key_size,
        lambda session_key, data: CMAC.new(session_key, data, AES).digest(),
        label=label.encode(),
        context=str(context).encode(),
    )


def get_device_id(master_key: bytes, time_counter: int) -> int:
    device_key = generate_kdf_key(master_key, HUBBLE_AES_KEY_SIZE,
                                  'DeviceKey', time_counter)
    device_id = generate_kdf_key(device_key, HUBBLE_DEVICE_ID_SIZE,
                                 'DeviceID', 0)
    return int.from_bytes(device_id, byteorder='big')


def get_nonce(master_key: bytes, time_counter: int, counter: int) -> bytes:
    nonce_key = generate_kdf_key(master_key, HUBBLE_AES_KEY_SIZE,
                                 "NonceKey", time_counter)
    return generate_kdf_key(nonce_key, HUBBLE_AES_NONCE_SIZE, "Nonce", counter)


def get_encryption_key(master_key: bytes, time_counter: int,
                       counter: int) -> bytes:
    encryption_key = generate_kdf_key(master_key, HUBBLE_AES_KEY_SIZE,
                                      "EncryptionKey", time_counter)
    return generate_kdf_key(encryption_key, HUBBLE_AES_KEY_SIZE, 'Key',
                            counter)


def get_auth_tag(key: bytes, ciphertext: bytes) -> bytes:
    return CMAC.new(key, ciphertext, AES).digest()[:HUBBLE_AES_TAG_SIZE]


def aes_encrypt(key: bytes, nonce_session: bytes, data: bytes):
    ciphertext = AES.new(key, AES.MODE_CTR,
                         nonce=nonce_session).encrypt(data)
    return ciphertext, get_auth_tag(key, ciphertext)


def generate_ble_adv(device_id, seq_no, auth_tag, encrypted_payload) -> bytes:
    if len(encrypted_payload) > 13:
        raise ValueError('Encrypted Payload is too long.')

    ble_adv = BitArray()
    protocol_version = 0b000000
    ble_adv.append(f'uint:6={protocol_version}')
    ble_adv.append(f'uint:10={seq_no}')
    ble_adv.append(f'uint:32={device_id}')
    ble_adv.append(auth_tag)
    ble_adv.append(encrypted_payload)
    return ble_adv.tobytes()


def time_counter_from_epoch_ms(
        epoch_ms: int, rotation_sec: int = HUBBLE_ROTATION_PERIOD_SEC) -> int:
    return epoch_ms // (rotation_sec * 1000)


def build_advertisement(master_key: bytes, epoch_ms: int, seq: int,
                        payload: bytes,
                        rotation_sec: int = HUBBLE_ROTATION_PERIOD_SEC
                        ) -> bytes:
    """Build the full advertisement (incl. the 0xA6 0xFC UUID prefix)."""
    tc = time_counter_from_epoch_ms(epoch_ms, rotation_sec)
    device_id = get_device_id(master_key, tc)
    nonce = get_nonce(master_key, tc, seq)
    enc_key = get_encryption_key(master_key, tc, seq)
    ciphertext, tag = aes_encrypt(enc_key, nonce, payload)
    return HUBBLE_UUID_PREFIX + generate_ble_adv(device_id, seq, tag,
                                                 ciphertext)


def decrypt_advertisement(master_key: bytes, adv: bytes, epoch_ms: int,
                          rotation_sec: int = HUBBLE_ROTATION_PERIOD_SEC):
    """Decrypt/verify an advertisement. Returns (device_id, seq, payload).

    Raises ValueError on a UUID-prefix, device-id, or auth-tag mismatch.
    """
    if adv[:2] != HUBBLE_UUID_PREFIX:
        raise ValueError('Bad UUID prefix')
    body = adv[2:]
    version = body[0] >> 2
    if version != 0:
        raise ValueError(f'Unexpected protocol version {version}')
    seq = ((body[0] & 0x03) << 8) | body[1]
    device_id = int.from_bytes(body[2:6], byteorder='big')
    tag = body[6:10]
    ciphertext = body[10:]

    tc = time_counter_from_epoch_ms(epoch_ms, rotation_sec)
    if device_id != get_device_id(master_key, tc):
        raise ValueError('Device ID mismatch')
    enc_key = get_encryption_key(master_key, tc, seq)
    if get_auth_tag(enc_key, ciphertext) != tag:
        raise ValueError('Auth tag mismatch')
    nonce = get_nonce(master_key, tc, seq)
    payload = AES.new(enc_key, AES.MODE_CTR, nonce=nonce).decrypt(ciphertext)
    return device_id, seq, payload
