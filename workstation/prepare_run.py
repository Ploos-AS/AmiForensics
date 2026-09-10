#!/usr/bin/env python3
"""Prepare a disposable AmiForensics workstation run.

M5.1 intentionally does not start an emulator or execute the sample.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import sys
from datetime import datetime, timezone
from pathlib import Path

SCHEMA = "amiforensics.workstation.run/1"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Prepare an isolated AmiForensics analysis run; does not execute the sample."
    )
    parser.add_argument("sample", type=Path)
    parser.add_argument(
        "--run-root",
        type=Path,
        default=Path(__file__).resolve().parent / "runs",
        help="directory that will contain disposable run directories",
    )
    args = parser.parse_args()

    sample = args.sample.expanduser().resolve()
    if not sample.is_file():
        parser.error(f"sample is not a regular file: {sample}")

    digest = sha256_file(sample)
    now = datetime.now(timezone.utc)
    run_id = f"{now.strftime('%Y%m%dT%H%M%SZ')}-{digest[:12]}"
    run_dir = args.run_root.expanduser().resolve() / run_id

    if run_dir.exists():
        print(f"refusing to reuse existing run directory: {run_dir}", file=sys.stderr)
        return 2

    input_dir = run_dir / "input"
    artifact_dir = run_dir / "artifacts"
    log_dir = run_dir / "logs"
    input_dir.mkdir(parents=True)
    artifact_dir.mkdir()
    log_dir.mkdir()

    copied = input_dir / sample.name
    shutil.copy2(sample, copied)
    copied.chmod(0o400)

    manifest = {
        "schema": SCHEMA,
        "run_id": run_id,
        "created_utc": now.isoformat(),
        "state": "prepared",
        "dynamic_execution": False,
        "sample": {
            "original_name": sample.name,
            "stored_path": f"input/{sample.name}",
            "size": sample.stat().st_size,
            "sha256": digest,
        },
        "artifacts_path": "artifacts",
        "logs_path": "logs",
    }
    (run_dir / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    print(run_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
