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

## Run layout

A prepared run is created under `workstation/runs/<run-id>/`:

- `input/` — copied sample
- `artifacts/` — guest/host output collected after analysis
- `logs/` — orchestration and emulator logs
- `profile/` — rendered emulator profile
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

The default profile disables `bsdsocket_library`; network-capable analysis will require a separate explicitly opted-in profile in a later milestone.
