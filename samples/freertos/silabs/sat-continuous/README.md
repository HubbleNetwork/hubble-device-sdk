# Hubble Network Satellite Continuous Sample on SiLabs (FreeRTOS)

This sample demonstrates how to continuously transmit Hubble satellite packets
on a Silicon Labs EFR32 device using FreeRTOS.

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  (optional), for testing with [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- Simplicity SDK, SLC CLI, the GCC toolchain and Simplicity Commander, see
  [Environment Setup](#environment-setup).
- A Silicon Labs radio board. The satellite link needs
  [at least +20 dBm](../../../../docs/integration_guides/common/prerequisites.rst)
  of transmit power. This sample has been tested on:
  - BRD4187C (EFR32MG24)
  - BRD4121A (EFR32MG26)
- A computer that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- A software defined radio (SDR) for validation (suggested, but not required).

## Overview

The project targets the **EFR32xG24** and **EFR32xG26** families. The satellite
PHY is provided by the Hubble Device SDK extension as a radio configuration, and
the RAIL code is generated from it when the project is generated.

Once running, the device initializes the Hubble Device SDK and then enters a
loop that builds and transmits a satellite packet every `SAT_TX_SLEEP_MS`
(10 seconds), logging to the virtual COM port at 115200 baud.

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/freertos/silabs/sat-continuous`

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

Install the [pyhubblenetwork](https://hubble.com/docs/guides/cli) tool for testing:

```sh
pip install pyhubblenetwork
```

## Configurations

This sample exposes the following option, in
[config/sat_cont_config.h](config/sat_cont_config.h). See
[Build-time Provisioning](#build-time-provisioning) for how to set it.

| Option | Default | Description |
| --- | --- | --- |
| `HUBBLE_DEVICE_KEY` | `""` | Hubble device cryptographic key, base64-encoded. |

> [!NOTE]
> The nonce-reuse check (`SISDK_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK`)
> is disabled in `sat_continuous.slcp`, since this sample transmits
> continuously, only applicable for testing purpose. This should be turned
> on for production.

## Build-time Provisioning

The device's Hubble key is embedded into the firmware at project generation
time, through the `HUBBLE_DEVICE_KEY` option. Pass it to `slc generate` with
`--configuration=HUBBLE_DEVICE_KEY:\"<your-base64-key>\"` in the build command
below, or set it in the Simplicity Studio configurator of the generated
project's `config/sat_cont_config.h`. The key must be registered as **AES-256**
with the **Device uptime** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).

## Build and Flash

### Command-line

Generate a Makefile project and build it. Replace `brd4187c` with your
target board (e.g. `brd4121a`):

```sh
$SLC generate --sdk $SISDK -tlcn gcc -o makefile -cp -p sat_continuous.slcp \
    -d generated_sample_app --with brd4187c \
    --configuration=HUBBLE_DEVICE_KEY:\"<your-base64-key>\"

cd generated_sample_app

# replace 8 with the number of parallel jobs you want
make -f sat_continuous.Makefile -j8
```

Flash the generated firmware onto the target device:

```sh
$COMMANDER flash build/debug/sat_continuous.s37
```

### Simplicity Studio

To use Simplicity Studio instead, generate the sample into your workspace,
then go to **PROJECTS > Open Project(s)**, then build and flash as usual:

```sh
$SLC generate --sdk $SISDK -tlcn gcc -np -p sat_continuous.slcp \
    -d /location/to/your/simplicity-studio/workspace/sat-continuous --with brd4187c \
    --configuration=HUBBLE_DEVICE_KEY:\"<your-base64-key>\"
```

### After Flashing

The device will continuously transmit satellite packets, and log to the
virtual COM port at 115200 baud.

## Test

Capture the packets with an SDR and decode them with your device key. See
[RF Verification with an SDR](../../../../docs/integration_guides/common/next-steps.rst#rf-verification-with-an-sdr).

```sh
hubblenetwork sat scan --key "<your base64 device key>"
```

Correctly decoded packets will stream to the console.
