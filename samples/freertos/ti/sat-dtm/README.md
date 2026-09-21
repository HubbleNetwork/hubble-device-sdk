# Hubble Network Satellite Direct Test Mode (DTM) on TI (FreeRTOS)

This sample demonstrates the **Direct Test Mode (DTM)** functions of the Hubble
Satellite Network on
[Texas Instruments (TI) SimpleLink Low Power F3](https://www.ti.com/tool/download/SIMPLELINK-LOWPOWER-F3-SDK)
devices, running on **FreeRTOS** and the
[Hubble Device SDK](https://github.com/HubbleNetwork/hubble-device-sdk).

DTM exposes the radio physical layer over a serial command-line interface so it
can be driven directly for **RF testing, certification (FCC/CE), and bring-up of
new hardware**.

## Overview

The application opens a UART, runs a small command-line interface, and lets you
configure the radio (channel, power, payload) and trigger transmissions
(single packet, continuous, channel sweep, or unmodulated carrier wave) from a
serial terminal.

The project targets the **CC27xx** (e.g. **LP_EM_CC2755P10**). It is based on the TI
[`CLI_driver_LP_EM_CC2340R5_freertos_ticlang`](https://github.com/TexasInstruments-Sandbox/SimpleLink-Low-Power-F3-Demos/tree/main/Projects/CLI_driver_LP_EM_CC2340R5_freertos_ticlang)
demo. The generic UART CLI driver was reused and extended to fit with the sample.

## Requirements

- A TI **CC27xx** (e.g. **LP_EM_CC2755P10**) development board.
- The [TI SimpleLink Low Power F3 SDK](https://www.ti.com/tool/download/SIMPLELINK-LOWPOWER-F3-SDK).
- The [TI toolchain](https://www.ti.com/tool/CCSTUDIO).
- The [Hubble Device SDK](https://github.com/HubbleNetwork/hubble-device-sdk) cloned locally.
- A serial terminal program (e.g. `minicom`, `screen`, PuTTY).
- A spectrum analyzer or RF test setup if you want to observe the transmissions.

## Building and Flashing

1. **Install Dependencies**

   Ensure that the TI SDK is installed on your
   system. Set *SYSCONFIG_TOOL*, *SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR* and *TICLANG_ARMCOMPILER*
   environment variables.

   #### **Linux and macOS**

   ```bash
   export TICLANG_ARMCOMPILER=/path/to/ti/ti-cgt-armllvm
   export SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR=/path/to/ti/simplelink_lowpower_f3_sdk
   export SYSCONFIG_TOOL=/path/to/ti/sysconfig/sysconfig_cli.sh
   ```

   #### **Windows**

   If using Bash, the above commands work.

   Command Prompt:

   ```bat
   set TICLANG_ARMCOMPILER=/path/to/ti/ti-cgt-armllvm
   set SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR=/path/to/ti/simplelink_lowpower_f3_sdk
   set SYSCONFIG_TOOL=/path/to/ti/sysconfig/sysconfig_cli.bat
   ```

   PowerShell:

   ```powershell
   $env:TICLANG_ARMCOMPILER = "/path/to/ti/ti-cgt-armllvm"
   $env:SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR = "/path/to/ti/simplelink_lowpower_f3_sdk"
   $env:SYSCONFIG_TOOL = "/path/to/ti/sysconfig/sysconfig_cli.bat"
   ```

   > [!WARNING]
   > For any shell, write the paths with **forward slashes** (e.g. `C:/ti/...`, not `C:\ti\...`).
   > Windows GNU Make strips backslashes out of the commands.

   > [!NOTE]
   > If not using Bash, you may need to include Unix tools on your `PATH`:
   > `C:\Program Files\Git\usr\bin`. It is not added by default when installing
   > Git. A Bash session will inherit this automatically.

   > [!NOTE]
   > If using Powershell or cmd, set the SYSCONFIG_TOOL to `sysconfig_cli.bat` instead of
   > `sysconfig_cli.sh`.

2. **Build the project**

   ```bash
   make -f cc2755p10.mk
   ```

3. **Flash the firmware**

   Flash the generated firmware (`sat-dtm.out`) onto the target device using your
   preferred flashing tool.

## Running the Sample

After flashing, connect to your device's serial port using a terminal emulator
(e.g., at 115200 baud).

On reset you will see:

```
DTM Application Initialized:
```

The interface echoes what you type and processes a command when you press Enter.
Type `help` for the list of available commands.

A typical session: set the radio up, then transmit:

```
channel 5
power 20
payload 4
transmit_continuously 1000
stop
```

## Commands

| Command                 | Description                                                                  | Arguments                    |
| :---------------------- | :--------------------------------------------------------------------------- | :--------------------------- |
| `power`                 | Set the radio TX power in dBm.                                               | `<dBm>`                      |
| `channel`               | Set the frequency channel.                                                   | `<0..18>`                    |
| `payload`               | Set the payload length in bytes, or `-1` for single-frame mode.              | `-1`, `0`, `4`, `9`, or `13` |
| `transmit`              | Transmit a single packet on the current channel.                             | None                         |
| `transmit_continuously` | Transmit packets repeatedly at a fixed interval.                             | `<interval_ms>`              |
| `transmit_sweep`        | Like `transmit_continuously`, hopping through channels.                      | `<interval_ms>`              |
| `wave`                  | Emit an unmodulated carrier wave on the current channel for 10 seconds.      | None                         |
| `stop`                  | Stop any ongoing packet transmission.                                        | None                         |
| `help`                  | List the available commands.                                                 | None                         |
| `clear`                 | Clear the terminal screen.                                                   | None                         |

## Key Files

- **src/main.c**: Entry point: initializes the board and Hubble, then starts the
  FreeRTOS scheduler and the application thread.
- **src/cli_dtm.c**: The DTM application: registers the commands, runs the UART
  read loop, and manages the transmit thread/timer.
- **src/cli_driver.c / src/cli_driver.h**: Generic UART command-line driver
  (command registration, parsing, and dispatch), reused from the TI CLI demo.
- **src/hubble_ti_crypto.c**: Dummy crypto implementation (we don't need this for DTM).
