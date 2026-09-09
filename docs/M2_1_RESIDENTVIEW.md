# M2.1 ResidentView

## Scope

M2.1 starts the live system-inspection phase with `ResidentView`.

Unlike the M1 tools, this utility reads live Exec state. It is therefore implemented conservatively and must not claim Amiga runtime qualification until tested in the target environment.

## Implemented snapshot core

The Amiga-native path snapshots:

- resident modules from `ExecBase->ResModules`
- loaded libraries
- devices
- public message ports
- Exec resources
- ready tasks
- waiting tasks
- the currently running task

Each record carries a kind, address, priority, name and version data where available.

## Resident modules

`ExecBase->ResModules` is treated as the NULL-terminated array of `struct Resident *` described by classic Exec documentation.

Each candidate is accepted only when:

- `rt_MatchWord == RTC_MATCHWORD` (`0x4AFC`)
- `rt_MatchTag` points back to the same `struct Resident`

Resident name, priority and version are copied into the local snapshot while task scheduling is forbidden. No resident initialization code is called.

## Snapshot safety

Exec lists can change while they are being inspected.

`ResidentView` therefore uses a short `Forbid()` / `Permit()` window to copy list data into a fixed local snapshot. Console output and ARexx result formatting happen after `Permit()` so the tool does not hold task scheduling disabled while performing I/O.

The current fixed snapshot capacity is 192 records. If that capacity is exceeded, output is explicitly marked truncated and the program returns a warning status.

## CLI output

Human-readable output is the default.

`--kv` emits normalized key/value records suitable for scripts, ARexx wrappers and the later workstation/report pipeline.

`--kind` supports singular and convenient plural filters, including:

- `all`
- `resident` / `residents`
- `library` / `libraries`
- `device` / `devices`
- `port` / `ports`
- `resource` / `resources`
- `tasks`
- `task-ready`
- `task-wait`
- `task-running`

## ARexx service

M2.1b adds a persistent ARexx service mode:

```text
ResidentView --serve
ResidentView --serve CUSTOM.PORT
```

The default public port name is `RESIDENTVIEW`.

Implemented commands:

- `PING` -> `PONG`
- `SNAPSHOT`
- `LIST ALL`
- `LIST LIBRARIES`
- `LIST DEVICES`
- `LIST PORTS`
- `LIST TASKS`
- `LIST RESOURCES`
- `LIST RESIDENTS`
- `QUIT` -> `BYE`

Snapshot/list commands return newline-separated records when the caller requests an ARexx result. The service opens `rexxsyslib.library`, creates one public Exec message port and replies to incoming `RexxMsg` messages. Unknown commands return an error rather than being silently ignored.

The ARexx server reuses the same `take_snapshot()` and filtering logic as the CLI. There is no second inspection implementation.

## Host CI

The non-Amiga build uses clearly labelled stubs. It exists only to test shared CLI parsing, resident filtering, service-mode selection and output formatting in GitHub Actions.

Host CI does **not** validate `ExecBase->ResModules`, Exec list traversal, `rexxsyslib.library`, public-port registration or actual ARexx message exchange.

## Qualification status

- snapshot/output source: implemented
- direct resident-module enumeration: implemented
- ARexx service source: implemented
- host compile/interface smoke test: configured
- m68k-amigaos-gcc build: not yet qualified
- Exec snapshot on AmigaOS 2.04+: not yet qualified
- resident enumeration on AmigaOS 2.04+: not yet qualified
- live ARexx message exchange: not yet qualified
- FS-UAE/AROS runtime: not yet qualified

The next qualification step should build the Amiga binary with the project toolchain and exercise both CLI snapshot output and the `RESIDENTVIEW` ARexx port under FS-UAE/AROS before M2.1 is called runtime-qualified.
