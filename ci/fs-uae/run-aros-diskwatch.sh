#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-diskwatch}"
SYSTEM_DIR="build/fs-uae/aros-system"
mkdir -p "$OUT_DIR"

[[ -f build/fs-uae/native-diskwatch/DiskWatch ]] || { echo "ERROR: native DiskWatch missing" >&2; exit 1; }
iso="$(bash ci/fs-uae/fetch-aros-system.sh "$SYSTEM_DIR" | tail -n 1)"
root_extract="$OUT_DIR/system-root"
rm -rf "$root_extract"; mkdir -p "$root_extract"
7z x -y -o"$root_extract" "$iso" >/dev/null
startup="$(find "$root_extract" -type f -ipath '*/s/startup-sequence' -print -quit)"
[[ -n "$startup" ]] || { echo "ERROR: AROS Startup-Sequence missing" >&2; exit 1; }
aros_root="$(dirname "$(dirname "$startup")")"
cp build/fs-uae/native-diskwatch/DiskWatch "$aros_root/DiskWatch"
cp "$startup" "$startup.amiforensics-original"

cat > "$startup" <<'EOF'
SYS:C/Echo "M4_4B_GUEST_STARTED=1" >SYS:diskwatch-started.txt
SYS:DiskWatch --kv --samples 2 --interval 1 >SYS:diskwatch.txt
SYS:C/Echo $RC >SYS:diskwatch-rc.txt
SYS:C/Echo "M4_4B_GUEST_AFTER=1" >SYS:diskwatch-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/diskwatch-{started,after,rc}.txt "$aros_root"/diskwatch.txt
config="$OUT_DIR/aros-diskwatch.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
fs_rc=$?
set -e

out="$aros_root/diskwatch.txt"
rcfile="$aros_root/diskwatch-rc.txt"
started="$aros_root/diskwatch-started.txt"
after="$aros_root/diskwatch-after.txt"
status=FAIL
observation=guest_result_missing

valid_output() {
  [[ -f "$out" ]] && \
    grep -q '^tool=DiskWatch' "$out" && \
    grep -q '^schema=amiforensics.trace.kv/1' "$out" && \
    grep -q '^mode=disk-change-watch' "$out" && \
    grep -q '^hooking=false' "$out" && \
    grep -q '^observation=TD_CHANGENUM' "$out" && \
    grep -q '^device=trackdisk.device' "$out" && \
    grep -q '^unit=0' "$out" && \
    grep -q '^samples_requested=2' "$out" && \
    grep -q '^interval_ticks=1' "$out" && \
    grep -Eq '^initial_change=[0-9]+$' "$out" && \
    grep -Eq '^final_change=[0-9]+$' "$out" && \
    grep -Eq '^record_count=[0-9]+$' "$out" && \
    grep -Eq '^truncated=(true|false)$' "$out"
}

if [[ -f "$started" ]] && valid_output; then
  status=PASS
  observation=guest_executed_disk_change_watch
elif [[ -f "$after" ]]; then
  observation=guest_executed_diskwatch_but_output_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_diskwatch_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M4_4B_AROS_GUEST_DISKWATCH"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "FS_UAE_EXIT=$fs_rc"
  echo "OBSERVATION=$observation"
  [[ -f "$rcfile" ]] && tr -d '\r' < "$rcfile" | sed 's/^/GUEST_RC=/'
  [[ -f "$out" ]] && { echo "--- DISKWATCH ---"; tr -d '\r' < "$out"; }
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
