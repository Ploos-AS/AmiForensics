#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-guest}"
SYSTEM_DIR="build/fs-uae/aros-system"
mkdir -p "$OUT_DIR"

[[ -f build/fs-uae/native/ResidentView ]] || { echo "ERROR: native ResidentView missing" >&2; exit 1; }
iso="$(bash ci/fs-uae/fetch-aros-system.sh "$SYSTEM_DIR" | tail -n 1)"
root_extract="$OUT_DIR/system-root"
rm -rf "$root_extract"; mkdir -p "$root_extract"
7z x -y -o"$root_extract" "$iso" >/dev/null
startup="$(find "$root_extract" -type f -ipath '*/s/startup-sequence' -print -quit)"
[[ -n "$startup" ]] || { echo "ERROR: AROS Startup-Sequence missing" >&2; exit 1; }
aros_root="$(dirname "$(dirname "$startup")")"
cp build/fs-uae/native/ResidentView "$aros_root/ResidentView"
cp "$startup" "$startup.amiforensics-original"

cat > "$startup" <<'EOF'
SYS:C/Echo "M2_1C_GUEST_STARTED=1" >SYS:residentview-started.txt
SYS:ResidentView --kind residents --kv >SYS:residentview-residents.txt
SYS:C/Echo $RC >SYS:residentview-rc.txt
SYS:C/Echo "M2_1C_GUEST_AFTER=1" >SYS:residentview-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/residentview-{started,residents,rc,after}.txt
config="$OUT_DIR/aros-guest.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
rc=$?
set -e

started="$aros_root/residentview-started.txt"
residents="$aros_root/residentview-residents.txt"
guest_rc="$aros_root/residentview-rc.txt"
after="$aros_root/residentview-after.txt"
status=FAIL
observation=guest_result_missing
if [[ -f "$started" && -f "$residents" ]] && grep -q '^tool=ResidentView' "$residents" && grep -q '^record_count=' "$residents"; then
  status=PASS
  observation=guest_executed_residentview_resident_snapshot
elif [[ -f "$after" ]]; then
  observation=guest_executed_residentview_but_output_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_residentview_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M2_1C_AROS_GUEST_RESIDENTVIEW"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "FS_UAE_EXIT=$rc"
  echo "OBSERVATION=$observation"
  [[ -f "$guest_rc" ]] && tr -d '\r' < "$guest_rc" | sed 's/^/GUEST_RC=/'
  [[ -f "$residents" ]] && { echo "--- GUEST_OUTPUT ---"; tr -d '\r' < "$residents"; }
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
