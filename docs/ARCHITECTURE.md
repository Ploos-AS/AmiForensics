# Architecture

## Design principles

AmiForensics tools should be:

- small and composable
- useful independently
- conservative around unknown samples
- scriptable
- compatible with AmigaOS 2.04+ unless a tool documents a stronger requirement
- 68000-compatible where practical
- usable both on real Amiga hardware and under emulation

## Native layer

The native layer owns Amiga-specific inspection that is difficult to reproduce accurately from a host:

- Exec structures
- residents
- libraries/devices
- vectors and patches
- tasks/processes
- message ports
- memory inspection
- trackdisk activity

Tools should prefer read-only observation whenever possible.

## Workstation layer

The workstation layer may provide:

- FS-UAE lifecycle control
- isolated analysis environments
- snapshots and reset
- sample ingress/egress controls
- artifact collection
- host-side hashing
- Ghidra project generation/import helpers
- timeline and report aggregation

The workstation layer must not be required for ordinary static native tools.

## Output

Human-readable CLI output is mandatory.

Machine-readable output should converge on a shared format so workstation automation does not need per-tool parsers. JSON is the preferred host interchange format; native implementation details remain open until resource cost is measured.

## ARexx

ARexx is a first-class integration mechanism where it provides useful live control, queries or orchestration. It is not added merely as a checkbox to every utility.

Tools that maintain live system state or benefit from interactive automation should normally expose an ARexx port. Likely candidates include ResidentView, PatchView, MemScan, TraceExec, DiskWatch, Compare and Report/workflow control.

Short-lived static commands such as FileInfo, Strings, HunkInfo and BootInfo may initially remain ordinary CLI programs when launching them from ARexx already provides the necessary automation. If persistent/batch operation later makes a dedicated ARexx port useful, the interface can be added without changing their analysis semantics.

Where an ARexx port is provided:

- commands and result codes must be documented
- query operations should be read-only by default
- machine-readable results should match the normal CLI/report schema where practical
- port naming should be stable and collision-aware
- ARexx support must not weaken the AmigaOS 2.04+ baseline without explicit documentation

## Dynamic-analysis boundary

Unknown samples are never assumed safe. Dynamic execution belongs in isolated disposable systems. Host integration must avoid exposing unnecessary host filesystems, networking or credentials to the guest.
