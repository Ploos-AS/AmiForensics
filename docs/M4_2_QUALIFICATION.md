# M4.2 TraceExec task lifecycle watch qualification

Status: **PASS — AROS/FS-UAE qualification scope**

## Scope

M4.2 adds bounded, polling-based task lifecycle observation to `TraceExec` without installing Exec hooks.

The qualified interface is:

```text
TraceExec [--kv] --watch-tasks SAMPLES [--interval TICKS]
```

The implementation takes short task-list snapshots under `Forbid()` / `Permit()`, compares consecutive snapshots outside the critical section, and emits `task-added` and `task-removed` events. Sampling is bounded to 2–16 snapshots and the interval is bounded to 1–50 ticks.

`hooking=false` is part of the output contract. M4.2 does not use `SetFunction()`, does not patch Exec vectors, and does not claim lossless tracing of short-lived tasks between samples.

## Qualification result

GitHub Actions workflow: `M4.2 TraceExec task lifecycle watch qualification`

- Run ID: `34504983609`
- Qualified HEAD: `10bc4da0093ee3f0410e35cbe270eda2fc9bf058`
- Job: `traceexec-task-watch-qualification`
- Result: **SUCCESS**

All gates passed:

1. Native 68000 TraceExec build with the pinned Bebbo toolchain.
2. Deterministic host contract test for bounded task lifecycle events and argument validation.
3. AROS/FS-UAE guest execution of the bounded task watch.
4. Qualification evidence upload.

Qualification artifact:

- Name: `m4-2-traceexec-task-watch-qualification`
- Artifact ID: `10163811031`
- SHA-256: `57905999811d9d0602e72cdf9b1ce6aba8a1b6411df79de90a37e4f6931fa087`

## Qualified properties

- native `-m68000 -mcrt=nix20` build succeeds with warnings treated as errors;
- host-side deterministic lifecycle comparison contract succeeds;
- AROS/FS-UAE guest can execute the bounded watch mode successfully;
- task-list snapshots are bounded;
- event storage is bounded;
- polling interval and sample count are bounded;
- output uses `amiforensics.trace.kv/1`;
- `hooking=false` remains explicit.

## Limitations

This PASS applies to the AROS/FS-UAE qualification environment only. It does **not** qualify real Commodore AmigaOS 2.04+ runtime behavior.

Polling can miss tasks that start and terminate completely between snapshots. Task addresses are observations, not stable identities across boots. A task appearing or disappearing is forensic evidence, not a malware verdict.

M4.2 is intentionally a low-risk observation stage before any optional future hook-based tracing.
