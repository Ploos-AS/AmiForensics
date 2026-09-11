# M7.3 v0.1.0 release candidate

Status: **IN PROGRESS — final gate pending**

## Candidate

Version: `0.1.0`

Target tag: `v0.1.0`

Package: `AmiForensics-v0.1.0.tar.gz`

The content set remains frozen by `docs/RELEASE_v0.1.0.md`.

## Required release evidence

The final release-candidate gate must verify:

- repository version is exactly `0.1.0`
- all 15 native commands remain in the frozen release inventory
- `AFReport.rexx` is included
- README, ROADMAP, LICENSE, changelog and release manifest are present
- M7.2 qualification record is present and records PASS
- `make package` succeeds
- package inventory matches the frozen v0.1.0 contract
- SHA-256 checksum is generated for the final archive
- release notes state the qualification boundaries and known limitations

## Known limitations

- real Commodore AmigaOS runtime qualification is not yet claimed
- RexxMast/ARexx runtime qualification remains separate and unverified by the static CI gate
- dynamic malware execution is intentionally not automatic; it requires an explicit operator action in an isolated environment
- optional Ghidra integration requires an externally installed Ghidra; it is not bundled
- the workstation FS-UAE path requires an operator-supplied system/AROS environment; ROM/system images are not bundled

## Release decision

Do not create the final `v0.1.0` tag until the M7.3 workflow completes successfully on the intended release commit and its archive/checksum evidence is retained.
