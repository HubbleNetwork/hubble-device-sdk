# Hubble Satellite Dual-Stack on Zephyr

This sample demonstrates running the **Hubble Terrestrial (BLE) Network** and the
**Hubble Satellite Network** together on a single Zephyr device.

## Overview

The application:

1. Provisions the device over a BLE GATT service (Unix time, satellite
   orbital parameters and device location).
2. Schedules a timer for the next satellite pass using the SDK's pass
   prediction APIs.
3. Advertises Hubble beacon packets over Bluetooth while it waits.
4. When the timer expires, stops advertising and transmits to the satellite.

## Requirements

- A cryptographic key provided by Hubble Network.
- A Hubble API token, for the provisioning script. See the
  [Hubble Platform API documentation](https://hubble.com/docs/api-specification/hubble-platform-api#generate-an-api-key).
- A board with Bluetooth LE support. Satellite-capable boards transmit for
  real; other boards mock the satellite radio (see
  `CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT`).
- The prebuilt radio blob for your target (see [Radio blobs](#radio-blobs)).

## Radio blobs

The satellite radio libraries are **not** fetched by `west update`. Pull them in
once after setting up the workspace:

```sh
west blobs fetch hubblenetwork-sdk
```

Silicon Labs targets use RAIL, which needs to be fetched along with
SiLabs HAL:

```sh
west blobs fetch hal_silabs
```

## Configuration

Options live under *"Hubble Network Dual Stack Sample options"* in `menuconfig`:

| Option                                    | Default | Description                                                                                  |
| ----------------------------------------- | ------- | -------------------------------------------------------------------------------------------- |
| `CONFIG_HUBBLE_DEVICE_KEY`                | `""`    | Hubble device cryptographic key, base64-encoded.                                             |
| `CONFIG_HUBBLE_SAMPLE_DEBUG`              | `n`     | Schedule the satellite transmission 120 s after boot instead of waiting for the next pass.   |
| `CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT` | `n`     | Provide mock satellite board APIs. Enable on boards that do not implement the real satellite radio.|

## Advertising parameters

The sample advertises with the following parameters by default. They can be modified.

| Parameter | Value | Set by |
| --- | --- | --- |
| Beacon interval | 1000–1200 ms | `ADV_INTERVAL_MIN_MS` / `ADV_INTERVAL_MAX_MS` in [src/app_ble.c](src/app_ble.c) |
| Provisioning interval (connectable) | 100–150 ms | GAP fast interval `BT_GAP_ADV_FAST_INT_MIN_2` / `BT_GAP_ADV_FAST_INT_MAX_2` in [src/app_ble.c](src/app_ble.c) |
| Tx power | 0 dBm | `CONFIG_BT_CTLR_TX_PWR_0` in [prj.conf](prj.conf) |

## Building

```sh
west build -b <board> samples/zephyr/sat-dual-stack \
    -- -DCONFIG_HUBBLE_DEVICE_KEY=\"<your-base64-key>\"
west flash
```

> [!NOTE]
> When building with nRF Connect SDK (NCS) and the SoftDevice Bluetooth
> controller, apply the `ncs-dual-stack` snippet provided by the SDK:
>
> ```sh
> west build -b nrf54l15dk/nrf54l15/cpuapp samples/zephyr/sat-dual-stack \
>     -S ncs-dual-stack \
>     -- -DCONFIG_HUBBLE_DEVICE_KEY=\"<your-base64-key>\"
> ```
>
> The snippet enables MPSL with a timeslot session so the satellite radio can
> coexist with the SoftDevice Bluetooth controller. It lives in
> `port/zephyr/snippets/ncs-dualstack` and is discoverable on any board, so the
> same invocation works for other NCS targets.

## Provisioning

On boot, the device starts a connectable BLE advertisement named **"Hubble-Zephyr"**
and waits for provisioning data. Use `dual-stack-companion.py` to push the current Unix time,
device location, and orbital parameters for the target satellites.

See [companion tool documentation](https://github.com/HubbleNetwork/hubble-device-sdk/blob/main/docs/satellite/companion-tool.rst)
for more information and instruction.

Install Python dependencies for the *dual-stack-companion.py* provisioning script:

```bash
pip install -r tools/requirements-companion.txt
```

### Windows

```ps1
$env:HUBBLE_API_TOKEN = "<your-hubble-api-token>"

python tools/dual-stack-companion.py
```

### Linux & macOS

```sh
export HUBBLE_API_TOKEN=<your-hubble-api-token>

python tools/dual-stack-companion.py
```

A successful run ends with `Provisioned device successfully`.

> [!NOTE]
> The device keeps the time, location and orbital parameters in RAM. It does not persist.
> Run the provisioning script again after every power cycle or reset.


## Program Flow

```text
                power on / reset
                       |
                       v
       +-------------------------------+
       |  Enable BLE, start            |
       |  connectable provisioning adv |
       +-------------------------------+
                       |
                       v
       +-------------------------------+
       |  GATT writes: time + location |
       |  orbital params, disconnect   |
       +-------------------------------+
                       |
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
