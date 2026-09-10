# M2.5b PortView runtime qualification

Status: **PASS**

This qualification covers the native 68000 build of `PortView` and execution inside an AROS/FS-UAE guest.

## Qualified revision

- Repository: `Ploos-AS/AmiForensics`
- Branch: `main`
- Commit: `218b3004bef56c23f0b865c7e72585d525342626`
- Workflow: `M2.5b PortView qualification`
- Workflow run: `34480449482`
- Job: `102881617499` (`portview-qualification`)

## Gate 1 — native Bebbo build

Result: **PASS**

The workflow built `src/portview.c` with the pinned Bebbo toolchain image:

`amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed`

Compiler/runtime profile:

- `m68k-amigaos-gcc`
- `-Os -Wall -Wextra -Werror`
- `-m68000`
- `-mcrt=nix20`

The produced file was recognized as an AmigaOS `loadseg()`-able executable.

Binary SHA-256:

`b298b5c008065492514fa161530eec248d4ade77e141ce95f27a8a21fce83f7b`

## Gate 2 — AROS/FS-UAE guest execution

Result: **PASS**

Guest profile:

- Machine model: A1200
- Kickstart: `internal` (AROS)
- Command: `PortView --kv`
- Guest return code: `0`
- FS-UAE host exit: `124` (expected timeout after guest evidence was written; the emulator remained running)
- Observation: `guest_executed_portview_snapshot`

Observed guest output:

```text
tool=PortView
record.0.address=0001D266
record.0.priority=-128
record.0.sigbit=0
record.0.sigtask=00000000
record.0.name=SetPatch-01
record.1.address=0001D288
record.1.priority=-128
record.1.sigbit=0
record.1.sigtask=00000000
record.1.name=SetPatch Port
record_count=2
truncated=false
```

This demonstrates that the native 68000 binary executed in the guest, traversed the Exec public message-port list, returned normalized KV records, and completed without truncation.

## Evidence artifact

- Artifact name: `m2-5b-portview-qualification`
- Artifact ID: `10153504024`
- Artifact size: `11442` bytes
- Artifact SHA-256: `55bfa07a22a694bdb65e1d04fa76978d3da6a9c0b84235397a705d94380bdee5`

The artifact contains the native binary and checksum, toolchain provenance, AROS source/checksum evidence, FS-UAE configuration/log, and the guest result file.

## Scope and limitations

This gate qualifies `PortView` on **AROS under FS-UAE**, not on a real Commodore AmigaOS 2.04+ installation.

`PortView` enumerates public message ports present in Exec's `PortList`; it does not claim to discover private/unregistered `MsgPort` objects elsewhere in memory.

The snapshot is read-only. The implementation briefly uses `Forbid()` while traversing and copying the public list, then releases it before formatting output.

A port appearing or disappearing, or changing address/metadata, is forensic evidence of system-state change but is not by itself proof of malware.
