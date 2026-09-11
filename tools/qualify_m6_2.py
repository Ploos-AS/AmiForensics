#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / "src" / "report.c"


def run(cmd, **kwargs):
    return subprocess.run(cmd, text=True, capture_output=True, **kwargs)


def require(condition, message):
    if not condition:
        raise SystemExit(f"FAIL: {message}")


def main():
    with tempfile.TemporaryDirectory() as td:
        tmp = pathlib.Path(td)
        exe = tmp / "Report"
        build = run(["cc", "-O2", "-Wall", "-Wextra", "-Werror", str(SRC), "-o", str(exe)])
        require(build.returncode == 0, f"host compile failed:\n{build.stderr}")

        compare = tmp / "compare.kv"
        compare.write_text(
            "tool=Compare\n"
            "schema=amiforensics.compare.kv/1\n"
            "added=1\nremoved=2\nmodified=3\nunchanged=4\n"
            "truncated=false\n",
            encoding="utf-8",
        )
        fileinfo = tmp / "fileinfo.kv"
        fileinfo.write_text(
            "tool=FileInfo\n"
            "schema=amiforensics.fileinfo.kv/1\n"
            "path=sample.bin\nsize=1234\nsha256=deadbeef\n",
            encoding="utf-8",
        )

        human = run([str(exe), str(compare), str(fileinfo)])
        require(human.returncode == 0, "human mode returned non-zero")
        require("AmiForensics Report" in human.stdout, "human header missing")
        require("Sources: 2" in human.stdout, "human source count wrong")
        require("Tool: Compare" in human.stdout, "Compare source missing")
        require("Tool: FileInfo" in human.stdout, "FileInfo source missing")

        kv = run([str(exe), "--kv", str(compare), str(fileinfo)])
        require(kv.returncode == 0, "KV mode returned non-zero")
        for expected in (
            "tool=Report",
            "schema=amiforensics.report.kv/1",
            "source.count=2",
            "warning.count=0",
            "source.0.tool=Compare",
            "source.0.schema=amiforensics.compare.kv/1",
            "source.1.tool=FileInfo",
            "source.1.schema=amiforensics.fileinfo.kv/1",
            "truncated=false",
        ):
            require(expected in kv.stdout, f"missing KV field: {expected}")

        malformed = tmp / "malformed.kv"
        malformed.write_text("tool=Broken\nthis-is-not-kv\nvalue=ok\n", encoding="utf-8")
        mal = run([str(exe), "--kv", str(malformed)])
        require(mal.returncode == 0, "malformed source should warn, not fail")
        require("warning.count=2" in mal.stdout, "missing schema + malformed warnings expected")
        require("source.0.malformed=1" in mal.stdout, "malformed count wrong")

        missing = run([str(exe), str(tmp / "missing.kv")])
        require(missing.returncode == 20, "missing input must return 20")

        usage = run([str(exe)])
        require(usage.returncode == 10, "no-args usage must return 10")

        many = tmp / "many.kv"
        with many.open("w", encoding="utf-8") as f:
            f.write("tool=Stress\nschema=stress/1\n")
            for i in range(600):
                f.write(f"k{i}=v{i}\n")
        trunc = run([str(exe), "--kv", str(many)])
        require(trunc.returncode == 5, "record overflow must return 5")
        require("truncated=true" in trunc.stdout, "record overflow must set truncated=true")

    print("M6.2 Report qualification: PASS")


if __name__ == "__main__":
    main()
