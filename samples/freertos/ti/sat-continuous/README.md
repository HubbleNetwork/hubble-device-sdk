# Hubble Satellite Network on TI (FreeRTOS)

This sample project demonstrates how to run the **Hubble Satellite Network** on a TI
device using FreeRTOS.

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  (optional), for testing with [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- The [TI SimpleLink Low Power F3 SDK](https://www.ti.com/tool/download/SIMPLELINK-LOWPOWER-F3-SDK).
- The [TI toolchain](https://www.ti.com/tool/CCSTUDIO).
- A TI development board: **LP_EM_CC2340R5** or **LP_EM_CC2755P10**. The
  satellite link needs [at least +20 dBm](../../../../docs/integration_guides/common/prerequisites.rst) of transmit power, so the
  CC2340R5 (max 8 dBm) needs an external amplifier.
- A computer that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- A software defined radio (SDR) for validation (suggested, but not required).

## Overview

This project is designed to:

- Demonstrate Hubble Satellite operation.
- Provide a practical starting point for developers integrating Hubble Satellite Network on TI hardware.

The project targets the **CC23xx** and **CC27xx** families and uses **FreeRTOS**
as the operating system. It is originally based on the TI
[`rfCarrierWave`](https://github.com/TexasInstruments/simplelink-prop_rf-examples/tree/main/examples/rtos/LP_EM_CC2340R5/prop_rf/rfCarrierWave)
example and has been extended to support Hubble Satellite transmission.

Once running, the device initializes the Hubble Device SDK and then enters a loop that
builds and transmits a satellite packet every `SLEEP_PERIOD_MS`.

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/freertos/ti/sat-continuous`

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

Install the [pyhubblenetwork](https://hubble.com/docs/guides/cli) tool for testing:

```sh
pip install pyhubblenetwork
```

## Build-time Provisioning

Create a file, `master.key`, and paste your Hubble device key into it. The key
must be registered as **AES-128** with the **Device uptime** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).

The `embed_key_time.py` script takes the key file and embeds it into the source
code (`src/key.c` and `src/time.c`). The build needs both files.

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

Flash the generated firmware (`build/sat-continuous.out`) onto the target
device using your preferred flashing tool (UniFlash, CCS, JLink, etc.).

## Test

Capture the packets with an SDR and decode them with your device key. See
[RF Verification with an SDR](../../../../docs/integration_guides/common/next-steps.rst#rf-verification-with-an-sdr).

```sh
hubblenetwork sat scan --key "<your base64 device key>"
```

Correctly decoded packets will stream to the console.
