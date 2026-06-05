# Hubble Network Satellite Direct Test Mode (DTM) Sample Application

## Overview

This sample enables the Direct Test Mode (DTM) functions for the Hubble
Satellite Network on ESP32-C6 hardware. It can be used for RF testing,
certification (FCC/CE), and bring-up of new hardware.

The sample initializes the Hubble SDK and starts an interactive shell over the
serial console.

## Requirements

- A serial terminal program (e.g., `minicom`, `screen`, PuTTY) or `idf.py monitor`.
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
- ESP32-C6 hardware
- A spectrum analyzer or test setup if you want to observe the transmissions.

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

Then `cd` to the sat-dtm example where you can build/flash/monitor:

```sh
idf.py build flash monitor
```

## Running the Sample

After flashing, connect to your device's serial port using a terminal emulator
(e.g., at 115200 baud), or use `idf.py monitor`.

You will see a prompt like this:

```
hubble>
```

Type `help` for the list of available commands.

## Shell Commands

This sample adds the following commands to the ESP-IDF console:

| Command                 | Description                                                                  | Arguments                    |
| :---------------------- | :--------------------------------------------------------------------------- | :--------------------------- |
| `power`                 | Set the radio TX power in dBm.                                               | `<power_dbm>`                |
| `channel`               | Set the frequency channel.                                                   | `<0..18>`                    |
| `payload`               | Set the payload length in bytes, or `-1` for single-frame mode (16 symbols). | `-1`, `0`, `4`, `9`, or `13` |
| `transmit`              | Transmit a single packet on the current channel.                             | None                         |
| `transmit_continuously` | Transmit packets repeatedly at a fixed interval.                             | `<interval_ms>`              |
| `transmit_sweep`        | Like `transmit_continuously`, hopping through channels.                      | `<interval_ms>`              |
| `wave`                  | Emit an unmodulated carrier wave on the current channel.                     | None                         |
| `stop`                  | Stop any ongoing transmission or carrier wave.                               | None                         |

### Notes

- `power`, `channel`, and `payload` set state that is applied to subsequent
  transmissions. Set them before issuing a `transmit*` or `wave` command.
- An interval of `0` for `transmit_continuously` / `transmit_sweep` transmits
  packets back-to-back.
- Only one transmission can run at a time. Use `stop` to end the current
  transmission or carrier wave before starting a new one.

## Example Session

```
hubble> power 20
hubble> channel 5
hubble> payload 9
hubble> transmit_continuously 1000
hubble> stop
hubble> wave
hubble> stop
```
