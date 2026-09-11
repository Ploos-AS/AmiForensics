# M6.3 ARexx qualification

Status: **PASS — static/host-side qualification**

## Scope

M6.3 qualifies the repository-side contract of `rexx/AFReport.rexx`.

The script orchestrates the native `Compare` and `Report` tools without embedding additional analysis logic in ARexx. This keeps the native tools independently usable and preserves the AmigaOS 2.04+ / 68000 baseline.

This qualification is deliberately limited to static/host-side validation. It does **not** claim successful execution under Commodore RexxMast, AROS RexxMast, FS-UAE, or real Amiga hardware.

## Qualified contract

- accepts before/after KV snapshots and explicit Compare/Report output paths
- invokes `Compare --kv`
- invokes `Report --kv` over before, after, and comparison evidence
- propagates ordinary Compare/Report failures
- treats return code `5` as bounded-input truncation and propagates it after report generation
- returns usage error `10` for incomplete arguments
- has no implicit sample execution or network activity

## Automation

Qualification harness: `tools/qualify_m6_3.py`

Workflow: `.github/workflows/m6-3-arexx.yml`

GitHub Actions run: **34561448779**

Qualified commit: `991d350260928b49311541cc3ef57f2a8d39669e`

Result: **success**

## Runtime status

ARexx runtime qualification remains **UNVERIFIED** until the script is executed with RexxMast in an Amiga-compatible runtime such as FS-UAE/AROS or real AmigaOS.
