#!/usr/bin/env python3
"""Host-side qualification for M6.5 batch preparation."""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BATCH = ROOT / "workstation" / "batch_prepare.py"
SCHEMA = "amiforensics.workstation.batch/1"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(BATCH), *args],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="amiforensics-m6-5-") as tmp:
        work = Path(tmp)
        run_root = work / "runs"
        output = work / "batch.json"
        a = work / "a.bin"
        b = work / "b.bin"
        a.write_bytes(b"alpha\n")
        b.write_bytes(b"beta\n")

        proc = run(str(a), str(b), "--run-root", str(run_root), "--output", str(output))
        require(proc.returncode == 0, f"batch prepare failed: {proc.stderr}")
        data = json.loads(output.read_text(encoding="utf-8"))
        require(data["schema"] == SCHEMA, "wrong batch schema")
        require(data["dynamic_execution"] is False, "batch must not enable dynamic execution")
        require(data["sample_count"] == 2, "wrong sample count")
        require(data["prepared_count"] == 2, "wrong prepared count")
        require(data["failed_count"] == 0, "unexpected failures")
        require(len(data["results"]) == 2, "wrong result count")

        expected = {str(a.resolve()): sha256(a), str(b.resolve()): sha256(b)}
        for item in data["results"]:
            require(item["status"] == "prepared", "sample was not prepared")
            require(item["returncode"] == 0, "unexpected sample return code")
            require(item["sha256"] == expected[item["sample"]], "wrong sample sha256")
            run_dir = Path(item["run_dir"])
            manifest = run_dir / "manifest.json"
            require(manifest.is_file(), "run manifest missing")
            manifest_data = json.loads(manifest.read_text(encoding="utf-8"))
            require(manifest_data["dynamic_execution"] is False, "prepared run enables dynamic execution")
            stored = run_dir / manifest_data["sample"]["stored_path"]
            require(stored.is_file(), "immutable sample copy missing")
            require((stored.stat().st_mode & 0o777) == 0o400, "prepared sample is not mode 0400")

        missing = work / "missing.bin"
        partial = work / "partial.json"
        proc = run(str(a), str(missing), "--run-root", str(work / "partial-runs"), "--output", str(partial))
        require(proc.returncode == 5, "partial batch failure must return 5")
        partial_data = json.loads(partial.read_text(encoding="utf-8"))
        require(partial_data["prepared_count"] == 1, "partial batch prepared count wrong")
        require(partial_data["failed_count"] == 1, "partial batch failed count wrong")
        require(any(item["status"] == "failed" for item in partial_data["results"]), "failed result missing")

        many = []
        for index in range(129):
            path = work / f"sample-{index:03d}.bin"
            path.write_bytes(b"x")
            many.append(str(path))
        too_many = work / "too-many.json"
        proc = run(*many, "--run-root", str(work / "many-runs"), "--output", str(too_many))
        require(proc.returncode == 10, "more than 128 samples must return 10")
        require(not too_many.exists(), "too-many request should not create output")

    print("M6.5 batch preparation qualification: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
