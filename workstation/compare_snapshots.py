#!/usr/bin/env python3
"""Compare AmiForensics workstation pre/post snapshots."""

from __future__ import annotations

import argparse
import json
from datetime import datetime, timezone
from pathlib import Path

SNAPSHOT_SCHEMA = "amiforensics.workstation.snapshot/1"
DIFF_SCHEMA = "amiforensics.workstation.snapshot-diff/1"


def load_snapshot(path: Path) -> dict:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema") != SNAPSHOT_SCHEMA:
        raise ValueError(f"unsupported snapshot schema in {path}")
    return data


def index_tree(entries: list[dict]) -> dict[str, dict]:
    return {entry["path"]: entry for entry in entries}


def compare_tree(before: list[dict], after: list[dict]) -> dict:
    pre = index_tree(before)
    post = index_tree(after)
    pre_paths = set(pre)
    post_paths = set(post)

    added = [post[path] for path in sorted(post_paths - pre_paths)]
    removed = [pre[path] for path in sorted(pre_paths - post_paths)]
    modified = []
    unchanged = 0

    for path in sorted(pre_paths & post_paths):
        a = pre[path]
        b = post[path]
        if a.get("sha256") == b.get("sha256") and a.get("size") == b.get("size"):
            unchanged += 1
            continue
        modified.append({
            "path": path,
            "before": a,
            "after": b,
        })

    return {
        "added": added,
        "removed": removed,
        "modified": modified,
        "unchanged_count": unchanged,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Compare pre/post AmiForensics snapshots.")
    parser.add_argument("run_dir", type=Path)
    args = parser.parse_args()

    run_dir = args.run_dir.expanduser().resolve()
    manifest_path = run_dir / "manifest.json"
    if not manifest_path.is_file():
        parser.error(f"missing manifest: {manifest_path}")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("schema") != "amiforensics.workstation.run/1":
        parser.error("unsupported or invalid workstation manifest schema")

    pre_path = run_dir / manifest.get("snapshots", {}).get("pre", "snapshots/pre.json")
    post_path = run_dir / manifest.get("snapshots", {}).get("post", "snapshots/post.json")
    if not pre_path.is_file() or not post_path.is_file():
        parser.error("both pre and post snapshots are required")

    try:
        pre = load_snapshot(pre_path)
        post = load_snapshot(post_path)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        parser.error(str(exc))

    if pre.get("run_id") != manifest.get("run_id") or post.get("run_id") != manifest.get("run_id"):
        parser.error("snapshot run_id does not match manifest")

    tree_names = sorted(set(pre.get("trees", {})) | set(post.get("trees", {})))
    trees = {}
    totals = {"added": 0, "removed": 0, "modified": 0, "unchanged": 0}
    for name in tree_names:
        result = compare_tree(pre.get("trees", {}).get(name, []), post.get("trees", {}).get(name, []))
        trees[name] = result
        totals["added"] += len(result["added"])
        totals["removed"] += len(result["removed"])
        totals["modified"] += len(result["modified"])
        totals["unchanged"] += result["unchanged_count"]

    output = run_dir / "snapshots" / "diff.json"
    diff = {
        "schema": DIFF_SCHEMA,
        "run_id": manifest.get("run_id"),
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "pre": str(pre_path.relative_to(run_dir)),
        "post": str(post_path.relative_to(run_dir)),
        "summary": totals,
        "trees": trees,
    }
    output.write_text(json.dumps(diff, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    manifest["snapshot_diff"] = "snapshots/diff.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(output)
    print(json.dumps(totals, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
