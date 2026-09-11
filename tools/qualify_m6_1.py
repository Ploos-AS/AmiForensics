#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src" / "compare.c"


def run(cmd: list[str], **kwargs) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, text=True, capture_output=True, check=False, **kwargs)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"FAIL: {message}")


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="amiforensics-m6-1-") as td:
        tmp = Path(td)
        binary = tmp / "Compare"
        cc = run(["cc", "-O2", "-Wall", "-Wextra", "-Werror", str(SRC), "-o", str(binary)])
        require(cc.returncode == 0, f"host compile failed:\n{cc.stderr}")

        before = tmp / "before.kv"
        after = tmp / "after.kv"
        before.write_text(
            "schema=example/1\n"
            "same=unchanged\n"
            "removed=old\n"
            "modified=before\n",
            encoding="utf-8",
        )
        after.write_text(
            "schema=example/1\n"
            "same=unchanged\n"
            "modified=after\n"
            "added=new\n",
            encoding="utf-8",
        )

        kv = run([str(binary), "--kv", str(before), str(after)])
        require(kv.returncode == 0, f"KV compare failed: {kv.stderr}")
        require("schema=amiforensics.compare.kv/1" in kv.stdout, "missing Compare schema")
        require("added=1" in kv.stdout, "added count mismatch")
        require("removed=1" in kv.stdout, "removed count mismatch")
        require("modified=1" in kv.stdout, "modified count mismatch")
        require("unchanged=2" in kv.stdout, "unchanged count mismatch")
        require("before=before" in kv.stdout or "change.modified" in kv.stdout, "missing modified before value")
        require("after=after" in kv.stdout or "change.modified" in kv.stdout, "missing modified after value")
        require("truncated=false" in kv.stdout, "unexpected truncation")

        human = run([str(binary), str(before), str(after)])
        require(human.returncode == 0, "human compare failed")
        require("+ added=new" in human.stdout, "human added output missing")
        require("- removed=old" in human.stdout, "human removed output missing")
        require("~ modified: before -> after" in human.stdout, "human modified output missing")
        require("Summary: +1 -1 ~1 =2" in human.stdout, "human summary mismatch")

        missing = run([str(binary), str(tmp / "missing.kv"), str(after)])
        require(missing.returncode == 20, "missing input must return 20")

        usage = run([str(binary)])
        require(usage.returncode == 10, "bad arguments must return 10")

        big_before = tmp / "big-before.kv"
        big_after = tmp / "big-after.kv"
        payload = "".join(f"k{i}=v{i}\n" for i in range(300))
        big_before.write_text(payload, encoding="utf-8")
        big_after.write_text(payload, encoding="utf-8")
        trunc = run([str(binary), "--kv", str(big_before), str(big_after)])
        require(trunc.returncode == 5, "truncation must return 5")
        require("truncated=true" in trunc.stdout, "truncation flag missing")

    print("M6.1 Compare qualification: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
