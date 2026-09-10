# M2.6 — Normalized system snapshot contract

Status: INITIAL CONTRACT

## Purpose

M2 has several read-only inspection tools that already expose machine-readable `--kv` output. M2.6 defines the common contract that lets host-side tooling capture, store, correlate, and later compare those outputs without forcing every Amiga binary to implement JSON itself.

The contract is deliberately small enough for AmigaOS 2.04+/68000 tools while remaining suitable for M5 workstation collection and M6 Compare/Report.

## Design rules

1. Amiga-side tools remain independently executable and keep `--kv` as the low-overhead interchange format.
2. A snapshot is immutable evidence from one observation point. Collection metadata belongs to the envelope, not to individual records.
3. Records are namespaced by tool and record index. Tool-specific fields are preserved rather than flattened away.
4. Addresses are observations, not stable identities across snapshots.
5. `truncated=true`, clipping, non-zero return codes, and unavailable collectors must be preserved explicitly. Partial evidence must never be silently presented as complete.
6. Ordering is evidence but must not be used as the sole identity of a record.
7. Host-side conversion to normalized JSON is planned for M5/M6; M2.6 standardizes the source contract first.

## Common KV header

Every system-inspection collector should converge on:

```text
tool=<ToolName>
schema=amiforensics.snapshot.kv/1
record_count=<decimal>
truncated=true|false
```

Tool-specific metadata may appear between `schema` and the records. Existing tools that do not yet emit `schema` remain readable as legacy schema version 0 until migrated.

## Common record fields

Where the concept exists, use these names consistently:

```text
record.N.kind=<type>
record.N.name=<name>
record.N.address=<8-digit uppercase hex>
record.N.priority=<signed decimal>
record.N.state=<state>
```

Additional tool-specific fields are allowed and expected.

### Address encoding

Classic 32-bit Amiga addresses use exactly eight uppercase hexadecimal digits without a `0x` prefix. A zero/null pointer is `00000000`.

### Boolean encoding

Booleans are lowercase `true` or `false`.

### Names

Names are emitted as observed, subject to each collector's bounded snapshot buffer. Consumers must not assume uniqueness.

## Collector mappings

### ResidentView

Canonical record kinds currently include:

- `resident`
- `library`
- `device`
- `port`
- `resource`
- `task-ready`
- `task-wait`
- `task-running`

Fields: `kind`, `address`, `priority`, `version`, `name`.

ResidentView is the broad Exec inventory collector. Its port/task records are useful for cross-checking but do not replace the richer dedicated collectors.

### TaskView

Canonical kind: `task`.

Fields: `kind`, `state`, `name`, `address`, `priority`.

States: `current`, `ready`, `wait`.

### ProcessView

Canonical kind: `process`.

Fields: `kind`, `state`, `name`, `address`, `priority`.

A process is identified conservatively from Exec task lists using `NT_PROCESS`; the current M2 collector does not dereference DOS Process internals.

### PortView

Canonical kind: `port`.

Fields: `kind`, `address`, `priority`, `sigbit`, `sigtask`, `name`.

PortView covers public named message ports present on Exec's `PortList`; it is not an inventory of every private `MsgPort` allocated in the system.

`sigtask` can be correlated with TaskView/ProcessView `address` within the same snapshot, but that relationship is observational and must not be assumed stable across snapshots.

### PatchView

Canonical kind is the inspected target kind: `library` or `device`.

Target-level fields include target name, base, negative size, available/requested/inspected vector counts, vector CRC32, and clipping status. Vector records retain `index`, `lvo`, `vector_address`, `opcode`, `direct_jmp`, and `target`.

PatchView is a vector-table observation. A CRC or target change is evidence of change, not by itself proof of malware.

## Snapshot envelope

The future host-side normalized representation should use this logical envelope:

```text
schema: amiforensics.snapshot/1
snapshot_id: host-generated opaque identifier
captured_at: host timestamp
platform: amiga
collectors:
  ResidentView: ...
  TaskView: ...
  ProcessView: ...
  PortView: ...
  PatchView: ...
```

The host must additionally preserve, per collector:

- executable identity/hash where available
- command line
- return code
- raw KV output
- parsed records
- completeness/truncation/clipping status

This keeps raw evidence available even if a future parser or schema changes.

## Correlation rules

Within one capture only:

- TaskView/ProcessView `address` may correlate to PortView `sigtask`.
- ResidentView library/device addresses may correlate to PatchView target bases.
- Names may support correlation but are not unique identifiers.
- Pointer equality across separately collected tools is useful evidence, but collection is not atomic; system state can change between commands.

Across captures:

- Prefer semantic identity (`kind` + `name` plus tool-specific identity fields) over address.
- Treat address changes separately from additions/removals.
- PatchView vector CRC32 is suitable as a compact change detector only when target and inspected vector range are equivalent.

## Return/completeness semantics

The established suite convention remains:

- RC 0: successful complete observation
- RC 5: usable but incomplete/clipped/truncated observation
- RC 10: usage/input error
- RC 20: operational failure such as unavailable target or memory allocation failure

Collectors may refine error reporting later, but consumers must preserve the numeric RC.

## M2.6 migration tasks

Before closing M2.6:

- add `schema=amiforensics.snapshot.kv/1` to the dedicated system-inspection KV outputs;
- add explicit `record.N.kind=task` to TaskView;
- add explicit `record.N.kind=process` to ProcessView;
- add explicit `record.N.kind=port` to PortView;
- ensure every snapshot collector emits `record_count` and an explicit completeness signal (`truncated`, or PatchView's `clipped` plus inspected count);
- add host CI assertions for the common fields;
- retain compatibility with existing field names used by qualification evidence.

## Non-goals for M2.6

M2.6 does not yet implement:

- JSON on the Amiga binaries;
- atomic whole-system capture;
- cryptographic signing of snapshots;
- malware verdicts;
- historical comparison logic;
- memory scanning.

Those belong to later milestones. M2.6 is the stable interchange boundary between M2 inspection and the M3–M6 analysis pipeline.
