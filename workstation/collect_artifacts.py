#!/usr/bin/env python3
"""Collect explicitly selected analysis artifacts into a prepared run."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path

SCHEMA = "amiforensics.workstation.artifacts/1"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description="Collect explicit files as AmiForensics artifacts.")
    parser.add_argument("run_dir", type=Path)
    parser.add_argument("sources", nargs="+", type=Path)
    parser.add_argument("--label", default="manual", help="artifact collection label")
    args = parser.parse_args()

    run_dir = args.run_dir.expanduser().resolve()
    manifest_path = run_dir / "manifest.json"
    if not manifest_path.is_file():
        parser.error(f"missing manifest: {manifest_path}")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != "amiforensics.workstation.run/1":
        parser.error("unsupported or invalid workstation manifest schema")

    dest_root = run_dir / "artifacts" / args.label
    dest_root.mkdir(parents=True, exist_ok=True)

    entries: list[dict[str, object]] = []
    used_names: set[str] = set()
    for raw in args.sources:
        source = raw.expanduser().resolve()
        if not source.is_file():
            parser.error(f"artifact source is not a regular file: {source}")

        name = source.name
        if name in used_names or (dest_root / name).exists():
            prefix = sha256_file(source)[:12]
            name = f"{prefix}-{name}"
        used_names.add(name)

        destination = dest_root / name
        shutil.copy2(source, destination)
        destination.chmod(0o400)
        entries.append({
            "source_name": source.name,
            "stored_path": destination.relative_to(run_dir).as_posix(),
            "size": destination.stat().st_size,
            "sha256": sha256_file(destination),
        })

    index_path = dest_root / "index.json"
    index = {
        "schema": SCHEMA,
        "label": args.label,
        "collected_utc": datetime.now(timezone.utc).isoformat(),
        "run_id": manifest.get("run_id"),
        "artifacts": entries,
    }
    index_path.write_text(json.dumps(index, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    manifest.setdefault("artifact_collections", []).append({
        "label": args.label,
        "index": index_path.relative_to(run_dir).as_posix(),
        "count": len(entries),
    })
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(index_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
