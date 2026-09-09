# M1.4 BootInfo

## Scope

M1.4 adds `BootInfo`, a read-only Amiga bootblock inspection utility.

The tool inspects the first 1024 bytes of a bootblock or disk image. It never executes boot code and does not modify the input.

## Implemented

- Amiga DOS signature detection (`DOS\x00` through other DOS type bytes)
- DOS type byte reporting
- stored bootblock checksum reporting
- Amiga end-around-carry checksum verification
- root-block field reporting
- count of non-zero bytes in the boot-code area
- `--hex` 1024-byte hexdump for disassembly/reverse-engineering workflows
- `--kv` machine-readable key/value output
- conservative string indicators for:
  - `trackdisk.device`
  - `exec.library`
  - `dos.library`
  - `RexxMast`
- host CI with a generated valid bootblock and a deliberately checksum-damaged variant

## Interpretation

An indicator is not a malware verdict. Legitimate boot code may reference system libraries or devices. The indicators are intended to highlight bootblocks that deserve closer inspection.

Likewise, a valid checksum only means the bootblock checksum is internally consistent. Malware can have a perfectly valid checksum.

## ARexx decision

`BootInfo` is a short-lived static tool. ARexx can invoke it directly and consume `--kv` output, so a resident ARexx port would not currently add useful semantics.

## Qualification status

- source/build structure: implemented
- host compile/smoke CI: configured
- checksum positive/negative fixture coverage: configured
- m68k-amigaos-gcc build: not yet qualified
- AmigaOS 2.04 runtime: not yet qualified
- FS-UAE/AROS runtime: not yet qualified

No Amiga runtime qualification claim should be made until those gates have passed.

## Follow-up candidates

- boot-code extraction as a raw file
- integration with a 68k disassembler
- known bootblock/signature database matching
- richer suspicious-operation heuristics
- comparison against known-good bootblocks
