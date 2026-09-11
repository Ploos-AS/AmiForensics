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

Status: **CLOSED — host-side CI qualification PASS**

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

### M5.3 Snapshots and artifact collection — implemented

- normalized pre/post workstation snapshots
- SHA-256 inventory of profile, logs and collected artifacts
- explicit file-only artifact collection
- read-only collected copies
- per-collection `index.json`
- collection metadata recorded in the run manifest

### M5.4 Snapshot comparison — implemented

- normalized pre/post diff schema
- per-tree added/removed/modified separation
- before/after metadata for modified files
- aggregate change summary
- content comparison based on SHA-256 plus size
- diff path recorded in the run manifest
- stable workstation input for M6 Compare/Report

### M5.5 Optional Ghidra integration — implemented

- optional external Ghidra dependency; nothing bundled or downloaded
- `analyzeHeadless` discovery through explicit path, `GHIDRA_HOME`, or `PATH`
- dry-run command review by default
- explicit `--analyze` opt-in
- immutable prepared sample imported into a disposable project
- static analysis only; sample is never executed
- log, state and return code recorded in the run manifest

### M5.6 Workstation qualification — PASS

- reproducible host-side qualification harness in `workstation/qualify_m5.py`
- validates immutable sample preparation and manifest schema
- validates network-disabled FS-UAE profile rendering
- validates FS-UAE dry-run without emulator start
- validates pre/post snapshots, explicit artifact collection, and snapshot diff
- validates Ghidra dry-run using a fake `analyzeHeadless`; no Ghidra analysis is executed
- validates no dynamic sample execution is enabled
- dedicated GitHub Actions workflow
- GitHub Actions run 34554439869 completed successfully

## M6 — Reporting and automation

### M6.1 Compare — CLOSED / CI qualification PASS

- native AmigaOS 2.04+ / 68000-compatible `Compare` CLI
- compares bounded key/value snapshots without executing samples
- reports added, removed, modified and unchanged fields
- human-readable output and normalized `amiforensics.compare.kv/1` output
- before/after values retained for modified fields
- bounded record storage and explicit truncation status
- qualification harness: `tools/qualify_m6_1.py`
- qualification record: `docs/M6_1_QUALIFICATION.md`
- GitHub Actions run 34560817369 completed successfully

### M6.2 Report — CLOSED / CI qualification PASS

- native AmigaOS 2.04+ / 68000-compatible `Report` CLI
- aggregates bounded normalized key/value sources
- human-readable source summary and normalized `amiforensics.report.kv/1` output
- source tool/schema metadata and warning accounting
- bounded source/record storage with explicit truncation status
- qualification harness: `tools/qualify_m6_2.py`
- qualification record: `docs/M6_2_QUALIFICATION.md`
- GitHub Actions run 34561039788 completed successfully

### M6.3 ARexx automation — CLOSED / STATIC CI qualification PASS

- `rexx/AFReport.rexx` orchestrates `Compare --kv` and `Report --kv`
- explicit input/output paths and return-code propagation
- bounded-input truncation (`RC 5`) preserved across the pipeline
- no implicit sample execution or network activity
- qualification harness: `tools/qualify_m6_3.py`
- qualification record: `docs/M6_3_QUALIFICATION.md`
- GitHub Actions run 34561448779 completed successfully
- RexxMast/Amiga runtime qualification remains separate and UNVERIFIED

### M6.4 Workstation JSON report — CLOSED / CI qualification PASS

- normalized `amiforensics.workstation.report/1` JSON schema
- aggregates an existing run manifest and selected analysis evidence
- manifest/evidence SHA-256 provenance
- deterministic JSON output for identical inputs
- bounded evidence count and evidence size
- no dynamic sample execution
- qualification harness: `workstation/qualify_m6_4.py`
- qualification record: `docs/M6_4_QUALIFICATION.md`
- GitHub Actions run 34561699368 completed successfully

### M6.5 Batch analysis workflow — IMPLEMENTATION STARTED

- prepare multiple samples as isolated workstation runs
- produce machine-readable batch index/status
- bounded sample count
- no implicit emulator start or sample execution
- continue-on-error semantics so one bad sample does not destroy the whole batch result

### Next

- complete M6.5 batch-analysis workflow and qualification
- later ARexx runtime qualification in FS-UAE/AROS and/or real AmigaOS

## Initial release target

The first public release should contain a meaningful static-analysis suite rather than a single proof-of-concept tool. Target contents:

- FileInfo
- Strings
- HunkInfo
- BootInfo
- shared output/reporting support
- documentation and qualification results
