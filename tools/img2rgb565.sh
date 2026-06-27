#!/usr/bin/env bash
#
# Crop/scale every image in tools/images_png to the 320x172 landscape screen and
# convert each to a raw RGB565 (big-endian) .bin file for the SD card image feature.
#
#   tools/img2rgb565.sh [input-dir] [output-dir]
#
# Defaults to tools/images_png -> tools/images_rgb565 (relative to this script).
# Scales to cover the screen, then center-crops to exactly 320x172.
set -euo pipefail

W=320
H=172

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IN_DIR="${1:-$SCRIPT_DIR/images_png}"
OUT_DIR="${2:-$SCRIPT_DIR/images_rgb565}"

command -v ffmpeg >/dev/null 2>&1 || {
  echo "ffmpeg not found on PATH." >&2
  exit 1
}

[ -d "$IN_DIR" ] || {
  echo "Input dir not found: $IN_DIR" >&2
  exit 1
}

mkdir -p "$OUT_DIR"

shopt -s nullglob nocaseglob
count=0
for IN in "$IN_DIR"/*.png "$IN_DIR"/*.jpg "$IN_DIR"/*.jpeg "$IN_DIR"/*.bmp; do
  base="$(basename "$IN")"
  OUT="$OUT_DIR/${base%.*}.bin"
  ffmpeg -y -i "$IN" \
    -vf "scale=${W}:${H}:force_original_aspect_ratio=increase,crop=${W}:${H}" \
    -f rawvideo -pix_fmt rgb565be "$OUT"
  echo "Wrote $OUT ($(stat -c%s "$OUT") bytes, expected $((W * H * 2)))."
  count=$((count + 1))
done

[ "$count" -gt 0 ] || echo "No images found in $IN_DIR." >&2
