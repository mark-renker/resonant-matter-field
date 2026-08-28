# Contributing

Thanks for helping improve Resonant Matter Field.

## Before opening a pull request

1. Open an issue first for substantial DSP, state-format, or interface changes.
2. Keep parameter identifiers stable; changing them can break host automation and saved sessions.
3. Preserve real-time safety: no allocation, file I/O, blocking lock, logging, or modal UI work inside `processBlock` or `RMFEngine::renderSample`.
4. Add or update tests for tuning, state recovery, stability, and changed DSP behavior.
5. Run the complete local build and test sequence documented in the README.
6. Describe the musical result as well as the implementation. Short dry before/after renders are especially useful for DSP changes.

## Style and scope

- Use modern C++20 and the existing JUCE conventions.
- Prefer small, reviewable changes over broad rewrites.
- Keep physical/electromechanical claims precise: the model is intentionally reduced and musically motivated.
- Do not describe wellness, spiritual, or tuning concepts as medical efficacy.
- Do not commit JUCE, generated build directories, installed plug-in bundles, or private tuning libraries.

## Testing priorities

- sample rates: 44.1, 48, and 96 kHz
- small and large host block sizes
- note/release/sustain/voice-stealing behavior
- maximum density, mode count, polyphony, and active impedance
- malformed and non-octave Scala files
- tuning recovery after the source `.scl` disappears
- AU/VST3 state parity where a host is available

By contributing, you agree that your contribution may be distributed under this repository's MIT License.
