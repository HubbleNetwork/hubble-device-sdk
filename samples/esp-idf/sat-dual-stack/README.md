# Hubble Network Satellite Dual-Stack Sample on ESP-IDF

This sample demonstrates how to run **BLE and the Hubble Satellite Network** on an ESP32 using ESP-IDF.

## Requirements

- Cryptographic key provided by Hubble Network
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
- ESP32-C6 hardware
- [ESP32-C6 Satellite PHY Blob](https://github.com/HubbleNetwork/hubble-device-sdk/blob/main/docs/integration_guides/esp-idf/index.rst#fetch-the-satellite-phy-blob-esp32-c6)
  <!-- TODO: Remove this requirement once Espressif ships the API upstream. -->

## Overview

This project is designed to:

- Demonstrate BLE and Hubble Satellite operation.
- Provide a practical starting point for developers integrating Hubble Satellite Network alongside BLE on ESP32 SoC.
- Show how to use a BLE GATT service for runtime device provisioning (time and satellite ephemeris).

> [!NOTE]
> All commands should be ran from the sample folder containing this file:
> `<SDK path>/samples/esp-idf/sat-dual-stack`

## Features

- Dual-stack application running Hubble Terrestrial (BLE) Network and the Hubble Satellite Network.
- GATT provisioning service that provides time and satellite orbital parameters over BLE.

## Environment Setup

First, set up the environment. This step assumes you've installed esp-idf
to `~/esp/esp-idf`. If you haven't, follow the initial steps in the
[Installation guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#installation)
for your OS. Use the following command to export any necessary temporary environment variables for ESP-IDF to function.

Install Python dependencies for the *dual-stack-companion.py* provisioning script:

```sh
pip install -r ../../../../tools/requirements-companion.txt
```

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
"Sat Dual Stack Sample Configuration"):

| Option                       | Default | Description                                                                          |
| ----------------------------- | ------- | ------------------------------------------------------------------------------------ |
| `CONFIG_HUBBLE_DEVICE_KEY`   | `""`    | Hubble device cryptographic key, base64-encoded.                                     |
| `CONFIG_HUBBLE_SAMPLE_DEBUG` | `n`     | Enable debug mode, schedule sat tx in 2 minutes instead of waiting for the next pass |

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

### 3. Provision the Device

On first boot, the device starts a connectable BLE advertisement named **"Hubble-ESP"**
and waits for provisioning data. Use *dual-stack-companion.py* to push the current Unix Epoch time,
device location, and orbital parameters data for the target satellites:

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

By default the device location is determined via IP geolocation. To provision an
explicit location, pass the latitude and longitude (in degrees) with `--location`:

```sh
python ../../../../tools/dual-stack-companion.py --location <lat> <lon>
```

Once provisioning completes, the device automatically transitions into its
satellite-pass scheduling loop and starts advertising the Hubble beacon.

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
       |  Initialize stacks            |
       |  (hubble_init + BLE)          |
       +-------------------------------+
                       |
                       v
       +------------------------------------+   no
       |  Provisioned?                      |-----------+
       |  (time, location, orbiral params)  |           |
       +------------------------------------+           v
                       |       +--------------------------------------+
                       |       | Connectable advertising "Hubble-ESP" |
                   yes |       |                                      |
                       |       | dual-stack-companion.py writes time, |
                       |       | location + orbital params over GATT  |
                       |       +--------------------------------------+
                       |                           |
                       |<--------------------------+
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
       +-------------------------------+                 |
       |  Sleep until pass time        |                 |
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
