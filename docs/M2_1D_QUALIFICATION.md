# M2.1d ResidentView ARexx qualification

## Status

**PENDING LOCAL RUNTIME QUALIFICATION**

Do not change this status to PASS until the complete guest result and host validator both pass on a real AmigaOS 2.04+ environment.

## Required environment record

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

## Gate

Guest procedure:

1. Start RexxMast if it is not already running.
2. Start `ResidentView-ARexx --serve`.
3. Execute `RX ResidentView.rexx`.
4. Copy `RAM:m2_1d_residentview_result.txt` back to the host.
5. Run:

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

## Guest evidence

```text
<paste complete m2_1d_residentview_result.txt here>
```

## Host validator evidence

```text
<paste validate-result.sh output here>
```

## Qualification conclusion

Pending.

The AROS GitHub preflight is not sufficient for this gate because the tested AROS image lacked RexxMast even though `rexxsyslib.library` and `RX` were present. M2.1d specifically requires live ARexx message exchange on the tested AmigaOS environment.
