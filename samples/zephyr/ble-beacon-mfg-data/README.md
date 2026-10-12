# Hubble Network BLE Beacon + Manufacturer Data Sample

This sample demonstrates how to emit a **single** BLE advertisement that carries
both the standard Hubble beacon payload **and** an application-owned
*manufacturer specific data* field.

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  (optional), for testing with [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- A [Zephyr west workspace](../../../docs/quickstart/zephyr/index.rst) that includes the Hubble Device SDK.
- A board with Bluetooth LE support.
- A computer with Bluetooth LE that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- The Hubble Connect app for
  [iOS](https://apps.apple.com/us/app/hubble-connect/id6751236191) or
  [Android](https://play.google.com/store/apps/details?id=com.hubble.connect)
  (optional), for the backend test.

## Overview

The manufacturer field holds a 4-byte counter that is incremented every 5
minutes. Use this as a starting point when you need to attach your own data to
a Hubble advertisement without sending it through the (encrypted) Hubble payload.

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/zephyr/ble-beacon-mfg-data`

### Advertisement layout

The advertisement is built from three AD structures:

| # | AD type | Contents |
| --- | --- | --- |
| 1 | `BT_DATA_UUID16_ALL` | `0xFCA6` (the Hubble service UUID) |
| 2 | `BT_DATA_SVC_DATA16` | Encrypted Hubble payload from `hubble_ble_advertise_get()` |
| 3 | `BT_DATA_MANUFACTURER_DATA` | `0xFCA6` (2B, little-endian) + counter (4B, little-endian) |

By BLE convention, manufacturer data begins with a 2-byte company identifier.
This sample reuses Hubble's `0xFCA6` identifier as that prefix so scanners can
recognize the field.

> [!NOTE]
> The counter is held in RAM and resets to 0 on reboot. Persisting it across
> reboots is out of scope for this sample.

### Size budget

Legacy BLE advertising allows 31 payload bytes. This sample passes no Hubble
user payload (`NULL, 0`), so the Hubble service data is 12 bytes and the whole
advertisement is about 26 bytes. If you add Hubble user payload (up to 13
bytes), it shares the same 31-byte budget with the manufacturer field, so you
cannot max out both at once.

## Environment Setup

First, set up the environment. This step assumes you've set up a west workspace
that includes the Hubble Device SDK. If you haven't, follow the [Zephyr quick start](../../../docs/quickstart/zephyr/index.rst).
Use the following command to activate the workspace's Python virtual environment.

### Windows

```ps1
<path_to_west_workspace>\.venv\Scripts\Activate.ps1
```

### Linux & macOS

```sh
source <path_to_west_workspace>/.venv/bin/activate
```

### Python Dependencies

Install the [pyhubblenetwork](https://hubble.com/docs/guides/cli) tool for testing:

```sh
pip install pyhubblenetwork
```

## BLE Advertising Parameters

The sample advertises with the following parameters by default. To change one,
edit the macro or option in the *Set by* column and rebuild.

| Parameter | Value | Set by |
| --- | --- | --- |
| Beacon interval | 1000–1200 ms | `ADV_INTERVAL_MIN_MS` / `ADV_INTERVAL_MAX_MS` in [src/main.c](src/main.c) |
| Tx power | 0 dBm | `CONFIG_BT_CTLR_TX_PWR_0` in [prj.conf](prj.conf) |

## Configurations

This sample exposes the following options (via `west build -t menuconfig`, under
"Hubble Network Beacon + Manufacturer Data Sample options"):

| Option | Default | Description |
| --- | --- | --- |
| `CONFIG_HUBBLE_MFG_DATA_SAMPLE_UPDATE_PERIOD` | `300` | Period in seconds at which the advertisement is refreshed and the counter is incremented. |

## Build-time Provisioning

Set `HUBBLE_KEY` to your Hubble device key, in base64. The key must be
registered as **AES-256** with the **Device uptime** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).
Add this variable in your build command below. The key is decoded into a byte
array at build time, so no base64 code runs on the device and no pre-build
script is needed.

## Build and Flash

Build for your board, passing your device key. Add `-p` for a pristine build.

```sh
west build -b <board> -- -DHUBBLE_KEY="<your base64 device key>"
```

e.g.
```sh
west build -b nrf52840dk/nrf52840 -- -DHUBBLE_KEY="<your base64 device key>"
```

Flash the board:

```sh
west flash
```

After flashing, the device advertises as a Hubble beacon with the extra
manufacturer-data counter, refreshing every 5 minutes.

## Test

### Local BLE Scanning

The `hubblenetwork ble scan` tool can be used to test the BLE beacon.
It scans for a device advertising with the given key and attempts to
decode its payload.

```sh
hubblenetwork ble scan --key "<your base64 device key>" --counter-mode device_uptime
```

Correctly decoded packets will stream to the console.

### Backend Test

Download the Hubble Connect app to your smartphone and sign in, then keep the
phone near the device with the app open. The phone acts as a scanner: the
device appears in the app, and its packets, with their payload data, appear on
[Hubble Dash](https://dash.hubble.com/dashboard).
