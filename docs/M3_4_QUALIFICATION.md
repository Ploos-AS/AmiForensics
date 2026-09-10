# M3.4 — SampleDump qualification

Status: QUALIFIED — AROS/FS-UAE

## Evidence

- Workflow: `M3.4 SampleDump qualification`
- Run: `34500911441`
- Job: `102951160623`
- Qualified HEAD: `23c6311c8d2f56b4a0ae7b026500f3b283886f47`
- Result: success
- Artifact: `10162171997` (`m3-4-sampledump-qualification`)
- Artifact digest: `sha256:acb2bef418491f7cb3d668c16329743d072e7fd088e5f9ab9d2daf426dc89f66`

## Qualified scope

SampleDump requires an explicit address, length and output path. The requested range must lie wholly within one known Exec MemHeader-backed region and the hard maximum dump size is 65536 bytes.

The qualification verifies native 68000 build, deterministic host bounded-dump behavior, rejection of oversized and out-of-range requests, and successful AROS/FS-UAE guest dumping with CRC32 and region metadata.

## Safety and semantics

SampleDump is read-only with respect to guest memory and never performs an implicit whole-memory dump. Output creation is explicit. If a write fails, the partial output is removed.

## Limitations

This result qualifies the current AROS/FS-UAE CI scope only. It does not qualify real Commodore AmigaOS hardware/runtime or establish that dumped data is malicious.
