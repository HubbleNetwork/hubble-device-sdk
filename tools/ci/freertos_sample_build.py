#!/usr/bin/env python3
#
# Copyright (c) 2026 Hubble Network, Inc.
#
# SPDX-License-Identifier: Apache-2.0

"""
Recursive search for sample.yaml files under a directory, and build each sample configuration.

The sample.yaml format is as follows:

.. code-block:: yaml

    sample:
      name: <optional, sample name, defaults to parent directory name>

    builds:
      - id: <build identifier (string), required>
        makefile: <ti: path to makefile, required>
        board: <silabs: board passed to slc generate --with, required>
        extra_args: <optional, list of extra arguments passed to make (ti)
                     or slc generate (silabs)>
"""

from __future__ import annotations

import argparse
import os
import shlex
import subprocess
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path

import yaml


@dataclass
class Build:
    """
    Single build configuration for a sample.
    """

    sample_name: str
    sample_dir: Path
    build_id: str
    makefile: str = ""
    slcp: Path | None = None
    project_name: str = ""
    board: str = ""
    extra_args: list[str] = field(default_factory=list)

    @property
    def label(self) -> str:
        return f"{self.sample_name} / {self.build_id}"


@dataclass
class Sample:
    """
    Container for 1 sample, which may have multiple build configurations.
    """

    name: str
    sample_dir: Path
    builds: list[Build] = field(default_factory=list)


def find_slcp(sample_dir: Path) -> tuple[Path, str]:
    """
    Find the .slcp file in a sample directory and read its project name.
    SLC names the generated makefile after it.

    Args:
        sample_dir: Sample directory to search.

    Returns:
        Tuple of (slcp path, project name).
    """
    slcps = sorted(sample_dir.glob("*.slcp"))
    if len(slcps) != 1:
        raise ValueError(f"{sample_dir}: expected one .slcp file, found {len(slcps)}")

    with slcps[0].open() as f:
        data = yaml.safe_load(f) or {}

    project_name = data.get("project_name")
    if not project_name:
        raise ValueError(f"{slcps[0]}: missing 'project_name'")

    return slcps[0], str(project_name)


def parse_sample_yaml(yaml_path: Path, platform: str) -> Sample:
    """
    Parse a sample.yaml file and return a Sample object.

    Args:
        yaml_path: Path to the `sample.yaml` file.
        platform: Target platform, `ti` or `silabs`.

    Returns:
        Sample object with the parsed configuration.
    """
    with yaml_path.open() as f:
        data = yaml.safe_load(f) or {}

    # extract the name or default to parent dir name
    sample_meta = data.get("sample") or {}
    name = str(sample_meta.get("name") or yaml_path.parent.name)
    sample_dir = yaml_path.parent

    # get build options
    builds_raw = data.get("builds")
    if not builds_raw:
        raise ValueError(f"{yaml_path}: 'builds' must be a non-empty list")

    required_key = "makefile" if platform == "ti" else "board"
    if platform == "silabs":
        slcp, project_name = find_slcp(sample_dir)

    builds: list[Build] = []
    seen_ids: set[str] = set()
    for entry in builds_raw:
        # basic validation to make sure we have the right fields
        if not isinstance(entry, dict):
            raise ValueError(f"{yaml_path}: each build entry must be a mapping")
        if "id" not in entry:
            raise ValueError(f"{yaml_path}: build entry missing 'id'")
        if required_key not in entry:
            raise ValueError(f"{yaml_path}: build entry '{entry['id']}' missing '{required_key}'")

        build_id = entry["id"]
        if build_id in seen_ids:
            raise ValueError(f"{yaml_path}: duplicate build id '{build_id}'")
        seen_ids.add(build_id)

        extra_args = entry.get("extra_args") or []
        if not isinstance(extra_args, list):
            raise ValueError(f"{yaml_path}: 'extra_args' for '{build_id}' must be a list")

        build = Build(
            sample_name=name,
            sample_dir=sample_dir,
            build_id=build_id,
            extra_args=[str(a) for a in extra_args],
        )

        if platform == "ti":
            # Resolve the makefile relative to the sample dir and
            # ensure it stays inside it
            makefile_raw = str(entry["makefile"])
            makefile_path = (sample_dir / makefile_raw).resolve()
            if not makefile_path.is_relative_to(sample_dir.resolve()):
                raise ValueError(
                    f"{yaml_path}: makefile '{makefile_raw}' is not in sample directory"
                )

            if not makefile_path.is_file():
                raise ValueError(f"{yaml_path}: makefile '{makefile_raw}' does not exist")

            build.makefile = makefile_raw
        else:
            # silabs case
            build.slcp = slcp
            build.project_name = project_name
            build.board = str(entry["board"])

        builds.append(build)

    return Sample(name=name, sample_dir=sample_dir, builds=builds)


