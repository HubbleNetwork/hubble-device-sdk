# Hubble Network Satellite Continuous Sample

This sample continuously builds Hubble satellite packets and transmits them in a
loop. Use it as a starting point for satellite (space) connectivity on a
supported radio.

## Requirements

- A [Hubble device key](https://hubble.com/docs/guides/cloud-integration/register-devices).
- A [Hubble API token](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key),
  (optional), for testing with [pyhubblenetwork](https://hubble.com/docs/guides/cli) commands.
- A [Zephyr west workspace](../../../docs/quickstart/zephyr/index.rst) that includes the Hubble Device SDK.
- A supported satellite-capable board. The satellite link needs [at least +20 dBm](../../../docs/integration_guides/common/prerequisites.rst).
  - `nrf54l15dk/nrf54l15/cpuapp` (Nordic nRF54L15, needs an external amplifier)
  - `nrf21540dk/nrf52840` (Nordic nRF52840 + FEM)
  - `thingy53/nrf5340/cpunet` (Nordic nRF5340 net core + FEM)
  - `xg24_rb4187c` (Silicon Labs xG24)
  - `xg26_rb4121a` (Silicon Labs xG26)
  - `xiao_mg24` (Seeed XIAO MG24)
- A computer that meets the
  [pyhubblenetwork requirements](https://hubble.com/docs/guides/cli#requirements).
- [pyhubblenetwork](https://hubble.com/docs/guides/cli) (optional), for testing.
- [Satellite PHY Blob](#satellite-phy-blob).
- A software defined radio (SDR) for validation (suggested, but not required).

## Overview

Once running, the device initializes the Hubble Device SDK and then enters a
loop that builds and transmits a satellite packet every
`CONFIG_SAMPLE_SAT_TX_INTERVAL_SECONDS` seconds.

> [!NOTE]
> All commands in this README are run from the sample folder containing this file:
> `<SDK path>/samples/zephyr/sat-continuous`

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

### Satellite PHY Blob

The satellite radio implementation for Nordic SoCs ships as a prebuilt static
library (a "blob") rather than source. These are **not** fetched by
`west update`; you must pull them in explicitly once after setting up the
workspace:

```sh
west blobs fetch hubblenetwork-sdk
```

This downloads the satellite libraries for the nRF52/nRF53/nRF54 families into
the module's `zephyr/blobs/` directory. If the blob for your target is missing,
the build fails to link the satellite radio. Run `west blobs list
hubblenetwork-sdk` to see what is available and whether it has been fetched.

Silicon Labs boards use the in-tree Gecko custom radio PHY
(`CONFIG_SOC_GECKO_CUSTOM_RADIO_PHY`), but that PHY links against Silicon Labs'
RAIL library, which is itself distributed as a blob. Fetch it once before
building for those boards:

```sh
west blobs fetch hal_silabs
```

## Configurations

This sample exposes the following options (via `west build -t menuconfig`, under
"Hubble Network Satellite Sample options"):

| Option | Default | Description |
| --- | --- | --- |
| `CONFIG_SAMPLE_HUBBLE_KEY` | `""` | Hubble device cryptographic key, base64-encoded. |
| `CONFIG_SAMPLE_SAT_TX_INTERVAL_SECONDS` | `10` | Seconds the sample sleeps between consecutive satellite transmissions. |
| `CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT` | `n` | Mock the board radio APIs, so the sample builds on targets without a satellite radio (e.g. `native_sim`). |

Boards that need extra devicetree tweaks supply a `boards/<board>.overlay` (for
example `boards/thingy53_nrf5340_cpunet.overlay`, which hooks the FEM up to the
radio).

> [!NOTE]
> `prj.conf` disables `CONFIG_HUBBLE_NETWORK_SECURITY_ENFORCE_NONCE_CHECK`, which
> persists the sequence counter to NVS and rejects reused or decreasing counters.
> This sample transmits continuously and needs no NVS.

## Build-time Provisioning

Set `CONFIG_SAMPLE_HUBBLE_KEY` to your Hubble device key, in base64. The key
must be registered as **AES-256** with the **Device uptime** counter source; see
[Register devices](https://hubble.com/docs/guides/cloud-integration/register-devices).
Add this config in your build command below.

## Build and Flash

Build for your board, passing your device key. Add `-p` for a pristine build.

```sh
west build -b <board> -- -DCONFIG_SAMPLE_HUBBLE_KEY=\"<your base64 device key>\"
```

e.g.
```sh
west build -b nrf54l15dk/nrf54l15/cpuapp -- -DCONFIG_SAMPLE_HUBBLE_KEY=\"<your base64 device key>\"
```

Flash the board:

```sh
west flash
```

## Test

Capture the packets with an SDR and decode them with your device key. See
[RF Verification with an SDR](../../../docs/integration_guides/common/next-steps.rst#rf-verification-with-an-sdr).

```sh
hubblenetwork sat scan --key "<your base64 device key>"
```

Correctly decoded packets will stream to the console.
