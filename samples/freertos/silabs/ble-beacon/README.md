# Hubble Network BLE Beacon Sample (SiLabs FreeRTOS)

This sample application demonstrates how to use the Hubble Device SDK to
create a BLE beacon that advertises its presence.

## Requirements

- Cryptographic key provided by Hubble Network
- Simplicity SDK, SLC CLI, the GCC toolchain and Simplicity Commander,
  installed with the Silicon Labs Tool (https://docs.silabs.com/simplicity-installer-slt/latest/slt-cli/install-slt)
- An EFR32 development board that supports BLE. This sample has been tested on BRD4187C (EFR32MG24).

## Overview

The beacon advertises data that can be picked up by the Hubble Network. The
advertised data is generated using the `hubble_ble_advertise_get()` function
from the Hubble BLE library, and is handed to the Bluetooth stack as
non-connectable legacy advertisements.

The application uses a non-resolvable private address managed by the stack, so
the device is not trackable across advertisement rotations.

The sample requires a master key and the current Unix time to be provisioned
into the device. This is done by running the `embed_key_time.py` script before
generating the project.

## Provisioning

The provisioning process embeds a master key and the current Unix time into the
firmware. This is a necessary step before generating, building and flashing the
application.

The `embed_key_time.py` script takes the key file and embeds it along with the
current timestamp into the source code (`src/key.c` and `src/time.c`).

**For a raw key file:**

```sh
<SDK_ROOT>/tools/embed_key_time.py master.key -o src/
```

**For a base64-encoded key file:**

Use the `-b` or `--base64` flag:

```sh
<SDK_ROOT>/tools/embed_key_time.py -b master.key -o src/
```

> [!NOTE]
> Both files (`key.c` and `time.c`) must exist before the project is generated.

## Installing the SDK extension

The Hubble Device SDK is consumed as a Simplicity SDK extension. SLC discovers
extensions under the SDK's `extension` directory, so link this repository there
and register it as trusted:

```sh
SDK=<path to simplicity_sdk>

mkdir -p $SDK/extension
ln -s <SDK_ROOT> $SDK/extension/hubble-device-sdk

slc configuration --sdk $SDK
slc signature trust --sdk $SDK
slc signature trust -extpath $SDK/extension/hubble-device-sdk
```

## Building and Running

### Simplicity Studio

> [!NOTE]
> This flow has been tested with Simplicity Studio 6.

If you prefer Simplicity Studio and already have the tools and dependencies
installed, then do:

```sh
slc generate -tlcn gcc -np -p ble_beacon.slcp \
    -d /location/to/your/simplicity-studio/workspace/ble-beacon --with brd4187c
```

Then in Simplicity Studio go to **PROJECTS > Open Project(s)**,
select the location you just generated `ble-beacon` directory,
then build and flash like you normally would.

### Command-line

Validate the project resolves against your SDK and board:

```sh
slc validate-project -p ble_beacon.slcp --with brd4187c
```

Generate a Makefile project:

```sh
slc generate -tlcn gcc -o makefile -cp -p ble_beacon.slcp \
    -d generated_sample_app --with brd4187c
```

Build it:

```sh
cd generated_sample_app
make -f ble_beacon.Makefile
```

Flash it:

```sh
commander flash build/debug/ble_beacon.s37
```

After flashing, the device will start advertising as a Hubble BLE beacon, and
log to the virtual COM port at 115200 baud.

## Testing

The `scan.py` tool can be used to test the BLE beacon. The script scans for
BLE devices, and when it finds a Hubble Network beacon, it attempts to decode
the advertisement data using the provided master key.

### Running the sample and testing it

To run the sample and test it, you need to provide the same master key that was provisioned into the device.

```sh
<SDK_ROOT>/tools/scan.py -k "your_b64_key"
```

## Troubleshooting

### `slt`, `slc` or `commander` not found

The Silicon Labs Tool (SLT) installs everything under `~/.silabs`, but does not
add the tools to `PATH`. Add the ones this sample needs to your shell profile:

| Tool | macOS | Linux |
| --- | --- | --- |
| `slt` | `/Applications/SimplicityInstaller/app/Contents/Resources` | `SimplicityInstaller/Resources` |
| `commander` | `~/.silabs/slt/installs/archive/Commander.app/Contents/MacOS` | `~/.silabs/slt/installs/archive/commander` |

SDKs and SDK extensions always live under `~/.silabs/slt/installs/conan/p`.
Other packages land in either `archive` or `conan/p`, depending on the package
type. See
[Useful paths](https://docs.silabs.com/ssv6ug/6.2.0/ssv6-tricks-tips/#useful-paths)
for the full list, including Windows.

### Locating the Simplicity SDK

Both `slc configuration --sdk` and the extension symlink described in
[Installing the SDK extension](#installing-the-sdk-extension) need the SDK path:

```sh
slt locate simplicity-sdk
```

### Installing missing tools

If `commander` or `slc` are not installed, add them with SLT:

```sh
slt install commander
slt install slc-cli
```

> [!WARNING]
> Install `slc-cli`, not `slc_cli`. Silicon Labs documents the latter as out of
> date.

For recipe-based installs, which pin a whole toolchain at once through a
`pkg.slt` file, see
[First-time installation setup](https://docs.silabs.com/simplicity-installer-slt/latest/slt-getting-started-slt-cli/first-time-installation-setup).
