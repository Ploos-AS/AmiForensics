# M7.1 Release baseline audit

Status: **IN PROGRESS**

## Repository baseline

Audit starting point: `220bd1eb917754a054fb6f503ad69c3687f464e3`

M7.1 reconciles the repository before integrated qualification and the first public release candidate.

## Native tool inventory

The Makefile currently builds 15 native commands:

1. FileInfo
2. Strings
3. HunkInfo
4. BootInfo
5. ResidentView
6. PatchView
7. TaskView
8. ProcessView
9. PortView
10. MemScan
11. SampleDump
12. TraceExec
13. DiskWatch
14. Compare
15. Report

All are represented in the reconciled README.

## Workstation inventory

Implemented host-side surfaces include:

- sample/run preparation
- FS-UAE profile preparation and explicit launch orchestration
- pre/post snapshots
- artifact collection
- snapshot comparison
- optional static Ghidra import/analysis
- deterministic JSON report generation
- bounded multi-sample batch preparation

## Automation and qualification inventory

Dedicated workflows exist for the M2, M3, M4, M5 and M6 qualification slices, including M6.1 through M6.5.

M5 and M6 are formally closed for their documented host/static CI scopes. M2 and M3 are closed for their documented AROS/FS-UAE scopes. Real Commodore AmigaOS qualification remains separate.

## Findings

### Resolved during M7.1

- README status was stale at M0 and has been reconciled with the implemented repository state.
- README tool inventory omitted TaskView, ProcessView and PortView; the inventory now matches the 15 Makefile targets.
- workstation, ARexx and qualification scope are now documented at top level.

### Release-gate gaps

- no explicit install/package target is present in the Makefile yet
- no single integrated qualification gate currently builds/checks the complete release surface as one M7 workflow
- release version/changelog/artifact/checksum policy is not yet defined
- real Commodore AmigaOS qualification remains outside the current automated closure scope

## M7.1 acceptance gate

M7.1 can close when:

- README/ROADMAP/tool inventory are consistent
- an explicit install/package surface is defined
- the first-release content set is frozen
- integrated M7.2 qualification inputs are defined

## Next

Implement the release packaging/install surface and freeze the v0.1.0 content manifest before starting M7.2 integrated qualification.
