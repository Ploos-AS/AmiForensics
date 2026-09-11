# M6.5 Batch analysis workflow qualification

Status: **PASS**

## Scope

M6.5 qualifies the host-side batch preparation workflow implemented by `workstation/batch_prepare.py`.

The workflow prepares multiple samples as independent AmiForensics workstation runs. It does not start an emulator and never executes a sample.

## Qualified behavior

- normalized `amiforensics.workstation.batch/1` batch schema
- multiple samples prepared as isolated run directories
- SHA-256 sample identity retained through the prepared run manifest
- prepared sample copies remain read-only (`0400`)
- `dynamic_execution=false` safety contract
- continue-on-error behavior for individual sample failures
- partial batch failure reported as return code `5`
- bounded batch size: maximum 128 samples
- oversized batch rejected with return code `10`
- no implicit emulator start or sample execution

## Automation

Qualification harness: `workstation/qualify_m6_5.py`

Workflow: `.github/workflows/m6-5-batch.yml`

GitHub Actions run: **34562026640**

Qualified commit: `da08a953063423f303190e6ca16da2468333fced`

Result: **success**

## Result

M6.5 is closed for host-side CI qualification.

This qualification covers safe batch preparation and machine-readable batch status. Dynamic analysis remains an explicit, separate operator-controlled action.
