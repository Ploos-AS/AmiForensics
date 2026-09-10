# M3.2b — MemScan explainable heuristic qualification

Status: QUALIFIED — AROS/FS-UAE

## Evidence

- Workflow: `M3.1b MemScan qualification`
- Run: `34492686417`
- Job: `102923190344`
- Qualified HEAD: `62c7b4a70a342093022a3bfafb35ba3a17190657`
- Result: success
- Artifact: `10158527554`
- Artifact digest: `sha256:913872ac0766d77d5d31961498b870e3f2b8667ac8f5f9fb77f37ea47ea8d192`

## Qualified scope

This requalification covers MemScan's explainable structural anomaly fields and normalized risk output in the AROS/FS-UAE guest path. Findings are evidence-oriented: invalid/zero ranges, free-space inconsistencies, odd boundaries and unnamed regions contribute explicit scores rather than opaque malware classification.

## Semantics

Risk values are evidence scores only. Return code is based on collection completeness/truncation and is not changed merely because a suspicious region is reported.

## Limitations

This result does not qualify real Commodore AmigaOS. The heuristic model is intentionally conservative and does not establish that any memory region contains malware.
