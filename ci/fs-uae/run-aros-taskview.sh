#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-taskview}"
SYSTEM_DIR="build/fs-uae/aros-system"
mkdir -p "$OUT_DIR"

[[ -f build/fs-uae/native-taskview/TaskView ]] || { echo "ERROR: native TaskView missing" >&2; exit 1; }
iso="$(bash ci/fs-uae/fetch-aros-system.sh "$SYSTEM_DIR" | tail -n 1)"
root_extract="$OUT_DIR/system-root"
rm -rf "$root_extract"; mkdir -p "$root_extract"
7z x -y -o"$root_extract" "$iso" >/dev/null
startup="$(find "$root_extract" -type f -ipath '*/s/startup-sequence' -print -quit)"
[[ -n "$startup" ]] || { echo "ERROR: AROS Startup-Sequence missing" >&2; exit 1; }
aros_root="$(dirname "$(dirname "$startup")")"
cp build/fs-uae/native-taskview/TaskView "$aros_root/TaskView"
cp "$startup" "$startup.amiforensics-original"

cat > "$startup" <<'EOF'
SYS:C/Echo "M2_3B_GUEST_STARTED=1" >SYS:taskview-started.txt
SYS:TaskView --kv >SYS:taskview-all.txt
SYS:C/Echo $RC >SYS:taskview-all-rc.txt
SYS:TaskView --state current --kv >SYS:taskview-current.txt
SYS:C/Echo $RC >SYS:taskview-current-rc.txt
SYS:C/Echo "M2_3B_GUEST_AFTER=1" >SYS:taskview-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/taskview-{started,all,all-rc,current,current-rc,after}.txt
config="$OUT_DIR/aros-taskview.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
fs_rc=$?
set -e

all_out="$aros_root/taskview-all.txt"
all_rc="$aros_root/taskview-all-rc.txt"
current_out="$aros_root/taskview-current.txt"
current_rc="$aros_root/taskview-current-rc.txt"
started="$aros_root/taskview-started.txt"
after="$aros_root/taskview-after.txt"
status=FAIL
observation=guest_result_missing

valid_all() {
  [[ -f "$all_out" ]] && \
    grep -q '^tool=TaskView' "$all_out" && \
    grep -q '^filter_state=all' "$all_out" && \
    grep -q '^record_count=' "$all_out" && \
    grep -q '^truncated=' "$all_out"
}

valid_current() {
  [[ -f "$current_out" ]] && \
    grep -q '^tool=TaskView' "$current_out" && \
    grep -q '^filter_state=current' "$current_out" && \
    grep -q '^record\.0\.state=current' "$current_out" && \
    grep -q '^record_count=' "$current_out"
}

if [[ -f "$started" ]] && valid_all && valid_current; then
  status=PASS
  observation=guest_executed_taskview_all_and_current
elif [[ -f "$after" ]]; then
  observation=guest_executed_taskview_but_output_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_taskview_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M2_3B_AROS_GUEST_TASKVIEW"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "FS_UAE_EXIT=$fs_rc"
  echo "OBSERVATION=$observation"
  [[ -f "$all_rc" ]] && tr -d '\r' < "$all_rc" | sed 's/^/ALL_GUEST_RC=/'
  [[ -f "$current_rc" ]] && tr -d '\r' < "$current_rc" | sed 's/^/CURRENT_GUEST_RC=/'
  [[ -f "$all_out" ]] && { echo "--- ALL ---"; tr -d '\r' < "$all_out"; }
  [[ -f "$current_out" ]] && { echo "--- CURRENT ---"; tr -d '\r' < "$current_out"; }
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
