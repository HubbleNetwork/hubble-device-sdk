# Hubble Network Satellite Dual-Stack Sample on SiLabs (FreeRTOS)

This sample demonstrates running the **Hubble Terrestrial (BLE) Network** and the
**Hubble Satellite Network** together on a single Silicon Labs EFR32 device using
FreeRTOS.

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  for `dual-stack-companion.py` and [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- Simplicity SDK, SLC CLI, the GCC toolchain and Simplicity Commander, see
  [Environment Setup](#environment-setup).
- A satellite-capable Silicon Labs board with Bluetooth LE support. The satellite
  link needs [at least +20 dBm](../../../../docs/integration_guides/common/prerequisites.rst)
  of transmit power. This sample has been tested on:
  - BRD4187C (EFR32MG24)
  - BRD4121A (EFR32MG26)
- A computer with Bluetooth LE and internet access that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- The Hubble Connect app for
  [iOS](https://apps.apple.com/us/app/hubble-connect/id6751236191) or
  [Android](https://play.google.com/store/apps/details?id=com.hubble.connect)
  (optional), for the backend test.
- A software defined radio (SDR) for validation (suggested, but not required).

## Overview

The application:

1. Provisions the device over a BLE GATT service (Unix epoch time, satellite
   orbital parameters and device location).
2. Schedules a timer for the next satellite pass using the SDK's pass
   prediction APIs.
3. Advertises Hubble beacon packets over Bluetooth while it waits.
4. When the timer expires, stops advertising and transmits to the satellite.

The provisioning GATT service is defined in
[config/btconf/gatt_configuration.btconf](config/btconf/gatt_configuration.btconf).

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/freertos/silabs/sat-dual-stack`

## Environment Setup

Install the [Silicon Labs Tool (SLT)](https://docs.silabs.com/simplicity-installer-slt/latest/slt-cli/install-slt),
then use it to install the Simplicity SDK and the tools this sample needs:

```sh
export SLT=/path/to/slt

$SLT install simplicity-sdk slc-cli gcc-arm-none-eabi commander
```

SLT does not add anything to `PATH`, so export the paths used throughout this
document.

> [!TIP]
> `$SLT where <package>` prints where a package was installed. See
> [Useful paths](https://docs.silabs.com/ssv6ug/6.2.0/ssv6-tricks-tips/#useful-paths)
> for the default install locations on each OS.

```sh
export HUBBLE_SDK=/path/to/hubble-device-sdk
export SISDK=/path/to/simplicity_sdk
export SLC=/path/to/slc_cli/slc
export COMMANDER=/path/to/commander
```

| Variable | What it points at |
| --- | --- |
| `SLT` | The `slt` executable |
| `HUBBLE_SDK` | The root of this repository |
| `SISDK` | The Simplicity SDK |
| `SLC` | The `slc` executable, `$($SLT where slc-cli)/slc` |
| `COMMANDER` | The `commander` executable. On macOS, `$($SLT where commander)/Contents/MacOS/commander` |

### Installing the SDK Extension

The Hubble Device SDK is consumed as a Simplicity SDK extension. SLC discovers
extensions under the SDK's `extension` directory, so link this repository there
and register it as trusted:

```sh
mkdir -p $SISDK/extension
ln -s $HUBBLE_SDK $SISDK/extension/hubble-device-sdk

$SLC configuration --sdk $SISDK
$SLC signature trust --sdk $SISDK
$SLC signature trust -extpath $SISDK/extension/hubble-device-sdk
```

### Python Dependencies

Then install the Python dependencies for the `dual-stack-companion.py`
provisioning script, and the [pyhubblenetwork](https://hubble.com/docs/guides/cli) tool for testing:

```sh
pip install -r ../../../../tools/requirements-companion.txt pyhubblenetwork
```

## BLE Advertising Parameters

The sample advertises with the following parameters by default. To change one,
edit the macro or option in the *Set by* column and rebuild.

| Parameter | Value | Set by |
| --- | --- | --- |
| Beacon interval | 1000–1200 ms | `ADV_INTERVAL_MIN_MS` / `ADV_INTERVAL_MAX_MS` in [src/app_ble.c](src/app_ble.c) |
| Provisioning interval (connectable) | 100–200 ms | Bluetooth stack default, see `sl_bt_advertiser_set_timing` |
| Tx power (beacon) | 0 dBm | `HUBBLE_ADV_TX_POWER_DDBM` in [src/app_ble.c](src/app_ble.c) |

## Configurations

This sample exposes the following options, in
[config/sat_dual_stack_config.h](config/sat_dual_stack_config.h):

| Option | Default | Description |
| --- | --- | --- |
| `HUBBLE_DEVICE_KEY` | `""` | Hubble device cryptographic key, base64-encoded. |
| `HUBBLE_SAMPLE_DEBUG` | `0` | Transmit to the satellite every 120 s instead of waiting for the next pass. |

> [!TIP]
> In Simplicity Studio, these options can be set in the configurator of the
> generated project's `config/sat_dual_stack_config.h`.

## Build-time Provisioning

The device's Hubble key is embedded into the firmware at project generation
time, through the `HUBBLE_DEVICE_KEY` option. Pass it to `slc generate` with
`--configuration=HUBBLE_DEVICE_KEY:\"<your-base64-key>\"` in the build command
below, or set it in the Simplicity Studio configurator of the generated
project's `config/sat_dual_stack_config.h`. The key must be registered as
**AES-256** with the **Unix time** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).

## Build and Flash

Replace `brd4187c` below with your target board. The device logs to the
virtual COM port at 115200 baud.

### Command-line

```sh
$SLC generate --sdk $SISDK -tlcn gcc -o makefile -cp -p sat_dual_stack.slcp \
    -d generated_sample_app --with brd4187c \
    --configuration=HUBBLE_DEVICE_KEY:\"<your-base64-key>\"

cd generated_sample_app

# replace 8 with the number of parallel jobs you want
make -f sat_dual_stack.Makefile -j8

$COMMANDER flash build/debug/sat_dual_stack.s37
```

### Simplicity Studio

Generate the project into your workspace, then go to
**PROJECTS > Open Project(s)**:

```sh
$SLC generate --sdk $SISDK -tlcn gcc -np -p sat_dual_stack.slcp \
    -d /location/to/your/simplicity-studio/workspace/sat-dual-stack --with brd4187c
```

In the generated project:

- **Sample options:** open `config/sat_dual_stack_config.h` in the configurator.
- **GATT database:** open `config/btconf/gatt_configuration.btconf` in the
  Bluetooth GATT Configurator (e.g. **Device Name** under **Generic Access**).
- Build and flash: see SiLabs
  [Build, Flash, or Debug a Project](https://docs.silabs.com/ss-vscode/latest/ss-vscode-projects/build-flash-debug-project#build-flash-or-debug-a-project).

## Runtime Provisioning

On first boot, the device starts a connectable BLE advertisement named **"Hubble-SiLabs"**
and waits for provisioning data. Use `dual-stack-companion.py` to push the current Unix Epoch time,
device location, and orbital parameters data for the target satellites.

See [companion tool documentation](../../../../docs/satellite/companion-tool.rst)
for more information and instruction.

### Windows

```ps1
$env:HUBBLE_API_TOKEN = "<your-hubble-api-token>"

python ../../../../tools/dual-stack-companion.py
```

### Linux & macOS

```sh
export HUBBLE_API_TOKEN=<your-hubble-api-token>

python ../../../../tools/dual-stack-companion.py
```

### After Provisioning

A successful run ends with `Provisioned device successfully`. The device then
automatically transitions into its satellite-pass scheduling loop and starts
advertising the Hubble beacon.

> [!NOTE]
> The device keeps the time, location and orbital parameters in RAM. It does not persist.
> Run the provisioning script again after every power cycle or reset.

## Program Flow

Once the firmware is flashed and the device has been provisioned:

1. The device enters its main loop, alternating between BLE beacon
   advertising and satellite transmission windows.
2. At pass time, the device wakes from sleep and prepares to transmit.
3. After the pass, the device returns to BLE beacon mode until the next pass.

The beacon advertising payload refreshes periodically at 1 hour interval.

The diagram below shows the full application life-cycle:

```text
                power on / reset
                       |
                       v
       +-------------------------------+
       |  Enable BLE                   |
       |  Bluetooth stack boots,       |
       |  advertising sets created     |
       +-------------------------------+
                       |
                       v
       +-------------------------------+
       |  Connectable advertising      |<----------------+
       |  "Hubble-SiLabs"              |                 |
       +-------------------------------+                 |
                       |                                 |
                       v                                 |
       +-------------------------------+                 |
       |  dual-stack-companion.py      |                 |
       |  writes time, location +      |                 |
       |  orbital params over GATT     |                 |
       +-------------------------------+                 |
                       |                                 |
                       v                                 |
       +-------------------------------+   time not set  |
       |  Peer disconnects             |-----------------+
       +-------------------------------+
                       | time set
                       v
       +-------------------------------+
       |  hubble_init +                |
       |  hubble_sat_satellites_set    |
       +-------------------------------+
                       |
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
       +-------------------------------+                 |
       |  Wait until pass time (timer) |                 |
       +-------------------------------+                 |
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
set `HUBBLE_SAMPLE_DEBUG` to `1` (see [Configurations](#configurations)), for
example by adding `--configuration=HUBBLE_SAMPLE_DEBUG:1` to the `slc generate`
command. The device then transmits 120 seconds after provisioning, and every
120 seconds after that.

Capture the packets with an SDR and decode them with your device key. See
[RF Verification with an SDR](../../../../docs/integration_guides/common/next-steps.rst#rf-verification-with-an-sdr).

```sh
hubblenetwork sat scan --key "<your base64 device key>"
```

Correctly decoded packets will stream to the console.

Set `HUBBLE_SAMPLE_DEBUG` back to `0` to transmit at each predicted pass.
