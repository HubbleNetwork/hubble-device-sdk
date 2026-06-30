"""Host-driven BLE round-trip tests over riglink.

The host sets a known key, a fixed (overridden) uptime, and a fixed sequence
counter, then asks the firmware to generate an advertisement. The result must
byte-match the reference oracle AND decrypt back to the input payload.
"""
import pytest

from hubble_ref_crypto import build_advertisement, decrypt_advertisement

KEY_HEX = "cd15a5abc060b67288a61e44e995ba77d140bd46564b88de41c15a9273b0ce85"
KEY = bytes.fromhex(KEY_HEX)
EPOCH_MS = 1760210751803

# (seq, payload_bytes)
CASES = [
    (0, b""),
    (1, bytes.fromhex("deadbeef")),
    (5, bytes.fromhex("00112233445566778899aabbcc")),  # 13 bytes (max)
    (1023, b"\x42"),                                    # max seq
]


def _make_adv(dev, seq, payload):
    assert dev.test_init(KEY_HEX, EPOCH_MS)["ret"] == 0
    assert dev.test_set_uptime(0)["ret"] is None
    assert dev.test_set_seq(seq)["ret"] is None
    res = dev.adv_get(payload.hex())
    assert res["ret"] == 0
    return bytes.fromhex(res["adv_hex"]), res["adv_len"]


@pytest.mark.parametrize("seq,payload", CASES)
def test_adv_matches_reference(dev, seq, payload):
    adv, adv_len = _make_adv(dev, seq, payload)
    assert adv_len == len(adv)
    assert adv == build_advertisement(KEY, EPOCH_MS, seq, payload)


@pytest.mark.parametrize("seq,payload", CASES)
def test_adv_decrypts_to_payload(dev, seq, payload):
    adv, _ = _make_adv(dev, seq, payload)
    _, got_seq, got_payload = decrypt_advertisement(KEY, adv, EPOCH_MS)
    assert got_seq == seq
    assert got_payload == payload


def test_adv_rejects_oversized_payload(dev):
    assert dev.test_init(KEY_HEX, EPOCH_MS)["ret"] == 0
    dev.test_set_uptime(0)
    dev.test_set_seq(0)
    res = dev.adv_get(("aa" * 14))  # 14 bytes > HUBBLE_BLE_MAX_DATA_LEN (13)
    assert res["ret"] != 0


def test_expiration_within_rotation_period(dev):
    assert dev.test_init(KEY_HEX, EPOCH_MS)["ret"] == 0
    dev.test_set_uptime(0)
    res = dev.hubble_ble_advertise_expiration_get()
    rotation_ms = 86400 * 1000
    assert 0 < res["ret"] <= rotation_ms
