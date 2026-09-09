# M1.3 HunkInfo

## Scope

M1.3 adds `HunkInfo`, a read-only parser for classic Amiga HUNK executables.

## Implemented

- HUNK_HEADER validation
- resident-name table skipping
- hunk table/range validation
- declared segment-size reporting
- HUNK_CODE parsing and byte totals
- HUNK_DATA parsing and byte totals
- HUNK_BSS parsing and byte totals
- HUNK_RELOC32 group parsing and relocation counts
- HUNK_SYMBOL parsing and symbol counts
- HUNK_DEBUG recognition and safe payload skipping
- HUNK_NAME and HUNK_UNIT safe skipping
- HUNK_END handling
- explicit malformed/truncated-stream failures
- explicit refusal to guess lengths for unsupported/unknown hunk types
- host-side CI coverage with synthetic valid and truncated HUNK files

## Defensive parsing policy

For forensic tooling, a plausible-looking but misparsed result is worse than a clear unsupported result. `HunkInfo` therefore stops when it encounters a hunk whose payload layout is not implemented. It does not scan forward heuristically for the next apparent hunk marker.

The current first slice intentionally leaves HUNK_EXT, HUNK_RELOC16 and HUNK_RELOC8 for a later extension.

## ARexx decision

`HunkInfo` is a short-lived static analyzer. It does not need a resident ARexx port at this stage; ARexx scripts can invoke the CLI directly. Structured output can be added when the shared AmiForensics report schema is introduced.

## Qualification status

- source/build structure: implemented
- host compile/smoke CI: configured
- synthetic valid-HUNK parse: configured
- malformed/truncated rejection: configured
- m68k-amigaos-gcc build: not yet qualified
- AmigaOS 2.04 runtime: not yet qualified
- FS-UAE/AROS runtime: not yet qualified

No Amiga runtime qualification claim is made yet.
