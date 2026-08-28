#!/bin/sh
set -eu

PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
JUCE_DIR=${JUCE_ROOT:-/Applications/JUCE}
BUILD_DIR=${RMF_BUILD_DIR:-"$PROJECT_DIR/build"}

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -G "Unix Makefiles" \
  -DJUCE_ROOT="$JUCE_DIR" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="${TARGET_ARCH:-arm64}"
cmake --build "$BUILD_DIR" -j "${BUILD_JOBS:-4}"
ctest --test-dir "$BUILD_DIR" --output-on-failure
