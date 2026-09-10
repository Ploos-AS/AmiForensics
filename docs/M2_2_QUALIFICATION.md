# M2.2 PatchView qualification

Status: **PASS**

Qualified revision: `af0850bc158f26fd0b4b8c98176740a4d7d2f53d`

GitHub Actions run: `34453755882` (`M2.2 PatchView qualification`, run 2)

## Gate 1 — native Bebbo build

- Status: PASS
- Gate: `M2_2_NATIVE_BEBBO_BUILD`
- Target: AmigaOS m68k, `-m68000`
- Toolchain image: `amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed`
- Binary: `build/fs-uae/native-patchview/PatchView`
- Binary SHA-256: `80d9d49252740bb69d868ae06e4fd7c8dd5f125f829f94c64dd8b98fc6d53747`
- `file(1)`: AmigaOS loadseg()able executable/binary

## Gate 2 — AROS guest runtime

- Status: PASS
- Gate: `M2_2_AROS_GUEST_PATCHVIEW`
- Model: A1200
- Kickstart: FS-UAE internal AROS ROM
- Observation: `guest_executed_patchview_exec_and_dos`
- `exec.library` guest RC: 0
- `dos.library` guest RC: 0
- FS-UAE host exit: 124, expected because the emulator remains running after the guest-side evidence has been written.

### exec.library evidence

- `neg_size=1116`
- `available_vectors=186`
- `requested_vectors=32`
- `inspected_vectors=32`
- `vector_crc32=6E7D5469`
- `clipped=false`
- `record_count=32`

### dos.library evidence

- `neg_size=1016`
- `available_vectors=169`
- `requested_vectors=32`
- `inspected_vectors=32`
- `vector_crc32=5B1DF053`
- `clipped=false`
- `record_count=32`

The qualification demonstrates that PatchView can safely bound vector-table inspection using `lib_NegSize`, decode representative library vectors, and produce a deterministic CRC32 fingerprint of the inspected vector bytes under the tested AROS/FS-UAE environment.

## Artifact

- Name: `m2-2-patchview-qualification`
- Artifact ID: `10142662582`
- Artifact ZIP SHA-256: `e96be55667fa529b8208582cd54658a4b49e877fb2c4351590ef714c538ac098`
- Files uploaded: 11

## Scope and limitations

This qualification is AROS/FS-UAE evidence, not a claim of runtime qualification on Commodore AmigaOS 2.04+.

PatchView currently inspects library negative-vector tables opened through `OpenLibrary()`. A direct JMP target or a changed fingerprint is evidence of a vector-table state, not by itself proof that malware performed a patch. Provenance/baseline comparison belongs in later comparison and reporting milestones.
