#!/usr/bin/env python3
"""Integrated release qualification for AmiForensics M7.2.

This harness performs a strict host portability build of the complete release
surface, validates the staged v0.1.0 package, and re-runs the closed host/static
qualification slices that feed the release. It does not claim real AmigaOS
runtime qualification and never executes an unknown sample.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PYTHON = sys.executable
TARGETS = [
    "FileInfo",
    "Strings",
    "HunkInfo",
    "BootInfo",
    "ResidentView",
    "PatchView",
    "TaskView",
    "ProcessView",
    "PortView",
    "MemScan",
    "SampleDump",
    "TraceExec",
    "DiskWatch",
    "Compare",
    "Report",
]


def run(command: list[str]) -> None:
    print("+", " ".join(command), flush=True)
    proc = subprocess.run(command, cwd=ROOT, text=True, check=False)
    if proc.returncode != 0:
        raise SystemExit(proc.returncode)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    strict_cflags = "-O2 -Wall -Wextra -Werror -Wno-unused-function"

    run(["make", "clean"])
    run(["make", "CC=gcc", f"CFLAGS={strict_cflags}", "package"])

    package_root = ROOT / "dist" / "AmiForensics-v0.1.0"
    archive = ROOT / "dist" / "AmiForensics-v0.1.0.tar.gz"
    require(package_root.is_dir(), "release package directory missing")
    require(archive.is_file(), "release package archive missing")

    packaged_tools = sorted(path.name for path in (package_root / "C").iterdir() if path.is_file())
    require(packaged_tools == sorted(TARGETS), "packaged native tool inventory does not match release manifest")
    require((package_root / "Rexx" / "AFReport.rexx").is_file(), "AFReport.rexx missing from package")
    require((package_root / "Docs" / "RELEASE_v0.1.0.md").is_file(), "release manifest missing from package")
    require((package_root / "Docs" / "README.md").is_file(), "README missing from package")
    require((package_root / "Docs" / "ROADMAP.md").is_file(), "ROADMAP missing from package")
    require((package_root / "Docs" / "LICENSE").is_file(), "LICENSE missing from package")

    for target in TARGETS:
        require((ROOT / target).is_file(), f"host portability build missing {target}")

    qualifications = [
        ROOT / "tools" / "qualify_m6_1.py",
        ROOT / "tools" / "qualify_m6_2.py",
        ROOT / "tools" / "qualify_m6_3.py",
        ROOT / "workstation" / "qualify_m5.py",
        ROOT / "workstation" / "qualify_m6_4.py",
        ROOT / "workstation" / "qualify_m6_5.py",
    ]
    for harness in qualifications:
        run([PYTHON, str(harness)])

    print("M7.2 INTEGRATED SUITE QUALIFICATION: PASS")
    print("scope=host-portability-build+package+closed-host-static-qualification")
    print("real_amigaos_runtime=false")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
