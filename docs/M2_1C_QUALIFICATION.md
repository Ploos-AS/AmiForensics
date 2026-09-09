# M2.1c ResidentView CLI qualification

## Qualified revision

- commit: `ee6a1ce48ddc1762c910a7ccded16e9787d7d46e`
- GitHub Actions run: `34401190230`
- workflow: `M2.1c ResidentView qualification`
- mode: `CLI_NO_AREXX_AUTOINIT`

## Native build gate

The Amiga CLI binary was built with the pinned Bebbo toolchain image:

`amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed`

Build constraints included `-m68000`, `-mcrt=nix20`, strict warnings and `-Werror`. The M2.1c build deliberately defines `RV_NO_AREXX=1`, so CLI qualification does not acquire an implicit `rexxsyslib.library` startup dependency.

Result:

- native Bebbo build: PASS
- output recognized as an AmigaOS `loadseg()` executable
- ResidentView binary SHA-256: `ea3030cf0ccbb9be669300a26197b1ccff7a1c1821f980cbdd95b937e836270d`

## FS-UAE / AROS guest gate

The workflow fetched the current AROS amiga-m68k boot ISO through the project qualification fetch script, extracted the guest system and booted it with FS-UAE using the internal replacement Kickstart path.

The guest executed:

`ResidentView --kind residents --kv`

Result:

- guest execution: PASS
- guest return code: `0`
- `tool=ResidentView`: present
- resident records: `67`
- `truncated=false`
- representative residents observed: `exec.library`, `dos.library`, `trackdisk.device`

FS-UAE itself was terminated by the outer workflow timeout after the evidence had been written. Therefore `FS_UAE_EXIT=124` is not a ResidentView failure in this run; the guest command had already completed successfully with RC 0 and valid output.

## Defect found and fixed during qualification

Earlier M2.1c attempts exposed two distinct runtime/build issues:

1. Direct ARexx symbol references caused `rexxsyslib.library` to become an effective startup dependency even for ordinary CLI execution. M2.1c now uses a CLI-only build mode without ARexx autoinit; ARexx is qualified separately in M2.1d.
2. `RVSnapshot` was originally stack-local. With 192 records it consumed roughly tens of kilobytes of classic-Amiga stack space and caused the AROS guest invocation to stall. The snapshot and ARexx result workspace were moved to heap allocation before the successful run.

## Qualification scope

M2.1c qualifies:

- Bebbo/m68k native compilation of the CLI path
- 68000-targeted binary generation
- FS-UAE/AROS execution
- direct `ExecBase->ResModules` enumeration in the AROS guest
- `--kind residents --kv` filtering/output

M2.1c does **not** qualify:

- real Commodore AmigaOS 2.04+ runtime behavior
- `rexxsyslib.library` availability or ABI behavior
- the persistent `RESIDENTVIEW` ARexx port
- `PING`, `LIST ...` or `QUIT` ARexx message exchange

Those are deliberately deferred to M2.1d and later real-AmigaOS qualification.
