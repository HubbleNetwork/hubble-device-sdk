#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
#
# Compare static analysis results between a base branch and a PR to identify
# new and fixed findings.  Produces a Markdown comment suitable for posting
# to a GitHub pull request.
#
# Usage:
#   python3 tools/ci/diff-static-analysis.py \
#       --base-dir /tmp/results-base \
#       --pr-dir   /tmp/results-pr   \
#       --output   /tmp/comment.md
#
# Exit codes:
#   0 — no new findings
#   1 — new findings detected

import argparse
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import NamedTuple

# GitHub comment body limit (bytes).  Leave headroom for the marker/header.
MAX_COMMENT_LEN = 60000

COMMENT_MARKER = "<!-- static-analysis-diff-comment -->"


class Finding(NamedTuple):
    tool: str
    filepath: str
    rule_id: str
    message: str


# ---------------------------------------------------------------------------
# Parsers
# ---------------------------------------------------------------------------

def parse_cppcheck_xml(xml_path):
    """Parse cppcheck XML into a set of Finding tuples.

    Line numbers are deliberately excluded so that findings which merely
    shifted position (due to code added/removed above them) are still
    recognised as the same finding.
    """
    findings = set()
    if not xml_path.exists():
        return findings

    try:
        tree = ET.parse(xml_path)
    except ET.ParseError:
        return findings

    for error in tree.getroot().iter("error"):
        error_id = error.get("id", "")
        if error_id in ("information", "missingInclude", "missingIncludeSystem"):
            continue

        msg = error.get("msg", "")
        location = error.find("location")
        filepath = location.get("file", "") if location is not None else ""
        findings.add(Finding("cppcheck", filepath, error_id, msg))

    return findings


def parse_coccinelle_text(txt_path):
    """Parse Coccinelle report-mode text into a set of Finding tuples.

    Expected formats produced by ``coccicheck --mode=report``:
      filepath:line:col-col: message
      filepath:line: message
    """
    findings = set()
    if not txt_path.exists():
        return findings

    for raw_line in txt_path.read_text().splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue

        # Split on the first colon that follows the filename.  Filenames may
        # contain colons on some systems but our codebase uses simple paths.
        parts = line.split(":", maxsplit=2)
        if len(parts) < 3:
            continue

        filepath = parts[0]
        # parts[1] is the line number — skip for line-agnostic matching.
        message = parts[2].strip()
        # Strip leading column range (e.g. "5-10: ") if present.
        col_msg = message.split(":", maxsplit=1)
        if len(col_msg) == 2 and col_msg[0].replace("-", "").strip().isdigit():
            message = col_msg[1].strip()

        findings.add(Finding("coccinelle", filepath, "", message))

    return findings


# ---------------------------------------------------------------------------
# Markdown formatting
# ---------------------------------------------------------------------------

def _findings_table(findings, include_tool=False):
    """Return a Markdown table for a set of findings."""
    columns = []
    if include_tool:
        columns.append("Tool")
    columns.extend(["File", "ID", "Message"])

    header = "| " + " | ".join(columns) + " |"
    sep = "| " + " | ".join("---" for _ in columns) + " |"
    rows = [header, sep]
    for f in sorted(findings):
        cells = []
        if include_tool:
            cells.append(f.tool)
        cells.append(f"`{f.filepath}`")
        cells.append(f"`{f.rule_id}`" if f.rule_id else "")
        cells.append(f.message)
        rows.append("| " + " | ".join(cells) + " |")
    return "\n".join(rows)


def format_markdown(new_findings, fixed_findings):
    """Format the diff as a GitHub PR comment."""
    lines = [COMMENT_MARKER, "## Static Analysis Results", ""]

    if not new_findings and not fixed_findings:
        lines.append(
            "No new static analysis findings. No findings fixed."
        )
        return "\n".join(lines)

    if new_findings:
        lines.append(
            f"**{len(new_findings)} new finding(s) introduced by this PR:**"
        )
        lines.append("")

        by_tool = {}
        for f in new_findings:
            by_tool.setdefault(f.tool, set()).add(f)

        for tool in sorted(by_tool):
            items = by_tool[tool]
            lines.append(
                f"<details><summary><b>{tool}</b> "
                f"({len(items)} finding(s))</summary>\n"
            )
            lines.append(_findings_table(items))
            lines.append("\n</details>\n")
    else:
        lines.append("**No new findings introduced by this PR.**")
        lines.append("")

    if fixed_findings:
        lines.append(
            f"**{len(fixed_findings)} finding(s) fixed by this PR:**"
        )
        lines.append("")
        lines.append("<details><summary>Fixed findings</summary>\n")
        lines.append(_findings_table(fixed_findings, include_tool=True))
        lines.append("\n</details>")

    body = "\n".join(lines)

    # Truncate if the comment would exceed the GitHub size limit.
    if len(body) > MAX_COMMENT_LEN:
        truncation_msg = (
            "\n\n---\n*Comment truncated — too many findings to display. "
            "Run `./tools/analyze` locally for full results.*"
        )
        body = body[: MAX_COMMENT_LEN - len(truncation_msg)] + truncation_msg

    return body


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Diff static analysis results between base and PR."
    )
    parser.add_argument(
        "--base-dir", required=True, help="Directory with base branch results"
    )
    parser.add_argument(
        "--pr-dir", required=True, help="Directory with PR branch results"
    )
    parser.add_argument(
        "--output", required=True, help="Path to write Markdown output"
    )
    args = parser.parse_args()

    base_dir = Path(args.base_dir)
    pr_dir = Path(args.pr_dir)

    base_findings = set()
    pr_findings = set()

    base_findings |= parse_cppcheck_xml(base_dir / "cppcheck.xml")
    base_findings |= parse_coccinelle_text(base_dir / "coccinelle.txt")

    pr_findings |= parse_cppcheck_xml(pr_dir / "cppcheck.xml")
    pr_findings |= parse_coccinelle_text(pr_dir / "coccinelle.txt")

    new_findings = pr_findings - base_findings
    fixed_findings = base_findings - pr_findings

    markdown = format_markdown(new_findings, fixed_findings)
    Path(args.output).write_text(markdown)

    print(f"Base findings:  {len(base_findings)}")
    print(f"PR findings:    {len(pr_findings)}")
    print(f"New findings:   {len(new_findings)}")
    print(f"Fixed findings: {len(fixed_findings)}")

    if new_findings:
        print("FAIL: New static analysis findings detected.")
        return 1

    print("PASS: No new static analysis findings.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
