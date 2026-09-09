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

- ResidentView
- PatchView
- task/process/library/device/port inspection
- snapshot-friendly normalized output

## M3 — Memory analysis

- MemScan
- suspicious memory-region heuristics
- resident/code discovery
- SampleDump

## M4 — Dynamic observation

- TraceExec
- DiskWatch
- controlled event logging
- pre/post state capture

## M5 — Analysis workstation

- FS-UAE orchestration
- disposable analysis profiles
- snapshots
- artifact collection
- host-side hashing and metadata
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
