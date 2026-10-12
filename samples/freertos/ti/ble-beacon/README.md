# Hubble Network BLE Beacon on TI (FreeRTOS)

This sample demonstrates how to run **Hubble Terrestrial Network** on a TI
device using FreeRTOS.

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  (optional), for testing with [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- The [TI SimpleLink Low Power F3 SDK](https://www.ti.com/tool/download/SIMPLELINK-LOWPOWER-F3-SDK).
- The [TI toolchain](https://www.ti.com/tool/CCSTUDIO).
- A TI CC23xx development board (e.g., **LP_EM_CC2340R5**).
- A computer with Bluetooth LE that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- The Hubble Connect app for
  [iOS](https://apps.apple.com/us/app/hubble-connect/id6751236191) or
  [Android](https://play.google.com/store/apps/details?id=com.hubble.connect)
  (optional), for the backend test.

## Overview

This project is designed to:

- Demonstrate the use of the TI SDK for BLE application development.
- Showcase the integration of the Hubble Device SDK for BLE-specific operations.
- Provide a starting point for developers working on BLE applications with TI devices.

The project targets the *CC23xx* family of devices and uses *FreeRTOS*
as the operating system. It is originally a copy of
[basic-ble](https://github.com/TexasInstruments/simplelink-ble5stack-examples/tree/lpf2-lpf3-7.41.00.17-8.20.00.119-ble5stack/examples/rtos/LP_EM_CC2340R5/ble5stack/basic_ble)
sample.

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/freertos/ti/ble-beacon`

## Environment Setup

First, set up the environment. This step assumes you've installed the TI SDK,
the TI toolchain and SysConfig. Set the `TICLANG_ARMCOMPILER`,
`SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR` and `SYSCONFIG_TOOL` environment variables.

When using the SysConfig-based build (`makefile-syscfg`), also set `HUBBLE_NETWORK_SDK`
to the root of this SDK. It defaults to four directories above the sample if unset.

### Windows

> [!WARNING]
> For any shell, write the paths with **forward slashes** (e.g. `C:/ti/...`, not `C:\ti\...`).
> Windows GNU Make strips backslashes out of the commands.

PowerShell:

```ps1
$env:TICLANG_ARMCOMPILER = "/path/to/ti/ti-cgt-armllvm"
$env:SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR = "/path/to/ti/simplelink_lowpower_f3_sdk"
$env:SYSCONFIG_TOOL = "/path/to/ti/sysconfig/sysconfig_cli.bat"
$env:HUBBLE_NETWORK_SDK = "/path/to/hubble-device-sdk"
```

Command Prompt:

```bat
set TICLANG_ARMCOMPILER=/path/to/ti/ti-cgt-armllvm
set SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR=/path/to/ti/simplelink_lowpower_f3_sdk
set SYSCONFIG_TOOL=/path/to/ti/sysconfig/sysconfig_cli.bat
set HUBBLE_NETWORK_SDK=/path/to/hubble-device-sdk
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
export HUBBLE_NETWORK_SDK=/path/to/hubble-device-sdk
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
| Beacon interval | 1000–1200 ms | `ADV_INTERVAL_MIN_MS` / `ADV_INTERVAL_MAX_MS` in [src/hubble_ble_adv.c](src/hubble_ble_adv.c) |
| Tx power | 0 dBm | `ADV_TX_POWER_DBM` in [src/hubble_ble_adv.c](src/hubble_ble_adv.c) |

## Build-time Provisioning

Create a file, `master.key`, and paste your Hubble device key into it. The key
must be registered as **AES-128** with the **Unix time** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).

The `embed_key_time.py` script takes the key file and embeds it along with the
current timestamp into the source code (`src/key.c` and `src/time.c`).

**For a raw key file:**

```sh
python ../../../../tools/embed_key_time.py master.key -o src/
```

**For a base64-encoded key file:**

Use the `-b` or `--base64` flag:

```sh
python ../../../../tools/embed_key_time.py -b master.key -o src/
```

After running the script, the key and timestamp will be compiled into the application.

## Build and Flash

Build the project using the provided `makefile`:

```sh
make
```

Alternatively, use the SysConfig-based build (`makefile-syscfg`), which generates driver and
peripheral configuration via SysConfig using both the TI SDK and Hubble Device SDK products:

```sh
make -f makefile-syscfg
```

Flash the generated firmware (`build/ble-beacon.out`) onto the target device using your
preferred flashing tool.

View the log using TI `tiutils`. See setup instruction at
`<TI_SDK_INSTALL_DIR>/tools/log/tiutils/README.md`. For example:

```sh
tilogger --elf ./build/ble-beacon.out uart /dev/tty.usbmodemLS470FPO1 3000000 stdout
```

## Test

### Local BLE Scanning

The `hubblenetwork ble scan` tool can be used to test the BLE beacon.
It scans for a device advertising with the given key and attempts to
decode its payload.

Pass the same key that was provisioned into the device, as base64:

```sh
hubblenetwork ble scan --key "<your base64 device key>"
```

Correctly decoded packets will stream to the console.

### Backend Test

Download the Hubble Connect app to your smartphone and sign in, then keep the
phone near the device with the app open. The phone acts as a scanner: the
device appears in the app, and its packets appear on
[Hubble Dash](https://dash.hubble.com/dashboard).
