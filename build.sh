#!/usr/bin/env bash
#
# Build the TeHyBug CO2 Desk firmware with arduino-cli.
#
# Usage: ./build.sh [debug|release|all]   (default: release)
#
#   release  - DEBUG_ENABLED=0: quiet serial port, what ships on devices
#   debug    - DEBUG_ENABLED=1: verbose serial logging at 115200 baud
#   all      - both modes
#
# Binaries land in build/<board>/<mode>/. A release build for the esp8285
# (the board the TeHyBug actually uses) also refreshes the prebuilt
# tehybug_co2_desk_firmware.ino.esp8285.bin shipped in the repo root.
#
# If a ./libraries directory exists (vendored libraries), it is used
# exclusively; otherwise the default arduino-cli user libraries apply.

set -euo pipefail

MODE="${1:-release}"
case "$MODE" in
    debug|release|all) ;;
    *) echo "Usage: $0 [debug|release|all]" >&2; exit 2 ;;
esac

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
SKETCH_NAME="tehybug_co2_desk_firmware"

# Boards to build (suffix of the esp8266:esp8266:<board> FQBN).
# esp8285 is the TeHyBug hardware; the others are common dev boards
# named in the firmware header.
BOARDS=(esp8285 nodemcuv2 d1_mini)

# arduino-cli requires the sketch directory to be named like the .ino,
# but this repo directory is not. Build through a symlink shim instead
# of renaming files.
SHIM_PARENT="$(mktemp -d)"
SHIM="$SHIM_PARENT/$SKETCH_NAME"
mkdir -p "$SHIM"
trap 'rm -rf "$SHIM_PARENT"' EXIT
ln -s "$REPO_DIR/$SKETCH_NAME.ino" \
      "$REPO_DIR/images.h" \
      "$REPO_DIR/Webinterface.h" \
      "$REPO_DIR/src" \
      "$SHIM/"

LIB_ARGS=()
if [ -d "$REPO_DIR/libraries" ]; then
    LIB_ARGS=(--libraries "$REPO_DIR/libraries")
    echo "Using vendored libraries from $REPO_DIR/libraries"
fi

build_one() {
    local board="$1" mode="$2"
    local out_dir="$REPO_DIR/build/$board/$mode"
    local mode_args=()
    if [ "$mode" = debug ]; then
        mode_args=(--build-property "compiler.cpp.extra_flags=-DDEBUG_ENABLED=1")
    fi

    echo "=== $board / $mode ==="
    arduino-cli compile \
        --fqbn "esp8266:esp8266:$board" \
        "${LIB_ARGS[@]}" \
        ${mode_args[0]+"${mode_args[@]}"} \
        --output-dir "$out_dir" \
        "$SHIM" \
        2>&1 | grep -E "^(Der Sketch|Globale|Sketch uses|Global variables)" || true

    local bin="$out_dir/$SKETCH_NAME.ino.bin"
    [ -f "$bin" ] || { echo "ERROR: $bin was not produced" >&2; exit 1; }
    echo "    -> $bin ($(wc -c < "$bin" | tr -d ' ') bytes)"

    # Keep the prebuilt binary in the repo root current.
    if [ "$board" = esp8285 ] && [ "$mode" = release ]; then
        cp "$bin" "$REPO_DIR/$SKETCH_NAME.ino.esp8285.bin"
        echo "    -> refreshed $SKETCH_NAME.ino.esp8285.bin"
    fi
}

MODES=("$MODE")
[ "$MODE" = all ] && MODES=(release debug)

for board in "${BOARDS[@]}"; do
    for mode in "${MODES[@]}"; do
        build_one "$board" "$mode"
    done
done

echo "Done."
