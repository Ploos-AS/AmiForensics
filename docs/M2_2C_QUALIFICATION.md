# M2.2c PatchView device qualification

Status: **PASS**

Qualified revision: `77a1239e467a955b9e03ee97b87f850b7009934b`

GitHub Actions workflow: `M2.2c PatchView device qualification`

- Run ID: `34478392254`
- Job ID: `102874724689`

## Gate 1 — native Bebbo build

Result: **PASS**

PatchView was built as a classic AmigaOS loadseg executable using the pinned Bebbo toolchain image:

`amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed`

Binary:

`build/fs-uae/native-patchview/PatchView`

SHA-256:

`2ac0914b622e95ac5ea3018d9d3ed4f6a8c158c5e3f6537adb909257d7e7b927`

## Gate 2 — AROS/FS-UAE device runtime

Result: **PASS**

Runtime configuration:

- Model: A1200
- Kickstart: internal AROS ROM
- Device: `trackdisk.device`
- Unit: `0`
- Observation: `guest_executed_patchview_trackdisk_device_unit0`
- FS-UAE exit: `124` (expected timeout after guest evidence was collected)
- PatchView guest RC: `5`

Guest RC 5 is expected here because the request deliberately asked for 32 vectors while the opened AROS `trackdisk.device` exposed only 6 complete 6-byte negative vector entries. PatchView safely clipped the inspection at `lib_NegSize` and reported that condition instead of reading beyond the device vector table.

Observed structured output:

- `kind=device`
- `device=trackdisk.device`
- `unit=0`
- `library_base=00AAE8EC`
- `neg_size=36`
- `available_vectors=6`
- `requested_vectors=32`
- `inspected_vectors=6`
- `vector_crc32=C4AF694F`
- `clipped=true`
- `record_count=6`

All six inspected entries were direct absolute JMP vectors (`0x4EF9`). Their observed targets in this AROS run were:

1. LVO -6  -> `00FB0C8C`
2. LVO -12 -> `00FB0C44`
3. LVO -18 -> `00FB0C3C`
4. LVO -24 -> `00FB0C40`
5. LVO -30 -> `00FB17FE`
6. LVO -36 -> `00FB1854`

The vector CRC32 is a snapshot fingerprint only. A changed CRC or vector target is not, by itself, evidence of malware or unauthorized patching.

## Evidence artifact

Artifact name: `m2-2c-patchview-device-qualification`

- Artifact ID: `10152580746`
- Uploaded files: 11
- Artifact ZIP SHA-256: `965f8ae5f393c99d65c2f2d3e38763964daff693d0a1d3cd14908433d0d06fb4`

## Qualification scope

This qualifies the PatchView device inspection path against an AROS m68k guest running in FS-UAE and confirms that:

- the code builds with the pinned Bebbo m68k AmigaOS toolchain;
- `OpenDevice()` can successfully open `trackdisk.device` unit 0 in the qualification guest;
- the device's embedded `struct Library` negative vector table can be inspected without issuing an I/O command;
- inspection remains bounded by `lib_NegSize`;
- clipping is reported with RC 5 and structured metadata;
- a stable vector-table CRC32 is produced over exactly the inspected entries.

This does **not** qualify the feature on a real Commodore AmigaOS 2.04+ installation. Real AmigaOS runtime qualification remains separate.
