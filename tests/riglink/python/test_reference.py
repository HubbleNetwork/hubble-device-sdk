"""Host-only tests for the reference crypto oracle.

Golden vectors are taken from tests/zephyr/ble-network/src/main.c so the
reference is validated against values the firmware is already known to
produce.
"""
import pytest

from hubble_ref_crypto import build_advertisement, decrypt_advertisement

KEY = bytes.fromhex(
    "cd15a5abc060b67288a61e44e995ba77d140bd46564b88de41c15a9273b0ce85"
)
EPOCH_MS = 1760210751803

GOLDEN = [
    (0, b"", bytes.fromhex("a6fc0000c048b6337f4f35bb")),
    (1, bytes.fromhex("deadbeef"),
     bytes.fromhex("a6fc0001c048b63345a8aec6c02eacf0")),
]


@pytest.mark.parametrize("seq,payload,expected", GOLDEN)
def test_build_matches_golden(seq, payload, expected):
    adv = build_advertisement(KEY, EPOCH_MS, seq, payload)
    assert adv == expected


@pytest.mark.parametrize("seq,payload,expected", GOLDEN)
def test_decrypt_round_trip(seq, payload, expected):
    _, got_seq, got_payload = decrypt_advertisement(KEY, expected, EPOCH_MS)
    assert got_seq == seq
    assert got_payload == payload


def test_decrypt_rejects_bad_tag():
    adv = build_advertisement(KEY, EPOCH_MS, 0, b"\x01\x02")
    tampered = bytearray(adv)
    tampered[-1] ^= 0xFF  # corrupt last ciphertext byte
    with pytest.raises(ValueError):
        decrypt_advertisement(KEY, bytes(tampered), EPOCH_MS)
