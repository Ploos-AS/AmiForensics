#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-memscan-residents}"
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
SYS:C/Echo "M3_3_GUEST_STARTED=1" >SYS:memscan-residents-started.txt
SYS:MemScan --kv --discover-residents >SYS:memscan-residents.txt
SYS:C/Echo $RC >SYS:memscan-residents-rc.txt
SYS:C/Echo "M3_3_GUEST_AFTER=1" >SYS:memscan-residents-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/memscan-residents-{started,after}.txt "$aros_root"/memscan-residents.txt "$aros_root"/memscan-residents-rc.txt
config="$OUT_DIR/aros-memscan-residents.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
fs_rc=$?
set -e

out="$aros_root/memscan-residents.txt"
rcfile="$aros_root/memscan-residents-rc.txt"
started="$aros_root/memscan-residents-started.txt"
after="$aros_root/memscan-residents-after.txt"
status=FAIL
observation=guest_result_missing

valid_output() {
  [[ -f "$out" ]] && \
    grep -q '^tool=MemScan' "$out" && \
    grep -q '^schema=amiforensics.snapshot.kv/1' "$out" && \
    grep -q '^mode=resident-code-discovery' "$out" && \
    grep -q '^discovery_source=exec-resmodules' "$out" && \
    grep -q '^record\.0\.kind=resident-code' "$out" && \
    grep -q '^record\.0\.address=' "$out" && \
    grep -q '^record\.0\.end_skip=' "$out" && \
    grep -q '^record\.0\.init=' "$out" && \
    grep -q '^record\.0\.match_valid=true' "$out" && \
    grep -Eq '^record_count=[1-9][0-9]*$' "$out" && \
    grep -q '^truncated=' "$out"
}

if [[ -f "$started" ]] && valid_output; then
  status=PASS
  observation=guest_executed_resident_code_discovery
elif [[ -f "$after" ]]; then
  observation=guest_executed_memscan_but_output_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_memscan_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M3_3_AROS_GUEST_RESIDENT_DISCOVERY"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "FS_UAE_EXIT=$fs_rc"
  echo "OBSERVATION=$observation"
  [[ -f "$rcfile" ]] && tr -d '\r' < "$rcfile" | sed 's/^/GUEST_RC=/'
  [[ -f "$out" ]] && { echo "--- RESIDENT DISCOVERY ---"; tr -d '\r' < "$out"; }
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
