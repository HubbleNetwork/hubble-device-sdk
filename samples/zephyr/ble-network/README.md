# Hubble Network BLE Sample Application

This sample application demonstrates how to use the Hubble Device SDK on a
Zephyr-based device to broadcast data over Bluetooth Low Energy (BLE).

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  (optional), for testing with [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- A [Zephyr west workspace](../../../docs/quickstart/zephyr/index.rst) that includes the Hubble Device SDK.
- A board with Bluetooth LE support.
- A serial terminal program (e.g., `minicom`, `screen`, PuTTY).
- A computer with Bluetooth LE that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- The Hubble Connect app for
  [iOS](https://apps.apple.com/us/app/hubble-connect/id6751236191) or
  [Android](https://play.google.com/store/apps/details?id=com.hubble.connect)
  (optional), for the backend test.

## Overview

The application initializes the Hubble Device SDK, sets an encryption key and a Unix
timestamp, and then advertises user-provided data within a BLE packet. It
provides a shell interface over the serial port to input the necessary
configuration and data.

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/zephyr/ble-network`

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

### Python Dependencies

Then install pyserial for the `provisioning-key.py` provisioning script, and the
[pyhubblenetwork](https://hubble.com/docs/guides/cli) tool for testing:

```sh
pip install pyserial pyhubblenetwork
```

## BLE Advertising Parameters

The sample advertises with the following parameters by default. To change one,
edit the macro or option in the *Set by* column and rebuild.

| Parameter | Value | Set by |
| --- | --- | --- |
| Beacon interval | 1000–1200 ms | `ADV_INTERVAL_MIN_MS` / `ADV_INTERVAL_MAX_MS` in [src/main.c](src/main.c) |
| Tx power | 0 dBm | `CONFIG_BT_CTLR_TX_PWR_0` in [prj.conf](prj.conf) |

## Build and Flash

Build for your board. Add `-p` for a pristine build.

```sh
west build -b <board>
```

e.g.
```sh
west build -b nrf52840dk/nrf52840
```

Flash the board:

```sh
west flash
```

## Runtime Provisioning

After flashing, connect to your device's serial port using a terminal emulator
(e.g., at 115200 baud).

You will see a prompt like this:

```text
Insert key and unix_time time to start. Type help for more information.
uart:~$
```

The application will not start advertising until both the device key and
the Unix time are set. The key must be registered as **AES-256** with the
**Unix time** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).

### Step 1: Set the Device Key

The application expects the key to be sent over the serial port as hex text.

1. In the sample's shell, type the `key` command and press Enter. The device
   will now wait to receive the key.

   ```text
   uart:~$ key
   Please transmit the key through the serial. e.g: xxd -p key > /dev/ttyX
   Loading...
   press ctrl-x ctrl-q to escape
   ```

2. From a separate terminal on your host machine, send the key to the board's
   serial port (e.g., `/dev/ttyACM0`). Create a file, `master.key`, and paste
   your Hubble device key into it, then run the `provisioning-key.py` script.
   Use the `-b` or `--base64` flag for a base64-encoded key file:

   ```sh
   python provisioning-key.py -b master.key /dev/ttyACM0
   ```

   For a raw key file, `xxd` works too:

   ```sh
   xxd -p master.key > /dev/ttyACM0
   ```

3. After sending the key, you must send an escape sequence to signal the end
   of the transmission. Press `Ctrl+X` followed by `Ctrl+Q` in your serial
   terminal.

   You should see `Number of bytes read: 32`, confirming that the key has been set.

### Step 2: Set the Unix Time

Set the current Unix time using the `time` command, providing the time as a Unix
timestamp (milliseconds since the epoch).

You can get the current Unix timestamp with the following command:

```sh
python -c "import time;print(int(time.time() * 1000))"
# or using coreutils
date +"%s%3N"
```

In the sample's shell, run the `time` command with the timestamp:

```text
uart:~$ time 1752865580290
```

Once the Unix time is set, the application will initialize the Hubble Device SDK and
the Bluetooth subsystem.

### Step 3: Advertise Data

Now you can provide the data you want to broadcast using the `data` command.

```text
uart:~$ data "Hello Hubble"
```

The application will take this string, process it with the Hubble Device SDK, and
place the resulting data in the service data field of its BLE advertisement
packet.

### Shell Commands

This sample adds the following custom commands to the Zephyr shell:

| Command | Description | Arguments |
| --- | --- | --- |
| `key` | Puts the device in key-loading mode. | None |
| `time` | Sets the Unix time. | `<timestamp>` (Unix timestamp in ms) |
| `data` | Sets the data payload to be advertised. | `<string>` (e.g., "your data") |

## Test

### Local BLE Scanning

The `hubblenetwork ble scan` tool can be used to test the BLE beacon.
It scans for a device advertising with the given key and attempts to
decode its payload.

Pass the same key that was provisioned into the device, as base64:

```sh
hubblenetwork ble scan --key "<your base64 device key>"
```

Correctly decoded packets will stream to the console.

### Backend Test

Download the Hubble Connect app to your smartphone and sign in, then keep the
phone near the device with the app open. The phone acts as a scanner: the
device appears in the app, and its packets appear on
[Hubble Dash](https://dash.hubble.com/dashboard).
