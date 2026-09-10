# M2.6 — Normalized system snapshot contract

Status: QUALIFIED — M2 CLOSED

## Purpose

M2 defines the common machine-readable snapshot contract used by ResidentView, PatchView, TaskView, ProcessView and PortView. The contract is intentionally lightweight for AmigaOS 2.04+/68000 while remaining suitable for later workstation collection, comparison and reporting.

## Common KV header

```text
tool=<ToolName>
schema=amiforensics.snapshot.kv/1
record_count=<decimal>
truncated=true|false
```

Tool-specific metadata and record fields are preserved. Classic addresses use eight uppercase hexadecimal digits without `0x`; booleans are lowercase `true`/`false`.

## Qualified collectors

- ResidentView — residents, libraries, devices, ports, resources and tasks
- PatchView — library/device vector-table observations and CRC32 fingerprints
- TaskView — current/ready/wait tasks
- ProcessView — conservative `NT_PROCESS` inventory
- PortView — public named Exec message ports

TaskView, ProcessView and PortView expose explicit record kinds. PatchView exposes vector records with `record.N.kind=vector`. ResidentView already provides per-record kinds.

## Correlation rules

Within one capture, pointer equality can be useful evidence: PortView `sigtask` can correlate with TaskView/ProcessView addresses, and ResidentView library/device addresses can correlate with PatchView target bases. Collection is not atomic, so address equality is observational rather than a durable identity.

Across captures, consumers should prefer semantic identity such as kind/name and tool-specific fields. Address changes should be reported separately from additions/removals. PatchView CRC32 comparisons are meaningful only when target and inspected vector range match.

## Return/completeness semantics

- RC 0 — successful complete observation
- RC 5 — usable but incomplete, clipped or truncated observation
- RC 10 — usage/input error
- RC 20 — operational failure

Partial evidence must not be silently treated as complete.

## Qualification closure

M2.6a normalized TaskView, ProcessView and PortView and passed host CI plus their dedicated AROS/FS-UAE runtime qualifications.

M2.6b normalized ResidentView and PatchView, corrected the ResidentView `ResModules` bounds-check order, and corrected PatchView so `--unit 0` without `--device` is rejected. The dedicated ResidentView and PatchView AROS/FS-UAE qualification workflows completed successfully on the M2.6b head.

This closes M2 System Inspection for the current AROS/FS-UAE qualification scope. Real Commodore AmigaOS qualification remains a separate runtime qualification concern and is not implied by the AROS result.

## Non-goals

M2 does not implement JSON on the Amiga binaries, atomic whole-system capture, malware verdicts, historical comparison logic or memory-content scanning. Those belong to later milestones.
