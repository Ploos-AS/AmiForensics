# M2.4b ProcessView qualification

Status: **PASS**

Qualified revision: `de8a48a2821b141c92bb8e27924b2657a7939dbe`

GitHub Actions run: `34467038137`

Job: `102837918965`

## Gates

1. Native Amiga build with the pinned Bebbo m68k-amigaos-gcc toolchain: PASS.
2. ProcessView execution in an AROS A1200 FS-UAE guest: PASS.
3. Qualification evidence artifact upload: PASS.

## Native evidence

Toolchain image:

`amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed`

Binary:

`build/fs-uae/native-processview/ProcessView`

Binary SHA-256:

`9215d9385c81266777c8ba029f07b6524a36ba6f16cd5960a2c4519b9e3726a4`

The output was identified as an AmigaOS loadseg()-able executable.

## Guest evidence

Gate: `M2_4B_AROS_GUEST_PROCESSVIEW`

Model: `A1200`

Kickstart: `internal`

FS-UAE exit: `124` (expected timeout after guest evidence was produced)

Observation: `guest_executed_processview_all_and_current`

`ALL_GUEST_RC=0`

`CURRENT_GUEST_RC=0`

The complete snapshot reported seven process records and `truncated=false`. The filtered current-process snapshot reported one record:

- state: `current`
- name: `SYS:ProcessView`
- address: `00028560`
- priority: `0`

The all-process snapshot also observed waiting processes including `DF0`, `DH0`, `Lib & Dev Loader Daemon`, `Workbench Handler`, `CON`, and `Boot Mount`.

## Artifact

Name: `m2-4b-processview-qualification`

Artifact ID: `10148027700`

Artifact ZIP SHA-256:

`9ad06e79c269c9e9fba4232bb34970559610c95d1bd20d358d555710f61cf260`

## Scope and limitations

This qualification covers the pinned Bebbo 68000 build and runtime execution under AROS in an A1200 FS-UAE guest. It does not by itself qualify every real Commodore AmigaOS 2.04+ configuration.

ProcessView is read-only. It identifies DOS processes by the Exec task node type `NT_PROCESS`, snapshots current/ready/wait process records under a short `Forbid()`/`Permit()` interval, and formats output after leaving the critical section. Process names, addresses, priorities, and scheduler states are point-in-time observations and may change immediately after the snapshot.

Process-specific DOS internals beyond the Exec task identity are intentionally not dereferenced in M2.4b; those can be added only where the lifetime and pointer-safety rules are clear enough for forensic use.
