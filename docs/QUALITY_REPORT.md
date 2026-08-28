# Resonant Matter Field 1.0 — quality report

## Release status

The supplied AU and VST3 are the same bundles installed in the standard per-user macOS plug-in folders. Both are native arm64 and pass deep strict ad-hoc signature verification. Apple's Audio Unit validator completes successfully, including custom UI, all 24 factory programs, 25 parameters, MIDI, parameter reset persistence, slicing, and renders from 11.025 through 192 kHz.

Two automated targets pass:

- `rmf_tests` checks Scala parsing, arbitrary/non-octave periods, malformed input, audible output, and bounded maximum-load DSP.
- `rmf_state_tests` loads Bohlen–Pierce, stores tuning/program/parameter state, deletes the source `.scl`, and proves a fresh processor restores the embedded 3:1/13-degree tuning and audible state.

The reduced Faust companion also compiles successfully. The included WAV/M4A/MP3 audition reel was verified as 48 kHz stereo; the M4A is approximately 45.75 seconds.

These are local ad-hoc-signed development binaries, not Developer ID-notarized retail binaries. No independent Steinberg VST3 validator was installed. The optional malformed JUCE-generated `moduleinfo.json` was removed before final VST3 signing; it is not required for VST3 discovery.

## Gauntlet Loop outcome

The frozen adversarial rubric used Madrona Labs Kaivo as the conceptual benchmark and AAS Chromaphone 3 as the interface/workflow benchmark.

- Round 1: Candidate Y defeated Candidate X, 52.7 to 18.1, but received **Revise** because the evidence packet did not yet match the actual build.
- Round 2: 61.4 after exact artifacts, state-recall proof, expressive MIDI, documentation, and final UI evidence. The critic found malformed optional VST3 metadata and packet-consistency problems.
- Round 3: 63.2 targeted review. The critic verified that all observed package-integrity defects were closed: clean archives and hashes, valid signed arm64 bundles, corrected AU sandbox declaration, removed malformed VST3 metadata, and a complete README-consistent handoff. A final addendum confirms the UI now visibly shows `A4 440 Hz` with no clipping.

The frozen verdict remains **Revise**, not because of an observed crash or installation defect, but because that commercial-benchmark rubric requires Ableton discovery/session-reopen proof plus a longer DSP/spatial measurement program. Ableton was intentionally left closed so its next normal user launch remains the authoritative scan. The full rubric and all three reports are retained under `Documentation/`.

## Next-launch verification

1. Open Ableton Live normally and allow background plug-in indexing to finish.
2. Locate **Plug-ins → VST3 → Resonant Matter Lab → Resonant Matter Field**; the AU is under **Audio Units** when enabled.
3. Instantiate RMF on a MIDI track, play notes, and confirm meters/output.
4. Load `Tunings/bohlen_pierce_13edt.scl`, save the Live Set, quit, reopen, and confirm the scale name, 3:1 period, mapping, and sound return.
5. If Live does not list it, use Live's Preferences plug-in rescan once, then consult `README.md` and `scripts/verify.sh`.
