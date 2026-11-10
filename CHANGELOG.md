# Changelog

## 1.0.2 — 2026-11-09

- Prebuilt installers: tagged releases now ship a macOS universal (arm64 + x86_64) `.pkg` installer with
  selectable AU / VST3 / standalone components, a raw-bundles zip for manual installs, and a CI-built Windows
  x64 VST3/standalone zip (`.github/workflows/release.yml`, `packaging/`).
- CMake: macOS builds default to universal binaries; Objective-C languages are only enabled on Apple so the
  project configures on Windows.
- README: "Install (no build needed)" section replaces the old download notes.
- CI: GitHub's `macos-14` runner label is retired; both workflows now use `macos-15` (v1.0.1 was tagged before
  this fix and has no CI-built assets).
- No DSP or behaviour changes.

## 1.0.0 — 2026-08-27

- Initial public release of Resonant Matter Field synthesis.
- AU, VST3, and standalone JUCE formats.
- Twelve voices, thirty-two spatial body modes, and four overlapping microcontact slots per voice.
- Runtime Scala tuning with arbitrary periods, nearest-mode and ascending-lattice mappings, and embedded state recovery.
- Electromechanical loading, inductance, BL coupling, back-EMF, and bounded active impedance.
- Interactive spatial field, two modal pickups, resizable interface, meters, and twenty-four factory sounds.
- Velocity, sustain, pitch bend, mod wheel, and pressure performance mappings.
- Dry WAV/M4A/MP3 demonstrations, tests, source, and reduced Faust companion.
