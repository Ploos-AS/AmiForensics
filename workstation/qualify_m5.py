#!/usr/bin/env python3
"""Reproducible host-side qualification for AmiForensics M5 workstation flow.

This harness never starts FS-UAE, never executes the sample, and never runs
Ghidra analysis. It validates orchestration, snapshots, artifact collection,
snapshot comparison, and Ghidra dry-run command construction.
"""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WS = ROOT / "workstation"
PYTHON = sys.executable


def run(*args: str) -> subprocess.CompletedProcess[str]:
    command = [PYTHON, *args]
    completed = subprocess.run(command, text=True, capture_output=True, check=False)
    if completed.returncode != 0:
        print("FAIL:", " ".join(command), file=sys.stderr)
        if completed.stdout:
            print(completed.stdout, file=sys.stderr)
        if completed.stderr:
            print(completed.stderr, file=sys.stderr)
        raise SystemExit(completed.returncode)
    return completed


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="amiforensics-m5-") as temp:
        root = Path(temp)
        runs = root / "runs"
        system_root = root / "aros-system"
        system_root.mkdir()

        sample = root / "sample.bin"
        sample.write_bytes(b"AmiForensics M5 qualification sample\n")

        prepared = run(str(WS / "prepare_run.py"), str(sample), "--run-root", str(runs))
        run_dir = Path(prepared.stdout.strip().splitlines()[-1]).resolve()
        require(run_dir.is_dir(), "prepare_run did not create a run directory")

        manifest_path = run_dir / "manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        require(manifest.get("schema") == "amiforensics.workstation.run/1", "bad run schema")
        require(manifest.get("dynamic_execution") is False, "dynamic execution unexpectedly enabled")

        run(
            str(WS / "prepare_profile.py"),
            str(run_dir),
            "--system-root",
            str(system_root),
        )
        profile = (run_dir / "profile" / "analysis.fs-uae").read_text(encoding="utf-8")
        require("bsdsocket_library = 0" in profile, "network isolation default missing")

        fsuae = run(str(WS / "run_fsuae.py"), str(run_dir))
        require("DRY-RUN:" in fsuae.stdout, "FS-UAE dry-run was not used")
        require("No emulator started" in fsuae.stdout, "FS-UAE safety message missing")

        run(str(WS / "snapshot.py"), str(run_dir), "pre")

        artifact_source = root / "guest-output.txt"
        artifact_source.write_text("controlled artifact\n", encoding="utf-8")
        run(
            str(WS / "collect_artifacts.py"),
            str(run_dir),
            str(artifact_source),
            "--label",
            "qualification",
        )

        ghidra_home = root / "fake-ghidra"
        support = ghidra_home / "support"
        support.mkdir(parents=True)
        analyze_headless = support / "analyzeHeadless"
        analyze_headless.write_text("#!/bin/sh\nexit 99\n", encoding="utf-8")
        analyze_headless.chmod(0o755)

        ghidra = run(
            str(WS / "ghidra_import.py"),
            str(run_dir),
            "--ghidra-home",
            str(ghidra_home),
        )
        require("DRY-RUN:" in ghidra.stdout, "Ghidra dry-run was not used")
        require("No Ghidra analysis started" in ghidra.stdout, "Ghidra safety message missing")

        run(str(WS / "snapshot.py"), str(run_dir), "post")
        run(str(WS / "compare_snapshots.py"), str(run_dir))

        diff = json.loads((run_dir / "snapshots" / "diff.json").read_text(encoding="utf-8"))
        require(
            diff.get("schema") == "amiforensics.workstation.snapshot-diff/1",
            "bad snapshot diff schema",
        )
        require(diff.get("summary", {}).get("added", 0) >= 1, "expected added artifact not detected")

        index_path = run_dir / "artifacts" / "qualification" / "index.json"
        require(index_path.is_file(), "artifact index missing")
        index = json.loads(index_path.read_text(encoding="utf-8"))
        require(index.get("schema") == "amiforensics.workstation.artifacts/1", "bad artifact schema")
        require(len(index.get("artifacts", [])) == 1, "unexpected artifact count")

        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        require(manifest.get("snapshot_diff") == "snapshots/diff.json", "diff not registered")
        require("pre" in manifest.get("snapshots", {}), "pre snapshot not registered")
        require("post" in manifest.get("snapshots", {}), "post snapshot not registered")
        require(manifest.get("dynamic_execution") is False, "qualification enabled dynamic execution")
        require("ghidra" not in manifest, "Ghidra dry-run unexpectedly mutated manifest")

        copied_sample = run_dir / manifest["sample"]["stored_path"]
        require(copied_sample.is_file(), "immutable sample copy missing")
        require((copied_sample.stat().st_mode & 0o222) == 0, "sample copy is writable")

        print("M5 WORKSTATION QUALIFICATION: PASS")
        print(f"run_dir={run_dir}")
        print(json.dumps(diff["summary"], sort_keys=True))
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
