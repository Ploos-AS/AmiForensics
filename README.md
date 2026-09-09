# AmiForensics

AmiForensics is a malware-analysis, reverse-engineering and digital-forensics toolkit for classic Amiga systems.

The project focuses on small, composable Amiga-native tools backed by optional workstation-side automation for controlled analysis in emulation.

## Goals

- AmigaOS 2.04+ baseline
- 68000-compatible native tools where practical
- CLI-first tools with ARexx support where useful
- Safe static inspection before dynamic execution
- Amiga-specific visibility into bootblocks, HUNK files, Exec state, resident modules, patches, devices and disk activity
- Machine-readable output suitable for automated analysis pipelines
- Optional workstation integration with FS-UAE, Ghidra and related host-side tooling

## Planned tools

| Tool | Purpose |
| --- | --- |
| FileInfo | Identify and fingerprint suspicious files and binaries |
| HunkInfo | Inspect Amiga HUNK structure, segments, relocations and symbols |
| BootInfo | Analyze bootblocks, checksums and suspicious boot code |
| Strings | Extract and classify useful strings and indicators |
| ResidentView | Inspect resident modules, libraries, devices, tasks and ports |
| PatchView | Inspect suspicious vectors, hooks and library patching |
| MemScan | Scan memory for suspicious or anomalous code/data |
| TraceExec | Trace selected Exec/DOS/device activity from a target |
| DiskWatch | Observe relevant disk and trackdisk operations |
| SampleDump | Safely dump selected memory/code regions for offline analysis |
| Compare | Compare machine state before and after execution |
| Report | Produce normalized human-readable and machine-readable reports |

Individual utilities intentionally do not use the `Ami` prefix.

## Architecture

AmiForensics is split into two cooperating layers:

1. **Amiga-native toolkit** — small native commands intended to remain useful on real hardware and in emulators.
2. **Analysis workstation** — optional host-side orchestration for snapshots, controlled execution, artifact collection, reverse engineering and report generation.

The native toolkit must remain useful without the workstation layer.

## Safety model

AmiForensics is designed for defensive malware analysis and preservation work. Dynamic samples should be executed only inside isolated disposable environments. Static analysis is preferred before executing unknown code.

## Status

M0 — project foundation.

See [ROADMAP.md](ROADMAP.md).

## License

MIT. Copyright Ploos AS.
