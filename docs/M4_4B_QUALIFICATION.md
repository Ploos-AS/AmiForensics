# M4.4b DiskWatch qualification

Status: **PASS**

## Qualified revision

- Repository: `Ploos-AS/AmiForensics`
- Branch: `main`
- Qualified HEAD: `fcbd3e0c40be28371faf7d04a1d8a462edc0ab74`
- Workflow: `.github/workflows/m4-4b-diskwatch.yml`
- Workflow run: `34517279993`
- Job: `diskwatch-qualification` (`103005734236`)

## Qualification gates

All gates completed successfully.

1. **Native build** — `DiskWatch` was built with the pinned Bebbo m68k-amigaos-gcc toolchain using the strict native build gate, targeting 68000 and the `nix20` runtime.
2. **Host contract** — the deterministic host model produced the expected bounded `TD_CHANGENUM` observation contract, including exactly one `disk-change-number-changed` event for the synthetic 7 -> 8 transition and validation of rejected out-of-range CLI arguments.
3. **AROS/FS-UAE runtime** — the native Amiga binary executed in the AROS guest under FS-UAE and queried `trackdisk.device` unit 0 through `TD_CHANGENUM`, producing the expected `amiforensics.trace.kv/1` DiskWatch output contract.

## Evidence artifact

- Artifact ID: `10168464862`
- Name: `m4-4b-diskwatch-qualification`
- Size: 13063 bytes
- SHA-256 digest: `3acdb00ebe21b063b739d05bb043be3da9630b85923b212c7ee92c97fa865d2d`
- Created: 2026-09-10T19:02:56Z
- Expires: 2026-12-09T18:55:22Z

The artifact contains the native binary and checksum, toolchain provenance, AROS source provenance, FS-UAE runtime evidence, and host-contract output.

## Scope and limitations

This qualifies the M4.4 DiskWatch foundation for the **AROS/FS-UAE qualification environment** only. It does not yet qualify behavior on real Commodore AmigaOS hardware or ROMs.

`TD_CHANGENUM` observes the trackdisk media change counter. It does **not** trace individual sector reads, sector writes, filesystem operations, or every disk-related I/O request. A run can legitimately contain zero change events when no media change occurs during the bounded observation window.

DiskWatch opens the requested device while observing it, so device open state changes transiently for the duration of the process. The implementation performs no disk-sector read or write, installs no Exec or device hooks, and patches no library or device vectors. `hooking=false` remains part of the machine-readable contract.

## Result

M4.4b is closed as **PASS** for its declared AROS/FS-UAE scope.
