#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
OUT_DIR="${1:-$ROOT/build/local/m2-1d}"
RESULT_FILE="${M2_1D_RESULT_FILE:-$OUT_DIR/m2_1d_residentview_result.txt}"
BINARY_SRC="${M2_1D_BINARY:-$ROOT/build/fs-uae/native-arexx/ResidentView-ARexx}"
REXX_SRC="$ROOT/ci/local/m2-1d/ResidentView.rexx"

mkdir -p "$OUT_DIR"

cat >"$OUT_DIR/README.txt" <<'EOF'
M2.1d local qualification bundle

Copy ResidentView-ARexx and ResidentView.rexx into a real AmigaOS 2.04+ FS-UAE guest.
Ensure RexxMast is running, then execute:

Run >NIL: RAM:ResidentView-ARexx --serve
RX RAM:ResidentView.rexx

Copy RAM:m2_1d_residentview_result.txt back into this directory and rerun qualify.sh.
EOF

if [[ -f "$BINARY_SRC" ]]; then
  cp "$BINARY_SRC" "$OUT_DIR/ResidentView-ARexx"
  sha256sum "$OUT_DIR/ResidentView-ARexx" >"$OUT_DIR/ResidentView-ARexx.sha256"
else
  echo "NOTE: ARexx binary not found at $BINARY_SRC" >"$OUT_DIR/binary-status.txt"
  echo "Build it first with: bash ci/fs-uae/build-native-arexx.sh" >>"$OUT_DIR/binary-status.txt"
fi

cp "$REXX_SRC" "$OUT_DIR/ResidentView.rexx"

{
  echo "GATE=M2_1D_LOCAL_AREXX_RUNTIME"
  echo "REPO_HEAD=$(git -C "$ROOT" rev-parse HEAD 2>/dev/null || echo unknown)"
  echo "FS_UAE_VERSION=${FS_UAE_VERSION:-UNRECORDED}"
  echo "KICKSTART_VERSION=${KICKSTART_VERSION:-UNRECORDED}"
  echo "WORKBENCH_VERSION=${WORKBENCH_VERSION:-UNRECORDED}"
  echo "CPU_PROFILE=${CPU_PROFILE:-UNRECORDED}"
} >"$OUT_DIR/environment.txt"

if [[ ! -f "$RESULT_FILE" ]]; then
  cat <<EOF
M2.1d bundle prepared at:
  $OUT_DIR

Next, run the included binary/script inside real AmigaOS 2.04+ FS-UAE,
copy RAM:m2_1d_residentview_result.txt back to:
  $RESULT_FILE
and rerun this command.
EOF
  exit 2
fi

"$ROOT/ci/local/m2-1d/validate-result.sh" "$RESULT_FILE" | tee "$OUT_DIR/validation.txt"
sha256sum "$RESULT_FILE" >"$OUT_DIR/m2_1d_residentview_result.txt.sha256"

cat >"$OUT_DIR/qualification-summary.txt" <<EOF
STATUS=PASS
GATE=M2_1D_LOCAL_AREXX_RUNTIME
RESULT_FILE=$RESULT_FILE
RESULT_SHA256=$(cut -d' ' -f1 "$OUT_DIR/m2_1d_residentview_result.txt.sha256")
EOF

cat "$OUT_DIR/qualification-summary.txt"
