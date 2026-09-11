#!/usr/bin/env python3
"""Host-side qualification for M6.4 workstation JSON reporting."""

from __future__ import annotations

import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "workstation" / "report.py"
SCHEMA = "amiforensics.workstation.report/1"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(REPORT), *args],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
    )


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="amiforensics-m6-4-") as tmp:
        work = Path(tmp)
        manifest = work / "manifest.json"
        evidence_json = work / "snapshot.json"
        evidence_text = work / "compare.kv"
        output = work / "report.json"

        manifest.write_text(
            json.dumps({"schema": "amiforensics.workstation.run/1", "run_id": "qualify-m6-4"}) + "\n",
            encoding="utf-8",
        )
        evidence_json.write_text(
            json.dumps({"schema": "amiforensics.workstation.snapshot/1", "phase": "post"}) + "\n",
            encoding="utf-8",
        )
        evidence_text.write_text(
            "tool=Compare\nschema=amiforensics.compare.kv/1\nmodified=1\n",
            encoding="utf-8",
        )

        proc = run(
            "--manifest", str(manifest),
            "--evidence", str(evidence_json),
            "--evidence", str(evidence_text),
            "--output", str(output),
        )
        require(proc.returncode == 0, f"report.py failed: {proc.stderr}")
        require(output.is_file(), "report output was not created")

        data = json.loads(output.read_text(encoding="utf-8"))
        require(data["schema"] == SCHEMA, "wrong report schema")
        require(data["manifest"]["name"] == "manifest.json", "wrong manifest name")
        require(data["manifest"]["sha256"] == sha256(manifest), "wrong manifest sha256")
        require(data["manifest"]["data"]["run_id"] == "qualify-m6-4", "manifest data missing")
        require(data["summary"]["evidence_count"] == 2, "wrong evidence count")
        require(len(data["evidence"]) == 2, "wrong evidence list length")
        require(data["evidence"][0]["format"] == "json", "JSON evidence not decoded")
        require(data["evidence"][0]["sha256"] == sha256(evidence_json), "wrong JSON evidence sha256")
        require(data["evidence"][1]["format"] == "text", "text evidence not retained as text")
        require("tool=Compare" in data["evidence"][1]["data"], "text evidence content missing")

        first = output.read_bytes()
        proc = run(
            "--manifest", str(manifest),
            "--evidence", str(evidence_json),
            "--evidence", str(evidence_text),
            "--output", str(output),
        )
        require(proc.returncode == 0, "second deterministic run failed")
        require(output.read_bytes() == first, "report output is not deterministic")

        too_many = []
        for index in range(33):
            path = work / f"evidence-{index:02d}.txt"
            path.write_text("x\n", encoding="utf-8")
            too_many.extend(["--evidence", str(path)])
        proc = run("--manifest", str(manifest), *too_many, "--output", str(work / "too-many.json"))
        require(proc.returncode != 0, "more than 32 evidence files should fail")

        oversized = work / "oversized.txt"
        with oversized.open("wb") as handle:
            handle.truncate(4 * 1024 * 1024 + 1)
        proc = run(
            "--manifest", str(manifest),
            "--evidence", str(oversized),
            "--output", str(work / "oversized.json"),
        )
        require(proc.returncode != 0, "oversized evidence should fail")

        bad_json = work / "bad.json"
        bad_json.write_text("{not-json}\n", encoding="utf-8")
        proc = run(
            "--manifest", str(manifest),
            "--evidence", str(bad_json),
            "--output", str(work / "bad-json-report.json"),
        )
        require(proc.returncode != 0, "malformed JSON evidence should fail")

        missing = work / "missing.json"
        proc = run("--manifest", str(missing), "--output", str(work / "missing-report.json"))
        require(proc.returncode != 0, "missing manifest should fail")

    print("M6.4 workstation JSON report qualification: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
