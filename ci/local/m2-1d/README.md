# M2.1d local ARexx qualification

The GitHub AROS preflight proves that the ARexx-enabled ResidentView binary builds, but the tested AROS image does not contain RexxMast. M2.1d therefore requires a real AmigaOS 2.04+ environment with ARexx available.

## Preconditions

- AmigaOS 2.04 or later
- RexxMast running
- `rexxsyslib.library` available
- `RX` available
- ARexx-enabled `ResidentView-ARexx` from the M2.1d native build
- `ResidentView.rexx` from this directory

## Qualification procedure

1. Copy `ResidentView-ARexx` to the guest, for example `RAM:ResidentView`.
2. Copy `ResidentView.rexx` to the guest, for example `RAM:ResidentView.rexx`.
3. Ensure RexxMast is running.
4. Start the server:

```text
Run >NIL: RAM:ResidentView --serve
```

5. Execute the qualification script:

```text
RX RAM:ResidentView.rexx
```

6. Copy the result back to the host:

```text
RAM:m2_1d_residentview_result.txt
```

7. Validate it from the repository root:

```sh
bash ci/local/m2-1d/validate-result.sh m2_1d_residentview_result.txt
```

A PASS requires all of the following:

- `PING_RC=0`
- `PING_RESULT=PONG`
- `LIST_RESIDENTS_RC=0`
- `LIST RESIDENTS` returned at least one `resident|...` record
- `QUIT_RC=0`
- `QUIT_RESULT=BYE`
- guest `STATUS=PASS`
- host validator `STATUS=PASS`

## Evidence

Record the complete guest result and validator output in `docs/M2_1D_QUALIFICATION.md`, together with:

- exact Git commit
- Kickstart and Workbench versions
- CPU/machine profile
- FS-UAE version
- ResidentView-ARexx SHA-256
- RexxMast/rexxsyslib versions where available

## Scope

This test qualifies live ARexx message exchange and ResidentView resident enumeration in the tested AmigaOS environment.

Do not treat the AROS preflight as an ARexx runtime PASS: the tested AROS nightly included `rexxsyslib.library` and `RX` but did not include RexxMast.
