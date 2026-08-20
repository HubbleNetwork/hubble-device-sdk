# Hubble Network BLE Beacon Sample on ESP-IDF

This sample application demonstrates how to use the Hubble Device SDK to
create a BLE beacon that advertises its presence.

## Requirements

- Cryptographic key provided by Hubble Network
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
- ESP SoC with Bluetooth LE support

## Overview

The beacon advertises data that can be picked up by the Hubble Network. The
advertised data is generated using the `hubble_ble_advertise_get()` function
from the Hubble BLE library.

The sample requires a master key and the current Unix time to be provisioned
into the device. This is done by running the `embed_key_time.py` script before
building the application.

> [!NOTE]
> All commands should be ran from the sample folder containing this file:
> `<SDK path>/samples/esp-idf/ble-beacon`

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

The provisioning process embeds a master key and the current Unix time into the
firmware. This is a necessary step before building and flashing the
application.

The `embed_key_time.py` script takes the key file and embeds it along with the
current timestamp into the source code (`src/key.c` and `src/time.c`).

**For a raw key file:**

```sh
<SDK_ROOT>/tools/embed_key_time.py master.key -o main/
```

**For a base64-encoded key file:**

Use the `-b` or `--base64` flag:

```sh
<SDK_ROOT>/tools/embed_key_time.py -b master.key -o main/
```

After running the script, the key and timestamp will be compiled into the application.

## Build

```sh
idf.py build flash monitor
```

After flashing, the device will start advertising as a Hubble BLE beacon.

## Test

The `scan.py` tool can be used to test the BLE
beacon. The script scans for BLE devices, and when it finds a Hubble Network
beacon, it attempts to decode the advertisement data using the provided master
key.

### Running the sample and testing it

To run the sample and test it, you need to provide the same master key that was provisioned into the device.

**For a raw key file:**

```sh
<SDK_ROOT>/tools/scan.py master.key
```

**For a base64-encoded key file:**

Use the `-b` or `--base64` flag:

```sh
<SDK_ROOT>/tools/scan.py -b master.key
```

