# Hubble Network Satellite Sample on ESP-IDF

This sample application demonstrates how to use the Hubble Device SDK to
continuously transmit packets to satellite.

## Requirements

- Cryptographic key provided by Hubble Network
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
- ESP32-C6 hardware
- [ESP32-C6 Satellite PHY Blob](https://github.com/HubbleNetwork/hubble-device-sdk/blob/main/docs/integration_guides/esp-idf/index.rst#fetch-the-satellite-phy-blob-esp32-c6)
  <!-- TODO: Remove this requirement once Espressif ships the API upstream. -->

## Overview

This project is designed to:

- Demonstrate the use of the ESP-IDF together with the Hubble Network SDK for
  satellite-enabled applications.
- Provide a practical starting point for developers integrating the Hubble
  Satellite Network on ESP32 hardware.

Once running, the device initializes the Hubble Device SDK and then enters a loop that
builds and transmits a satellite packet with normal reliability.

> [!NOTE]
> The sample uses the **device uptime** counter source
> (`CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME`), so it does not need UTC time
> provisioned: the EID counter used for encryption starts at 0 and advances with
> device uptime. Only the master key has to be embedded before building.

> [!WARNING]
> Make sure that your generated secret key from [Hubble API](https://hubble.com/docs/api-specification/register-new-devices)
> is using the same key size (AES-128 or AES-256) and counter source (DEVICE_UPTIME) as the sample.

> [!NOTE]
> All commands should be ran from the sample folder containing this file:
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

## Pre-build

Set the target chip.

```sh
idf.py set-target <chip>
```

e.g.
```sh
idf.py set-target esp32c6
```

## Provisioning

This sample exposes the following options (via `idf.py menuconfig`, under
"Sat Continuous Sample Configuration"):

| Option                     | Default | Description                                              |
| -------------------------- | ------- | -------------------------------------------------------- |
| `CONFIG_HUBBLE_DEVICE_KEY` | `""`    | Hubble device cryptographic key, base64-encoded.         |

Configure the device key:

```sh
idf.py menuconfig
```

or add the config `CONFIG_HUBBLE_DEVICE_KEY="your_b64_key"` to
`sdkconfig.defaults`.

## Build

```sh
idf.py build flash monitor
```

After flashing, the device continuously transmits a packet to satellite every
SAT_TX_SLEEP_MS.
