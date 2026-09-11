# M7.2b Batch 1 qualification

Status: **PENDING LOCAL COMMODORE AMIGAOS RUNTIME**

## Tools

1. FileInfo
2. Strings
3. HunkInfo
4. BootInfo
5. ResidentView

## Required environments

- Commodore AmigaOS 2.04 — required release-baseline PASS
- Commodore AmigaOS 3.1 — compatibility regression
- 68000-compatible v0.1.0 binaries
- visible FS-UAE local runtime

## Evidence to record

- qualified Git commit:
- host SHA-256 for each binary:
- FS-UAE version:
- FS-UAE profile/config:
- Kickstart version:
- Workbench version:
- CPU/memory configuration:

## Results

| Tool | 2.04 startup | 2.04 representative operation | 2.04 error path | 3.1 regression | Result |
| --- | --- | --- | --- | --- | --- |
| FileInfo | PENDING | PENDING | PENDING | PENDING | PENDING |
| Strings | PENDING | PENDING | PENDING | PENDING | PENDING |
| HunkInfo | PENDING | PENDING | PENDING | PENDING | PENDING |
| BootInfo | PENDING | PENDING | PENDING | PENDING | PENDING |
| ResidentView | PENDING | PENDING | PENDING | PENDING | PENDING |

## Acceptance

Batch 1 passes only when all five tools satisfy the M7.2b per-tool acceptance gate on AmigaOS 2.04 and complete the AmigaOS 3.1 compatibility regression without a release-blocking defect.
