#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-memscan}"
SYSTEM_DIR="build/fs-uae/aros-system"
mkdir -p "$OUT_DIR"

[[ -f build/fs-uae/native-memscan/MemScan ]] || { echo "ERROR: native MemScan missing" >&2; exit 1; }
iso="$(bash ci/fs-uae/fetch-aros-system.sh "$SYSTEM_DIR" | tail -n 1)"
root_extract="$OUT_DIR/system-root"
rm -rf "$root_extract"; mkdir -p "$root_extract"
7z x -y -o"$root_extract" "$iso" >/dev/null
startup="$(find "$root_extract" -type f -ipath '*/s/startup-sequence' -print -quit)"
[[ -n "$startup" ]] || { echo "ERROR: AROS Startup-Sequence missing" >&2; exit 1; }
aros_root="$(dirname "$(dirname "$startup")")"
cp build/fs-uae/native-memscan/MemScan "$aros_root/MemScan"
cp "$startup" "$startup.amiforensics-original"

cat > "$startup" <<'EOF'
SYS:C/Echo "M3_1B_GUEST_STARTED=1" >SYS:memscan-started.txt
SYS:MemScan --kv >SYS:memscan-all.txt
SYS:C/Echo $RC >SYS:memscan-all-rc.txt
SYS:C/Echo "M3_1B_GUEST_AFTER=1" >SYS:memscan-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/memscan-{started,all,all-rc,after}.txt
config="$OUT_DIR/aros-memscan.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
fs_rc=$?
set -e

all_out="$aros_root/memscan-all.txt"
all_rc="$aros_root/memscan-all-rc.txt"
started="$aros_root/memscan-started.txt"
after="$aros_root/memscan-after.txt"
status=FAIL
observation=guest_result_missing

valid_all() {
  [[ -f "$all_out" ]] && \
    grep -q '^tool=MemScan' "$all_out" && \
    grep -q '^schema=amiforensics.snapshot.kv/1' "$all_out" && \
    grep -q '^mode=memory-region-inventory' "$all_out" && \
    grep -q '^record\.0\.kind=memory-region' "$all_out" && \
    grep -q '^record\.0\.lower=' "$all_out" && \
    grep -q '^record\.0\.upper=' "$all_out" && \
    grep -q '^record\.0\.risk_hint=' "$all_out" && \
    grep -Eq '^record_count=[1-9][0-9]*$' "$all_out" && \
    grep -q '^truncated=' "$all_out"
}

if [[ -f "$started" ]] && valid_all; then
  status=PASS
  observation=guest_executed_memscan_region_inventory
elif [[ -f "$after" ]]; then
  observation=guest_executed_memscan_but_output_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_memscan_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M3_1B_AROS_GUEST_MEMSCAN"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "FS_UAE_EXIT=$fs_rc"
  echo "OBSERVATION=$observation"
  [[ -f "$all_rc" ]] && tr -d '\r' < "$all_rc" | sed 's/^/GUEST_RC=/'
  [[ -f "$all_out" ]] && { echo "--- MEMORY REGIONS ---"; tr -d '\r' < "$all_out"; }
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
