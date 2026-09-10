# M3.3b — Bounded resident signature scan qualification

Status: QUALIFIED — AROS/FS-UAE

## Evidence

- Workflow: `M3.3b MemScan resident signature scan qualification`
- Run: `34494725780`
- Job: `102930132337`
- Qualified HEAD: `12ebf1b8624de337c52203f7923f185b99d00b63`
- Result: success
- Artifact: `10159542712` (`m3-3b-resident-signature-scan-qualification`)
- Artifact digest: `sha256:a01d93c26c8597a59b618c00bffd580d60448848b6807207068c14f872daadfa`

## Qualified scope

The gate verifies the explicit `--scan-residents` mode, native 68000 build, deterministic host contract and bounded AROS/FS-UAE guest execution.

The scanner examines only bounded MemHeader-backed windows: at most 64 KiB from an individual region and 256 KiB total. Candidates require the Resident match word, self-matching `rt_MatchTag`, and an `rt_EndSkip` contained by the source region. Candidates are correlated with the already snapshotted Exec `ResModules` list.

## Completeness semantics

Zero candidates is a valid successful observation. RC 5 is also a usable result when configured scan limits make the observation intentionally incomplete; it is not a scanner failure.

## Limitations

The current implementation samples the first bounded window of each eligible memory region and can therefore miss candidates outside those windows. Name and ID pointers are reported but intentionally not dereferenced. A structural candidate is evidence only, not proof of malware. Real Commodore AmigaOS remains separately unqualified.
