#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-traceexec-task-watch}"
SYSTEM_DIR="build/fs-uae/aros-system"
mkdir -p "$OUT_DIR"

[[ -f build/fs-uae/native-traceexec/TraceExec ]] || { echo "ERROR: native TraceExec missing" >&2; exit 1; }
iso="$(bash ci/fs-uae/fetch-aros-system.sh "$SYSTEM_DIR" | tail -n 1)"
root_extract="$OUT_DIR/system-root"
rm -rf "$root_extract"; mkdir -p "$root_extract"
7z x -y -o"$root_extract" "$iso" >/dev/null
startup="$(find "$root_extract" -type f -ipath '*/s/startup-sequence' -print -quit)"
[[ -n "$startup" ]] || { echo "ERROR: AROS Startup-Sequence missing" >&2; exit 1; }
aros_root="$(dirname "$(dirname "$startup")")"
cp build/fs-uae/native-traceexec/TraceExec "$aros_root/TraceExec"
cp "$startup" "$startup.amiforensics-original"

cat > "$startup" <<'EOF'
SYS:C/Echo "M4_2_GUEST_STARTED=1" >SYS:traceexec-watch-started.txt
SYS:TraceExec --kv --watch-tasks 3 --interval 1 >SYS:traceexec-watch.txt
SYS:C/Echo $RC >SYS:traceexec-watch-rc.txt
SYS:C/Echo "M4_2_GUEST_AFTER=1" >SYS:traceexec-watch-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/traceexec-watch-{started,after,rc}.txt "$aros_root"/traceexec-watch.txt
config="$OUT_DIR/aros-traceexec-task-watch.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
fs_rc=$?
set -e

out="$aros_root/traceexec-watch.txt"
rcfile="$aros_root/traceexec-watch-rc.txt"
started="$aros_root/traceexec-watch-started.txt"
after="$aros_root/traceexec-watch-after.txt"
status=FAIL
observation=guest_result_missing

valid_output() {
  [[ -f "$out" ]] && \
    grep -q '^tool=TraceExec' "$out" && \
    grep -q '^schema=amiforensics.trace.kv/1' "$out" && \
    grep -q '^mode=task-lifecycle-watch' "$out" && \
    grep -q '^hooking=false' "$out" && \
    grep -q '^samples_requested=3' "$out" && \
    grep -q '^interval_ticks=1' "$out" && \
    grep -Eq '^record_count=[0-9]+$' "$out" && \
    grep -Eq '^taskset_truncated=(true|false)$' "$out" && \
    grep -Eq '^truncated=(true|false)$' "$out"
}

if [[ -f "$started" ]] && valid_output; then
  status=PASS
  observation=guest_executed_bounded_task_lifecycle_watch
elif [[ -f "$after" ]]; then
  observation=guest_executed_task_watch_but_output_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_task_watch_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M4_2_AROS_GUEST_BOUNDED_TASK_WATCH"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "FS_UAE_EXIT=$fs_rc"
  echo "OBSERVATION=$observation"
  [[ -f "$rcfile" ]] && tr -d '\r' < "$rcfile" | sed 's/^/GUEST_RC=/'
  [[ -f "$out" ]] && { echo "--- TRACEEXEC TASK WATCH ---"; tr -d '\r' < "$out"; }
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
