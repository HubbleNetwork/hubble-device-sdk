# Hubble Satellite Dual-Stack on SiLabs (FreeRTOS)

Welcome to this sample project demonstrating the integration of the
[Silicon Labs Simplicity SDK](https://github.com/SiliconLabs/simplicity_sdk)
with the [Hubble Device SDK](https://github.com/HubbleNetwork/hubble-device-sdk).

This sample demonstrates running the **Hubble Terrestrial (BLE) Network** and the
**Hubble Satellite Network** together on a single Silicon Labs EFR32 device using
FreeRTOS.

## Overview

The application:

1. Provisions the device over a BLE GATT service (Unix epoch time, satellite
   orbital parameters and device location).
2. Schedules a timer for the next satellite pass using the SDK's pass
   prediction APIs.
3. Advertises Hubble beacon packets over Bluetooth while it waits.
4. When the timer expires, stops advertising and transmits to the satellite.

The provisioning GATT service is defined in
[config/btconf/gatt_configuration.btconf](config/btconf/gatt_configuration.btconf).

## Requirements

- A cryptographic key provided by Hubble Network.
- A satellite-capable Silicon Labs board with Bluetooth LE support.
- The Simplicity SDK, SLC CLI, the GCC toolchain and Simplicity Commander,
  installed with the
  [Silicon Labs Tool (SLT)](https://docs.silabs.com/simplicity-installer-slt/latest/slt-cli/install-slt).
- The [Hubble Device SDK](https://github.com/HubbleNetwork/hubble-device-sdk) cloned locally.

## Configuration

Options live in [config/sat_dual_stack_config.h](config/sat_dual_stack_config.h):

| Option                | Default | Description                                                                         |
| --------------------- | ------- | ----------------------------------------------------------------------------------- |
| `HUBBLE_DEVICE_KEY`   | `""`    | Hubble device cryptographic key, base64-encoded.                                    |
| `HUBBLE_SAMPLE_DEBUG` | `0`     | Transmit to the satellite every 120 s instead of waiting for the next pass.         |

> [!TIP]
> In Simplicity Studio, these options can be set in the configurator of the
> generated project's `config/sat_dual_stack_config.h`.

## Advertising parameters

The sample advertises with the following parameters by default. They can be modified.

| Parameter | Value | Set by |
| --- | --- | --- |
| Beacon interval | 1000–1200 ms | `ADV_INTERVAL_MIN_MS` / `ADV_INTERVAL_MAX_MS` in [src/app_ble.c](src/app_ble.c) |
| Provisioning interval (connectable) | 100–200 ms | Bluetooth stack default, see `sl_bt_advertiser_set_timing` |
| Tx power (beacon) | 0 dBm | `HUBBLE_ADV_TX_POWER_DDBM` in [src/app_ble.c](src/app_ble.c) |

## Building

Set up the Simplicity SDK and the Hubble Device SDK extension by following
steps 1–2 of the [sat-continuous setup instructions](../sat-continuous/README.md#setup-instructions).
Replace `brd4187c` below with your target board.

### Command line

```sh
$SLC generate --sdk $SISDK -tlcn gcc -o makefile -cp -p sat_dual_stack.slcp \
    -d generated_sample_app --with brd4187c \
    --configuration=HUBBLE_DEVICE_KEY:\"<your-base64-key>\"

cd generated_sample_app

# replace 8 with the number of parallel jobs you want
make -f sat_dual_stack.Makefile -j8

$COMMANDER flash build/debug/sat_dual_stack.s37
```

### Simplicity Studio

Generate the project into your workspace, then go to
**PROJECTS > Open Project(s)**:

```sh
$SLC generate --sdk $SISDK -tlcn gcc -np -p sat_dual_stack.slcp \
    -d /location/to/your/simplicity-studio/workspace/sat-dual-stack --with brd4187c
```

In the generated project:

- **Sample options:** open `config/sat_dual_stack_config.h` in the configurator.
- **GATT database:** open `config/btconf/gatt_configuration.btconf` in the
  Bluetooth GATT Configurator (e.g. **Device Name** under **Generic Access**).
- Build and flash: see SiLabs
  [Build, Flash, or Debug a Project](https://docs.silabs.com/ss-vscode/latest/ss-vscode-projects/build-flash-debug-project#build-flash-or-debug-a-project).

The device logs to the virtual COM port at 115200 baud.

## Provision the Device

Install Python dependencies for the *dual-stack-companion.py* provisioning script:

```sh
pip install -r ../../../../tools/requirements-companion.txt
```

On first boot, the device starts a connectable BLE advertisement named **"Hubble-SiLabs"**
and waits for provisioning data. Use *dual-stack-companion.py* to push the current Unix Epoch time,
device location, and orbital parameters data for the target satellites:

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
       |  Enable BLE                   |
       |  Bluetooth stack boots,       |
       |  advertising sets created     |
       +-------------------------------+
                       |
                       v
       +-------------------------------+
       |  Connectable advertising      |<----------------+
       |  "Hubble-SiLabs"              |                 |
       +-------------------------------+                 |
                       |                                 |
                       v                                 |
       +-------------------------------+                 |
       |  dual-stack-companion.py      |                 |
       |  writes time, location +      |                 |
       |  orbital params over GATT     |                 |
       +-------------------------------+                 |
                       |                                 |
                       v                                 |
       +-------------------------------+   time not set  |
       |  Peer disconnects             |-----------------+
       +-------------------------------+
                       | time set
                       v
       +-------------------------------+
       |  hubble_init +                |
       |  hubble_sat_satellites_set    |
       +-------------------------------+
                       |
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
       |  Wait until pass time (timer) |                 |
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
