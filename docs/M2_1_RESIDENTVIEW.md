# M2.1 ResidentView

## Scope

M2.1 starts the live system-inspection phase with `ResidentView`.

Unlike the M1 tools, this utility reads live Exec state. It is therefore implemented conservatively and must not claim Amiga runtime qualification until tested in the target environment.

## Implemented snapshot core

The Amiga-native path snapshots:

- loaded libraries
- devices
- public message ports
- Exec resources
- ready tasks
- waiting tasks
- the currently running task

Each record carries a kind, address, priority, name and (for library-compatible records) version/revision data.

## Snapshot safety

Exec lists can change while they are being inspected.

`ResidentView` therefore uses a short `Forbid()` / `Permit()` window to copy list data into a fixed local snapshot. Console output happens after `Permit()` so the tool does not hold task scheduling disabled while performing I/O.

The current fixed snapshot capacity is 192 records. If that capacity is exceeded, output is explicitly marked truncated and the program returns a warning status.

## Output

Human-readable output is the default.

`--kv` emits normalized key/value records suitable for scripts, ARexx wrappers and the later workstation/report pipeline.

`--kind` filters output. Supported filters currently include:

- `all`
- `library`
- `device`
- `port`
- `resource`
- `tasks`
- `task-ready`
- `task-wait`
- `task-running`

## Resident modules

Despite the tool name, direct enumeration of Exec resident-module structures is intentionally not claimed in this first slice.

The exact `ExecBase` resident-module representation and traversal rules must be verified against the target SDK/runtime before following resident pointer structures. This is safer than assuming a layout and potentially walking invalid memory.

Resident-module enumeration remains part of M2.1 follow-up work.

## ARexx

ResidentView is a strong candidate for a real ARexx service interface because repeated live queries benefit from a persistent command endpoint.

The implementation is structured so snapshot collection and formatting are independent of CLI parsing. A later M2.1 follow-up can add an ARexx port without duplicating inspection logic.

Candidate commands:

- `PING`
- `SNAPSHOT`
- `LIST LIBRARIES`
- `LIST DEVICES`
- `LIST PORTS`
- `LIST TASKS`
- `LIST RESOURCES`
- `LIST RESIDENTS`
- `QUIT`

The ARexx port is not yet implemented in this slice and must not be reported as available.

## Host CI

The non-Amiga build uses a clearly labelled host stub. It exists only to test shared CLI parsing and output formatting in GitHub Actions.

Host CI does **not** validate access to Exec lists.

## Qualification status

- snapshot/output source: implemented
- host compile and interface smoke test: configured
- m68k-amigaos-gcc build: not yet qualified
- Exec list snapshot on AmigaOS 2.04+: not yet qualified
- direct resident-module enumeration: deferred
- ARexx service port: deferred
- FS-UAE/AROS runtime: not yet qualified
