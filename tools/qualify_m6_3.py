#!/usr/bin/env python3
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "rexx" / "AFReport.rexx"


def require(text: str, needle: str) -> None:
    if needle not in text:
        raise AssertionError(f"missing expected text: {needle}")


def main() -> int:
    text = SCRIPT.read_text(encoding="utf-8")

    require(text, "parse arg before after compareOut reportOut")
    require(text, "address command")
    require(text, "Compare --kv")
    require(text, "Report --kv")
    require(text, "compareRC = rc")
    require(text, "reportRC = rc")
    require(text, "exit 10")
    require(text, "exit compareRC")
    require(text, "exit reportRC")
    require(text, "exit 5")
    require(text, "exit 0")

    compare_cmd = re.search(r"'Compare --kv .*?>.*?'", text)
    report_cmd = re.search(r"'Report --kv .*?>.*?'", text)
    if not compare_cmd:
        raise AssertionError("Compare command/redirection not found")
    if not report_cmd:
        raise AssertionError("Report command/redirection not found")

    if '"\'before\'"' not in text or '"\'after\'"' not in text:
        raise AssertionError("input paths are not quoted")
    if '"\'compareOut\'"' not in text or '"\'reportOut\'"' not in text:
        raise AssertionError("output paths are not quoted")

    if "compareRC ~= 0 & compareRC ~= 5" not in text:
        raise AssertionError("Compare RC policy missing")
    if "reportRC ~= 0 & reportRC ~= 5" not in text:
        raise AssertionError("Report RC policy missing")
    if "compareRC = 5 | reportRC = 5" not in text:
        raise AssertionError("truncation propagation missing")

    forbidden = ("RexxMast", "ADDRESS AMIFORENSICS", "OpenLibrary", "CreatePort")
    for token in forbidden:
        if token in text:
            raise AssertionError(f"unexpected embedded/runtime-port dependency: {token}")

    print("M6.3 ARexx static qualification: PASS")
    print("scope=static-script-contract")
    print("runtime=UNVERIFIED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as exc:
        print(f"M6.3 ARexx static qualification: FAIL: {exc}", file=sys.stderr)
        raise
