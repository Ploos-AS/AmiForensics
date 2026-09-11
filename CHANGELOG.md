# Changelog

All notable changes to AmiForensics are documented in this file.

## [0.1.0] - Release candidate

First public release candidate.

### Native Amiga toolkit

- FileInfo — file identification, fingerprints and HUNK recognition
- Strings — bounded string extraction and Amiga-specific indicators
- HunkInfo — HUNK structure, relocation and symbol inspection
- BootInfo — bootblock identification and checksum inspection
- ResidentView — resident-module inspection
- PatchView — vector, library and device patch inspection
- TaskView — Exec task inspection
- ProcessView — DOS process inspection
- PortView — message-port inspection
- MemScan — bounded memory-region and resident-signature inspection
- SampleDump — bounded explicit memory/code extraction
- TraceExec — controlled execution-state observation
- DiskWatch — disk/trackdisk state observation
- Compare — normalized pre/post key/value comparison
- Report — normalized evidence aggregation

### Automation

- `AFReport.rexx` for Compare/Report orchestration
- deterministic workstation JSON reporting
- bounded multi-sample batch preparation

### Analysis workstation

- immutable sample preparation with SHA-256 provenance
- disposable FS-UAE/AROS run profiles with networking disabled by default
- explicit emulator launch; no automatic sample execution
- pre/post snapshots and normalized comparison
- explicit artifact collection
- optional static Ghidra integration

### Release engineering

- AmigaOS 2.04+ / 68000 native baseline where practical
- complete install/package surface for the 15-command suite
- integrated M7.2 CI qualification gate
- release package inventory and SHA-256 evidence

### Qualification boundary

AROS/FS-UAE, host-side and static CI qualification claims are recorded per milestone. Real Commodore AmigaOS runtime qualification and RexxMast/ARexx runtime qualification remain separate and are not implied by v0.1.0 CI results.
