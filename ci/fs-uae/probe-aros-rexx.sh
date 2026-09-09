#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-rexx-probe}"
SYSTEM_DIR="build/fs-uae/aros-system"
mkdir -p "$OUT_DIR"

iso="$(bash ci/fs-uae/fetch-aros-system.sh "$SYSTEM_DIR" | tail -n 1)"
root_extract="$OUT_DIR/system-root"
rm -rf "$root_extract"
mkdir -p "$root_extract"
7z x -y -o"$root_extract" "$iso" >/dev/null

{
  echo "ISO=$iso"
  echo "AROS_SOURCE=$(tr '\n' ' ' < "$SYSTEM_DIR/source.txt" 2>/dev/null || true)"
  echo "AROS_ARCHIVE_SHA256=$(awk '{print $1}' "$SYSTEM_DIR/archive.sha256" 2>/dev/null || true)"
} > "$OUT_DIR/provenance.txt"

find_one() {
  local pattern="$1"
  find "$root_extract" -type f -iname "$pattern" -print | sort | head -n 1
}

rexxmast="$(find_one 'RexxMast' || true)"
rexxlib="$(find_one 'rexxsyslib.library' || true)"
rxcmd="$(find_one 'RX' || true)"

{
  echo "REXXMAST=${rexxmast:-MISSING}"
  echo "REXXSYSLIB=${rexxlib:-MISSING}"
  echo "RX_COMMAND=${rxcmd:-MISSING}"
} | tee "$OUT_DIR/paths.txt"

status=PASS
observation=aros_has_required_arexx_runtime
if [[ -z "$rexxmast" || -z "$rexxlib" ]]; then
  status=UNAVAILABLE
  observation=aros_missing_required_arexx_runtime
fi

printf 'STATUS=%s\nGATE=M2_1D_AROS_REXX_PREFLIGHT\nOBSERVATION=%s\n' \
  "$status" "$observation" | tee "$OUT_DIR/result.txt"

# UNAVAILABLE is a valid, evidence-producing outcome for this preflight. It
# means M2.1d must continue on a real AmigaOS environment rather than faking
# ARexx support in AROS.
exit 0
