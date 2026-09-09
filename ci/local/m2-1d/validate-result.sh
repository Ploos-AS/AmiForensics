#!/usr/bin/env bash
set -euo pipefail

result="${1:-m2_1d_residentview_result.txt}"
[[ -f "$result" ]] || { echo "ERROR: result file not found: $result" >&2; exit 20; }

require_line() {
  local expected="$1"
  grep -Fxq "$expected" "$result" || {
    echo "FAIL: missing expected line: $expected" >&2
    exit 5
  }
}

require_line 'GATE=M2_1D_AREXX_RUNTIME'
require_line 'PORT=RESIDENTVIEW'
require_line 'PING_RC=0'
require_line 'PING_RESULT=PONG'
require_line 'LIST_RESIDENTS_RC=0'
require_line 'QUIT_RC=0'
require_line 'QUIT_RESULT=BYE'
require_line 'STATUS=PASS'

grep -Fq 'resident|' "$result" || {
  echo 'FAIL: LIST RESIDENTS contained no resident record' >&2
  exit 5
}

count="$(grep -Fc 'resident|' "$result" || true)"

echo 'STATUS=PASS'
echo 'GATE=M2_1D_AREXX_RUNTIME_VALIDATION'
echo "RESIDENT_RECORD_LINES=$count"
echo "RESULT_FILE=$result"
