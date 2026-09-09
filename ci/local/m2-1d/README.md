# M2.1d local ARexx qualification

The GitHub AROS preflight proves that the ARexx-enabled ResidentView binary builds, but the tested AROS image does not contain RexxMast. M2.1d therefore requires a real AmigaOS 2.04+ environment with ARexx available.

## Preconditions

- AmigaOS 2.04 or later
- RexxMast running
- `rexxsyslib.library` available
- `RX` available
- ARexx-enabled `ResidentView-ARexx` from the M2.1d native build
- `ResidentView.rexx` from this directory

## Preferred one-command flow

From the repository root, prepare the qualification bundle:

```sh
bash ci/fs-uae/build-native-arexx.sh
bash ci/local/m2-1d/qualify.sh
```

The second command creates `build/local/m2-1d/` containing the binary, ARexx script, SHA-256 evidence and environment metadata template.

Copy `ResidentView-ARexx` and `ResidentView.rexx` from that directory into a real AmigaOS 2.04+ FS-UAE guest. Ensure RexxMast is running, then execute:

```text
Run >NIL: RAM:ResidentView-ARexx --serve
RX RAM:ResidentView.rexx
```

Copy `RAM:m2_1d_residentview_result.txt` back to `build/local/m2-1d/m2_1d_residentview_result.txt`, then rerun:

```sh
bash ci/local/m2-1d/qualify.sh
```

A PASS now automatically writes:

- `validation.txt`
- result SHA-256
- `qualification-summary.txt`

Set environment values before the final run when known, for example:

```sh
FS_UAE_VERSION='3.2.35' \
KICKSTART_VERSION='37.175' \
WORKBENCH_VERSION='37.67' \
CPU_PROFILE='A1200/68020' \
bash ci/local/m2-1d/qualify.sh
```

## PASS requirements

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
