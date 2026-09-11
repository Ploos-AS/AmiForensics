# M7.2b Batch 1 Commodore AmigaOS runtime runbook

Status: **READY FOR LOCAL EXECUTION**

This runbook qualifies Batch 1 on real Commodore AmigaOS under visible FS-UAE before the v0.1.0 Aminet release.

## Batch

- FileInfo
- Strings
- HunkInfo
- BootInfo
- ResidentView

## Required targets

Primary release baseline:

- Commodore AmigaOS 2.04
- 68000 CPU profile
- visible FS-UAE session

Compatibility regression:

- Commodore AmigaOS 3.1
- same Batch 1 binaries and test data

Record exact Kickstart, Workbench, FS-UAE and memory configuration in `docs/M7_2B_BATCH1_QUALIFICATION.md`.

## Host preparation

Build the release binaries with the Amiga toolchain, then stage the qualification kit:

```sh
make clean
make
python3 tools/prepare_m7_2b_batch1.py
```

The helper creates:

```text
build/m7-2b-batch1/
  C/
    FileInfo
    Strings
    HunkInfo
    BootInfo
    ResidentView
  TestData/
    strings.txt
    bootblock.bin
    short.bin
  Results/
  SHA256SUMS
```

Mount or copy `build/m7-2b-batch1` into the AmigaOS guest. Do not rebuild binaries between the 2.04 and 3.1 runs.

## AmigaOS 2.04 qualification

Run from the staged directory in an AmigaShell.

### 1. FileInfo

Representative operation:

```text
C/FileInfo --kv C/FileInfo >Results/FileInfo.kv
```

PASS requires normal termination and output containing at least `file=`, `size=`, `crc32=`, `sha256=` and `type=`.

Error path:

```text
C/FileInfo TestData/does-not-exist
```

PASS requires a clean error message and non-zero return code, without crash or requester loop.

### 2. Strings

Representative operation:

```text
C/Strings --indicators TestData/strings.txt >Results/Strings.txt
```

PASS requires identification of Amiga-relevant indicators including `exec.library`, `trackdisk.device`, `REXXMAST` and `SYS:Tools/AmiForensics`.

Error path:

```text
C/Strings -n 0 TestData/strings.txt
```

PASS requires rejection of the invalid minimum length with a non-zero return code.

### 3. HunkInfo

Representative operation:

```text
C/HunkInfo C/FileInfo >Results/HunkInfo.txt
```

PASS requires recognition of the built Amiga executable as `HUNK_HEADER` and a sane hunk/segment summary.

Error path:

```text
C/HunkInfo TestData/strings.txt
```

PASS requires clean rejection as not a HUNK_HEADER executable, without crash.

### 4. BootInfo

Representative operation:

```text
C/BootInfo --kv TestData/bootblock.bin >Results/BootInfo.kv
```

PASS requires a complete report including `dos_signature=yes`, checksum state, root block and indicators. The deterministic fixture intentionally has an invalid boot checksum; reporting `checksum_valid=no` is expected.

Error path:

```text
C/BootInfo TestData/short.bin
```

PASS requires clean rejection because the input is shorter than 1024 bytes.

### 5. ResidentView

Representative operations:

```text
C/ResidentView >Results/ResidentView.txt
C/ResidentView --kv --kind residents >Results/ResidentView-residents.kv
C/ResidentView --kv --kind libraries >Results/ResidentView-libraries.kv
```

PASS requires a real Exec snapshot, not the host stub, with plausible resident/library records and no crash or system instability.

Error path:

```text
C/ResidentView --kind definitely-invalid
```

The command must terminate safely. An empty filtered result is acceptable for the current CLI contract; crash, hang or Exec corruption is not.

## AmigaOS 3.1 regression

Boot the 3.1 target using the exact same staged kit and repeat all five representative operations plus error paths.

The 3.1 regression passes when behavior is equivalent or intentionally compatible and no release-blocking regression appears.

## Evidence

Retain:

- `SHA256SUMS` from the staged kit
- all files under `Results/` for both OS runs
- exact git commit
- FS-UAE version and profile/config name
- Kickstart version
- Workbench version
- CPU and memory configuration
- any screenshots needed to document abnormal behavior

## Acceptance

Batch 1 is PASS only when all five tools pass the required AmigaOS 2.04 runtime checks and the AmigaOS 3.1 compatibility regression.

A failure is release-blocking for v0.1.0 until fixed and rerun with a newly staged kit.
