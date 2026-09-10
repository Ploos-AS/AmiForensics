#!/usr/bin/env bash
set -euo pipefail

OUT_DIR="${1:-build/fs-uae/aros-sampledump}"
SYSTEM_DIR="build/fs-uae/aros-system"
mkdir -p "$OUT_DIR"

[[ -f build/fs-uae/native-sampledump/SampleDump ]] || { echo "ERROR: native SampleDump missing" >&2; exit 1; }
iso="$(bash ci/fs-uae/fetch-aros-system.sh "$SYSTEM_DIR" | tail -n 1)"
root_extract="$OUT_DIR/system-root"
rm -rf "$root_extract"; mkdir -p "$root_extract"
7z x -y -o"$root_extract" "$iso" >/dev/null
startup="$(find "$root_extract" -type f -ipath '*/s/startup-sequence' -print -quit)"
[[ -n "$startup" ]] || { echo "ERROR: AROS Startup-Sequence missing" >&2; exit 1; }
aros_root="$(dirname "$(dirname "$startup")")"
cp build/fs-uae/native-sampledump/SampleDump "$aros_root/SampleDump"
cp "$startup" "$startup.amiforensics-original"

cat > "$startup" <<'EOF'
SYS:C/Echo "M3_4_GUEST_STARTED=1" >SYS:sampledump-started.txt
SYS:SampleDump --kv --address 0x400 --length 32 --output SYS:sampledump.bin >SYS:sampledump.txt
SYS:C/Echo $RC >SYS:sampledump-rc.txt
SYS:C/Echo "M3_4_GUEST_AFTER=1" >SYS:sampledump-after.txt
SYS:C/Execute SYS:S/Startup-Sequence.amiforensics-original
EOF

rm -f "$aros_root"/sampledump-{started,after}.txt "$aros_root"/sampledump.txt "$aros_root"/sampledump-rc.txt "$aros_root"/sampledump.bin
config="$OUT_DIR/aros-sampledump.fs-uae"
sed "s|@AROS_ROOT@|$PWD/$aros_root|" ci/fs-uae/aros-guest.fs-uae > "$config"
fs-uae --version > "$OUT_DIR/fs-uae-version.txt" 2>&1 || true
set +e
timeout 45s xvfb-run -a fs-uae "$config" > "$OUT_DIR/fs-uae.log" 2>&1
fs_rc=$?
set -e

out="$aros_root/sampledump.txt"
rcfile="$aros_root/sampledump-rc.txt"
bin="$aros_root/sampledump.bin"
started="$aros_root/sampledump-started.txt"
after="$aros_root/sampledump-after.txt"
status=FAIL
observation=guest_result_missing

valid_output() {
  [[ -f "$out" && -f "$bin" ]] && \
    grep -q '^tool=SampleDump' "$out" && \
    grep -q '^schema=amiforensics.sampledump.kv/1' "$out" && \
    grep -q '^mode=bounded-memory-dump' "$out" && \
    grep -q '^address=00000400' "$out" && \
    grep -q '^length=32' "$out" && \
    grep -Eq '^crc32=[0-9A-F]{8}$' "$out" && \
    grep -q '^complete=true' "$out" && \
    [[ "$(wc -c < "$bin")" -eq 32 ]]
}

if [[ -f "$started" ]] && valid_output; then
  status=PASS
  observation=guest_executed_bounded_sample_dump
elif [[ -f "$after" ]]; then
  observation=guest_executed_sampledump_but_output_invalid
elif [[ -f "$started" ]]; then
  observation=guest_started_but_sampledump_did_not_return
fi

{
  echo "STATUS=$status"
  echo "GATE=M3_4_AROS_GUEST_SAMPLEDUMP"
  echo "MODEL=A1200"
  echo "KICKSTART=internal"
  echo "FS_UAE_EXIT=$fs_rc"
  echo "OBSERVATION=$observation"
  [[ -f "$rcfile" ]] && tr -d '\r' < "$rcfile" | sed 's/^/GUEST_RC=/'
  [[ -f "$out" ]] && { echo "--- SAMPLEDUMP ---"; tr -d '\r' < "$out"; }
  [[ -f "$bin" ]] && sha256sum "$bin" | sed 's/^/DUMP_SHA256=/'
} | tee "$OUT_DIR/result.txt"

[[ "$status" == PASS ]]
