# Hubble Network BLE Beacon + Manufacturer Data Sample

This sample demonstrates how to emit a **single** BLE advertisement that carries
both the standard Hubble beacon payload **and** an application-owned
*manufacturer specific data* field. The manufacturer field holds a 4-byte
counter that is incremented every 5 minutes.

Use this as a starting point when you need to attach your own data to a Hubble
advertisement without sending it through the (encrypted) Hubble payload.

The sample uses the **device uptime** counter source
(`CONFIG_HUBBLE_COUNTER_SOURCE_DEVICE_UPTIME`), so it does not need UTC time
provisioned: the EID counter starts at 0 and advances with device uptime. Only
the master key has to be embedded before building.

## Advertisement layout

The advertisement is built from three AD structures:

| # | AD type | Contents |
|---|---------|----------|
| 1 | `BT_DATA_UUID16_ALL` | `0xFCA6` (the Hubble service UUID) |
| 2 | `BT_DATA_SVC_DATA16` | Encrypted Hubble payload from `hubble_ble_advertise_get()` |
| 3 | `BT_DATA_MANUFACTURER_DATA` | `0xFCA6` (2B, little-endian) + counter (4B, little-endian) |

By BLE convention, manufacturer data begins with a 2-byte company identifier.
This sample reuses Hubble's `0xFCA6` identifier as that prefix so scanners can
recognize the field.

> [!NOTE]
> The counter is held in RAM and resets to 0 on reboot. Persisting it across
> reboots is out of scope for this sample.

## Size budget

Legacy BLE advertising allows 31 payload bytes. This sample passes no Hubble
user payload (`NULL, 0`), so the Hubble service data is 12 bytes and the whole
advertisement is about 26 bytes. If you add Hubble user payload (up to 13
bytes), it shares the same 31-byte budget with the manufacturer field, so you
cannot max out both at once.

## Requirements

- A cryptographic key provided by Hubble Network.

## Building and running

Pass your base64 master key (the string Hubble gave you, already base64) on the
build command line with `-DHUBBLE_KEY`. It is decoded into a byte array at build
time, so no base64 code runs on the device and no pre-build script is needed.

```sh
west build -b nrf52840dk/nrf52840 . -- -DHUBBLE_KEY="<your base64 key>"
west flash
```

After flashing, the device advertises as a Hubble beacon with the extra
manufacturer-data counter, refreshing every 5 minutes.

> [!NOTE]
> The decoded key length must match `CONFIG_HUBBLE_KEY_SIZE` (32 bytes for a
> 256-bit key, 16 for 128-bit). A wrong-size key fails the build with a clear
> message; if `-DHUBBLE_KEY` is omitted the build uses an all-zero placeholder
> (with a warning) so the sample still compiles in CI.

> [!TIP]
> `-DHUBBLE_KEY` is cached in the build directory, so subsequent incremental
> `west build` runs reuse it without repeating the flag. Pass it again (or use
> `-p always`) to change the key.

## Configuration

- `-DHUBBLE_KEY`: the base64-encoded Hubble master key, passed at build time and
  decoded into the firmware (see above).
- `CONFIG_HUBBLE_MFG_DATA_SAMPLE_UPDATE_PERIOD`: period in seconds at which the
  advertisement is refreshed and the counter is incremented (default `300`).
