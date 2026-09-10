#!/usr/bin/env python3
"""Capture a normalized workstation snapshot for an AmiForensics run."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from datetime import datetime, timezone
from pathlib import Path

SCHEMA = "amiforensics.workstation.snapshot/1"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def inventory(root: Path) -> list[dict[str, object]]:
    result: list[dict[str, object]] = []
    if not root.exists():
        return result
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        stat = path.stat()
        result.append({
            "path": path.relative_to(root).as_posix(),
            "size": stat.st_size,
            "mtime_ns": stat.st_mtime_ns,
            "sha256": sha256_file(path),
        })
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="Capture pre/post run filesystem metadata.")
    parser.add_argument("run_dir", type=Path)
    parser.add_argument("phase", choices=("pre", "post"))
    args = parser.parse_args()

    run_dir = args.run_dir.expanduser().resolve()
    manifest_path = run_dir / "manifest.json"
    if not manifest_path.is_file():
        parser.error(f"missing manifest: {manifest_path}")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != "amiforensics.workstation.run/1":
        parser.error("unsupported or invalid workstation manifest schema")

    snapshot_dir = run_dir / "snapshots"
    snapshot_dir.mkdir(exist_ok=True)
    output = snapshot_dir / f"{args.phase}.json"

    snapshot = {
        "schema": SCHEMA,
        "phase": args.phase,
        "captured_utc": datetime.now(timezone.utc).isoformat(),
        "run_id": manifest.get("run_id"),
        "cwd": os.getcwd(),
        "trees": {
            "artifacts": inventory(run_dir / "artifacts"),
            "logs": inventory(run_dir / "logs"),
            "profile": inventory(run_dir / "profile"),
        },
    }
    output.write_text(json.dumps(snapshot, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    manifest.setdefault("snapshots", {})[args.phase] = f"snapshots/{args.phase}.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
