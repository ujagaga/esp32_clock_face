#!/usr/bin/env bash
#
# Crop/scale an image to the 320x172 landscape screen and convert it to a raw
# RGB565 (big-endian) .bin file for the SD card image feature.
#
#   tools/img2rgb565.sh input.png [output.bin]
#
# Without an output name, writes alongside the input with a .bin extension.
# Scales to cover the screen, then center-crops to exactly 320x172.
set -euo pipefail

W=320
H=172

if [ $# -lt 1 ]; then
  echo "Usage: $(basename "$0") input-image [output.bin]" >&2
  exit 1
fi

IN="$1"
OUT="${2:-${IN%.*}.bin}"

command -v ffmpeg >/dev/null 2>&1 || {
  echo "ffmpeg not found on PATH." >&2
  exit 1
}

ffmpeg -y -i "$IN" \
  -vf "scale=${W}:${H}:force_original_aspect_ratio=increase,crop=${W}:${H}" \
  -f rawvideo -pix_fmt rgb565be "$OUT"

echo "Wrote $OUT ($(stat -c%s "$OUT") bytes, expected $((W * H * 2)))."
