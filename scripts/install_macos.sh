#!/bin/sh
set -eu

PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
VST_SOURCE="$PROJECT_DIR/Binaries/Resonant Matter Field.vst3"
AU_SOURCE="$PROJECT_DIR/Binaries/Resonant Matter Field.component"
VST_DEST="$HOME/Library/Audio/Plug-Ins/VST3/Resonant Matter Field.vst3"
AU_DEST="$HOME/Library/Audio/Plug-Ins/Components/Resonant Matter Field.component"

test -d "$VST_SOURCE"
test -d "$AU_SOURCE"
mkdir -p "$(dirname "$VST_DEST")" "$(dirname "$AU_DEST")"
ditto "$VST_SOURCE" "$VST_DEST"
ditto "$AU_SOURCE" "$AU_DEST"
codesign --verify --deep --strict "$VST_DEST"
codesign --verify --deep --strict "$AU_DEST"
printf '%s\n' "Installed Resonant Matter Field AU and VST3. Reopen Ableton Live to scan them normally."
