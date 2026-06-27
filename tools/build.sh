#!/usr/bin/env bash
#
# Build (and optionally flash) the ESP32-C6 clock firmware.
#
#   tools/build.sh            compile only
#   tools/build.sh upload     compile, then flash
#
# Overrides via env:
#   FQBN=esp32:esp32:esp32c6   target board
#   PORT=/dev/ttyACM0          serial port for upload
set -euo pipefail

SKETCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FQBN="${FQBN:-esp32:esp32:esp32c6}"
PORT="${PORT:-/dev/ttyACM0}"

command -v arduino-cli >/dev/null 2>&1 || {
  echo "arduino-cli not found on PATH. Install: https://arduino.github.io/arduino-cli/latest/installation/" >&2
  exit 1
}

echo "Compiling $SKETCH_DIR for $FQBN ..."
arduino-cli compile --fqbn "$FQBN" "$SKETCH_DIR"

if [ "${1:-}" = "upload" ]; then
  echo "Uploading to $PORT ..."
  arduino-cli upload --fqbn "$FQBN" -p "$PORT" "$SKETCH_DIR"
fi
