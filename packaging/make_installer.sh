#!/bin/bash
# Build a macOS .pkg installer for Resonant Matter Field (AU, VST3, standalone app).
#
#   packaging/make_installer.sh [--build] [BUILD_DIR]     # default BUILD_DIR = build-universal
#
# Output: dist/ResonantMatterField-<version>-macOS.pkg (+ a .zip of the raw bundles for manual installs).
# The installer lets the user tick the formats and installs to /Library/Audio/Plug-Ins/{Components,VST3}
# and /Applications. Bundles are ad-hoc signed unless DEVELOPER_ID_APP / DEVELOPER_ID_INSTALLER / NOTARY_PROFILE
# are set, in which case the script signs and notarizes. Files installed by a .pkg are not quarantined, so the
# plug-ins load in any host once the installer has been opened (right-click > Open the first time if Gatekeeper
# complains about the unsigned installer).
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
DIST="$ROOT/dist"
WORK="$(mktemp -d /tmp/rmf-pkgwork.XXXXXX)"
trap 'rm -rf "$WORK"' EXIT

BUILD=""
BUILD_DIR="build-universal"
for a in "$@"; do
  case "$a" in
    --build) BUILD=1 ;;
    *) BUILD_DIR="$a" ;;
  esac
done
VERSION="$(sed -n 's/^project(ResonantMatterField VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")"
ORG="com.resonantmatterlab"
if [[ -n "$BUILD" ]]; then
  cmake -S "$ROOT" -B "$ROOT/$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release ${JUCE_ROOT:+-DJUCE_ROOT="$JUCE_ROOT"}
  cmake --build "$ROOT/$BUILD_DIR" -j "$(sysctl -n hw.ncpu)"
fi
ART="$ROOT/$BUILD_DIR/ResonantMatterField_artefacts/Release"

check () {
  [[ -d "$1" ]] || { echo "missing bundle: $1" >&2; exit 1; }
  local exe; exe="$(find "$1/Contents/MacOS" -type f | head -1)"
  local archs; archs="$(lipo -archs "$exe")"
  echo "  $(basename "$1"): $archs"
  [[ "$archs" == *arm64* && "$archs" == *x86_64* ]] || echo "  WARNING: $(basename "$1") is not universal ($archs)" >&2
}

SIGN_APP="${DEVELOPER_ID_APP:--}"
SIGN_PKG="${DEVELOPER_ID_INSTALLER:-}"
mkdir -p "$WORK/pkgs" "$WORK/res" "$DIST"

component () { # <bundle> <install-location> <pkg-id>
  local bundle="$1" loc="$2" pid="$3" stage="$WORK/stage-$3"
  check "$bundle"
  mkdir -p "$stage"
  ditto "$bundle" "$stage/$(basename "$bundle")"
  if [[ "$SIGN_APP" == "-" ]]; then
    codesign --force --deep -s - "$stage/$(basename "$bundle")"
  else
    codesign --force --deep --options runtime --timestamp -s "$SIGN_APP" "$stage/$(basename "$bundle")"
  fi
  pkgbuild --analyze --root "$stage" "$WORK/$pid.plist" >/dev/null
  plutil -replace 0.BundleIsRelocatable -bool NO "$WORK/$pid.plist"
  pkgbuild --root "$stage" --component-plist "$WORK/$pid.plist" \
           --install-location "$loc" --identifier "$pid" --version "$VERSION" \
           "$WORK/pkgs/$pid.pkg" >/dev/null
}
component "$ART/AU/Resonant Matter Field.component"   "/Library/Audio/Plug-Ins/Components" "$ORG.rmf.au"
component "$ART/VST3/Resonant Matter Field.vst3"      "/Library/Audio/Plug-Ins/VST3"       "$ORG.rmf.vst3"
component "$ART/Standalone/Resonant Matter Field.app" "/Applications"                      "$ORG.rmf.app"

sed -e "s/@VERSION@/$VERSION/g" "$HERE/resources/welcome.html" > "$WORK/res/welcome.html"
cp "$HERE/resources/conclusion.html" "$WORK/res/conclusion.html"
cat > "$WORK/distribution.xml" <<XML
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Resonant Matter Field $VERSION</title>
  <organization>$ORG</organization>
  <welcome file="welcome.html" mime-type="text/html"/>
  <conclusion file="conclusion.html" mime-type="text/html"/>
  <options customize="always" require-scripts="false" hostArchitectures="arm64,x86_64"/>
  <domains enable_localSystem="true"/>
  <os-version min="12.0"/>
  <choices-outline>
    <line choice="au"/><line choice="vst3"/><line choice="app"/>
  </choices-outline>
  <choice id="au"   title="Audio Unit (AU)" description="Logic, GarageBand, Live, Bitwig. Installs to /Library/Audio/Plug-Ins/Components."><pkg-ref id="$ORG.rmf.au"/></choice>
  <choice id="vst3" title="VST3"            description="Live, Bitwig, Reaper, Cubase, Studio One. Installs to /Library/Audio/Plug-Ins/VST3."><pkg-ref id="$ORG.rmf.vst3"/></choice>
  <choice id="app"  title="Standalone app"  description="Play without a DAW (MIDI in, audio out from your interface). Installs to /Applications."><pkg-ref id="$ORG.rmf.app"/></choice>
  <pkg-ref id="$ORG.rmf.au">$ORG.rmf.au.pkg</pkg-ref>
  <pkg-ref id="$ORG.rmf.vst3">$ORG.rmf.vst3.pkg</pkg-ref>
  <pkg-ref id="$ORG.rmf.app">$ORG.rmf.app.pkg</pkg-ref>
</installer-gui-script>
XML

OUT="$DIST/ResonantMatterField-$VERSION-macOS.pkg"
rm -f "$OUT"
productbuild --distribution "$WORK/distribution.xml" --package-path "$WORK/pkgs" \
             --resources "$WORK/res" ${SIGN_PKG:+--sign "$SIGN_PKG"} "$OUT" >/dev/null
if [[ -n "$SIGN_PKG" && -n "${NOTARY_PROFILE:-}" ]]; then
  xcrun notarytool submit "$OUT" --keychain-profile "$NOTARY_PROFILE" --wait
  xcrun stapler staple "$OUT"
fi

ZIP="$DIST/ResonantMatterField-$VERSION-macOS-bundles.zip"
rm -f "$ZIP"
BUNDLES="$WORK/Resonant Matter Field $VERSION"
mkdir -p "$BUNDLES"
for d in "$WORK"/stage-*; do cp -R "$d"/* "$BUNDLES"/; done
cp -R "$ROOT/Tunings" "$BUNDLES/Tunings"
cp "$HERE/INSTALL-macOS-manual.txt" "$BUNDLES/"
ditto -c -k --keepParent "$BUNDLES" "$ZIP"
echo "wrote $OUT"
echo "wrote $ZIP"
