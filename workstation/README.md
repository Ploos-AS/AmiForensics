# AmiForensics analysis workstation

This directory contains the host-side orchestration layer for controlled Amiga malware analysis.

## M5.1 foundation

M5.1 defines the workstation contract without executing unknown samples automatically.

Principles:

- disposable analysis runs
- explicit sample opt-in
- static inspection before dynamic execution
- one run directory per analysis session
- immutable input copy plus SHA-256 metadata
- predictable artifact locations for later Compare/Report integration
- FS-UAE remains an external dependency; ROMs and copyrighted system files are never bundled

## Run layout

A prepared run is created under `workstation/runs/<run-id>/`:

- `input/` — copied sample
- `artifacts/` — guest/host output collected after analysis
- `logs/` — orchestration and emulator logs
- `manifest.json` — normalized run metadata

The M5.1 runner only prepares a run. Starting FS-UAE and executing a sample are deliberately separate operations for now.

## Usage

```sh
python3 workstation/prepare_run.py path/to/sample
```

Use `--run-root` to put disposable runs outside the repository.

```sh
python3 workstation/prepare_run.py sample.bin --run-root /tmp/amiforensics-runs
```

The command prints the created run directory on success.
