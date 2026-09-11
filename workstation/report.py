#!/usr/bin/env python3
"""Build a deterministic host-side AmiForensics JSON report.

This tool only reads existing metadata/evidence files. It never executes a sample.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

SCHEMA = "amiforensics.workstation.report/1"
MAX_EVIDENCE_FILES = 32
MAX_EVIDENCE_BYTES = 4 * 1024 * 1024


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def evidence_entry(path: Path) -> dict[str, Any]:
    stat = path.stat()
    if stat.st_size > MAX_EVIDENCE_BYTES:
        raise ValueError(f"evidence file too large: {path}")

    entry: dict[str, Any] = {
        "name": path.name,
        "size": stat.st_size,
        "sha256": sha256_file(path),
    }

    if path.suffix.lower() == ".json":
        entry["format"] = "json"
        entry["data"] = load_json(path)
    else:
        entry["format"] = "text"
        entry["data"] = path.read_text(encoding="utf-8", errors="replace")
    return entry


def build_report(manifest: Path, evidence: list[Path]) -> dict[str, Any]:
    if len(evidence) > MAX_EVIDENCE_FILES:
        raise ValueError(f"too many evidence files: {len(evidence)} > {MAX_EVIDENCE_FILES}")

    manifest_data = load_json(manifest)
    return {
        "schema": SCHEMA,
        "manifest": {
            "name": manifest.name,
            "sha256": sha256_file(manifest),
            "data": manifest_data,
        },
        "evidence": [evidence_entry(path) for path in evidence],
        "summary": {
            "evidence_count": len(evidence),
        },
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--evidence", action="append", default=[], type=Path)
    parser.add_argument("--output", required=True, type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    report = build_report(args.manifest, args.evidence)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
