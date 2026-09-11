# M6.2 Report qualification

Status: **PASS**

## Scope

M6.2 qualifies the native `Report` CLI as a bounded AmigaOS 2.04+ / 68000-compatible aggregator for normalized AmiForensics key/value output.

The qualification covers host compilation with warnings treated as errors and semantic behavior of the portable C implementation. It does not claim qualification on real Commodore hardware.

## Qualified behavior

- host build with `-Wall -Wextra -Werror`
- multiple key/value input sources
- human-readable source summary
- normalized `amiforensics.report.kv/1` output
- source tool/schema metadata
- record/source counts
- malformed-input warning accounting
- bounded source and record storage
- usage error return code `10`
- input/read error return code `20`
- truncation signaling with return code `5`
- `truncated=true` in normalized output when limits are exceeded

## Automation

Qualification harness: `tools/qualify_m6_2.py`

Workflow: `.github/workflows/m6-2-report.yml`

GitHub Actions run: **34561039788**

Qualified commit: `76c31c87e8452567b2cdd5e59908a461714874e9`

Result: **success**

## Result

M6.2 is closed for the automated host-side qualification scope.

Real Commodore AmigaOS/hardware qualification remains separate.
