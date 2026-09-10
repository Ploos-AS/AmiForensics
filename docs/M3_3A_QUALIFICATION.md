# M3.3a — Resident/code discovery qualification

Status: QUALIFIED — AROS/FS-UAE

## Evidence

- Workflow: `M3.3 MemScan resident discovery qualification`
- Run: `34493666641`
- Job: `102926517171`
- Qualified HEAD: `5af7d5952f145579b23bd931a208578ebb898505`
- Result: success
- Artifact: `10159007695` (`m3-3-resident-discovery-qualification`)
- Artifact digest: `sha256:74ec266829e4fc8fca5ae1c652bb77a53890af4e2e49cc9f23c1a657050694cb`

## Qualified scope

The qualification verifies native 68000 MemScan build, deterministic host-side resident discovery contract, and AROS/FS-UAE guest execution of resident discovery sourced from Exec `ResModules`.

Discovered entries are validated as Resident structures and correlated with snapshotted MemHeader regions where possible.

## Limitations

This path discovers registered Exec resident modules. It is not a hidden-resident scanner and does not search arbitrary address space. Resident metadata is structural evidence and is not a malware verdict. Real Commodore AmigaOS remains separately unqualified.
