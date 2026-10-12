# Hubble Satellite Dual-Stack on Zephyr

This sample demonstrates running the **Hubble Terrestrial (BLE) Network** and the
**Hubble Satellite Network** together on a single Zephyr device.

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  for `dual-stack-companion.py` and [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- A [Zephyr west workspace](../../../docs/quickstart/zephyr/index.rst) that includes the Hubble Device SDK.
- A board with Bluetooth LE support. Satellite-capable boards transmit for real; other boards mock the satellite radio (see `CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT`). The satellite link needs [at least +20 dBm](../../../docs/integration_guides/common/prerequisites.rst) of transmit power, so some devices may need a power amplifier.
- A computer with Bluetooth LE and internet access that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- The Hubble Connect app for
  [iOS](https://apps.apple.com/us/app/hubble-connect/id6751236191) or
  [Android](https://play.google.com/store/apps/details?id=com.hubble.connect)
  (optional), for the backend test.
- [Satellite PHY Blob](#satellite-phy-blob).
- A software defined radio (SDR) for validation (suggested, but not required).

## Overview

The application:

1. Provisions the device over a BLE GATT service (Unix time, satellite
   orbital parameters and device location).
2. Schedules a timer for the next satellite pass using the SDK's pass
   prediction APIs.
3. Advertises Hubble beacon packets over Bluetooth while it waits.
4. When the timer expires, stops advertising and transmits to the satellite.

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/zephyr/sat-dual-stack`

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

Then install the Python dependencies for the `dual-stack-companion.py`
provisioning script, and the [pyhubblenetwork](https://hubble.com/docs/guides/cli) tool for testing:

```sh
pip install -r ../../../tools/requirements-companion.txt pyhubblenetwork
```

### Satellite PHY Blob

The satellite radio libraries are **not** fetched by `west update`. Fetch them
once per workspace:

```sh
west blobs fetch hubblenetwork-sdk
```

Silicon Labs boards use RAIL, which needs to be fetched along with the
Silicon Labs HAL:

```sh
west blobs fetch hal_silabs
```

## BLE Advertising Parameters

The sample advertises with the following parameters by default. To change one,
edit the macro or option in the *Set by* column and rebuild.

| Parameter | Value | Set by |
| --- | --- | --- |
| Beacon interval | 1000–1200 ms | `ADV_INTERVAL_MIN_MS` / `ADV_INTERVAL_MAX_MS` in [src/app_ble.c](src/app_ble.c) |
| Provisioning interval (connectable) | 100–150 ms | GAP fast interval `BT_GAP_ADV_FAST_INT_MIN_2` / `BT_GAP_ADV_FAST_INT_MAX_2` in [src/app_ble.c](src/app_ble.c) |
| Tx power | 0 dBm | `CONFIG_BT_CTLR_TX_PWR_0` in [prj.conf](prj.conf) |

## Configurations

This sample exposes the following options (via `west build -t menuconfig`, under
"Hubble Network Dual Stack Sample options"):

| Option | Default | Description |
| --- | --- | --- |
| `CONFIG_HUBBLE_DEVICE_KEY` | `""` | Hubble device cryptographic key, base64-encoded. |
| `CONFIG_HUBBLE_SAMPLE_DEBUG` | `n` | Debug mode: transmit 120 seconds after provisioning, then every 120 seconds, instead of at each predicted pass. |
| `CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT` | `n` | Provide mock satellite board APIs. Enable on boards that do not implement the real satellite radio. |

## Build-time Provisioning

Set `CONFIG_HUBBLE_DEVICE_KEY` to your Hubble device key, in base64. The key
must be registered as **AES-256** with the **Unix time** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).
Add this config in your build command below.

## Build and Flash

Build for your board, passing your device key. Add `-p` for a pristine build.

```sh
west build -b <board> -- -DCONFIG_HUBBLE_DEVICE_KEY=\"<your base64 device key>\"
```

e.g.
```sh
west build -b nrf54l15dk/nrf54l15/cpuapp -- -DCONFIG_HUBBLE_DEVICE_KEY=\"<your base64 device key>\"
```

Flash the board:

```sh
west flash
```

> [!NOTE]
> When building with nRF Connect SDK (NCS) and the SoftDevice Bluetooth
> controller, apply the `ncs-dual-stack` snippet provided by the SDK:
>
> ```sh
> west build -b nrf54l15dk/nrf54l15/cpuapp -S ncs-dual-stack \
>     -- -DCONFIG_HUBBLE_DEVICE_KEY=\"<your base64 device key>\"
> ```
>
> The snippet enables MPSL with a timeslot session so the satellite radio can
> coexist with the SoftDevice Bluetooth controller. It lives in
> `port/zephyr/snippets/ncs-dualstack` and is discoverable on any board, so the
> same invocation works for other NCS targets.

## Runtime Provisioning

On boot, the device starts a connectable BLE advertisement named **"Hubble-Zephyr"**
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
       |  Connectable advertising "Hubble-Zephyr"  |
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
set `CONFIG_HUBBLE_SAMPLE_DEBUG=y` (see [Configurations](#configurations)), for
example by adding `-DCONFIG_HUBBLE_SAMPLE_DEBUG=y` to the build command.
The device then transmits 120 seconds after provisioning, and every 120 seconds after that.

Capture the packets with an SDR and decode them with your device key. See
[RF Verification with an SDR](../../../docs/integration_guides/common/next-steps.rst#rf-verification-with-an-sdr).

```sh
hubblenetwork sat scan --key "<your base64 device key>"
```

Correctly decoded packets will stream to the console.

Turn `CONFIG_HUBBLE_SAMPLE_DEBUG` back off to transmit at each predicted pass.
