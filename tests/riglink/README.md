# riglink BLE Test

A host-driven test of the BLE advertisement API in `include/hubble/ble.h`. A
Python host drives the SDK firmware over a serial link using
[riglink](https://github.com/buckleypaul/riglink): it sets a known master
key, a fixed (overridden) uptime, and a fixed sequence counter, asks the
firmware to generate an advertisement via `hubble_ble_advertise_get()`, then
verifies the result both byte-matches and decrypts against the reference
oracle in `tools/hubble_ref_crypto.py`.

No radio is used (`CONFIG_BT=n`); the firmware only exposes the API over
riglink. The same firmware therefore runs unchanged on `native_sim` and on an
`nrf52840dk`.

## Layout

- `src/main.c` — firmware that exposes the BLE advertisement API over riglink.
- `boards/` — board overlays (`native_sim`, `native_sim_native_64`,
  `nrf52840dk_nrf52840`).
- `python/` — the pytest harness:
  - `conftest.py` — builds and launches the `native_sim` app and connects
    riglink over its pseudotty; with `--riglink-port` it instead targets an
    attached device.
  - `test_ble.py` — integration tests driving the firmware over riglink.
  - `test_reference.py` — host-only tests of the reference oracle.
  - `requirements.txt` — host Python dependencies.
- The reference oracle lives at `tools/hubble_ref_crypto.py`.

This is a standalone Zephyr application plus a pytest harness; it is **not**
run via `west twister`.

## Prerequisites

The `riglink` and `jcon` modules are declared in the workspace `west.yml`.
Fetch them with:

```sh
west update riglink jcon
```

Host Python dependencies (`pycryptodome`, `bitstring`, `pytest`, and
`riglink`) are listed in `tests/riglink/python/requirements.txt`. Note that
`riglink` is not on PyPI; install it from the workspace clone at
`modules/lib/riglink/python`.

## Run on native_sim

Zephyr's `native_sim` (POSIX arch) only builds on Linux. On macOS or Windows,
run the suite inside a Linux Docker container.

### Docker (macOS / Windows / any host)

The image `ghcr.io/zephyrproject-rtos/ci:v0.28.7` bundles Zephyr SDK 0.17.4,
matching the checked-out Zephyr. Mount the whole west workspace at
`/workspace`:

```sh
docker run --rm \
  -v <workspace-root>:/workspace \
  -w /workspace/hubble-device-sdk/tests/riglink/python \
  -e ZEPHYR_BASE=/workspace/zephyr \
  -e ZEPHYR_SDK_INSTALL_DIR=/opt/toolchains/zephyr-sdk-0.17.4 \
  -e HOME=/tmp \
  ghcr.io/zephyrproject-rtos/ci:v0.28.7 \
  bash -c "pip install -q pycryptodome bitstring pytest && pip install -q -e /workspace/modules/lib/riglink/python && python -m pytest -v"
```

`<workspace-root>` is the west topdir — the directory that contains `zephyr/`,
`modules/`, and `hubble-device-sdk/`.

### Native Linux

On a Linux host that already has a Zephyr environment and `west`:

```sh
cd tests/riglink/python
pip install -r requirements.txt
python -m pytest -v
```

The conftest auto-builds `native_sim/native/64` and connects to the firmware
over its pseudotty.

## Run on an attached nrf52840dk

Flash the app, then point pytest at the device's serial port:

```sh
west build -b nrf52840dk/nrf52840 tests/riglink
west flash
cd tests/riglink/python
python -m pytest -v -p no:cacheprovider --riglink-port /dev/ttyACM0
```

The `--riglink-port` option is provided by riglink's own pytest plugin. Its
device-path value (e.g. `/dev/ttyACM0`) looks like a filesystem path to
pytest's `rootdir` detection, which then tries to write its cache under that
read-only location; `-p no:cacheprovider` disables the unused cache and avoids
the resulting warning.

## Scope and follow-ons

This test covers the v1 configuration: 256-bit master key, PSA crypto backend,
and the Unix-time counter source. Planned follow-ons:

- 128-bit master key.
- MbedTLS crypto backend.
- Real RF broadcast and scan (radio enabled).
- twister / CI integration.
