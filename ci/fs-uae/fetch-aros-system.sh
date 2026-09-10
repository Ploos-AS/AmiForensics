#!/usr/bin/env bash
set -euo pipefail

AROS_INDEX_URL="https://aros.sourceforge.io/cgi-bin/files?lang=en&type=nightly2"
AROS_TARGET="amiga-m68k-boot-iso"
OUT_DIR="${1:-build/fs-uae/aros-system}"
mkdir -p "$OUT_DIR"
index_html="$OUT_DIR/aros-nightly-index.html"

CURL_COMMON=(--fail --location --retry 3 --retry-delay 2 --connect-timeout 15 --max-time 180)

echo "AROS_FETCH_PHASE=index" >&2
AROS_URL=""
for attempt in 1 2 3 4; do
  curl "${CURL_COMMON[@]}" "$AROS_INDEX_URL" -o "$index_html"
  AROS_URL="$(
    { grep -oE 'href="[^"]*amiga-m68k-boot-iso[^"]*"' "$index_html" || true; } \
      | head -n 1 \
      | sed -e 's/^href="//' -e 's/"$//' -e 's/&amp;/\&/g'
  )"
  if [[ -n "$AROS_URL" ]]; then
    break
  fi
  bytes="$(wc -c < "$index_html" | tr -d ' ')"
  echo "AROS_INDEX_ATTEMPT=$attempt bytes=$bytes target_missing=1" >&2
  if [[ "$attempt" -lt 4 ]]; then sleep $((attempt * 3)); fi
done
[[ -n "$AROS_URL" ]] || { echo "ERROR: AROS system URL not found after 4 index attempts" >&2; exit 1; }
case "$AROS_URL" in
  http://*|https://*) ;;
  //*) AROS_URL="https:${AROS_URL}" ;;
  /*) AROS_URL="https://aros.sourceforge.io${AROS_URL}" ;;
  *) AROS_URL="https://aros.sourceforge.io/${AROS_URL}" ;;
esac
url_path="${AROS_URL%%\?*}"
if [[ "$url_path" == */download ]]; then AROS_ARCHIVE="$(basename "$(dirname "$url_path")")"; else AROS_ARCHIVE="$(basename "$url_path")"; fi
[[ "$AROS_ARCHIVE" == *"$AROS_TARGET"* ]] || { echo "ERROR: unexpected AROS archive: $AROS_ARCHIVE" >&2; exit 1; }
archive="$OUT_DIR/$AROS_ARCHIVE"
echo "AROS_FETCH_PHASE=archive" >&2
if ! timeout --signal=TERM 210s curl "${CURL_COMMON[@]}" "$AROS_URL" -o "$archive"; then
  echo "ERROR: AROS archive download timed out or failed" >&2
  exit 1
fi
sha256sum "$archive" | tee "$OUT_DIR/archive.sha256"
rm -rf "$OUT_DIR/archive-extracted"; mkdir -p "$OUT_DIR/archive-extracted"
echo "AROS_FETCH_PHASE=extract" >&2
case "$AROS_ARCHIVE" in
  *.lha|*.LHA) timeout --signal=TERM 120s lha xw="$OUT_DIR/archive-extracted" "$archive" >/dev/null ;;
  *.zip|*.ZIP) timeout --signal=TERM 120s unzip -q "$archive" -d "$OUT_DIR/archive-extracted" ;;
  *) echo "ERROR: unsupported AROS archive format" >&2; exit 1 ;;
esac
iso="$(find "$OUT_DIR/archive-extracted" -type f \( -iname '*.iso' -o -iname '*.ISO' \) -print -quit)"
[[ -n "$iso" ]] || { echo "ERROR: no ISO found" >&2; exit 1; }
cp "$iso" "$OUT_DIR/system.iso"
printf 'AROS_INDEX_URL=%s\nAROS_TARGET=%s\nAROS_ARCHIVE=%s\nAROS_URL=%s\nISO=%s\n' "$AROS_INDEX_URL" "$AROS_TARGET" "$AROS_ARCHIVE" "$AROS_URL" "$OUT_DIR/system.iso" > "$OUT_DIR/source.txt"
echo "$OUT_DIR/system.iso"
