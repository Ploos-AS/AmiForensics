#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-patchview-device}"
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
SYS:C/Echo "M2_2C_GUEST_STARTED=1" >SYS:patchview-device-started.txt
SYS:PatchView --device trackdisk.device --unit 0 --count 32 --kv >SYS:patchview-device.txt
SYS:C/Echo $RC >SYS:patchview-device-rc.txt
SYS:C/Echo "M2_2C_GUEST_AFTER=1" >SYS:patchview-device-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/patchview-device-{started,rc,after}.txt "$aros_root/patchview-device.txt"
config="$OUT_DIR/aros-patchview-device.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
fs_rc=$?
set -e

out="$aros_root/patchview-device.txt"
rcfile="$aros_root/patchview-device-rc.txt"
started="$aros_root/patchview-device-started.txt"
after="$aros_root/patchview-device-after.txt"
status=FAIL
observation=guest_result_missing

valid_output() {
  [[ -f "$out" ]] && \
    grep -q '^tool=PatchView' "$out" && \
    grep -q '^kind=device' "$out" && \
    grep -q '^device=trackdisk.device' "$out" && \
    grep -q '^unit=0' "$out" && \
    grep -q '^neg_size=' "$out" && \
    grep -q '^inspected_vectors=' "$out" && \
    grep -Eq '^vector_crc32=[0-9A-F]{8}$' "$out" && \
    grep -q '^record_count=' "$out"
}

if [[ -f "$started" ]] && valid_output; then
  status=PASS
  observation=guest_executed_patchview_trackdisk_device_unit0
elif [[ -f "$after" ]]; then
  observation=guest_executed_patchview_but_output_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_patchview_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M2_2C_AROS_GUEST_PATCHVIEW_DEVICE"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "DEVICE=trackdisk.device"
  echo "UNIT=0"
  echo "FS_UAE_EXIT=$fs_rc"
  echo "OBSERVATION=$observation"
  [[ -f "$rcfile" ]] && tr -d '\r' < "$rcfile" | sed 's/^/GUEST_RC=/'
  [[ -f "$out" ]] && { echo "--- DEVICE ---"; tr -d '\r' < "$out"; }
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
