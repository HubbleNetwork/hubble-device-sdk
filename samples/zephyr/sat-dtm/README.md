# Hubble Network Satellite Direct Test Mode (DTM) Sample Application

## Overview

This sample enables the Direct Test Mode functions for the Hubble Satellite Network,
which can be used for RF testing, certification (FCC/CE), and bring-up of new hardware.

## Requirements

- A serial terminal program (e.g., `minicom`, `screen`, PuTTY).
- A target board with a Hubble-supported radio (e.g., nRF52, nRF54L15, SiLabs xG24, etc.).
- A spectrum analyzer or test setup if you want to observe the transmissions.

## Building and Flashing

1.  Open a terminal in the project directory (`samples/zephyr/sat-dtm`).
2.  Build the application using `west`, replacing `<your_board_here>` with your
    target board's identifier (e.g., `xg24_rb4187c`).

    ```sh
    west build -b <your_board_here>
    ```

3.  Flash the application to your board:

    ```sh
    west flash
    ```

## Running the Sample

After flashing, connect to your device's serial port using a terminal emulator
(e.g., at 115200 baud).

You will see a prompt like this:

```
uart:~$
```

Type `help` for the list of available commands.

## Shell Commands
 
This sample adds the following commands to the Zephyr shell:

| Command                 | Description                                                                  | Arguments                    |
| :---------------------- | :--------------------------------------------------------------------------- | :--------------------------- |
| `power`                 | Set the radio TX power in dBm.                                               | `<dBm>`                      |
| `channel`               | Set the frequency channel.                                                   | `<0..18>`                    |
| `payload`               | Set the payload length in bytes, or `-1` for single-frame mode (16 symbols). | `-1`, `0`, `4`, `9`, or `13` |
| `transmit`              | Transmit a single packet on the current channel.                             | None                         |
| `transmit_continuously` | Transmit packets repeatedly at a fixed interval.                             | `<interval_ms>`              |
| `transmit_sweep`        | Like `transmit_continuously`, hopping through channels.                      | `<interval_ms>`              |
| `wave`                  | Emit an unmodulated carrier wave on the current channel.                     | None                         |
| `stop`                  | Stop any ongoing transmission or carrier wave.                               | None                         |
| `toggle_log`            | Toggle logging.                                                              | None                         |
