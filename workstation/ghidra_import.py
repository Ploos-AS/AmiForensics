#!/usr/bin/env python3
"""Optional static Ghidra integration for an AmiForensics workstation run.

The integration imports the immutable sample copy into a disposable Ghidra
project. It never executes the sample. Execution is opt-in with --analyze;
without it the command is printed for review only.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
from pathlib import Path

RUN_SCHEMA = "amiforensics.workstation.run/1"


def resolve_analyze_headless(ghidra_home: Path | None) -> Path | None:
    if ghidra_home is not None:
        candidate = ghidra_home.expanduser().resolve() / "support" / "analyzeHeadless"
        return candidate if candidate.is_file() else None

    env_home = os.environ.get("GHIDRA_HOME")
    if env_home:
        candidate = Path(env_home).expanduser().resolve() / "support" / "analyzeHeadless"
        if candidate.is_file():
            return candidate

    executable = shutil.which("analyzeHeadless")
    return Path(executable).resolve() if executable else None


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Import an AmiForensics run sample into a disposable Ghidra project."
    )
    parser.add_argument("run_dir", type=Path)
    parser.add_argument(
        "--ghidra-home",
        type=Path,
        help="Ghidra installation root; otherwise GHIDRA_HOME/PATH is used",
    )
    parser.add_argument(
        "--analyze",
        action="store_true",
        help="run Ghidra headless analysis; default is dry-run",
    )
    args = parser.parse_args()

    run_dir = args.run_dir.expanduser().resolve()
    manifest_path = run_dir / "manifest.json"
    if not manifest_path.is_file():
        parser.error(f"missing manifest: {manifest_path}")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != RUN_SCHEMA:
        parser.error("unsupported or invalid workstation manifest schema")

    stored_path = manifest.get("sample", {}).get("stored_path")
    if not isinstance(stored_path, str) or not stored_path:
        parser.error("manifest does not contain sample.stored_path")

    sample = (run_dir / stored_path).resolve()
    try:
        sample.relative_to(run_dir)
    except ValueError:
        parser.error("sample path escapes run directory")
    if not sample.is_file():
        parser.error(f"prepared sample not found: {sample}")

    analyze_headless = resolve_analyze_headless(args.ghidra_home)
    if analyze_headless is None:
        parser.error(
            "Ghidra analyzeHeadless not found; use --ghidra-home, GHIDRA_HOME, or PATH"
        )

    project_dir = run_dir / "ghidra"
    project_dir.mkdir(exist_ok=True)
    project_name = "AmiForensics"
    log_dir = run_dir / "logs"
    log_dir.mkdir(exist_ok=True)
    log_path = log_dir / "ghidra.log"

    command = [
        str(analyze_headless),
        str(project_dir),
        project_name,
        "-import",
        str(sample),
        "-overwrite",
    ]

    if not args.analyze:
        print("DRY-RUN: " + " ".join(command))
        print("No Ghidra analysis started. Re-run with --analyze after reviewing the command.")
        return 0

    manifest["ghidra"] = {
        "mode": "static-headless",
        "project_dir": "ghidra",
        "project_name": project_name,
        "sample": stored_path,
        "sample_execution": False,
        "command": command,
        "log": "logs/ghidra.log",
        "state": "started",
    }
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    with log_path.open("ab") as log:
        completed = subprocess.run(
            command,
            stdout=log,
            stderr=subprocess.STDOUT,
            check=False,
        )

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest["ghidra"]["state"] = "completed" if completed.returncode == 0 else "failed"
    manifest["ghidra"]["returncode"] = completed.returncode
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
