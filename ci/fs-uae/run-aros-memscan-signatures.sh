#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-memscan-signatures}"
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
SYS:C/Echo "M3_3B_GUEST_STARTED=1" >SYS:memscan-signatures-started.txt
SYS:MemScan --kv --scan-residents >SYS:memscan-signatures.txt
SYS:C/Echo $RC >SYS:memscan-signatures-rc.txt
SYS:C/Echo "M3_3B_GUEST_AFTER=1" >SYS:memscan-signatures-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/memscan-signatures-{started,after}.txt "$aros_root"/memscan-signatures.txt "$aros_root"/memscan-signatures-rc.txt
config="$OUT_DIR/aros-memscan-signatures.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
fs_rc=$?
set -e

out="$aros_root/memscan-signatures.txt"
rcfile="$aros_root/memscan-signatures-rc.txt"
started="$aros_root/memscan-signatures-started.txt"
after="$aros_root/memscan-signatures-after.txt"
status=FAIL
observation=guest_result_missing

guest_rc=""
if [[ -f "$rcfile" ]]; then guest_rc="$(tr -d '\r\n ' < "$rcfile")"; fi

valid_output() {
  [[ -f "$out" ]] && \
    grep -q '^tool=MemScan' "$out" && \
    grep -q '^schema=amiforensics.snapshot.kv/1' "$out" && \
    grep -q '^mode=resident-signature-scan' "$out" && \
    grep -q '^scan_source=memheader-bounded' "$out" && \
    grep -q '^scan_region_limit=65536' "$out" && \
    grep -q '^scan_total_limit=262144' "$out" && \
    grep -Eq '^record_count=[0-9]+$' "$out" && \
    grep -Eq '^source_region_count=[1-9][0-9]*$' "$out" && \
    grep -Eq '^scan_region_count=[1-9][0-9]*$' "$out" && \
    grep -Eq '^scan_bytes=[1-9][0-9]*$' "$out" && \
    grep -Eq '^scan_limited=(true|false)$' "$out" && \
    grep -Eq '^truncated=(true|false)$' "$out"
}

if [[ -f "$started" ]] && valid_output && { [[ "$guest_rc" == "0" ]] || [[ "$guest_rc" == "5" ]]; }; then
  status=PASS
  observation=guest_executed_bounded_resident_signature_scan
elif [[ -f "$after" ]]; then
  observation=guest_executed_memscan_but_output_or_rc_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_memscan_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M3_3B_AROS_GUEST_BOUNDED_RESIDENT_SCAN"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "FS_UAE_EXIT=$fs_rc"
  echo "OBSERVATION=$observation"
  [[ -n "$guest_rc" ]] && echo "GUEST_RC=$guest_rc"
  [[ -f "$out" ]] && { echo "--- RESIDENT SIGNATURE SCAN ---"; tr -d '\r' < "$out"; }
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
