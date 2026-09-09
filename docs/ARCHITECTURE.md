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

ARexx support is desirable for tools that benefit from orchestration or live queries. It is not required where a simple command invocation and structured output are sufficient.

## Dynamic-analysis boundary

Unknown samples are never assumed safe. Dynamic execution belongs in isolated disposable systems. Host integration must avoid exposing unnecessary host filesystems, networking or credentials to the guest.
