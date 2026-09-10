#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-traceexec}"
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
SYS:C/Echo "M4_1B_GUEST_STARTED=1" >SYS:traceexec-started.txt
SYS:TraceExec --kv >SYS:traceexec.txt
SYS:C/Echo $RC >SYS:traceexec-rc.txt
SYS:C/Echo "M4_1B_GUEST_AFTER=1" >SYS:traceexec-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/traceexec-{started,after}.txt "$aros_root"/traceexec.txt "$aros_root"/traceexec-rc.txt
config="$OUT_DIR/aros-traceexec.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
fs_rc=$?
set -e

out="$aros_root/traceexec.txt"
rcfile="$aros_root/traceexec-rc.txt"
started="$aros_root/traceexec-started.txt"
after="$aros_root/traceexec-after.txt"
status=FAIL
observation=guest_result_missing

valid_output() {
  [[ -f "$out" ]] && \
    grep -q '^tool=TraceExec' "$out" && \
    grep -q '^schema=amiforensics.trace.kv/1' "$out" && \
    grep -q '^mode=event-observation' "$out" && \
    grep -q '^hooking=false' "$out" && \
    grep -q '^record.0.kind=event' "$out" && \
    grep -q '^record.0.event=snapshot-task' "$out" && \
    grep -Eq '^record.0.address=[0-9A-F]{8}$' "$out" && \
    grep -Eq '^record_count=[1-9][0-9]*$' "$out" && \
    grep -q '^truncated=false' "$out"
}

if [[ -f "$started" ]] && valid_output; then
  status=PASS
  observation=guest_executed_traceexec_foundation
elif [[ -f "$after" ]]; then
  observation=guest_executed_traceexec_but_output_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_traceexec_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M4_1B_AROS_GUEST_TRACEEXEC_FOUNDATION"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "FS_UAE_EXIT=$fs_rc"
  echo "OBSERVATION=$observation"
  [[ -f "$rcfile" ]] && tr -d '\r' < "$rcfile" | sed 's/^/GUEST_RC=/'
  [[ -f "$out" ]] && { echo "--- TRACEEXEC ---"; tr -d '\r' < "$out"; }
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
