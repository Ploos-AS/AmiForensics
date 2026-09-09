# M1.2 Strings

## Scope

M1.2 adds `Strings`, a read-only static analyzer for printable strings and Amiga-relevant indicators.

## Implemented

- ASCII string extraction
- configurable minimum length with `-n`
- source offset reporting
- indicator classification
- `--indicators` mode for triage-focused output
- 68000-oriented build target
- host-side CI smoke coverage

## Indicator classes

Current classes are intentionally simple and explainable:

- `library` — strings containing `.library`
- `device` — strings containing `.device`
- `arexx` — ARexx/RexxMast references
- `network` — common URL schemes
- `system` — known high-value Amiga system references
- `amiga-path-or-port` — strings containing `:` that may represent Amiga paths, assigns, volumes or port-like names
- `text` — printable string with no current indicator match

These are triage labels, not malware verdicts.

## ARexx decision

`Strings` is short-lived and stateless. A resident ARexx port is not useful at this stage; ARexx scripts can invoke the CLI directly. The output format is deliberately predictable for scripting.

If persistent batch analysis later provides a measurable benefit, an ARexx interface may be added without changing the tool's basic CLI contract.

## Qualification status

- Source/build structure: implemented
- Host compile/smoke CI: configured
- m68k-amigaos-gcc build: not yet qualified
- AmigaOS 2.04 runtime: not yet qualified
- FS-UAE/AROS runtime: not yet qualified

No runtime qualification claim should be made until those gates actually pass.
