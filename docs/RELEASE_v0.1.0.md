# AmiForensics v0.1.0 release manifest

Status: **CONTENT FROZEN — qualification pending**

## Release intent

v0.1.0 is the first public AmiForensics release candidate line. The release freezes a meaningful native Amiga analysis suite together with the minimum reporting/automation surface required to use it coherently.

## Native commands

The release contains these 15 Amiga-native commands:

1. FileInfo
2. Strings
3. HunkInfo
4. BootInfo
5. ResidentView
6. PatchView
7. TaskView
8. ProcessView
9. PortView
10. MemScan
11. SampleDump
12. TraceExec
13. DiskWatch
14. Compare
15. Report

## ARexx

- `Rexx/AFReport.rexx`

The ARexx script orchestrates Compare and Report. Repository/static qualification is included; RexxMast runtime qualification remains a separate claim.

## Package layout

```text
AmiForensics-v0.1.0/
  C/
    FileInfo
    Strings
    HunkInfo
    BootInfo
    ResidentView
    PatchView
    TaskView
    ProcessView
    PortView
    MemScan
    SampleDump
    TraceExec
    DiskWatch
    Compare
    Report
  Rexx/
    AFReport.rexx
  Docs/
    README.md
    ROADMAP.md
    LICENSE
    RELEASE_v0.1.0.md
```

## Build and package contract

- `make` builds the complete 15-command native suite.
- `make check` runs repository static checks.
- `make install DESTDIR=<root>` stages the native commands, ARexx script and top-level documentation.
- `make package` creates `dist/AmiForensics-v0.1.0.tar.gz` after build, static checks and package-layout checks.
- `make package-check` verifies that all 15 native commands and required release files are present.

## Qualification claims

The release may reference only qualification scopes already evidenced in the repository:

- M2 system inspection: AROS/FS-UAE qualification scope
- M3 memory analysis: AROS/FS-UAE qualification scope
- M5 workstation: host-side CI qualification
- M6 reporting/automation: CI/static qualification as documented

Real Commodore AmigaOS qualification is not implied.

## Release gate

v0.1.0 must not be tagged as final until M7.2 integrated qualification passes and M7.3 release-candidate checks produce release artifacts and checksums successfully.
