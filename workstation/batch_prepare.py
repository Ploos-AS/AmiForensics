#!/usr/bin/env python3
"""Prepare a bounded batch of isolated AmiForensics workstation runs.

This orchestration never starts an emulator and never executes a sample. Each
sample is delegated to prepare_run.py and the results are summarized in JSON.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

SCHEMA = "amiforensics.workstation.batch/1"
MAX_SAMPLES = 128
ROOT = Path(__file__).resolve().parents[1]
PREPARE_RUN = ROOT / "workstation" / "prepare_run.py"


def prepare_sample(sample: Path, run_root: Path) -> dict[str, Any]:
    proc = subprocess.run(
        [
            sys.executable,
            str(PREPARE_RUN),
            str(sample),
            "--run-root",
            str(run_root),
        ],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )

    entry: dict[str, Any] = {
        "sample": str(sample),
        "returncode": proc.returncode,
        "status": "prepared" if proc.returncode == 0 else "failed",
    }

    if proc.returncode == 0:
        run_dir = Path(proc.stdout.strip())
        entry["run_dir"] = str(run_dir)
        manifest = run_dir / "manifest.json"
        if manifest.is_file():
            data = json.loads(manifest.read_text(encoding="utf-8"))
            entry["run_id"] = data.get("run_id")
            entry["sha256"] = data.get("sample", {}).get("sha256")
    else:
        entry["error"] = proc.stderr.strip() or proc.stdout.strip()

    return entry


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("samples", nargs="+", type=Path)
    parser.add_argument("--run-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if len(args.samples) > MAX_SAMPLES:
        print(f"too many samples: {len(args.samples)} > {MAX_SAMPLES}", file=sys.stderr)
        return 10

    run_root = args.run_root.expanduser().resolve()
    run_root.mkdir(parents=True, exist_ok=True)

    results = [prepare_sample(sample.expanduser().resolve(), run_root) for sample in args.samples]
    prepared = sum(1 for item in results if item["status"] == "prepared")
    failed = len(results) - prepared

    document = {
        "schema": SCHEMA,
        "dynamic_execution": False,
        "sample_count": len(results),
        "prepared_count": prepared,
        "failed_count": failed,
        "results": results,
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(document, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    return 5 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
