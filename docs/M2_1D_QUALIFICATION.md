# M2.1d ResidentView ARexx qualification

## Status

**PENDING LOCAL RUNTIME QUALIFICATION**

Do not change this status to PASS until the complete guest result and host validator both pass on a real AmigaOS 2.04+ environment.

## GitHub Actions preflight

GitHub Actions run `34402356672` on commit `108e258dcf672de52c85e9644af742f3ecc77896` completed successfully as an environment/build preflight.

### ARexx-enabled native build

- status: `PASS`
- gate: `M2_1D_NATIVE_AREXX_BUILD`
- mode: `AREXX_ENABLED`
- binary: `build/fs-uae/native-arexx/ResidentView-ARexx`
- binary SHA-256: `e1ace74ca58fc4e36d13b22232bb06794326fe26c9bd4e5ba2c002ea88e08e19`
- toolchain: `amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed`

### AROS ARexx preflight

The inspected AROS system contained:

- `rexxsyslib.library`: present
- `RX`: present
- `RexxMast`: **missing**

The preflight therefore correctly reported:

- `STATUS=UNAVAILABLE`
- `GATE=M2_1D_AROS_REXX_PREFLIGHT`
- `OBSERVATION=aros_missing_required_arexx_runtime`

This is an environment limitation, not a ResidentView runtime failure. The AROS image must not be used to claim an end-to-end ARexx PASS.

Evidence artifact:

- artifact: `m2-1d-residentview-arexx-preflight`
- artifact ID: `10124025490`
- artifact ZIP SHA-256: `a484724c351a7a86522a8b828610f7d21a192ada07c81c5f84099b05893bb90d`

## Required environment record for local qualification

- Git commit: `<record>`
- Date: `<record>`
- FS-UAE version: `<record>`
- Machine profile: `<record>`
- CPU: `<record>`
- Kickstart version: `<record>`
- Workbench version: `<record>`
- RexxMast: `<record running/version if available>`
- rexxsyslib.library: `<record version if available>`
- ResidentView-ARexx SHA-256: `<record>`

## Local runtime gate

Use the canonical harness in `ci/local/m2-1d/`.

Guest procedure:

1. Boot a real AmigaOS 2.04+ environment in visible FS-UAE.
2. Ensure `RexxMast` is running.
3. Start `ResidentView-ARexx --serve`.
4. Execute `RX ResidentView.rexx`.
5. Copy `RAM:m2_1d_residentview_result.txt` back to the host.
6. Run:

```sh
bash ci/local/m2-1d/validate-result.sh m2_1d_residentview_result.txt
```

PASS requires:

- `PING_RC=0`
- `PING_RESULT=PONG`
- `LIST_RESIDENTS_RC=0`
- at least one returned `resident|...` record
- `QUIT_RC=0`
- `QUIT_RESULT=BYE`
- guest `STATUS=PASS`
- host validator `STATUS=PASS`

This exercises actual ARexx message exchange through the public `RESIDENTVIEW` port rather than only testing compilation or CLI behavior.

## Guest evidence

```text
<paste complete m2_1d_residentview_result.txt here>
```

## Host validator evidence

```text
<paste validate-result.sh output here>
```

## Qualification conclusion

Pending real AmigaOS runtime evidence.

M2.1c already qualifies the ResidentView CLI snapshot under FS-UAE/AROS. M2.1d remains deliberately separate: it qualifies the live ARexx service path and will not be marked PASS from the AROS preflight alone.
