# AmiForensics

AmiForensics is a malware-analysis, reverse-engineering and digital-forensics toolkit for classic Amiga systems.

The project combines small, composable Amiga-native tools with optional workstation-side automation for controlled analysis in emulation.

## Goals

- AmigaOS 2.04+ baseline
- 68000-compatible native tools where practical
- CLI-first tools with ARexx support where useful
- safe static inspection before dynamic execution
- Amiga-specific visibility into bootblocks, HUNK files, Exec state, resident modules, patches, devices, memory and disk activity
- machine-readable output suitable for automated analysis pipelines
- optional workstation integration with FS-UAE, Ghidra and related host-side tooling

## Native tool suite

| Tool | Purpose |
| --- | --- |
| FileInfo | Identify and fingerprint suspicious files and binaries |
| Strings | Extract and classify useful strings and indicators |
| HunkInfo | Inspect Amiga HUNK structure, segments, relocations and symbols |
| BootInfo | Analyze bootblocks, checksums and suspicious boot code |
| ResidentView | Inspect resident modules and related Exec state |
| PatchView | Inspect suspicious vectors, hooks and library/device patching |
| TaskView | Inspect Exec tasks |
| ProcessView | Inspect DOS processes |
| PortView | Inspect message ports |
| MemScan | Inventory memory and identify suspicious regions/resident signatures |
| SampleDump | Bounded explicit memory/code extraction for offline analysis |
| TraceExec | Controlled observation and pre/post state capture |
| DiskWatch | Observe relevant disk and trackdisk state/activity |
| Compare | Compare normalized machine state before and after analysis |
| Report | Aggregate normalized evidence into human- and machine-readable reports |

Individual utilities intentionally do not use the `Ami` prefix.

## Analysis workstation

The optional host-side workstation layer provides:

- immutable sample preparation and SHA-256 provenance
- disposable run directories and manifests
- parameterized FS-UAE/AROS profiles with networking disabled by default
- explicit emulator start; samples are never automatically executed
- pre/post filesystem snapshots and normalized diffs
- explicit artifact collection
- optional static Ghidra integration
- deterministic JSON reports
- bounded multi-sample batch preparation

## ARexx

`rexx/AFReport.rexx` provides an ARexx automation path around the native `Compare` and `Report` tools. Its repository-side contract is statically CI-qualified. RexxMast/Amiga runtime qualification remains separate and is not implied by that result.

## Architecture

AmiForensics is split into two cooperating layers:

1. **Amiga-native toolkit** — small native commands intended to remain useful on real hardware and in emulators.
2. **Analysis workstation** — optional host-side orchestration for snapshots, controlled execution, artifact collection, reverse engineering and report generation.

The native toolkit remains useful without the workstation layer.

## Safety model

AmiForensics is designed for defensive malware analysis and preservation work. Dynamic samples should be executed only inside isolated disposable environments. Static analysis is preferred before executing unknown code. Host-side preparation, reporting and batch workflows do not implicitly execute samples.

## Build

The native suite currently contains 15 commands and is built with the Bebbo-style `m68k-amigaos-gcc` toolchain:

```sh
make
make check
```

The default build uses `-m68000` and targets the AmigaOS 2.04+ baseline where practical.

## Qualification status

- M2 system inspection: closed for AROS/FS-UAE qualification scope
- M3 memory analysis: closed for AROS/FS-UAE qualification scope
- M5 analysis workstation: closed, host-side CI qualification PASS
- M6 reporting and automation: closed, CI qualification PASS
- M7 release engineering and integrated qualification: in progress

Real Commodore AmigaOS qualification is tracked separately and is not implied by AROS/FS-UAE or host-side CI results.

See [ROADMAP.md](ROADMAP.md) and the qualification records under `docs/` for exact scopes and evidence.

## License

MIT. Copyright Ploos AS.
