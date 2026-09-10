#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-patchview}"
SYSTEM_DIR="build/fs-uae/aros-system"
mkdir -p "$OUT_DIR"

[[ -f build/fs-uae/native-patchview/PatchView ]] || { echo "ERROR: native PatchView missing" >&2; exit 1; }
iso="$(bash ci/fs-uae/fetch-aros-system.sh "$SYSTEM_DIR" | tail -n 1)"
root_extract="$OUT_DIR/system-root"
rm -rf "$root_extract"; mkdir -p "$root_extract"
7z x -y -o"$root_extract" "$iso" >/dev/null
startup="$(find "$root_extract" -type f -ipath '*/s/startup-sequence' -print -quit)"
[[ -n "$startup" ]] || { echo "ERROR: AROS Startup-Sequence missing" >&2; exit 1; }
aros_root="$(dirname "$(dirname "$startup")")"
cp build/fs-uae/native-patchview/PatchView "$aros_root/PatchView"
cp "$startup" "$startup.amiforensics-original"

cat > "$startup" <<'EOF'
SYS:C/Echo "M2_2_GUEST_STARTED=1" >SYS:patchview-started.txt
SYS:PatchView --library exec.library --count 32 --kv >SYS:patchview-exec.txt
SYS:C/Echo $RC >SYS:patchview-exec-rc.txt
SYS:PatchView --library dos.library --count 32 --kv >SYS:patchview-dos.txt
SYS:C/Echo $RC >SYS:patchview-dos-rc.txt
SYS:C/Echo "M2_2_GUEST_AFTER=1" >SYS:patchview-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/patchview-{started,exec,exec-rc,dos,dos-rc,after}.txt
config="$OUT_DIR/aros-patchview.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
fs_rc=$?
set -e

exec_out="$aros_root/patchview-exec.txt"
exec_rc="$aros_root/patchview-exec-rc.txt"
dos_out="$aros_root/patchview-dos.txt"
dos_rc="$aros_root/patchview-dos-rc.txt"
started="$aros_root/patchview-started.txt"
after="$aros_root/patchview-after.txt"
status=FAIL
observation=guest_result_missing

valid_output() {
  local f="$1" lib="$2"
  [[ -f "$f" ]] && \
    grep -q '^tool=PatchView' "$f" && \
    grep -q "^library=$lib" "$f" && \
    grep -q '^available_vectors=' "$f" && \
    grep -q '^inspected_vectors=' "$f" && \
    grep -q '^record_count=' "$f"
}

if [[ -f "$started" ]] && valid_output "$exec_out" exec.library && valid_output "$dos_out" dos.library; then
  status=PASS
  observation=guest_executed_patchview_exec_and_dos
elif [[ -f "$after" ]]; then
  observation=guest_executed_patchview_but_output_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_patchview_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M2_2_AROS_GUEST_PATCHVIEW"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "FS_UAE_EXIT=$fs_rc"
  echo "OBSERVATION=$observation"
  [[ -f "$exec_rc" ]] && tr -d '\r' < "$exec_rc" | sed 's/^/EXEC_GUEST_RC=/'
  [[ -f "$dos_rc" ]] && tr -d '\r' < "$dos_rc" | sed 's/^/DOS_GUEST_RC=/'
  [[ -f "$exec_out" ]] && { echo "--- EXEC_LIBRARY ---"; tr -d '\r' < "$exec_out"; }
  [[ -f "$dos_out" ]] && { echo "--- DOS_LIBRARY ---"; tr -d '\r' < "$dos_out"; }
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
