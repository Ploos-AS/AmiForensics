# M3.1b — MemScan baseline qualification

Status: QUALIFIED — AROS/FS-UAE

## Evidence

- Workflow: `M3.1b MemScan qualification`
- Run: `34486646711`
- Job: `102902518472`
- Qualified HEAD: `5fb87df12430574252d2822dd1ec090a5beaa31c`
- Result: success
- Artifact: `10156092407` (`m3-1b-memscan-qualification`)
- Artifact digest: `sha256:e893dd9108087a7326bc13e87a8a756cda0c4174a36ef510aee105207cd88735`

## Qualified scope

The gate built MemScan as a native 68000 Amiga executable with the pinned Bebbo toolchain and executed the MemScan collector in an AROS guest under FS-UAE.

This qualifies the initial read-only MemHeader inventory path for the current AROS/FS-UAE CI scope.

## Limitations

This result does not qualify real Commodore AmigaOS hardware/runtime. It does not imply arbitrary raw-memory scanning, malware detection, or a malware verdict.
