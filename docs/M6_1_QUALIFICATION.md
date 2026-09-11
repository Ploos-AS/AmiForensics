# M6.1 Compare qualification

Status: **PASS**

## Scope

M6.1 qualifies the native `Compare` CLI as a bounded AmigaOS 2.04+ / 68000-compatible key/value snapshot comparison tool.

The qualification covers host compilation with warnings treated as errors and semantic behavior of the portable C implementation. It does not claim qualification on real Commodore hardware.

## Qualified behavior

- host build with `-Wall -Wextra -Werror`
- added, removed, modified and unchanged record accounting
- human-readable change output
- normalized `amiforensics.compare.kv/1` output
- retained before/after values for modified records
- usage error return code `10`
- input/read error return code `20`
- bounded-input truncation signaling with return code `5`
- `truncated=true` in normalized output when limits are exceeded

## Automation

Qualification harness: `tools/qualify_m6_1.py`

Workflow: `.github/workflows/m6-1-compare.yml`

GitHub Actions run: **34560817369**

Qualified commit: `319e8980264c3e3daf03e3b9874f2951fed09431`

Result: **success**

The repository's general CI workflow also passed for the same commit.

## Result

M6.1 is closed for the automated host-side qualification scope.

Real Commodore AmigaOS/hardware qualification remains separate.
