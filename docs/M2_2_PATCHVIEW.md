# M2.2 PatchView

## Purpose

`PatchView` inspects live Amiga library jump vectors without modifying them.

The initial M2.2 implementation is deliberately observational. It reports the current vector entry bytes and direct jump targets, but it does **not** label a vector malicious or patched without an external baseline or stronger evidence.

## CLI

```text
PatchView [--kv] [--library NAME] [--count N]
```

Defaults:

- library: `exec.library`
- vector count: 32
- maximum vector count: 256

Examples:

```text
PatchView --library exec.library --count 32
PatchView --library dos.library --count 64 --kv
```

## What is recorded

For each 6-byte library vector slot the tool records:

- ordinal index
- LVO offset (`-6`, `-12`, ...)
- vector address
- first opcode word
- whether the slot is a direct absolute-long `JMP` (`0x4EF9`)
- direct jump target when that encoding is present

On classic 68k Amiga library bases this provides a normalized view suitable for later pre/post comparison and known-good baseline matching.

## Safety model

`PatchView` is read-only.

It does not call `SetFunction()`, write to a vector table, restore vectors, execute discovered targets, or attempt automatic repair.

Non-`0x4EF9` entries are reported as observations rather than automatically classified as suspicious because alternate encodings, stubs, platform differences, or legitimate software may exist.

## Host CI

The non-Amiga build provides deterministic vector records solely to exercise argument parsing and human/KV output in CI.

Host CI does not qualify real Amiga library-vector layout or runtime memory reads.

## Qualification status

- CLI and normalized output: implemented
- host smoke coverage: implemented
- Amiga-native source path: implemented
- exec.library vector inspection on target AmigaOS: not yet runtime-qualified
- dos.library vector inspection on target AmigaOS: not yet runtime-qualified
- known-good baseline comparison: not yet implemented
- ARexx service: not yet implemented

## Next slice

M2.2b should add Amiga-native qualification under FS-UAE and capture stable evidence for at least `exec.library` and `dos.library` before adding baseline/diff semantics.
