# AmiForensics analysis workstation

This directory contains the host-side orchestration layer for controlled Amiga malware analysis.

## Safety principles

- disposable analysis runs
- explicit sample opt-in
- static inspection before dynamic execution
- one run directory per analysis session
- immutable input copy plus SHA-256 metadata
- predictable artifact locations for later Compare/Report integration
- FS-UAE remains an external dependency; ROMs and copyrighted system files are never bundled
- the workstation does not auto-execute samples
- optional Ghidra integration is static-only and never executes the sample

## Run layout

A prepared run is created under `workstation/runs/<run-id>/`:

- `input/` — copied sample
- `artifacts/` — collected guest/host output
- `logs/` — orchestration, emulator and optional Ghidra logs
- `profile/` — rendered emulator profile
- `snapshots/` — normalized pre/post workstation snapshots and diff
- `ghidra/` — optional disposable Ghidra project
- `manifest.json` — normalized run metadata

## M5.1 — Workstation foundation

Prepare an isolated run:

```sh
python3 workstation/prepare_run.py path/to/sample
```

Use `--run-root` to put disposable runs outside the repository:

```sh
python3 workstation/prepare_run.py sample.bin --run-root /tmp/amiforensics-runs
```

## M5.2 — FS-UAE orchestration

Render the default disposable AROS/A1200 profile against an operator-supplied system root:

```sh
python3 workstation/prepare_profile.py \
  /tmp/amiforensics-runs/<run-id> \
  --system-root /path/to/aros-system
```

Review the generated `profile/analysis.fs-uae`, then inspect the launch command without starting FS-UAE:

```sh
python3 workstation/run_fsuae.py /tmp/amiforensics-runs/<run-id>
```

Explicitly start the emulator only after review:

```sh
python3 workstation/run_fsuae.py /tmp/amiforensics-runs/<run-id> --start
```

Starting FS-UAE does not auto-run the sample. The run directory is mounted separately so later guest-side tooling can collect controlled artifacts without modifying the original source file.

The default profile disables `bsdsocket_library`; network-capable analysis requires a separate explicitly opted-in profile.

## M5.3 — Snapshots and artifact collection

Capture a normalized pre-analysis snapshot:

```sh
python3 workstation/snapshot.py /tmp/amiforensics-runs/<run-id> pre
```

After the controlled analysis session, capture the post-analysis state:

```sh
python3 workstation/snapshot.py /tmp/amiforensics-runs/<run-id> post
```

Snapshots inventory workstation-visible `artifacts/`, `logs/` and `profile/` trees with size, timestamp and SHA-256 metadata.

Collect only explicitly selected files:

```sh
python3 workstation/collect_artifacts.py \
  /tmp/amiforensics-runs/<run-id> \
  /path/to/output1 /path/to/output2 \
  --label guest
```

Collected files are copied below `artifacts/<label>/`, made read-only, hashed with SHA-256 and described by an `index.json`. The run manifest records each collection. The collector never recursively sweeps arbitrary host directories and does not execute collected files.

## M5.4 — Snapshot comparison

After both snapshots exist, generate a normalized diff:

```sh
python3 workstation/compare_snapshots.py /tmp/amiforensics-runs/<run-id>
```

The command writes `snapshots/diff.json` and records it as `snapshot_diff` in `manifest.json`.

For each tracked tree (`artifacts`, `logs`, `profile`) the diff separates:

- added files
- removed files
- modified files, including before/after metadata
- unchanged file count

A top-level summary provides aggregate added/removed/modified/unchanged counts. Content changes are determined from SHA-256 plus size rather than timestamps alone. The schema `amiforensics.workstation.snapshot-diff/1` is intended as stable host-side input for the native/host `Compare` and `Report` work in M6.

## M5.5 — Optional Ghidra integration

Ghidra is an optional external dependency. AmiForensics does not download or bundle it.

Review the headless import command without starting Ghidra:

```sh
python3 workstation/ghidra_import.py \
  /tmp/amiforensics-runs/<run-id> \
  --ghidra-home /opt/ghidra
```

`GHIDRA_HOME` or an `analyzeHeadless` executable on `PATH` can be used instead of `--ghidra-home`.

After reviewing the command, explicitly start static headless analysis:

```sh
python3 workstation/ghidra_import.py \
  /tmp/amiforensics-runs/<run-id> \
  --ghidra-home /opt/ghidra \
  --analyze
```

The integration imports only the immutable sample copy from the prepared run. It creates a disposable project below `ghidra/`, writes Ghidra output to `logs/ghidra.log`, records state and return code in `manifest.json`, and never executes the sample.
