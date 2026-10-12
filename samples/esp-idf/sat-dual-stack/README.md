# Hubble Network Satellite Dual-Stack Sample on ESP-IDF

This sample demonstrates how to run **BLE and the Hubble Satellite Network** on an ESP32 using ESP-IDF.

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  for `dual-stack-companion.py` and [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html).
- ESP32-C6 or ESP32-S31 hardware.
- A computer with Bluetooth LE and internet access that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- The Hubble Connect app for
  [iOS](https://apps.apple.com/us/app/hubble-connect/id6751236191) or
  [Android](https://play.google.com/store/apps/details?id=com.hubble.connect)
  (optional), for the backend test.
- Satellite PHY Blob (requirement depends on SoC and ESP-IDF version - see
  [here](../../../docs/integration_guides/esp-idf/index.rst#fetch-the-satellite-phy-blob)).
- A software defined radio (SDR) for validation (suggested, but not required).

## Overview

This project is designed to:

- Demonstrate BLE and Hubble Satellite operation.
- Provide a practical starting point for developers integrating Hubble Satellite Network alongside BLE on ESP32 SoC.
- Show how to use a BLE GATT service for runtime device provisioning (time, location, and satellite ephemeris).

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/esp-idf/sat-dual-stack`

## Environment Setup

First, set up the environment. This step assumes you've installed esp-idf
to `~/esp/esp-idf`. If you haven't, follow the initial steps in the
[Installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#installation)
for your OS. Use the following command to export any necessary temporary environment variables for ESP-IDF to function.

### Windows

ESP-IDF does not support MSys, which is used by Git Bash. You must use PowerShell.

```ps1
& "C:\esp\<esp_idf_version>\esp-idf\export.ps1"
```

### Linux & macOS

```sh
source ~/esp/esp-idf/export.sh
```

### Python Dependencies

Then install the Python dependencies for the `dual-stack-companion.py`
provisioning script, and the [pyhubblenetwork](https://hubble.com/docs/guides/cli) tool for testing:

```sh
pip install -r ../../../tools/requirements-companion.txt pyhubblenetwork
```

### Satellite PHY Blob

If a satellite PHY blob is needed, based on your board and ESP-IDF version, follow
[these steps](../../../docs/integration_guides/esp-idf/index.rst#fetch-the-satellite-phy-blob) to install it.

### Set target chip

Set the target chip. This is needed once per build directory, and again after
editing `sdkconfig.defaults`.

```sh
idf.py set-target <chip>
```

e.g.
```sh
idf.py set-target esp32c6
```

## BLE Advertising Parameters

The sample advertises with the following parameters by default. To change one,
edit the macro or option in the *Set by* column and rebuild.

| Parameter | Value | Set by |
| --- | --- | --- |
| Beacon interval | 1000–1200 ms | `ADV_INTERVAL_MIN_MS` / `ADV_INTERVAL_MAX_MS` in [main/app_ble.c](main/app_ble.c) |
| Provisioning interval (connectable) | 100–150 ms | NimBLE GAP fast interval `BLE_GAP_ADV_FAST_INTERVAL2_MIN` / `BLE_GAP_ADV_FAST_INTERVAL2_MAX` in [main/app_ble.c](main/app_ble.c) |
| Tx power | 0 dBm | `CONFIG_BT_LE_DFT_TX_POWER_LEVEL_N0` in [sdkconfig.defaults](sdkconfig.defaults) |

## Configurations

This sample exposes the following options (via `idf.py menuconfig`, under
"Sat Dual Stack Sample Configuration"):

| Option | Default | Description |
| --- | --- | --- |
| `CONFIG_HUBBLE_DEVICE_KEY` | `""` | Hubble device cryptographic key, base64-encoded. |
| `CONFIG_HUBBLE_SAMPLE_DEBUG` | `n` | Debug mode: transmit 120 seconds after provisioning, then every 120 seconds, instead of at each predicted pass. |

## Build-time Provisioning

Set `CONFIG_HUBBLE_DEVICE_KEY` to your Hubble device key, in base64. The key
must be registered as **AES-256** with the **Unix time** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).
Set it in menuconfig:

```sh
idf.py menuconfig
```

or add the config `CONFIG_HUBBLE_DEVICE_KEY="your_b64_key"` to
`sdkconfig.defaults`.

> [!WARNING]
> `sdkconfig.defaults` is tracked by git, so a key added there is easy to commit
> by accident. `menuconfig` saves it to `sdkconfig` instead, which git ignores.

## Build and Flash

Build, flash, and open a serial logger. These steps can be done individually.

```sh
idf.py build flash monitor
```

If more than one board is connected, pick the port with `-p <PORT>`. Exit the
monitor with `Ctrl+]`.

## Runtime Provisioning

On boot, the device starts a connectable BLE advertisement named **"Hubble-ESP"**
and waits for provisioning data. Use `dual-stack-companion.py` to push the current Unix time,
device location, and orbital parameters for the target satellites.

See [companion tool documentation](../../../docs/satellite/companion-tool.rst)
for more information and instruction.

### Windows

```ps1
$env:HUBBLE_API_TOKEN = "<your-hubble-api-token>"

python ../../../tools/dual-stack-companion.py
```

### Linux & macOS

```sh
export HUBBLE_API_TOKEN=<your-hubble-api-token>

python ../../../tools/dual-stack-companion.py
```

### After Provisioning

A successful run ends with `Provisioned device successfully`.

> [!NOTE]
> The device keeps the time, location and orbital parameters in RAM. It does not persist.
> Run the provisioning script again after every power cycle or reset.

## Program Flow

Once the firmware is flashed and the device has been provisioned:

1. The device enters its main loop, computes the next satellite pass and starts
   a timer for it.
2. Until the pass, the device advertises as a BLE beacon.
3. At pass time, the timer fires. The device stops BLE advertising and
   transmits to the satellite.
4. The loop repeats for the next pass.

The beacon advertising payload refreshes periodically at 1 hour interval.

The diagram below shows the full application life-cycle:

```text
                power on / reset
                       |
                       v
       +-------------------------------+
       |  Initialize BLE               |
       |  (ble_init)                   |
       +-------------------------------+
                       |
                       v
       +-------------------------------------------+
       |  Connectable advertising "Hubble-ESP"     |
       |                                           |
       |  dual-stack-companion.py writes orbital   |
       |  params, location + time over GATT        |
       |  (re-advertises if the peer disconnects   |
       |  before the time is written)              |
       +-------------------------------------------+
                       |  time set, peer disconnected
                       v
       +-------------------------------+
       |  Initialize the SDK           |
       |  (hubble_init +               |
       |  hubble_sat_satellites_set)   |
       +-------------------------------+
                       |
                       v
      ==================  MAIN LOOP  ==================
                       |
                       v
       +-------------------------------+
       |  Compute next satellite pass  |<----------------+
       |  (hubble_sat_next_pass_get)   |                 |
       +-------------------------------+                 |
                       |                                 |
                       v                                 |
       +-------------------------------+                 |
       |  BLE beacon advertising       |                 |
       |  (payload refreshes hourly)   |                 |
       +-------------------------------+                 |
                       |                                 |
                       v                                 |
       +---------------------------------+               |
       |  Advertise BLE until next pass  |               |
       +---------------------------------+               |
                       |                                 |
                       v                                 |
       +-------------------------------+                 |
       |  Stop BLE advertising         |                 |
       +-------------------------------+                 |
                       |                                 |
                       v                                 |
       +-------------------------------+   next pass     |
       |  Satellite transmission       |-----------------+
       |  (hubble_sat_packet_send)     |
       +-------------------------------+
```

## Test

### Terrestrial (BLE) Network

#### Local BLE Scanning

The `hubblenetwork ble scan` tool can be used to test the BLE beacon.
It scans for a device advertising with the given key and attempts to
decode its payload.

Pass the same key that was provisioned into the device, as base64:

```sh
hubblenetwork ble scan --key "<your base64 device key>"
```

Correctly decoded packets will stream to the console.

#### Backend Test

Download the Hubble Connect app to your smartphone and sign in, then keep the
phone near the device with the app open. The phone acts as a scanner: the
device appears in the app, and its packets appear on
[Hubble Dash](https://dash.hubble.com/dashboard).

### Satellite Network

To check the satellite transmission on the bench without waiting for a pass,
set `CONFIG_HUBBLE_SAMPLE_DEBUG=y` in menuconfig (see [Configurations](#configurations)).
The device then transmits 120 seconds after provisioning, and every 120 seconds after that.

Capture the packets with an SDR and decode them with your device key. See
[RF Verification with an SDR](../../../docs/integration_guides/common/next-steps.rst#rf-verification-with-an-sdr).

```sh
hubblenetwork sat scan --key "<your base64 device key>"
```

Correctly decoded packets will stream to the console.

Turn `CONFIG_HUBBLE_SAMPLE_DEBUG` back off to transmit at each predicted pass.
