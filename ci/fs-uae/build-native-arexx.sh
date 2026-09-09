#!/usr/bin/env bash
set -euo pipefail

IMAGE="${AMIFORENSICS_BEBBO_IMAGE:-amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed}"
OUT_DIR="${1:-build/fs-uae/native-arexx}"
mkdir -p "$OUT_DIR"

docker pull "$IMAGE"
docker image inspect "$IMAGE" --format '{{join .RepoDigests "\n"}}' | tee "$OUT_DIR/toolchain-image.txt"

docker run --rm \
  -v "$PWD:/work" \
  -w /work \
  "$IMAGE" \
  m68k-amigaos-gcc \
    -Os -Wall -Wextra -Werror -m68000 \
    -o ResidentView-ARexx \
    src/residentview.c \
    -mcrt=nix20

cp ResidentView-ARexx "$OUT_DIR/ResidentView-ARexx"
file "$OUT_DIR/ResidentView-ARexx" | tee "$OUT_DIR/file.txt"
sha256sum "$OUT_DIR/ResidentView-ARexx" | tee "$OUT_DIR/ResidentView-ARexx.sha256"

if ! grep -Eiq 'AmigaOS|Amiga.*executable|loadseg' "$OUT_DIR/file.txt"; then
  echo "ERROR: ARexx-enabled native output is not recognized as an Amiga executable" >&2
  exit 1
fi

printf 'STATUS=PASS\nGATE=M2_1D_NATIVE_AREXX_BUILD\nMODE=AREXX_ENABLED\nIMAGE=%s\nBINARY=%s\n' \
  "$IMAGE" "$OUT_DIR/ResidentView-ARexx" | tee "$OUT_DIR/result.txt"
