Welcome to the sample project that demonstrates the integration of the
[Texas Instruments (TI) SDK](https://www.ti.com/tool/download/SIMPLELINK-LOWPOWER-F3-SDK)
with the [HubbleNetwork-SDK](https://github.com/HubbleNetwork/hubble-device-sdk). This
project showcases the development of a BLE (Bluetooth Low Energy)
application using FreeRTOS, leveraging the capabilities of both SDKs.

## Overview

This project is designed to:

+ Demonstrate the use of the TI SDK for BLE application development.
+ Showcase the integration of the HubbleNetwork SDK for BLE-specific operations.
+ Provide a starting point for developers working on BLE applications with TI devices.

The project targets the *CC23xx* family of devices and uses *FreeRTOS*
as the operating system. It is originally a copy of
[basic-ble](https://github.com/TexasInstruments/simplelink-ble5stack-examples/tree/lpf2-lpf3-7.41.00.17-8.20.00.119-ble5stack/examples/rtos/LP_EM_CC2340R5/ble5stack/basic_ble)
sample.

### Features

+ Integration with the HubbleNetwork-SDK for BLE-specific operations.
+ FreeRTOS-based task management.
+ Modular and extensible codebase.

### Requirements

To build and run this project, you will need:

+ A TI CC23xx development board (e.g., **LP_EM_CC2340R5**).
+ The [TI SDK](https://www.ti.com/tool/download/SIMPLELINK-LOWPOWER-F3-SDK) installed on your system.
* [TI toolchain](https://www.ti.com/tool/CCSTUDIO)
+ The [HubbleNetwork-SDK](https://github.com/HubbleNetwork/hubble-device-sdk) cloned into the project directory.
+ Python 3 for running the *embed_key_time.py* script.

### Project Structure
+ **app**: Contains application-specific source files.
+ **common**: Contains shared utilities and startup code.
+ **freertos**: FreeRTOS-specific configuration and build files.

### Setup Instructions

1. **Install Dependencies**

   Ensure that the TI SDK is installed on your
   system. Set *SYSCONFIG_TOOL*, *SIMPLELINK_LOWPOWER_F3_SDK_INSTALL_DIR* and *TICLANG_ARMCOMPILER*
   environment variables.

   When using the SysConfig-based build (*makefile-syscfg*), also set *HUBBLE_NETWORK_SDK* to the
   root of this SDK (defaults to four directories above the sample if unset):
   ```bash
   export HUBBLE_NETWORK_SDK=/path/to/hubble-device-sdk
   ```

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


2. **Embed Key and Unix Time**

   Use the *embed_key_time.py* script to provision a BLE key and Unix timestamp:

```bash
# Script is located in SDK_BASE/tools

python ../../../../tools/embed_key_time.py --base64 <path-to-key> -o src/
```

3. **Build the Project**

   Build the project using the provided *makefile*:

```bash
make
```

   Alternatively, use the SysConfig-based build (*makefile-syscfg*), which generates driver and
   peripheral configuration via SysConfig using both the TI SDK and Hubble Device SDK products:

```bash
make -f makefile-syscfg
```

4. **Flash the Firmware**

   Flash the generated firmware (*ble-beacon.out*) onto the target device using your preferred flashing tool.

5. **View Log**

   View log using TI `tiutils`. See setup instruction at `<TI_SDK_INSTALL_DIR>/tools/log/tiutils/README.md`.

   Example:

```bash
   tilogger --elf ./build/ble-beacon.out uart /dev/tty.usbmodemLS470FPO1 3000000 stdout
```

### Usage

Once the firmware is flashed:

1. Power on the development board.
2. The device will start BLE advertising.

### Key Files

+ **src/hubble_ble_adv.c**: BLE advertising implementation.
+ **src/hubble_ble_ti.c**: This is a core file to integrate with HubbleNetwork SDK. It implements the required cryptograhic API.
+ **makefile**: Build system for the project.
+ **makefile-syscfg**: SysConfig-based build system; generates driver configuration using both the TI SDK and Hubble Device SDK SysConfig products.
+ **ble-beacon-hubble.syscfg**: SysConfig script that configures drivers, BLE stack, FreeRTOS, and the Hubble Device SDK module.
