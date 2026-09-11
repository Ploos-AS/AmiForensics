# M6.4 Workstation JSON report qualification

Status: **PASS**

## Scope

M6.4 qualifies the host-side `workstation/report.py` report builder.

The tool aggregates an existing workstation run manifest and selected evidence into a deterministic JSON document. It only reads existing files and does not execute the prepared sample.

## Qualified behavior

- normalized `amiforensics.workstation.report/1` schema
- embedded run-manifest metadata with SHA-256 provenance
- JSON evidence decoding
- text evidence preservation
- SHA-256 provenance for evidence files
- deterministic output for identical inputs
- maximum of 32 evidence files
- maximum individual evidence size of 4 MiB
- malformed JSON evidence rejection
- missing manifest rejection

## Automation

Qualification harness: `workstation/qualify_m6_4.py`

Workflow: `.github/workflows/m6-4-workstation-report.yml`

GitHub Actions run: **34561699368**

Qualified commit: `b902b180a691a719f57f5206dbd07881eed241ef`

Result: **success**

## Result

M6.4 is closed for the host-side CI qualification scope.

The report builder performs no dynamic sample execution.