def discover(root: Path, platform: str) -> list[Sample]:
    """
    Recursively search for sample.yaml files under the given root directory
    and return a list of Sample objects.

    Args:
        root: Directory to search for sample.yaml files.
        platform: Target platform, `ti` or `silabs`.

    Raises:
        SystemExit: If no sample.yaml files are found, or if there is any error.

    Returns:
        List of Sample objects discovered.
    """
    yamls = sorted(root.rglob("sample.yaml"))
    if not yamls:
        raise SystemExit(f"No sample.yaml found under {root}")

    samples: list[Sample] = []
    errors: list[str] = []
    for path in yamls:
        try:
            samples.append(parse_sample_yaml(path, platform))
        except (yaml.YAMLError, ValueError) as e:
            errors.append(str(e))

    if errors:
        for msg in errors:
            print(f"error: {msg}", file=sys.stderr)
        raise SystemExit(2)

    return samples


def run_command(cmd: list[str], captured: list[str]) -> bool:
    """
    Run a command and capture its output.

    Args:
        cmd: Command to run, as a list of argv elements.
        captured: List to append the command and its output to.

    Returns:
        True if the command succeeded, False otherwise.
    """
    captured.append(f"$ {shlex.join(cmd)}\n")
    result = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )
    captured.append(result.stdout)
    if result.returncode != 0:
        captured.append(f"command failed with exit code {result.returncode}\n")
    return result.returncode == 0


def run_build(build: Build) -> tuple[bool, float, str]:
    """
    Run a single build command and capture its output along with the execution time.

    Args:
        build: Build object containing the build configuration.

    Returns:
        Tuple of (success: bool, duration: float, output: str)
    """
    start = time.monotonic()
    captured: list[str] = []

    build_dir = build.sample_dir / "build"

    # TI case (no .slcp file)
    if build.slcp is None:
        make_cmd = ["make", "-C", str(build.sample_dir), "-f", build.makefile]
        make_cmd.extend(build.extra_args)

        status = run_command(make_cmd, captured=captured)
    else:
        # generate a makefile project with SLC, then build it
        generate_cmd = [
            os.environ.get("SLC", "slc"),
            "generate",
            "-tlcn",
            "gcc",
            "-o",
            "makefile",
            "-cp",
            "-p",
            str(build.slcp),
            "-d",
            str(build_dir),
            "--with",
            build.board,
        ]
        generate_cmd.extend(build.extra_args)

        status = run_command(generate_cmd, captured=captured)
        if status:
            make_cmd = [
                "make",
                "-C",
                str(build_dir),
                "-f",
                f"{build.project_name}.Makefile",
                f"-j{os.cpu_count() or 1}",
            ]
            status = run_command(make_cmd, captured=captured)

    # clean up / remove build dir
    cleanup_cmd = ["rm", "-rf", str(build_dir)]
    run_command(cleanup_cmd, captured=captured)

    return status, time.monotonic() - start, "".join(captured)


