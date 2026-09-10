#!/usr/bin/env python3
"""Render an FS-UAE workstation profile for a prepared AmiForensics run."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description="Render an isolated FS-UAE profile for a prepared run.")
    parser.add_argument("run_dir", type=Path)
    parser.add_argument("--system-root", required=True, type=Path)
    parser.add_argument(
        "--template",
        type=Path,
        default=Path(__file__).resolve().parent / "profiles" / "aros-a1200.fs-uae.in",
    )
    args = parser.parse_args()

    run_dir = args.run_dir.expanduser().resolve()
    manifest_path = run_dir / "manifest.json"
    if not manifest_path.is_file():
        parser.error(f"missing prepared-run manifest: {manifest_path}")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != "amiforensics.workstation.run/1":
        parser.error("unsupported or invalid workstation manifest schema")
    if manifest.get("state") != "prepared":
        parser.error(f"run is not in prepared state: {manifest.get('state')!r}")

    system_root = args.system_root.expanduser().resolve()
    if not system_root.is_dir():
        parser.error(f"system root is not a directory: {system_root}")

    template = args.template.expanduser().resolve()
    if not template.is_file():
        parser.error(f"profile template not found: {template}")

    text = template.read_text(encoding="utf-8")
    text = text.replace("@SYSTEM_ROOT@", str(system_root))
    text = text.replace("@RUN_DIR@", str(run_dir))

    profile_dir = run_dir / "profile"
    profile_dir.mkdir(exist_ok=True)
    output = profile_dir / "analysis.fs-uae"
    output.write_text(text, encoding="utf-8")

    manifest["profile"] = {
        "engine": "fs-uae",
        "template": template.name,
        "config": "profile/analysis.fs-uae",
        "system_root": str(system_root),
        "network_bridge": False,
    }
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
