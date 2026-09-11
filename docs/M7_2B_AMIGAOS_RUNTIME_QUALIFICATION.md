# M7.2b Commodore AmigaOS runtime qualification

Status: **IN PROGRESS**

## Purpose

Qualify the complete AmiForensics native v0.1.0 tool suite on Commodore AmigaOS before the Aminet release candidate is finalized.

This gate is intentionally separate from the existing host/static CI and AROS/FS-UAE qualification claims.

## Target environment

Primary release baseline:

- CPU: 68000-compatible build
- OS: Commodore AmigaOS 2.04
- emulator: visible FS-UAE local runtime
- network: not required
- binaries: the same release-intended native binaries produced for v0.1.0

Compatibility follow-up:

- Commodore AmigaOS 3.1

A PASS on AmigaOS 2.04 is required for the advertised minimum OS baseline. AmigaOS 3.1 is used as a compatibility regression check.

## Batch strategy

The 15 native commands are qualified in three batches of five.

### Batch 1 — static/file analysis

1. FileInfo
2. Strings
3. HunkInfo
4. BootInfo
5. ResidentView

Qualification record: `docs/M7_2B_BATCH1_QUALIFICATION.md`

### Batch 2 — system and memory inspection

1. PatchView
2. TaskView
3. ProcessView
4. PortView
5. MemScan

Qualification record: `docs/M7_2B_BATCH2_QUALIFICATION.md`

### Batch 3 — extraction, observation and reporting

1. SampleDump
2. TraceExec
3. DiskWatch
4. Compare
5. Report

Qualification record: `docs/M7_2B_BATCH3_QUALIFICATION.md`

## Per-tool acceptance gate

Each command must demonstrate, where applicable:

- executable starts successfully on Commodore AmigaOS 2.04
- no missing-library/device startup failure
- help/usage or expected argument error path behaves safely
- representative normal operation completes
- invalid or unavailable input is handled without crashing
- output is readable and consistent with the documented CLI contract
- no Guru Meditation or emulator crash occurs
- return code is appropriate for the exercised path

Tools that inspect live Exec/DOS state must also demonstrate that the expected system structures can be read safely on Commodore AmigaOS.

## Evidence

For every batch record, retain:

- exact Git commit
- binary SHA-256 values recorded on the host before guest execution
- FS-UAE configuration/profile identification
- Kickstart/Workbench versions
- CPU and memory configuration
- commands executed
- return codes
- observed PASS/FAIL per tool
- relevant console output and screenshots where useful

## ARexx follow-up

`AFReport.rexx` is not one of the 15 native-command batches. After Batch 3 passes, perform a separate RexxMast runtime qualification of `AFReport.rexx` using the qualified Compare and Report binaries.

## Release gate

M7.3/Aminet release preparation remains blocked until:

1. Batch 1 passes on Commodore AmigaOS 2.04.
2. Batch 2 passes on Commodore AmigaOS 2.04.
3. Batch 3 passes on Commodore AmigaOS 2.04.
4. AmigaOS 3.1 compatibility regression is completed for the release set.
5. `AFReport.rexx` receives a RexxMast runtime PASS or is explicitly excluded from the release with its status documented.

Only after these checks may the v0.1.0 Aminet package be treated as release-ready.
