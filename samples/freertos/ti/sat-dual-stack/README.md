# Hubble Satellite Dual-Stack on TI (FreeRTOS)

This sample demonstrates how to run **BLE and the Hubble Satellite Network** on a TI
device using FreeRTOS.

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  for `dual-stack-companion.py` and [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- The [TI SimpleLink Low Power F3 SDK](https://www.ti.com/tool/download/SIMPLELINK-LOWPOWER-F3-SDK) (`9.20.00.81` or newer).
- The [TI ARM-CLANG toolchain](https://www.ti.com/tool/CCSTUDIO).
- A TI development board: **LP_EM_CC2340R5** or **LP_EM_CC2755P10**. The
  satellite link needs [at least +20 dBm](../../../../docs/integration_guides/common/prerequisites.rst) of transmit power, so the
  CC2340R5 (max 8 dBm) needs an external amplifier.
- A computer with Bluetooth LE and internet access that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- The Hubble Connect app for
  [iOS](https://apps.apple.com/us/app/hubble-connect/id6751236191) or
  [Android](https://play.google.com/store/apps/details?id=com.hubble.connect)
  (optional), for the backend test.
- A software defined radio (SDR) for validation (suggested, but not required).

## Overview

This project is designed to:

- Demonstrate BLE and Hubble Satellite operation.
- Provide a practical starting point for developers integrating Hubble Satellite Network alongside BLE on TI hardware.
- Show how to use a BLE GATT service for runtime device provisioning (time, location, and satellite ephemeris).

The project targets the **CC23xx** and **CC27xx** families and uses **FreeRTOS**. It is originally based on the TI
[`mac_sensor_ble_basic`](https://dev.ti.com/tirex/explore/node?isTheia=false&node=A__ABIptMuvmFu84E8hivwcdQ__com.ti.SIMPLELINK_LOWPOWER_F3_SDK__58mgN04__LATEST)
example and has been extended to support Hubble Satellite transmission and a
custom GATT provisioning service.

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/freertos/ti/sat-dual-stack`

## Environment Setup

First, set up the environment. This step assumes you've installed the TI SDK,
the TI toolchain and SysConfig. Set the `TICLANG_ARMCOMPILER`,
`SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR` and `SYSCONFIG_TOOL` environment variables.

### Windows

> [!WARNING]
> For any shell, write the paths with **forward slashes** (e.g. `C:/ti/...`, not `C:\ti\...`).
> Windows GNU Make strips backslashes out of the commands.

PowerShell:

```ps1
$env:TICLANG_ARMCOMPILER = "/path/to/ti/ti-cgt-armllvm"
$env:SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR = "/path/to/ti/simplelink_lowpower_f3_sdk"
$env:SYSCONFIG_TOOL = "/path/to/ti/sysconfig/sysconfig_cli.bat"
```

Command Prompt:

```bat
set TICLANG_ARMCOMPILER=/path/to/ti/ti-cgt-armllvm
set SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR=/path/to/ti/simplelink_lowpower_f3_sdk
set SYSCONFIG_TOOL=/path/to/ti/sysconfig/sysconfig_cli.bat
```

If using Bash, the Linux & macOS commands work.

> [!NOTE]
> If not using Bash, you may need to include Unix tools on your `PATH`:
> `C:\Program Files\Git\usr\bin`. It is not added by default when installing
> Git. A Bash session will inherit this automatically.

### Linux & macOS

```sh
export TICLANG_ARMCOMPILER=/path/to/ti/ti-cgt-armllvm
export SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR=/path/to/ti/simplelink_lowpower_f3_sdk
export SYSCONFIG_TOOL=/path/to/ti/sysconfig/sysconfig_cli.sh
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
| Provisioning interval (connectable) | 100–150 ms | `CONN_ADV_INTERVAL_MIN_MS` / `CONN_ADV_INTERVAL_MAX_MS` in [src/app_ble.c](src/app_ble.c) |
| Tx power | 0 dBm | `ADV_TX_POWER_DBM` in [src/app_ble.c](src/app_ble.c) |

## Configurations

This sample exposes the following options as `make` variables:

| Option | Default | Description |
| --- | --- | --- |
| `DEBUG` | `0` | Debug mode (`1`): transmit 120 seconds after provisioning, then every 120 seconds, instead of at each predicted pass. |

## Build-time Provisioning

Create a file, `master.key`, and paste your Hubble device key into it. The key
must be registered as **AES-128** with the **Unix time** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).

The `embed_key_time.py` script takes the key file and embeds it into the source
code (`src/key.c`). It also writes `src/time.c`, which this sample doesn't use:
the time comes from the provisioning script.

**For a raw key file:**

```sh
python ../../../../tools/embed_key_time.py master.key -o src/
```

**For a base64-encoded key file:**

Use the `-b` or `--base64` flag:

```sh
python ../../../../tools/embed_key_time.py -b master.key -o src/
```

After running the script, the key will be compiled into the application.

## Build and Flash

Pick the makefile for your target board:

```sh
# For LP_EM_CC2340R5
make -f cc2340r5.mk

# For LP_EM_CC2755P10
make -f cc2755p10.mk
```

Flash the generated firmware (`build/sat-dual-stack.out`) onto the target
device using your preferred flashing tool (UniFlash, CCS, JLink, etc.).

View the log using TI `tiutils`. See setup instruction at
`<TI_SDK_INSTALL_DIR>/tools/log/tiutils/README.md`. For example:

```sh
tilogger --elf ./build/sat-dual-stack.out uart /dev/tty.usbmodemLS470FPO1 3000000 stdout
```

## Runtime Provisioning

On boot, the device starts a connectable BLE advertisement named **"Hubble-TI"**
and waits for provisioning data. Use `dual-stack-companion.py` to push the current Unix time,
device location, and orbital parameters for the target satellites.

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
       |  Connectable advertising "Hubble-TI"      |
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
build with `DEBUG=1` (see [Configurations](#configurations)). Clean first, so
`make` rebuilds with the new flag:

```sh
make -f cc2755p10.mk clean
make -f cc2755p10.mk DEBUG=1
```

The device then transmits 120 seconds after provisioning, and every 120 seconds after that.

Capture the packets with an SDR and decode them with your device key. See
[RF Verification with an SDR](../../../../docs/integration_guides/common/next-steps.rst#rf-verification-with-an-sdr).

```sh
hubblenetwork sat scan --key "<your base64 device key>"
```

Correctly decoded packets will stream to the console.

Clean and rebuild without `DEBUG=1` to transmit at each predicted pass.
