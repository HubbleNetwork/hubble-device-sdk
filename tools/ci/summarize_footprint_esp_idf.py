#!/usr/bin/env python3
#
# Copyright (c) 2026 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""
Summarize RAM and ROM per ESP-IDF sample/target.

Usage:
    summarize_footprint_esp_idf.py <artifacts_dir>
        [--revision REVISION] [--idf-version IDF_VERSION] [--format FORMAT]

Note:
    Expects JSONs named:
        size-components-<sample>-<target>.json
        size-files-<sample>-<target>.json
    <sample> may contain hyphens; <target> never does.
"""

import argparse
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path


SAMPLE_COLUMN_SIZE = 30
TARGET_COLUMN_SIZE = 25
ROM_COLUMN_SIZE = 15
RAM_COLUMN_SIZE = 15
TOTAL_COLUMN_SIZE = (
    SAMPLE_COLUMN_SIZE + TARGET_COLUMN_SIZE + ROM_COLUMN_SIZE + RAM_COLUMN_SIZE
)

# TODO: find a better way for this
# but ESP-IDF emits the individual RAM and ROM name, so we have
# the allow list here (would get out of hand pretty quick though)
ROM_MEMORY_TYPES = ("Flash Code", "Flash Data")
RAM_MEMORY_TYPES = ("DIRAM", "DRAM", "IRAM", "LP SRAM", "RTC SLOW", "RTC FAST")

# Port directories that contributes to esp-idf
PORT_DIRS = (
    "port/esp-idf",
    "port/freertos",
)

SDK_COMPONENT_ARCHIVE = "libhubblenetwork-sdk.a"
SIZE_COMPONENTS_FROM_FILENAME = re.compile(r"size-components-(.+)-([^-]+)\.json$")


def format_size(size_bytes):
    """Format size in bytes to human-readable format."""
    if size_bytes is None or size_bytes < 0:
        return "N/A"
    for unit in ["B", "KB", "MB"]:
        if size_bytes < 1024.0:
            return f"{size_bytes:.2f} {unit}"
        size_bytes /= 1024.0
    return f"{size_bytes:.2f} GB"


def memory_size_extract(memory_types):
    """Return (rom, ram) bytes from a memory_types dict."""
    rom = sum(memory_types.get(k, {}).get("size", 0) for k in ROM_MEMORY_TYPES)
    ram = sum(memory_types.get(k, {}).get("size", 0) for k in RAM_MEMORY_TYPES)
    return rom, ram


def sdk_total_extract(size_data):
    """
    Sum ROM and RAM for hubblenetwork-sdk component.
    2 cases:
    - collect the component size it self in size-components
    - collect the port size in size-files
    """
    rom_total = 0
    ram_total = 0
    matched = False
    for path, entry in size_data.items():
        is_archive_level = entry.get("abbrev_name") == SDK_COMPONENT_ARCHIVE
        is_file_in_sdk = SDK_COMPONENT_ARCHIVE in path
        if not (is_archive_level or is_file_in_sdk):
            continue
        rom, ram = memory_size_extract(entry.get("memory_types", {}))
        rom_total += rom
        ram_total += ram
        matched = True

        # An archive-level entry already aggregates everything.
        if is_archive_level:
            return rom_total, ram_total
    if not matched:
        return None, None
    return rom_total, ram_total


def port_file_names_collect(port_dirs):
    """
    Collect all the .c files under the port
    """
    result = {}
    for port_dir in port_dirs:
        path = Path(port_dir)
        if not path.is_dir():
            continue
        basenames = {c_file.stem + ".c.obj" for c_file in path.rglob("*.c")}
        if basenames:
            result[port_dir] = basenames
    return result


def port_footprints_extract(size_data, port_file_names_by_label):
    """
    Pulling ROM and RAM from the port files and sum them up.
    Returns a list of (port_label, rom, ram)
    """
    results = []
    for label, file_names in port_file_names_by_label.items():
        rom_total = 0
        ram_total = 0
        for _, entry in size_data.items():
            if entry.get("abbrev_name") not in file_names:
                continue
            rom, ram = memory_size_extract(entry.get("memory_types", {}))
            rom_total += rom
            ram_total += ram
        results.append((label, rom_total, ram_total))
    return results


def json_load(path):
    """Load JSON with error checking"""
    try:
        with open(path, "r") as f:
            return json.load(f)
    except (json.JSONDecodeError, OSError) as e:
        print(f"Warning: could not read {path}: {e}", file=sys.stderr)
        return None


def entries_collect(artifacts_dir, port_file_names_by_label):
    """
    Parse artifacts (json files) and extract entries for the report
    (sample, target, rom, ram, ports)
    """
    entries = []
    artifacts_path = Path(artifacts_dir)
    for components_path in sorted(artifacts_path.rglob("size-components-*.json")):
        match = SIZE_COMPONENTS_FROM_FILENAME.search(components_path.name)
        if not match:
            print(
                f"Warning: skipping unrecognized filename: {components_path.name}",
                file=sys.stderr,
            )
            continue
        sample, target = match.group(1), match.group(2)

        # Total SDK footprint comes from size-components
        components_data = json_load(components_path) or {}
        rom, ram = sdk_total_extract(components_data)

        # Port files breakdown comes from size-files
        ports = []
        files_path = components_path.parent / f"size-files-{sample}-{target}.json"
        if files_path.exists():
            files_data = json_load(files_path)
            if files_data:
                ports = port_footprints_extract(files_data, port_file_names_by_label)

        entries.append((sample, target, rom, ram, ports))
    return entries


def format_table(entries, revision, idf_version, run_date):
    """Output as plain-text table"""
    print()
    print("hubblenetwork-sdk Footprint Summary")
    print("=" * TOTAL_COLUMN_SIZE)
    print(f"Archive:         {SDK_COMPONENT_ARCHIVE}")
    print(f"Revision:        {revision}")
    print(f"ESP-IDF Version: {idf_version}")
    print(f"Run Date:        {run_date}")
    print("=" * TOTAL_COLUMN_SIZE)
    print(
        f"{'Sample':<{SAMPLE_COLUMN_SIZE}}{'Target':<{TARGET_COLUMN_SIZE}}"
        f"{'ROM':<{ROM_COLUMN_SIZE}}{'RAM':<{RAM_COLUMN_SIZE}}"
    )
    print("-" * TOTAL_COLUMN_SIZE)
    for sample, target, rom, ram, ports in entries:
        print(f"{sample:<{SAMPLE_COLUMN_SIZE}}{target:<{TARGET_COLUMN_SIZE}}")

        indent = " " * SAMPLE_COLUMN_SIZE
        print(
            f"{indent}{'Hubblenetwork-SDK:':<{TARGET_COLUMN_SIZE}}"
            f"{format_size(rom):<{ROM_COLUMN_SIZE}}"
            f"{format_size(ram):<{RAM_COLUMN_SIZE}}"
        )

        # Per-port sub-rows under Hubblenetwork-SDK
        for label, port_rom, port_ram in ports:
            print(
                f"{indent}{'    ' + label + ':':<{TARGET_COLUMN_SIZE}}"
                f"{format_size(port_rom):<{ROM_COLUMN_SIZE}}"
                f"{format_size(port_ram):<{RAM_COLUMN_SIZE}}"
            )
    print("-" * TOTAL_COLUMN_SIZE)
    print(f"\nSummary: {len(entries)} build(s) processed")
    print("=" * TOTAL_COLUMN_SIZE)


def format_csv(entries, revision, idf_version):
    """
    Output CSV
    """
    print(
        "Sample,Target,Component,"
        "ROM (bytes),RAM (bytes),"
        "ROM (formatted),RAM (formatted),"
        "Revision,ESP-IDF Version"
    )

    def s(v):
        return str(v) if v is not None else "N/A"

    for sample, target, rom, ram, ports in entries:
        print(
            f"{sample},{target},total,"
            f"{s(rom)},{s(ram)},"
            f"{format_size(rom)},{format_size(ram)},"
            f"{revision},{idf_version}"
        )
        for label, port_rom, port_ram in ports:
            print(
                f"{sample},{target},{label},"
                f"{s(port_rom)},{s(port_ram)},"
                f"{format_size(port_rom)},{format_size(port_ram)},"
                f"{revision},{idf_version}"
            )


def format_markdown(entries, revision, idf_version, run_date):
    """
    Output markdown for $GITHUB_STEP_SUMMARY
    """
    print("## hubblenetwork-sdk Footprint Summary")
    print()
    print(f"- **Archive:** `{SDK_COMPONENT_ARCHIVE}`")
    print(f"- **Revision:** `{revision}`")
    print(f"- **ESP-IDF Version:** {idf_version}")
    print(f"- **Run Date:** {run_date}")
    print()
    print("| Sample | Target | Component | ROM | RAM |")
    print("|---|---|---|---|---|")
    for sample, target, rom, ram, ports in entries:
        print(
            f"| {sample} | {target} | **total** "
            f"| {format_size(rom)} | {format_size(ram)} |"
        )
        for label, port_rom, port_ram in ports:
            print(
                f"| {sample} | {target} | {label} "
                f"| {format_size(port_rom)} | {format_size(port_ram)} |"
            )


def main():
    parser = argparse.ArgumentParser(
        description="Summarize hubblenetwork-sdk RAM/ROM from idf.py size output"
    )
    parser.add_argument(
        "artifacts_dir",
        help="Directory containing size-components-<sample>-<target>.json "
        "and size-files-<sample>-<target>.json files",
    )
    parser.add_argument(
        "--revision",
        default="unknown",
        help="SDK Git revision (short SHA) to print in the header",
    )
    parser.add_argument(
        "--idf-version",
        default="unknown",
        help="ESP-IDF version string to print in the header",
    )
    parser.add_argument(
        "--format",
        choices=["table", "csv", "markdown"],
        default="table",
        help="Output format (default: table)",
    )
    args = parser.parse_args()

    artifacts_path = Path(args.artifacts_dir)
    if not artifacts_path.is_dir():
        print(f"Error: directory not found: {args.artifacts_dir}", file=sys.stderr)
        return 1

    port_file_names_by_label = port_file_names_collect(PORT_DIRS)
    if not port_file_names_by_label:
        print(
            f"Warning: no .c files found under any of: {', '.join(PORT_DIRS)}; "
            f"port breakdown will be skipped.",
            file=sys.stderr,
        )

    entries = entries_collect(artifacts_path, port_file_names_by_label)
    if not entries:
        print(
            f"Warning: no size-components-*.json files found under "
            f"{args.artifacts_dir}",
            file=sys.stderr,
        )
        return 0

    run_date = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")

    if args.format == "csv":
        format_csv(entries, args.revision, args.idf_version)
    elif args.format == "markdown":
        format_markdown(entries, args.revision, args.idf_version, run_date)
    else:
        format_table(entries, args.revision, args.idf_version, run_date)
    return 0


if __name__ == "__main__":
    sys.exit(main())
