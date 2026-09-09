# M2.1d ResidentView ARexx preflight

## Result

GitHub Actions preflight completed successfully for commit `108e258dcf672de52c85e9644af742f3ecc77896` in workflow run `34402356672`.

### Native ARexx-enabled build

- status: PASS
- mode: `AREXX_ENABLED`
- toolchain: `amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed`
- binary: `ResidentView-ARexx`
- SHA-256: `e1ace74ca58fc4e36d13b22232bb06794326fe26c9bd4e5ba2c002ea88e08e19`

### AROS ARexx runtime probe

The tested AROS image contained:

- `Libs/rexxsyslib.library`
- `Rexxc/RX`

but did **not** contain RexxMast.

The preflight therefore reports:

```text
STATUS=UNAVAILABLE
GATE=M2_1D_AROS_REXX_PREFLIGHT
OBSERVATION=aros_missing_required_arexx_runtime
```

This is not a ResidentView failure. It means the tested AROS nightly is unsuitable for qualifying a live public ARexx port because the ARexx master process is absent.

Artifact ID: `10124025490`
Artifact SHA-256: `a484724c351a7a86522a8b828610f7d21a192ada07c81c5f84099b05893bb90d`

## Decision

M2.1d runtime qualification moves to a local FS-UAE environment running real AmigaOS 2.04+ with RexxMast, `rexxsyslib.library`, and RX available.

The local qualification must verify real message exchange against `RESIDENTVIEW` with at least:

- `PING` -> `PONG`
- `LIST RESIDENTS` -> one or more resident records
- `QUIT` -> `BYE`

The reproducible guest-side procedure lives under `ci/local/m2-1d/`.

## Remaining qualification boundary

M2.1c qualifies the CLI/no-ARexx ResidentView path under FS-UAE/AROS. M2.1d will qualify the ARexx-enabled server only after a real AmigaOS 2.04+ run produces PASS evidence.
