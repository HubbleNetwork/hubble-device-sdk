# Hubble Network BLE Beacon Sample (SiLabs FreeRTOS)

This sample application demonstrates how to use the Hubble Device SDK to
create a BLE beacon that advertises its presence.

## Requirements

- Cryptographic key provided by Hubble Network
- Simplicity SDK, SLC CLI, the GCC toolchain and Simplicity Commander, see
  [Install Dependencies](#install-dependencies)
- An EFR32 development board that supports BLE. This sample has been tested on BRD4187C (EFR32MG24).

## Install Dependencies

Install the [Silicon Labs Tool (SLT)](https://docs.silabs.com/simplicity-installer-slt/latest/slt-cli/install-slt),
then use it to install the Simplicity SDK and the tools this sample needs:

```sh
export SLT=/path/to/slt

$SLT install simplicity-sdk slc-cli gcc-arm-none-eabi commander
```

SLT does not add anything to `PATH`, so export the paths used throughout this
document.

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

## Overview

The beacon advertises data that can be picked up by the Hubble Network. The
advertised data is generated using the `hubble_ble_advertise_get()` function
from the Hubble BLE library, and is handed to the Bluetooth stack as
non-connectable legacy advertisements.

The application uses a non-resolvable private address managed by the stack, so
the device is not trackable across advertisement rotations.

## Provisioning

The provisioning process embeds a master key and the current Unix time into the
firmware. This is a necessary step before generating, building and flashing the
application.

The `embed_key_time.py` script takes the key file and embeds it along with the
current timestamp into the source code (`src/key.c` and `src/time.c`).

**For a raw key file:**

```sh
$HUBBLE_SDK/tools/embed_key_time.py master.key -o src/
```

**For a base64-encoded key file:**

Use the `-b` or `--base64` flag:

```sh
$HUBBLE_SDK/tools/embed_key_time.py -b master.key -o src/
```

> [!NOTE]
> Both files (`key.c` and `time.c`) must exist before the project is generated.

## Installing the SDK extension

The Hubble Device SDK is consumed as a Simplicity SDK extension. SLC discovers
extensions under the SDK's `extension` directory, so link this repository there
and register it as trusted:

```sh
mkdir -p $SISDK/extension
ln -s $HUBBLE_SDK $SISDK/extension/hubble-device-sdk

$SLC configuration --sdk $SISDK
$SLC signature trust --sdk $SISDK
$SLC signature trust -extpath $SISDK/extension/hubble-device-sdk
```

## Building and Running

### Simplicity Studio

> [!NOTE]
> This flow has been tested with Simplicity Studio 6.

If you prefer Simplicity Studio and already have the tools and dependencies
installed, then do:

```sh
# replace 'brd4187c' with your target board
$SLC generate -tlcn gcc -np -p ble_beacon.slcp \
    -d /location/to/your/simplicity-studio/workspace/ble-beacon --with brd4187c
```

Then in Simplicity Studio go to **PROJECTS > Open Project(s)**,
select the location you just generated `ble-beacon` directory,
then build and flash like you normally would.

### Command-line

For each command, replace `brd4187c` with your target board.

Validate the project resolves against your SDK and board:

```sh
$SLC validate-project -p ble_beacon.slcp --with brd4187c
```

Generate a Makefile project:

```sh
$SLC generate -tlcn gcc -o makefile -cp -p ble_beacon.slcp \
    -d generated_sample_app --with brd4187c
```

Build it:

```sh
cd generated_sample_app
make -f ble_beacon.Makefile
```

Flash it:

```sh
$COMMANDER flash build/debug/ble_beacon.s37
```

After flashing, the device will start advertising as a Hubble BLE beacon, and
log to the virtual COM port at 115200 baud.

## Testing

The `scan.py` tool can be used to test the BLE beacon. The script scans for
BLE devices, and when it finds a Hubble Network beacon, it attempts to decode
the advertisement data using the provided master key. You need to provide
the same master key that was provisioned into the device.

```sh
$HUBBLE_SDK/tools/scan.py -k "your_b64_key"
```

## Troubleshooting

### `slt` not found

The Silicon Labs Tool (SLT) installs everything under `~/.silabs`, but does not
add itself to `PATH`. Add it to your shell profile:

| macOS | Linux |
| --- | --- |
| `/Applications/SimplicityInstaller/app/Contents/Resources` | `SimplicityInstaller/Resources` |

SDKs and SDK extensions always live under `~/.silabs/slt/installs/conan/p`.
Other packages land in either `archive` or `conan/p`, depending on the package
type.

### Installing missing tools

If `commander` or `slc` are not installed, add them with SLT:

```sh
$SLT install commander
$SLT install slc-cli
```

> [!WARNING]
> Install `slc-cli`, not `slc_cli`. Silicon Labs documents the latter as out of
> date.

For recipe-based installs, which pin a whole toolchain at once through a
`pkg.slt` file, see
[First-time installation setup](https://docs.silabs.com/simplicity-installer-slt/latest/slt-getting-started-slt-cli/first-time-installation-setup).
