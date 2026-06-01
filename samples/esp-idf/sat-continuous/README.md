# Hubble Network Sat Continuous Sample

This sample application demonstrates how to use the Hubble Network SDK to
continuously transmit packets to satellite.

## Requirements

- Cryptographic key provided by Hubble Network
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
- ESP32-C6 hardware
- The [Hubble Connect app](https://hubble.com/docs/guides/hubble-connect) (only required when time
  sync is enabled)

## Overview

This project is designed to:

- Demonstrate the use of the ESP-IDF together with the HubbleNetwork-SDK for
  satellite-enabled applications.
- Provide a practical starting point for developers integrating the Hubble
  Satellite Network on ESP32-C6 hardware.

Once running, the device initializes the Hubble SDK and then enters a loop that
builds and transmits a satellite packet with normal reliability.


## Configuration

The sample exposes the following options (via `idf.py menuconfig`, under
"Sat Continuous Sample Configuration"):

| Option                     | Default | Description                                              |
| -------------------------- | ------- | -------------------------------------------------------- |
| `CONFIG_HUBBLE_DEVICE_KEY` | `""`    | Hubble device cryptographic key, base64-encoded.         |
| `CONFIG_SAMPLE_SYNC_TIME`  | `y`     | Enable BLE time sync with the Hubble Connect app.        |

## Building and Running

First, set up the environment. This step assumes you've installed esp-idf
to `~/esp/esp-idf`. If you haven't, follow the initial steps in the [Installation
guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#installation) for your OS.

```sh
source ~/esp/esp-idf/export.sh
```

Set the target chip to ESP32-C6:

```sh
idf.py set-target esp32c6
```

Configure the device key:

```sh
idf.py menuconfig
```

or add the config `CONFIG_HUBBLE_DEVICE_KEY="your_b64_key"` to
`sdkconfig.defaults`.

Next, `cd` to the sat-continuous example where you can build/flash/monitor:

```sh
idf.py build flash monitor
```

After flashing, if time sync is enabled the device starts advertising over BLE
and waits for time sync. Use the Hubble Connect app to provision unix epoch time.
Once synced, the device continuously transmits a packet to satellite with
normal reliability.
