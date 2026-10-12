# Hubble Network Satellite Direct Test Mode (DTM) Sample Application

This sample puts a Zephyr board in DTM for satellite RF testing.

## Requirements

- A [Zephyr west workspace](../../../docs/quickstart/zephyr/index.rst) that includes the Hubble Device SDK.
- A board with a Hubble-supported satellite radio (e.g., nRF52, nRF54L15, Silicon Labs xG24).
- A serial terminal program (e.g., `minicom`, `screen`, PuTTY).
- [Satellite PHY Blob](#satellite-phy-blob).
- A spectrum analyzer or test setup if you want to observe the transmissions.

A Hubble key is not needed for this application.

## Overview

This sample enables the Direct Test Mode functions for the Hubble Satellite Network,
which can be used for RF testing, certification (FCC/CE), and bring-up of new hardware.

The sample initializes the Hubble Device SDK and adds the DTM commands to the
Zephyr shell over the serial console.

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/zephyr/sat-dtm`

## Environment Setup

First, set up the environment. This step assumes you've set up a west workspace
that includes the Hubble Device SDK. If you haven't, follow the [Zephyr quick start](../../../docs/quickstart/zephyr/index.rst).
Use the following command to activate the workspace's Python virtual environment.

### Windows

```ps1
<path_to_west_workspace>\.venv\Scripts\Activate.ps1
```

### Linux & macOS

```sh
source <path_to_west_workspace>/.venv/bin/activate
```

### Satellite PHY Blob

The satellite radio libraries are **not** fetched by `west update`. Fetch them
once per workspace:

```sh
west blobs fetch hubblenetwork-sdk
```

Silicon Labs boards use RAIL, which needs to be fetched along with the
Silicon Labs HAL:

```sh
west blobs fetch hal_silabs
```

## Configurations

This sample exposes the following options (via `west build -t menuconfig`, under
"Hubble Network Satellite Sample options"):

| Option | Default | Description |
| --- | --- | --- |
| `CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT` | `n` | Mock the board radio APIs, so the sample builds on targets without a satellite radio. |

## Build and Flash

Build for your board. Add `-p` for a pristine build.

```sh
west build -b <board>
```

e.g.
```sh
west build -b xg24_rb4187c
```

Flash the board:

```sh
west flash
```

## Test

After flashing, connect to your device's serial port using a terminal emulator
(e.g., at 115200 baud).

You will see a prompt like this:

```text
uart:~$
```

Type `help` for the list of available commands.

Connect to your spectrum analyzer for testing.

### Shell Commands

This sample adds the following commands to the Zephyr shell:

| Command | Description | Arguments |
| --- | --- | --- |
| `power` | Set the radio TX power in dBm. | `<dBm>` |
| `channel` | Set the frequency channel. | `<0..18>` |
| `payload` | Set the payload length in bytes, or `-1` for single-frame mode (16 symbols). | `-1`, `0`, `4`, `9`, or `13` |
| `transmit` | Transmit a single packet on the current channel. | None |
| `transmit_continuously` | Transmit packets repeatedly at a fixed interval. | `<interval_ms>` |
| `transmit_sweep` | Like `transmit_continuously`, hopping through channels. | `<interval_ms>` |
| `wave` | Emit an unmodulated carrier wave on the current channel. | None |
| `stop` | Stop any ongoing transmission or carrier wave. | None |
| `toggle_log` | Toggle logging. | None |

#### Notes

- `power`, `channel`, and `payload` set state that is applied to subsequent
  transmissions. Set them before issuing a `transmit*` or `wave` command.
- An interval of `0` for `transmit_continuously` / `transmit_sweep` transmits
  packets back-to-back.
- Only one transmission can run at a time. Use `stop` to end the current
  transmission or carrier wave before starting a new one.

### Example Session

```text
uart:~$ power 20
uart:~$ channel 5
uart:~$ payload 9
uart:~$ transmit_continuously 1000
uart:~$ stop
uart:~$ wave
uart:~$ stop
```
