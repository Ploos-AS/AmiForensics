# M1.1 FileInfo

## Scope

M1.1 introduces the first AmiForensics analysis utility: `FileInfo`.

Current implementation is deliberately read-only and does not execute the target file.

## Implemented

- AmigaOS-oriented 68000 build target
- binary file reading
- file size reporting
- CRC32 fingerprint
- Amiga HUNK header recognition (`HUNK_HEADER`)
- basic Amiga DOS disk/bootblock-data recognition
- non-zero DOS-style return codes for usage/open/read failures
- host-side CI compile and smoke test

## Deferred within FileInfo

- stronger cryptographic hashes
- detailed HUNK classification
- library/device/executable heuristics beyond HUNK recognition
- packed/crunched-data indicators
- entropy metrics
- shared machine-readable output schema
- richer indicator extraction

These are intended to be added incrementally before FileInfo is considered feature-complete for the first public release.

## ARexx decision

FileInfo is a short-lived static analyzer. M1.1 does not add a resident ARexx port because ARexx can invoke the CLI directly without losing analysis functionality.

A dedicated ARexx interface remains an option if batch/persistent operation later makes it useful. AmiForensics treats ARexx as a first-class integration mechanism where it adds useful live control or query semantics.

## Qualification status

- Source/build structure: implemented
- Host compile/smoke CI: configured
- m68k-amigaos-gcc build: not yet qualified in this milestone
- AmigaOS 2.04 runtime: not yet qualified
- FS-UAE/AROS runtime: not yet qualified

No runtime qualification claim should be made until those gates have actually passed.
