# AmiForensics Roadmap

## M0 — Foundation

- Define project scope and safety model
- Establish AmigaOS 2.04+ / 68000 baseline
- Define initial tool suite
- Add MIT license and repository structure

## M1 — Static analysis core

First usable release slice. No sample execution is required.

### M1.1 FileInfo

- File type and size
- CRC32 / MD5 / SHA-1 / SHA-256 where practical
- HUNK recognition
- executable/library/device heuristics
- entropy/simple packed-data heuristics
- machine-readable output

### M1.2 Strings

- ASCII string extraction
- configurable minimum length
- Amiga path/library/device/command indicators
- output suitable for reports

### M1.3 HunkInfo

- HUNK header and segment inventory
- CODE/DATA/BSS sizes
- relocation overview
- symbol/debug information where available
- malformed-HUNK diagnostics

### M1.4 BootInfo

- Amiga bootblock identification
- bootblock checksum verification
- disassembly-friendly dump
- suspicious bootblock behavior indicators

## M2 — System inspection

Status: **CLOSED — AROS/FS-UAE qualification scope**

- ResidentView
- PatchView
- task/process/library/device/port inspection
- snapshot-friendly normalized output

## M3 — Memory analysis

Status: **CLOSED — AROS/FS-UAE qualification scope**

- MemScan baseline memory-region inventory — qualified
- explainable suspicious memory-region heuristics — qualified
- registered resident/code discovery through Exec `ResModules` — qualified
- bounded resident-signature scanning — qualified
- SampleDump bounded explicit memory extraction — qualified

Qualification records:

- `docs/M3_1B_QUALIFICATION.md`
- `docs/M3_2B_QUALIFICATION.md`
- `docs/M3_3A_QUALIFICATION.md`
- `docs/M3_3B_QUALIFICATION.md`
- `docs/M3_4_QUALIFICATION.md`

Real Commodore AmigaOS qualification remains separate and is not implied by AROS/FS-UAE closure.

## M4 — Dynamic observation

- TraceExec
- DiskWatch
- controlled event logging
- pre/post state capture

## M5 — Analysis workstation

### M5.1 Workstation foundation — implemented

- disposable run-directory contract
- explicit sample preparation without automatic execution
- immutable input copy
- host-side SHA-256 and metadata manifest
- artifact and log directories

### M5.2 FS-UAE orchestration — implemented

- parameterized disposable AROS/A1200 profile
- operator-supplied system root; no bundled ROM/system image
- run directory mounted separately for controlled artifacts
- network integration disabled by default
- dry-run launch inspection
- explicit emulator start; no sample auto-execution
- emulator state and return code recorded in the run manifest

### Next

- M5.3 snapshots and artifact collection
- optional Ghidra integration

## M6 — Reporting and automation

- Compare
- Report
- normalized JSON schema
- ARexx automation where useful
- batch-analysis workflow

## Initial release target

The first public release should contain a meaningful static-analysis suite rather than a single proof-of-concept tool. Target contents:

- FileInfo
- Strings
- HunkInfo
- BootInfo
- shared output/reporting support
- documentation and qualification results
