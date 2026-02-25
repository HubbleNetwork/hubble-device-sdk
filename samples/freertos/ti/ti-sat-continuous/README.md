# Hubble Satellite Network on TI (FreeRTOS)

Welcome to this sample project demonstrating the integration of the
[Texas Instruments (TI) SimpleLink Low Power F3 SDK](https://www.ti.com/tool/download/SIMPLELINK-LOWPOWER-F3-SDK)
with the [HubbleNetwork-SDK](https://github.com/HubbleNetwork/hubble-device-sdk).

This project showcases how to integrate the Hubble Satellite Network
on TI device using FreeRTOS.

## Overview

This project is designed to:

- Demonstrate the use of the TI SDK together with the HubbleNetwork-SDK for satellite-enabled applications.
- Provide a practical starting point for developers integrating Hubble Satellite Network on TI hardware.

The project targets the **CC23xx** family and uses **FreeRTOS**
as the operating system. It is originally based on the TI
[`rfCarrierWave`](https://github.com/TexasInstruments/simplelink-prop_rf-examples/tree/main/examples/rtos/LP_EM_CC2340R5/prop_rf/rfCarrierWave)
example and has been extended to support Hubble Satellite transmission.

## Features

- Integration with the **HubbleNetwork-SDK** for satellite-specific transmission.
- FreeRTOS-based task and synchronization management.
- Modular and extensible code structure suitable for customization.

## Requirements

To build and run this project, you will need:

- A TI **CC23xx** development board (e.g. **LP_EM_CC2340R5**).
- The [TI SimpleLink Low Power F3 SDK](https://www.ti.com/tool/download/SIMPLELINK-LOWPOWER-F3-SDK).
- The [TI toolchain](https://www.ti.com/tool/CCSTUDIO).
- The [HubbleNetwork-SDK](https://github.com/HubbleNetwork/hubble-device-sdk) cloned locally.

### Setup Instructions

1. **Install Dependencies**

   Ensure that the TI SDK is installed on your
   system. Set *SYSCONFIG_TOOL*, *SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR* and *TICLANG_ARMCOMPILER*
   environment variables. e.g:
```bash
export TICLANG_ARMCOMPILER=/Applications/ti/ccs2040/ccs/tools/compiler/ti-cgt-armllvm_4.0.2.LTS/
export SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR=/Applications/ti/simplelink_lowpower_f3_sdk_9_20_00_07_ea
export SYSCONFIG_TOOL=/Applications/ti/sysconfig_1.23.2/sysconfig_cli.sh
```

2. **Build the Project**

   Build the project using the provided *makefile*:

```bash
make -f cc2340r5.mk

# or make -f cc2755p10.mk
```

3. **Flash the Firmware**

   Flash the generated firmware (*sat-continuous.out*) onto the target device using your preferred flashing tool.

### Usage

Once the firmware is flashed:

1. Power on the development board.
2. The device will start satellite transmission.

### Key Files

+ **src/mainc.c**: Main thread that continously transmit to satellite.
+ **src/hubble_ti_crypto.c**: This is a core file to integrate with HubbleNetwork SDK. It implements the required cryptograhic API.
+ **makefile**: Build system for the project.

### Project Notes

- Besides the base config provided by the [rfcarrierwave](https://github.com/TexasInstruments/simplelink-prop_rf-examples/tree/main/examples/rtos/LP_EM_CC2340R5/prop_rf/rfCarrierWave)
sample, the project added the `LGPTimer` componenent with interrupt set to Highest Priority.

- Make sure the RCL Command Symbol in Sysconfig Custom RF name is `rclPacketTxCmdGenericTxTest_ble_gen_0`
(set Symbol Name Generation Method to Automatic).
- The make file includes the `-DCC23X0 CFLAGS`.
