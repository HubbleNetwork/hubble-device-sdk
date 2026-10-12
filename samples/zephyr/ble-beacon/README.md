# Hubble Network BLE Beacon Sample

This sample application demonstrates how to use the Hubble Device SDK to
create a BLE beacon that advertises its presence.

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  (optional), for testing with [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- A [Zephyr west workspace](../../../docs/quickstart/zephyr/index.rst) that includes the Hubble Device SDK.
- A board with Bluetooth LE support.
- A computer with Bluetooth LE that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- The Hubble Connect app for
  [iOS](https://apps.apple.com/us/app/hubble-connect/id6751236191) or
  [Android](https://play.google.com/store/apps/details?id=com.hubble.connect)
  (optional), for the backend test.
- To keep the key out of the firmware, see
  [Using a key stored in the KMU](#using-a-key-stored-in-the-kmu). That mode
  requires an nRF54L15 device and the nRF Connect SDK.

## Overview

The beacon advertises data that can be picked up by the Hubble Network. The
advertised data is generated using the `hubble_ble_advertise_get()` function
from the Hubble BLE library.

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/zephyr/ble-beacon`

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

Install the [pyhubblenetwork](https://hubble.com/docs/guides/cli) tool for testing:

```sh
pip install pyhubblenetwork
```

## BLE Advertising Parameters

The sample advertises with the following parameters by default. To change one,
edit the macro or option in the *Set by* column and rebuild.

| Parameter | Value | Set by |
| --- | --- | --- |
| Beacon interval | 1000–1200 ms | `ADV_INTERVAL_MIN_MS` / `ADV_INTERVAL_MAX_MS` in [src/main.c](src/main.c) |
| Time sync interval (connectable) | 100–150 ms | GAP fast interval `BT_GAP_ADV_FAST_INT_MIN_2` / `BT_GAP_ADV_FAST_INT_MAX_2` in [src/main.c](src/main.c) |
| Tx power | 0 dBm | `CONFIG_BT_CTLR_TX_PWR_0` in [prj.conf](prj.conf) |

## Configurations

This sample exposes the following options (via `west build -t menuconfig`, under
"Hubble Network Beacon Sample options"):

| Option | Default | Description |
| --- | --- | --- |
| `CONFIG_HUBBLE_BEACON_SAMPLE_ADDITIONAL_ADV` | `n` | Include additional custom service data in the advertisement. |
| `CONFIG_HUBBLE_BEACON_SAMPLE_ADDITIONAL_ADV_UUID` | `0xefbe` | The UUID for the additional service data. |
| `CONFIG_HUBBLE_BEACON_SAMPLE_ADDITIONAL_ADV_DATA` | `0xefbeadde` | The data for the additional service. |
| `CONFIG_HUBBLE_BEACON_SAMPLE_USE_CTS` | `n` | Sync Unix time at runtime over the *Current Time Service (CTS)* instead of embedding it at build time. |
| `CONFIG_HUBBLE_BEACON_SAMPLE_UPDATE_ADV_PERIOD` | `360` | Period in seconds to generate a new advertisement. |

> [!TIP]
> The application can be configured to sync time at runtime using *Current time Service (CTS)*,
> this option is recommended for long running tests because it is possible to sync time after
> reboots without changing the firmware. To enable this option add the following option
> in `prj.conf`
>
> ```text
> CONFIG_HUBBLE_BEACON_SAMPLE_USE_CTS=y
> ```

## Build-time Provisioning

Create a file, `master.key`, and paste your Hubble device key into it. The key
must be registered as **AES-256** with the **Unix time** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).

The `embed_key_time.py` script takes the key file and embeds it along with the
current timestamp into the source code (`src/key.c` and `src/time.c`).

**For a raw key file:**

```sh
python ../../../tools/embed_key_time.py master.key -o src/
```

**For a base64-encoded key file:**

Use the `-b` or `--base64` flag:

```sh
python ../../../tools/embed_key_time.py -b master.key -o src/
```

After running the script, the key and timestamp will be compiled into the application.

### Using a key stored in the KMU

Instead of embedding the key material in the firmware, the SDK can reference
the key by its PSA key identifier. When
`CONFIG_HUBBLE_NETWORK_CRYPTO_PSA_USE_KEY_ID` is enabled, the SDK never needs
nor has access to the key material: the key is provisioned into the Key
Management Unit (KMU) and every cryptographic operation is performed by CRACEN
on the key it holds internally, so the key never leaves the secure storage.

This mode requires:

- An nRF54L15 device, for example `nrf54l15dk/nrf54l15/cpuapp`. The KMU and the
  CRACEN cryptographic accelerator are only available on this family.
- The nRF Connect SDK, which provides the CRACEN PSA driver.
- `nrfutil` with the `device` command, used to provision the key.

#### 1. Generate the key attributes

The `ncs-generate-key.py` script describes the key for `nrfutil`: it takes the
base64-encoded key and writes a JSON file with the key material and its PSA
attributes (AES-CMAC, sign usage, PROTECTED usage scheme).

```sh
python ../../../tools/ncs-generate-key.py --id 1 --key "$(cat master.key)" --file keyslot.json
```

The sample expects the key in **KMU slot 1**. `src/key_id.c` builds the key
identifier with `PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(
CRACEN_KMU_KEY_USAGE_SCHEME_PROTECTED, PSA_KEY_ID_USER_MIN)`, and
`PSA_KEY_ID_USER_MIN` is slot 1.

> [!NOTE]
> A KMU slot holds 128 bits of key material, so a 256-bit key is stored across
> slots 1 and 2. Pass `--key-bits 128` for a 128-bit key, and make sure the
> size matches the one selected with `CONFIG_HUBBLE_NETWORK_KEY_256` or
> `CONFIG_HUBBLE_NETWORK_KEY_128`.

Run `python ../../../tools/ncs-generate-key.py --help` for the remaining
options, such as `--persistence` to control whether the slot can be reused,
revoked, or written only once.

#### 2. Provision the key

```sh
nrfutil device x-provision-keys --key-file keyslot.json
```

#### 3. Build the sample

The `ncs_cracen.conf` fragment selects the CRACEN driver and enables the key id
support in the SDK:

```sh
west build -b nrf54l15dk/nrf54l15/cpuapp -- -DEXTRA_CONF_FILE=ncs_cracen.conf
```

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

After flashing, the device will start advertising as a Hubble BLE beacon.

> [!WARNING]
> If the application was built with `CONFIG_HUBBLE_BEACON_SAMPLE_USE_CTS` enabled, it is necessary
> to sync time. This can be done using the `sync_time.py` script:
>
> ```sh
> pip install bleak
> python sync_time.py
> ```

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

#### Ingesting Data into Hubble Network

The tool can also ingest the scanned data into the Hubble Network. To do
this, set the `HUBBLE_API_TOKEN` and `HUBBLE_ORG_ID` environment variables and
add the `--ingest` flag.

```sh
export HUBBLE_API_TOKEN=<your-hubble-api-token>
export HUBBLE_ORG_ID=<your-hubble-org-id>

hubblenetwork ble scan --key "<your base64 device key>" --ingest
```

Correctly decoded packets will stream to the console and appear on
[Hubble Dash](https://dash.hubble.com/dashboard).

### Backend Test

Download the Hubble Connect app to your smartphone and sign in, then keep the
phone near the device with the app open. The phone acts as a scanner: the
device appears in the app, and its packets appear on
[Hubble Dash](https://dash.hubble.com/dashboard).
