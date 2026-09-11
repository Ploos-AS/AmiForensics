# M7.1 Release baseline audit

Status: **CLOSED — baseline reconciled**

## Repository baseline

Audit starting point: `220bd1eb917754a054fb6f503ad69c3687f464e3`

M7.1 reconciles the repository before integrated qualification and the first public release candidate.

## Native tool inventory

The Makefile builds 15 native commands:

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

All are represented in the reconciled README and the frozen v0.1.0 release manifest.

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

## Resolved during M7.1

- README status was stale at M0 and has been reconciled with the implemented repository state.
- README tool inventory omitted TaskView, ProcessView and PortView; the inventory now matches the 15 Makefile targets.
- workstation, ARexx and qualification scope are documented at top level.
- Makefile now exposes explicit `install`, `package-stage`, `package-check` and `package` targets.
- package layout is deterministic and contains all 15 native commands, ARexx automation and release documentation.
- v0.1.0 content is frozen in `docs/RELEASE_v0.1.0.md`.

## Remaining release-gate work

- M7.2 must provide a single integrated qualification gate that exercises the complete release surface.
- M7.3 must define changelog/release notes, artifact checksum generation and final release-candidate validation.
- real Commodore AmigaOS qualification remains outside the current automated closure scope.

## M7.1 acceptance gate

- README/ROADMAP/tool inventory consistent — PASS
- explicit install/package surface defined — PASS
- first-release content set frozen — PASS
- integrated M7.2 qualification inputs defined — PASS: complete native suite, package layout, ARexx/static contract and workstation/reporting interoperability

## Result

M7.1 is closed. The repository baseline and v0.1.0 release contents are now frozen for M7.2 integrated qualification.

## Next

Implement M7.2 integrated qualification and CI around the frozen v0.1.0 release surface.
