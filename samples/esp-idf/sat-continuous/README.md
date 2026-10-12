# Hubble Network Satellite Sample on ESP-IDF

This sample application demonstrates how to use the Hubble Device SDK to
continuously transmit packets to satellite.

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  (optional), for testing with [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html).
- ESP32-C6 or ESP32-S31 hardware.
- A computer that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- Satellite PHY Blob (requirement depends on SoC and ESP-IDF version - see
  [here](../../../docs/integration_guides/esp-idf/index.rst#fetch-the-satellite-phy-blob)).
- A software defined radio (SDR) for validation (suggested, but not required).

## Overview

This project is designed to:

- Demonstrate the use of the ESP-IDF together with the Hubble Network SDK for
  satellite-enabled applications.
- Provide a practical starting point for developers integrating the Hubble
  Satellite Network on ESP32 hardware.

Once running, the device initializes the Hubble Device SDK and then enters a loop that
builds and transmits a satellite packet every SAT_TX_SLEEP_MS milliseconds.

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/esp-idf/sat-continuous`

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

Install the [pyhubblenetwork](https://hubble.com/docs/guides/cli) tool for testing:

```sh
pip install pyhubblenetwork
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

## Configurations

This sample exposes the following options (via `idf.py menuconfig`, under
"Sat Continuous Sample Configuration"):

| Option | Default | Description |
| --- | --- | --- |
| `CONFIG_HUBBLE_DEVICE_KEY` | `""` | Hubble device cryptographic key, base64-encoded. |

## Build-time Provisioning

Set `CONFIG_HUBBLE_DEVICE_KEY` to your Hubble device key, in base64. The key
must be registered as **AES-256** with the **Device uptime** counter source; see
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

## Test

Capture the packets with an SDR and decode them with your device key. See
[RF Verification with an SDR](../../../docs/integration_guides/common/next-steps.rst#rf-verification-with-an-sdr).

```sh
hubblenetwork sat scan --key "<your base64 device key>"
```

Correctly decoded packets will stream to the console.
