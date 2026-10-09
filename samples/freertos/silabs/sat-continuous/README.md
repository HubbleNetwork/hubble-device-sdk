# Hubble Satellite Network on SiLabs (FreeRTOS)

Welcome to this sample project demonstrating the integration of the
[Silicon Labs Simplicity SDK](https://github.com/SiliconLabs/simplicity_sdk)
with the [Hubble Device SDK](https://github.com/HubbleNetwork/hubble-device-sdk).

This project showcases how to integrate the Hubble Satellite Network
on Silicon Labs EFR32 devices using FreeRTOS.

## Overview

This project is designed to:

- Demonstrate the use of the Simplicity SDK together with the Hubble Device SDK for satellite-enabled applications.
- Provide a practical starting point for developers integrating Hubble Satellite Network on Silicon Labs hardware.

The project targets the **EFR32xG24** and **EFR32xG26** families and uses
**FreeRTOS** as the operating system. The satellite PHY is provided by the
Hubble Device SDK extension as a radio configuration, and the RAIL code is
generated from it when the project is generated.

> [!NOTE]
> The sample uses the **DEVICE UPTIME** counter source
> (`SISDK_HUBBLE_COUNTER_SOURCE`), so it does not need UTC time
> provisioned: the EID counter used for encryption starts at 0 and advances with
> device uptime. Only the master key has to be provided when generating the
> project.

> [!NOTE]
> The nonce-reuse check (`SISDK_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK`)
> is disabled in `sat_continuous.slcp`, since this sample transmits
> continuously, only applicable for testing purpose. This should be turned
> on for production.

> [!WARNING]
> Make sure that your generated secret key from [Hubble API](https://hubble.com/docs/api-specification/register-new-devices)
> is using the same key size (AES-256) and counter source (DEVICE_UPTIME) as the sample.

## Features

- Integration with the **Hubble Device SDK** for satellite-specific transmission.
- FreeRTOS-based task and synchronization management.
- Modular and extensible code structure suitable for customization.

## Requirements

To build and run this project, you will need:

- Cryptographic key provided by Hubble Network
- A Silicon Labs **EFR32xG24** (e.g. **BRD4187C**) or **EFR32xG26** (e.g. **BRD4121A**) radio board
- The Simplicity SDK, SLC CLI, the GCC toolchain and Simplicity Commander, installed with the
  [Silicon Labs Tool (SLT)](https://docs.silabs.com/simplicity-installer-slt/latest/slt-cli/install-slt).
- The [Hubble Device SDK](https://github.com/HubbleNetwork/hubble-device-sdk) cloned locally.

### Setup Instructions

1. **Install Dependencies**

   Install SLT, then use it to install the Simplicity SDK and the tools this
   sample needs:

   ```sh
   export SLT=/path/to/slt

   $SLT install simplicity-sdk slc-cli gcc-arm-none-eabi commander
   ```

   SLT does not add anything to `PATH`, so export the paths used throughout
   this document.

   > [!TIP]
   > `$SLT where <package>` prints where a package was installed. See
   > [Useful paths](https://docs.silabs.com/ssv6ug/6.2.0/ssv6-tricks-tips/#useful-paths)
   > for the default install locations on each OS.

   ```sh
   export HUBBLE_SDK=/path/to/hubble-device-sdk
   export SISDK=/path/to/simplicity_sdk
   export SLC=/path/to/slc_cli/slc
   export COMMANDER=/path/to/commander
   ```

   | Variable | What it points at |
   | --- | --- |
   | `SLT` | The `slt` executable |
   | `HUBBLE_SDK` | The root of this repository |
   | `SISDK` | The Simplicity SDK |
   | `SLC` | The `slc` executable, `$($SLT where slc-cli)/slc` |
   | `COMMANDER` | The `commander` executable. On macOS, `$($SLT where commander)/Contents/MacOS/commander` |

2. **Install the SDK extension**

   The Hubble Device SDK is consumed as a Simplicity SDK extension. SLC
   discovers extensions under the SDK's `extension` directory, so link this
   repository there and register it as trusted:

   ```sh
   mkdir -p $SISDK/extension
   ln -s $HUBBLE_SDK $SISDK/extension/hubble-device-sdk

   $SLC signature trust -extpath $SISDK/extension/hubble-device-sdk
   ```

3. **Provision Key**

   The device's Hubble key is embedded into the firmware at project generation
   time, through the `HUBBLE_DEVICE_KEY` option in
   [config/sat_cont_config.h](config/sat_cont_config.h):

   | Option              | Default | Description                                      |
   | ------------------- | ------- | ------------------------------------------------ |
   | `HUBBLE_DEVICE_KEY` | `""`    | Hubble device cryptographic key, base64-encoded. |

   Pass it to `slc generate` with
   `--configuration=HUBBLE_DEVICE_KEY:\"<your-base64-key>\"` (see the next
   step), or set it in the Simplicity Studio configurator of the generated
   project's `config/sat_cont_config.h`.

4. **Build the Project**

   Generate a Makefile project and build it. Replace `brd4187c` with your
   target board (e.g. `brd4121a`):

   ```sh
   $SLC generate --sdk $SISDK -tlcn gcc -o makefile -cp -p sat_continuous.slcp \
       -d generated_sample_app --with brd4187c \
       --configuration=HUBBLE_DEVICE_KEY:\"<your-base64-key>\"

   cd generated_sample_app

   # replace 8 with the number of parallel jobs you want
   make -f sat_continuous.Makefile -j8
   ```

   To use Simplicity Studio instead, generate with `-np` into your workspace,
   then go to **PROJECTS > Open Project(s)**, then build and flash as usual:

   ```sh
   $SLC generate --sdk $SISDK -tlcn gcc -np -p sat_continuous.slcp \
       -d /location/to/your/simplicity-studio/workspace/sat-continuous --with brd4187c \
       --configuration=HUBBLE_DEVICE_KEY:\"<your-base64-key>\"
   ```

   > [!TIP]
   > In Simplicity Studio, the master key can be set in the configurator of the
   > generated project's `config/sat_cont_config.h`.

5. **Flash the Firmware**

   Flash the generated firmware onto the target device:

   ```sh
   $COMMANDER flash build/debug/sat_continuous.s37
   ```

### Usage

Once the firmware is flashed:

1. Power on the development board.
2. The device will continuously transmit satellite packets, and log to the
   virtual COM port at 115200 baud.
