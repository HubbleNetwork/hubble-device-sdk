# Hubble Satellite Dual-Stack on TI (FreeRTOS)

Welcome to this sample project demonstrating the integration of the
[Texas Instruments (TI) SimpleLink Low Power F3 SDK](https://www.ti.com/tool/download/SIMPLELINK-LOWPOWER-F3-SDK)
with the [HubbleNetwork SDK](https://github.com/HubbleNetwork/hubble-device-sdk).

This project showcases how to run **BLE and the Hubble Satellite Network** on a TI device using FreeRTOS.

## Overview

This project is designed to:

- Demonstrate BLE and Hubble Satellite operation.
- Provide a practical starting point for developers integrating Hubble Satellite Network alongside BLE on TI hardware.
- Show how to use a BLE GATT service for runtime device provisioning (time and satellite ephemeris).

The project targets the **CC23xx** and **CC27xx** families and uses **FreeRTOS**. It is originally based on the TI
[`mac_sensor_ble_basic`](https://dev.ti.com/tirex/explore/node?isTheia=false&node=A__ABIptMuvmFu84E8hivwcdQ__com.ti.SIMPLELINK_LOWPOWER_F3_SDK__58mgN04__LATEST)
example and has been extended to support Hubble Satellite transmission and a
custom GATT provisioning service.

## Features

- Dual-stack application running Hubble Terrestrial (BLE) Network and the Hubble Satellite Network.
- GATT provisioning service that provides time and satellite orbital parameters over BLE.

## Requirements

To build and run this project, you will need:

- A TI development board:
  - **LP_EM_CC2340R5**, or
  - **LP_EM_CC2755P10**.
- The [TI SimpleLink Low Power F3 SDK](https://www.ti.com/tool/download/SIMPLELINK-LOWPOWER-F3-SDK) (`9.20.00.81` or newer).
- The [TI ARM-CLANG toolchain](https://www.ti.com/tool/CCSTUDIO).
- The [HubbleNetwork SDK](https://github.com/HubbleNetwork/hubble-device-sdk) cloned locally.

## Setup Instructions

### 1. Install Dependencies

Ensure the TI SDK and toolchain are installed on your system. Set the
*SYSCONFIG_TOOL*, *SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR*, and
*TICLANG_ARMCOMPILER* environment variables. For example:

```bash
export TICLANG_ARMCOMPILER=/Applications/ti/ti-cgt-armllvm_4.0.4.LTS
export SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR=/Applications/ti/simplelink_lowpower_f3_sdk_9_20_00_81
export SYSCONFIG_TOOL=/Applications/ti/sysconfig_1.26.3/sysconfig_cli.sh
```

Install Python dependencies for the *dual-stack-companion.py* provisioning script:

```bash
pip install -r ../../../../tools/requirements-companion.txt
```

### 2. Embed Device Key

The device's Hubble key must be baked into the firmware at build time. Use the
*embed_key_time.py* script to generate the key in hex, and copy that into
`_hubble_key` array:

```bash
python ../../../../tools/embed_key_time.py --base64 <path-to-key> -d
```

### 3. Build the Project

Pick the makefile for your target board:

```bash
# For LP_EM_CC2340R5
make -f cc2340r5.mk

# For LP_EM_CC2755P10
make -f cc2755p10.mk
```

**Debug Mode:** schedule sat transmission in 120s instead of the actual next pass:

```bash
make -f cc2755p10.mk DEBUG=1
```

### 4. Flash the Firmware

Flash the generated firmware (`build/sat-dual-stack.out`) onto the target
device using your preferred flashing tool (UniFlash, CCS, JLink, etc.).

### 5. Provision the Device

On first boot, the device starts a connectable BLE advertisement named **"Hubble-TI"**
and waits for provisioning data. Use *dual-stack-companion.py* to push the current UTC time, the
device location, and the ephemeris data for the target satellites:

```bash
export HUBBLE_API_TOKEN=<your-hubble-api-token>

python ../../../../tools/dual-stack-companion.py
```

By default the device location is determined via IP geolocation. To provision an
explicit location, pass the latitude and longitude (in degrees) with `--location`:

```bash
python ../../../../tools/dual-stack-companion.py --location <lat> <lon>
```

Once provisioning completes, the device automatically transitions into its
satellite-pass scheduling loop and starts advertising the Hubble beacon.

### 6. View Log

View log using TI `tiutils`. See setup instruction at `<TI_SDK_INSTALL_DIR>/tools/log/tiutils/README.md`.

Example:

```bash
tilogger --elf ./build/sat-dual-stack.out uart /dev/tty.usbmodemLS470FPO1 3000000 stdout
```

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
       +-------------------------------+   no
       |  Provisioned?                 |-----------+
       |  (UTC time + orbital params)  |           |
       +-------------------------------+           v
                       |       +-------------------------------------+
                       |       | Connectable advertising "Hubble-TI" |
                   yes |       |                                     |
                       |       | dual-stack-companion.py writes time,|
                       |       | location + orbital params over GATT |
                       |       +-------------------------------------+
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
