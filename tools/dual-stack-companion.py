#!/usr/bin/env python3
#
# Copyright (c) 2026 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""
Hubble Companion Script

Scans for a Hubble device connects, and writes Unix epoch (ms), orbital parameters,
and device location data.

Usage:
    python dual-stack-companion.py
    python dual-stack-companion.py --label <device_label> --location <lat> <lon>
    python dual-stack-companion.py -v

If --location is omitted, the device location is determined via IP geolocation.

Environment variables (for orbital parameters fetch):
    HUBBLE_API_TOKEN            - Bearer token for the API
    HUBBLE_TARGET_SATELLITE_IDS - Comma-separated sat IDs (optional; default = all)

"""

from __future__ import annotations

import argparse
import asyncio
import logging
import os
import struct
import sys
import time
from dataclasses import dataclass

import geocoder
import httpx
from bleak import BleakClient, BleakScanner

log = logging.getLogger(__name__)

HUBBLE_EPHEMERIS_URL = "https://api.hubble.com/api/satellite/ephemeris"
HUBBLE_SERVICE_UUID = "0000fca7-0000-1000-8000-00805f9b34fb"
HUBBLE_CHR_UUID = "00000005-fca7-4000-8000-00805f9b34fb"
HUBBLE_CMD = 0x01
HUBBLE_CMD_UNIX_EPOCH = 0x02
HUBBLE_CMD_ORBITAL_PARAMS = 0x03
HUBBLE_CMD_DEVICE_LOCATION = 0x04

SCAN_TIMEOUT_S = 30.0
CONNECT_TIMEOUT_S = 15.0
WRITE_TIMEOUT_S = 5.0


@dataclass(slots=True)
class OrbitalParams:
    t0: int  # uint64 unix epoch seconds
    n0: float
    ndot: float
    raan0: float
    raandot: float
    aop0: float
    aopdot: float
    inclination: float
    eccentricity: float
    satellite_id: int  # uint32

    def to_bytes(self) -> bytes:
        """
        Pack as little-endian struct:
        uint64 t0, 8x double, uint32 sat_id.
        """
        return struct.pack(
            "<Q8dI",
            self.t0,
            self.n0,
            self.ndot,
            self.raan0,
            self.raandot,
            self.aop0,
            self.aopdot,
            self.inclination,
            self.eccentricity,
            self.satellite_id,
        )


# =============== Command Framing ===============
def encode_unix_epoch_cmd() -> bytes:
    """
    [HUBBLE_CMD][HUBBLE_CMD_UNIX_EPOCH][uint64 LE ms] = 10 bytes.

    Returns:
        A bytes object containing the encoded unix epoch ms command.
    """
    now_ms = int(time.time() * 1000)
    return bytes([HUBBLE_CMD, HUBBLE_CMD_UNIX_EPOCH]) + struct.pack("<Q", now_ms)


def encode_orb_params_cmd(orb_params: OrbitalParams) -> bytes:
    """
    [HUBBLE_CMD][HUBBLE_CMD_ORBITAL_PARAMS][76-byte struct] = 78 bytes.

    Args:
        orb_params: OrbitalParams instance containing the orbital parameters data to encode.

    Returns:
        A bytes object containing the encoded orbital parameters command.
    """
    return bytes([HUBBLE_CMD, HUBBLE_CMD_ORBITAL_PARAMS]) + orb_params.to_bytes()


def encode_device_location_cmd(lat: float, lon: float) -> bytes:
    """
    [HUBBLE_CMD][HUBBLE_CMD_DEVICE_LOCATION][double LE lat][double LE lon] = 18 bytes.

    Args:
        lat: Device latitude in degrees.
        lon: Device longitude in degrees.

    Returns:
        A bytes object containing the encoded device location command.
    """
    return bytes([HUBBLE_CMD, HUBBLE_CMD_DEVICE_LOCATION]) + struct.pack("<2d", lat, lon)


def resolve_device_location() -> tuple[float, float]:
    """
    Determine the device location via IP geolocation.

    Returns:
        A (lat, lon) tuple in degrees.
    """
    g = geocoder.ip("me")
    if not g.ok or g.latlng is None:
        raise RuntimeError(f"IP geolocation failed: {g.status}")
    lat, lon = g.latlng
    return lat, lon


async def fetch_orb_params() -> list[OrbitalParams]:
    """
    Fetch orbital parameters for target satellites from the Hubble cloud.

    Returns:
        A list of OrbitalParams instances for the target satellites.
    """
    api_token = os.environ.get("HUBBLE_API_TOKEN")
    if not api_token:
        raise RuntimeError(
            "HUBBLE_API_TOKEN env var must be set, e.g. `export HUBBLE_API_TOKEN=your_token`"
        )

    # get the list of sats
    target_ids_str = os.environ.get("HUBBLE_TARGET_SATELLITE_IDS", "").strip()
    target_ids: set[int] = (
        {int(x) for x in target_ids_str.split(",") if x.strip()} if target_ids_str else set()
    )

    async with httpx.AsyncClient(timeout=30.0) as http:
        resp = await http.get(
            HUBBLE_EPHEMERIS_URL, headers={"Authorization": f"Bearer {api_token}"}
        )
        resp.raise_for_status()
        data = resp.json()

    # extract orbital params from the fetched data
    results: list[OrbitalParams] = []
    for sat in data.get("satellites", []):
        sat_id = sat.get("satellite_id")
        if target_ids and sat_id not in target_ids:
            continue
        orb_params = sat.get("orbital_params")
        if orb_params is None:
            log.debug("No orb_params for satellite %s", sat_id)
            continue
        try:
            results.append(
                OrbitalParams(
                    t0=int(orb_params["t0"]),
                    n0=orb_params["n0"],
                    ndot=orb_params["ndot"],
                    raan0=orb_params["raan0"],
                    raandot=orb_params["raandot"],
                    aop0=orb_params["aop0"],
                    aopdot=orb_params["aopdot"],
                    inclination=orb_params["inclination"],
                    eccentricity=orb_params["eccentricity"],
                    satellite_id=sat_id,
                )
            )
        except (KeyError, ValueError) as e:
            log.warning("Failed to parse orbital params for sat %s: %s", sat_id, e)

    log.info("Fetched orbital params for %d satellites", len(results))
    return results


# =============== BLE Ops ===============
async def scan_for_device(label: str | None = None, timeout: float = SCAN_TIMEOUT_S):
    """
    Scan for a Hubble Connectable Device.

    Args:
        label: Optional name filter. If provided, only matches devices whose
                scan-response local_name equals this string.
        timeout: Max time to wait, in seconds.

    Returns:
        The first matching device, or None if no device found before timeout.
    """
    log.info(
        "Scanning for Hubble service %s%s (timeout: %.0fs)...",
        HUBBLE_SERVICE_UUID,
        f", label='{label}'" if label else "",
        timeout,
    )

    found_device = None
    event = asyncio.Event()

    def on_detection(device, adv_data):
        nonlocal found_device

        # Service UUID filter
        if HUBBLE_SERVICE_UUID not in adv_data.service_uuids:
            return

        # Name filter (if provided)
        if label is not None and adv_data.local_name != label:
            return

        found_device = device
        event.set()

    scanner = BleakScanner(detection_callback=on_detection)
    await scanner.start()
    try:
        await asyncio.wait_for(event.wait(), timeout=timeout)
    except asyncio.TimeoutError:
        pass
    finally:
        await scanner.stop()

    return found_device


async def provision(label: str | None, location: tuple[float, float] | None) -> bool:
    """
    Provision a Hubble device by scanning, connecting, and writing orbital params,
    device location, and UTC time.

    Args:
        label: Optional device name filter for scanning.
        location: Optional (lat, lon) tuple. If None, resolved via IP geolocation.
    """

    # Fetch orbital params first so we can fail fast
    orb_params_list: list[OrbitalParams] = []
    try:
        orb_params_list = await fetch_orb_params()
    except Exception as e:
        log.error("Failed to fetch orbital params: %s", e)
        return False

    # Resolve device location (fail fast before connecting)
    try:
        lat, lon = location if location is not None else resolve_device_location()
    except Exception as e:
        log.error("Failed to resolve device location: %s", e)
        return False

    log.info("Device location: lat=%f, lon=%f", lat, lon)

    device = await scan_for_device(label=label)
    if device is None:
        log.error("Device not found")
        return False

    addr = device.address
    log.info("Found device at %s", addr)

    try:
        async with BleakClient(addr, timeout=CONNECT_TIMEOUT_S) as client:
            log.info("Connected. MTU: %d", client.mtu_size)

            # Write all orbital params entries first, then UTC last.
            for i, orb_params in enumerate(orb_params_list):
                cmd = encode_orb_params_cmd(orb_params)
                log.info(
                    "Writing orbital params %d/%d (sat %d, %d bytes)",
                    i + 1,
                    len(orb_params_list),
                    orb_params.satellite_id,
                    len(cmd),
                )

                await asyncio.wait_for(
                    client.write_gatt_char(HUBBLE_CHR_UUID, cmd, response=True),
                    timeout=WRITE_TIMEOUT_S,
                )

            loc_cmd = encode_device_location_cmd(lat, lon)
            log.info("Writing device location (%d bytes)", len(loc_cmd))
            await asyncio.wait_for(
                client.write_gatt_char(HUBBLE_CHR_UUID, loc_cmd, response=True),
                timeout=WRITE_TIMEOUT_S,
            )

            unix_cmd = encode_unix_epoch_cmd()
            log.info("Writing UNIX epoch (%d bytes)", len(unix_cmd))
            await asyncio.wait_for(
                client.write_gatt_char(HUBBLE_CHR_UUID, unix_cmd, response=True),
                timeout=WRITE_TIMEOUT_S,
            )

            log.info("Provisioning complete; disconnecting...")

    except asyncio.TimeoutError:
        log.error("Timeout during provisioning")
        return False
    except Exception as e:
        log.exception("Provisioning failed: %s", e)
        return False

    log.info("Provisioned device successfully")
    return True


def main():
    parser = argparse.ArgumentParser(description="BLE provisioning utility for Hubble devices")
    parser.add_argument(
        "--label",
        default=None,
        help="Optional name filter",
    )
    parser.add_argument(
        "--location",
        type=float,
        nargs=2,
        metavar=("LAT", "LON"),
        default=None,
        help="Device location as latitude and longitude in degrees (default: IP geolocation)",
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="Enable debug logging")
    args = parser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(asctime)s %(levelname)-8s %(name)s: %(message)s",
    )

    location = tuple(args.location) if args.location is not None else None
    success = asyncio.run(provision(args.label, location))
    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
