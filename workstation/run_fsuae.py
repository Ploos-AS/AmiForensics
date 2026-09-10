#!/usr/bin/env python3
"""Launch FS-UAE for a prepared AmiForensics run.

The workstation never auto-executes the sample. Starting the emulator itself
requires an explicit --start flag.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description="Launch FS-UAE for a prepared AmiForensics run.")
    parser.add_argument("run_dir", type=Path)
    parser.add_argument("--fs-uae", default="fs-uae", help="FS-UAE executable")
    parser.add_argument("--start", action="store_true", help="actually start FS-UAE")
    args = parser.parse_args()

    run_dir = args.run_dir.expanduser().resolve()
    manifest_path = run_dir / "manifest.json"
    profile = run_dir / "profile" / "analysis.fs-uae"

    if not manifest_path.is_file():
        parser.error(f"missing manifest: {manifest_path}")
    if not profile.is_file():
        parser.error("missing rendered profile; run prepare_profile.py first")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != "amiforensics.workstation.run/1":
        parser.error("unsupported or invalid workstation manifest schema")

    executable = shutil.which(args.fs_uae)
    command = [executable or args.fs_uae, str(profile)]

    if not args.start:
        print("DRY-RUN: " + " ".join(command))
        print("No emulator started. Re-run with --start after reviewing the profile.")
        return 0

    if executable is None:
        parser.error(f"FS-UAE executable not found: {args.fs_uae}")

    log_dir = run_dir / "logs"
    log_dir.mkdir(exist_ok=True)
    log_path = log_dir / "fs-uae.log"

    manifest["state"] = "emulator_started"
    manifest["dynamic_execution"] = False
    manifest["emulator"] = {
        "engine": "fs-uae",
        "command": command,
        "log": "logs/fs-uae.log",
        "sample_autoexec": False,
    }
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    with log_path.open("ab") as log:
        completed = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=False)

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest["state"] = "emulator_exited"
    manifest["emulator"]["returncode"] = completed.returncode
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
