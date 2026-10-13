#!/usr/bin/env bash
# Build obs-apple-borders on macOS (Apple Silicon or Intel).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
CONFIG="${CONFIG:-Release}"

# Point at your OBS build or deps prefix if CMake cannot find libobs automatically.
# Examples:
#   export OBS_DIR="$HOME/obs-studio/build"
#   export CMAKE_PREFIX_PATH="/path/to/obs-deps:$CMAKE_PREFIX_PATH"
OBS_DIR="${OBS_DIR:-}"

ARCH_ARGS=()
# Universal binary (optional): ARCHS="arm64;x86_64" ./scripts/build-macos.sh
if [[ -n "${ARCHS:-}" ]]; then
  ARCH_ARGS+=(-DCMAKE_OSX_ARCHITECTURES="${ARCHS}")
fi

cmake -S "$ROOT" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE="$CONFIG" \
  ${OBS_DIR:+-DOBS_DIR="$OBS_DIR"} \
  "${ARCH_ARGS[@]}" \
  "$@"

cmake --build "$BUILD_DIR" --config "$CONFIG"
cmake --install "$BUILD_DIR" --config "$CONFIG"

echo
echo "Built plugin bundle:"
echo "  $BUILD_DIR/rundir/obs-apple-borders.plugin"
echo "Installed (default) to:"
echo "  $HOME/Library/Application Support/obs-studio/plugins/obs-apple-borders.plugin"
echo
echo "Restart OBS Studio, then add filters: Apple Border / Apple Picture-in-Picture"