def write_build_summary(
    results: list[tuple[Build, bool, float]],
    total: int,
) -> None:
    """
    Output the build results only if running in a GitHub Action environment.

    Args:
        results: List containing build results.
        total: Total number of builds that were attempted.
    """

    summary_file = os.environ.get("GITHUB_STEP_SUMMARY")
    if not summary_file:
        return

    failed = [(build, duration) for build, success, duration in results if not success]
    passed = [(build, duration) for build, success, duration in results if success]
    not_attempted = total - len(results)

    lines = ["## Sample Build Results", ""]

    if not_attempted:
        lines.append(
            f"**Stopped on first failure** - {len(passed)} passed, "
            f"{len(failed)} failed, {not_attempted} not attempted"
        )
    else:
        pct = (len(passed) / total * 100) if total else 0.0
        lines.append(
            f"**{len(passed)} of {total} builds passed ({pct:.1f}%)** - {len(failed)} failed"
        )
    lines.append("")

    if failed:
        lines.append("### Failed")
        lines.append("")
        for build, duration in failed:
            lines.append(f"- `{build.label}` ({duration:.1f}s)")
        lines.append("")
    if passed:
        lines.append("### Passed")
        lines.append("")
        for build, duration in passed:
            lines.append(f"- `{build.label}` ({duration:.1f}s)")
        lines.append("")
    if not_attempted:
        lines.append(f"### Not attempted: {not_attempted}")
        lines.append("")

    summary = "\n".join(lines)
    with open(summary_file, "a") as f:
        f.write(summary + "\n")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Recursively build samples under a given directory"
    )
    parser.add_argument("path", type=Path, help="Directory to search for sample.yaml files")
    parser.add_argument(
        "--platform", choices=["ti", "silabs"], required=True, help="Target platform"
    )
    args = parser.parse_args()

    root = args.path.resolve()
    if not root.is_dir():
        print(f"error: {root} is not a directory", file=sys.stderr)
        return 2

    if args.platform == "silabs" and not os.environ.get("SISDK"):
        print("error: SISDK environment variable is not set", file=sys.stderr)
        return 2

    samples = discover(root, args.platform)
    all_builds = [build for s in samples for build in s.builds]
    total = len(all_builds)
    print(f"Discovered {total} build(s) across {len(samples)} sample(s) under {root}\n")

    idx_width = len(str(total)) if total else 1
    label_width = max((len(build.label) for build in all_builds), default=0)

    results: list[tuple[Build, bool, float]] = []
    build_index = 0

    # loop through each sample's builds, capture results, and stop on first failure
    stop = False
    for sample in samples:
        for build in sample.builds:
            build_index += 1
            success, duration, output = run_build(build)
            status = "PASS" if success else "FAIL"
            print(
                f"[{build_index:>{idx_width}}/{total}] {status}  "
                f"{build.label:<{label_width}}  ({duration:.1f}s)"
            )
            results.append((build, success, duration))
            if not success:
                print(output, end="")
                print(f"::error::Build failed: {build.label}")
                stop = True
                break
        if stop:
            break

    # Go through the results and count pass/failed and stored the failed builds
    # to print out the details later
    failed_builds = []
    passed_count = 0
    for build, success, _ in results:
        if success:
            passed_count += 1
        else:
            failed_builds.append(build)

    not_attempted = total - len(results)

    print()
    if not_attempted:
        print(
            f"Stopped on first failure. {passed_count} passed, "
            f"{len(failed_builds)} failed, {not_attempted} not attempted."
        )
    else:
        pct = (passed_count / total * 100) if total else 0.0
        print(f"{passed_count} of {total} builds passed ({pct:.1f}%), {len(failed_builds)} failed.")

    if failed_builds:
        print()
        print("Failed builds:")
        for build in failed_builds:
            print(f"  - {build.label}")

    write_build_summary(results, total)

    return 1 if failed_builds else 0


if __name__ == "__main__":
    sys.exit(main())
