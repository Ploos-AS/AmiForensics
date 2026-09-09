# M1.1 FileInfo

## Scope

M1.1 introduces the first AmiForensics analysis utility: `FileInfo`.

Current implementation is deliberately read-only and does not execute the target file.

## Implemented

- AmigaOS-oriented 68000 build target
- binary file reading
- file size reporting
- CRC32 fingerprint
- self-contained SHA-256 fingerprinting with no external crypto dependency
- Amiga HUNK header recognition (`HUNK_HEADER`)
- basic Amiga DOS disk/bootblock-data recognition
- byte-diversity metric
- conservative packed/compressed-data hint (`low`, `possible`, `high`)
- stable key/value output via `--kv`
- non-zero DOS-style return codes for usage/open/read failures
- host-side CI compile and smoke tests for both human and structured output

## Structured output

`FileInfo --kv <file>` currently emits one stable `key=value` field per line:

- `file`
- `size`
- `crc32`
- `sha256`
- `type`
- `byte_diversity`
- `packed_hint`

This intentionally avoids requiring a JSON library on the native Amiga tool. A workstation-side layer can translate the stable native format into the common AmiForensics JSON report schema later.

## Analysis note

`packed_hint` is a triage heuristic based on byte diversity, not a malware verdict and not proof that a file is packed or compressed. It should only influence analyst prioritization.

## Still deferred within FileInfo

- full HUNK structural parsing (belongs primarily in `HunkInfo`)
- library/device/executable subtype heuristics beyond the initial classification
- true Shannon entropy or block-wise entropy view
- richer indicator extraction
- common workstation-side JSON schema

These can be added incrementally without turning FileInfo into a replacement for the specialized analyzers.

## ARexx decision

FileInfo is a short-lived static analyzer. M1.1 does not add a resident ARexx port because ARexx can invoke the CLI directly without losing analysis functionality.

A dedicated ARexx interface remains an option if batch/persistent operation later makes it useful. AmiForensics treats ARexx as a first-class integration mechanism where it adds useful live control or query semantics.

## Qualification status

- Source/build structure: implemented
- Host compile/smoke CI: configured and expanded for SHA-256 / `--kv`
- m68k-amigaos-gcc build: not yet qualified in this milestone
- AmigaOS 2.04 runtime: not yet qualified
- FS-UAE/AROS runtime: not yet qualified

No runtime qualification claim should be made until those gates have actually passed.
