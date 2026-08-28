#!/bin/sh
set -eu

VST="$HOME/Library/Audio/Plug-Ins/VST3/Resonant Matter Field.vst3"
AU="$HOME/Library/Audio/Plug-Ins/Components/Resonant Matter Field.component"

test -d "$VST"
test -d "$AU"
codesign --verify --deep --strict --verbose=2 "$VST"
codesign --verify --deep --strict --verbose=2 "$AU"
file "$VST/Contents/MacOS/Resonant Matter Field"
file "$AU/Contents/MacOS/Resonant Matter Field"
xattr -p com.apple.quarantine "$VST" 2>/dev/null && exit 1 || true
xattr -p com.apple.quarantine "$AU" 2>/dev/null && exit 1 || true
auval -v aumu Rmf1 Rmfl
