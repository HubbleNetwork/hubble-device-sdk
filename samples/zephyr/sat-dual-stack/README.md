# Hubble Satellite Dual-Stack on Zephyr

This sample demonstrates running the **Hubble Terrestrial (BLE) Network** and the
**Hubble Satellite Network** together on a single Zephyr device.

## Overview

The application:

1. Provisions the device over a BLE GATT service (Unix epoch time, satellite
   orbital parameters and device location).
2. Schedules a timer for the next satellite pass using the SDK's pass
   prediction APIs.
3. Advertises Hubble beacon packets over Bluetooth while it waits.
4. When the timer expires, stops advertising and transmits to the satellite.

## Requirements

- A cryptographic key provided by Hubble Network.
- A board with Bluetooth LE support. Satellite-capable boards transmit for
  real; other boards mock the satellite radio (see
  `CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT`).

## Configuration

Options live under *"Hubble Network Dual Stack Sample options"* in `menuconfig`:

| Option                                    | Default | Description                                                                                  |
| ----------------------------------------- | ------- | -------------------------------------------------------------------------------------------- |
| `CONFIG_HUBBLE_DEVICE_KEY`                | `""`    | Hubble device cryptographic key, base64-encoded.                                             |
| `CONFIG_HUBBLE_SAMPLE_DEBUG`              | `n`     | Schedule the satellite transmission 120 s after boot instead of waiting for the next pass.   |
| `CONFIG_SAMPLE_PROVIDE_SAT_BOARD_SUPPORT` | `y`     | Provide mock satellite board APIs. Disable on boards that implement the real satellite radio.|

## Building

```sh
west build -b <board> samples/zephyr/sat-dual-stack \
    -- -DCONFIG_HUBBLE_DEVICE_KEY=\"<your-base64-key>\"
west flash
```

## Provisioning

On boot the device starts a connectable advertisement named **"Hubble-Zephyr"**
(UUID `0xFCA7`) and waits for provisioning. A companion app, or provided script,
connects and writes, to the provisioning characteristic, a 2-byte command header
followed by a payload:

| Command                | Header (bytes) | Payload                                              |
| ---------------------- | -------------- | ---------------------------------------------------- |
| Set UTC time           | `0x01 0x02`    | `uint64_t` Unix time in milliseconds (little-endian) |
| Add orbital parameters | `0x01 0x03`    | 76-byte packed `hubble_sat_orbital_params`           |
| Set device location    | `0x01 0x04`    | 16 bytes for latitude(double) and longitude(double)  |

The orbital-parameters payload is packed as:

```text
offset  size  field
  0      8    t0            (uint64)
  8      8    n0            (double)
 16      8    ndot          (double)
 24      8    raan0         (double)
 32      8    raandot       (double)
 40      8    aop0          (double)
 48      8    aopdot        (double)
 56      8    inclination   (double)
 64      8    eccentricity  (double)
 72      4    satellite_id  (uint32)
```

Up to 6 satellites may be provisioned. Once the time is set and the peer
disconnects, the device initializes the Hubble stack and enters the
beacon / satellite-pass loop.

Install the Python dependencies for the *dual-stack-companion.py* provisioning
script:

```bash
pip install -r ../../../tools/requirements-companion.txt
```

Then set your Hubble API token and run the script:

```bash
export HUBBLE_API_TOKEN=<your-hubble-api-token>

python ../../../tools/dual-stack-companion.py
```

By default the device location is determined via IP geolocation. To provision an
explicit location, pass the latitude and longitude (in degrees) with `--location`:

```bash
python ../../../tools/dual-stack-companion.py --location <lat> <lon>
```

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
