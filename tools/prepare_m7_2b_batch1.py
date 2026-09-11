#!/usr/bin/env python3
"""Prepare the M7.2b Batch 1 Commodore AmigaOS runtime qualification kit.

This helper does not run an emulator and does not qualify AmigaOS runtime.
It stages already-built m68k binaries plus deterministic non-malicious test data
for transfer/mounting into the local FS-UAE Commodore AmigaOS environment.
"""

from __future__ import annotations

import hashlib
import shutil
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build" / "m7-2b-batch1"
TOOLS = ("FileInfo", "Strings", "HunkInfo", "BootInfo", "ResidentView")


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def write_boot_fixture(path: Path) -> None:
    # Deliberately invalid checksum; the runtime test expects BootInfo to report
    # the fixture cleanly, not to treat it as valid boot code.
    data = bytearray(1024)
    data[0:4] = b"DOS\x00"
    data[8:12] = struct.pack(">I", 880)
    marker = b"AmiForensics Batch1 exec.library trackdisk.device"
    data[12 : 12 + len(marker)] = marker
    path.write_bytes(data)


def main() -> int:
    missing = [name for name in TOOLS if not (ROOT / name).is_file()]
    if missing:
        print("Missing built binaries: " + ", ".join(missing))
        print("Run 'make' with m68k-amigaos-gcc before preparing the kit.")
        return 2

    if OUT.exists():
        shutil.rmtree(OUT)
    (OUT / "C").mkdir(parents=True)
    (OUT / "TestData").mkdir()
    (OUT / "Results").mkdir()

    sums = []
    for name in TOOLS:
        src = ROOT / name
        dst = OUT / "C" / name
        shutil.copy2(src, dst)
        sums.append(f"{sha256(dst)}  C/{name}")

    text = OUT / "TestData" / "strings.txt"
    text.write_bytes(
        b"plain text\n"
        b"exec.library\n"
        b"trackdisk.device\n"
        b"REXXMAST\n"
        b"SYS:Tools/AmiForensics\n"
    )
    sums.append(f"{sha256(text)}  TestData/strings.txt")

    boot = OUT / "TestData" / "bootblock.bin"
    write_boot_fixture(boot)
    sums.append(f"{sha256(boot)}  TestData/bootblock.bin")

    short = OUT / "TestData" / "short.bin"
    short.write_bytes(b"DOS\x00")
    sums.append(f"{sha256(short)}  TestData/short.bin")

    (OUT / "SHA256SUMS").write_text("\n".join(sums) + "\n", encoding="ascii")

    print(f"M7.2b Batch 1 kit prepared: {OUT}")
    print("Copy or mount this directory into the visible FS-UAE AmigaOS guest.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
