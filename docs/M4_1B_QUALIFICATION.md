# M4.1b TraceExec qualification

Status: **PASS — AROS/FS-UAE qualification scope**

This qualification covers the M4.1 TraceExec foundation only. It verifies the native 68000 build, the host-side output contract, and execution in an AROS guest under FS-UAE. It does **not** qualify live Exec hooking, persistent tracing, real Commodore AmigaOS 2.04+ runtime, or ARexx control.

## Qualified revision

- Commit: `005d2c524d402a3dd66495222a6eb39758007aed`
- Workflow: `M4.1b TraceExec qualification`
- Run ID: `34502957870`
- Job ID: `102957989898`
- Result: success

## Gate 1 — native Bebbo build

- Result: PASS
- Target: Motorola 68000
- CRT: `-mcrt=nix20`
- Toolchain image: `amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed`
- Binary: `build/fs-uae/native-traceexec/TraceExec`
- Binary SHA-256: `672030f4f51e22d52f4297e9f3810518e9fd25bd7f5842934c82b778c303af50`
- `file` identification: AmigaOS loadseg executable/binary

## Gate 2 — host contract

Result: PASS.

The deterministic host implementation produced the trace schema expected by M4.1:

- `tool=TraceExec`
- `schema=amiforensics.trace.kv/1`
- `mode=event-observation`
- `hooking=false`
- bounded event records with sequence, event, name, and 32-bit address
- `record_count=2`
- `truncated=false`

The gate also verified that an unsupported CLI argument is rejected.

## Gate 3 — AROS / FS-UAE runtime

Result: PASS.

- Machine model: A1200
- Kickstart: internal AROS ROM
- Guest return code: `0`
- FS-UAE host timeout return: `124`, expected because the emulator can remain running after guest evidence has been written
- Observation marker: `guest_executed_traceexec_foundation`

Observed guest output included:

```text
tool=TraceExec
schema=amiforensics.trace.kv/1
mode=event-observation
hooking=false
record.0.kind=event
record.0.sequence=0
record.0.event=snapshot-task
record.0.name=SYS:TraceExec
record.0.address=00022178
record_count=1
truncated=false
```

This proves that the m68k TraceExec foundation executes in the AROS/FS-UAE guest and can safely capture the currently running task as a bounded observation record.

## Evidence artifact

- Artifact name: `m4-1b-traceexec-qualification`
- Artifact ID: `10163065135`
- Artifact size: 12026 bytes
- Artifact SHA-256: `0da7749406f6797084c383a78097aba4acbaa100fa392e09a13bad7ba5aeb20f`

The artifact contains the native binary and checksum, toolchain provenance, AROS source provenance, FS-UAE configuration/logs, guest result evidence, and the host TraceExec output.

## Scope and limitations

M4.1b qualifies the **foundation**, not a live system-call tracer. `hooking=false` is intentional. TraceExec currently performs a short read-only snapshot of the current task and emits normalized observation records.

Not qualified here:

- Exec `SetFunction()` or other vector interception
- persistent background tracing
- process-launch interception
- filesystem/device observation
- ARexx service/control port
- real Commodore AmigaOS 2.04+ runtime

Those capabilities belong to later M4 work and must retain explicit, reversible, bounded behavior.