#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-portview}"
SYSTEM_DIR="build/fs-uae/aros-system"
mkdir -p "$OUT_DIR"

[[ -f build/fs-uae/native-portview/PortView ]] || { echo "ERROR: native PortView missing" >&2; exit 1; }
iso="$(bash ci/fs-uae/fetch-aros-system.sh "$SYSTEM_DIR" | tail -n 1)"
root_extract="$OUT_DIR/system-root"
rm -rf "$root_extract"; mkdir -p "$root_extract"
7z x -y -o"$root_extract" "$iso" >/dev/null
startup="$(find "$root_extract" -type f -ipath '*/s/startup-sequence' -print -quit)"
[[ -n "$startup" ]] || { echo "ERROR: AROS Startup-Sequence missing" >&2; exit 1; }
aros_root="$(dirname "$(dirname "$startup")")"
cp build/fs-uae/native-portview/PortView "$aros_root/PortView"
cp "$startup" "$startup.amiforensics-original"

cat > "$startup" <<'EOF'
SYS:C/Echo "M2_5B_GUEST_STARTED=1" >SYS:portview-started.txt
SYS:PortView --kv >SYS:portview-all.txt
SYS:C/Echo $RC >SYS:portview-all-rc.txt
SYS:C/Echo "M2_5B_GUEST_AFTER=1" >SYS:portview-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/portview-{started,all,all-rc,after}.txt
config="$OUT_DIR/aros-portview.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
fs_rc=$?
set -e

all_out="$aros_root/portview-all.txt"
all_rc="$aros_root/portview-all-rc.txt"
started="$aros_root/portview-started.txt"
after="$aros_root/portview-after.txt"
status=FAIL
observation=guest_result_missing

valid_all() {
  [[ -f "$all_out" ]] && \
    grep -q '^tool=PortView' "$all_out" && \
    grep -q '^record\.0\.address=' "$all_out" && \
    grep -q '^record\.0\.sigbit=' "$all_out" && \
    grep -q '^record\.0\.sigtask=' "$all_out" && \
    grep -q '^record_count=' "$all_out" && \
    grep -q '^truncated=' "$all_out"
}

if [[ -f "$started" ]] && valid_all; then
  status=PASS
  observation=guest_executed_portview_snapshot
elif [[ -f "$after" ]]; then
  observation=guest_executed_portview_but_output_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_portview_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M2_5B_AROS_GUEST_PORTVIEW"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "FS_UAE_EXIT=$fs_rc"
  echo "OBSERVATION=$observation"
  [[ -f "$all_rc" ]] && tr -d '\r' < "$all_rc" | sed 's/^/GUEST_RC=/'
  [[ -f "$all_out" ]] && { echo "--- PORTS ---"; tr -d '\r' < "$all_out"; }
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
