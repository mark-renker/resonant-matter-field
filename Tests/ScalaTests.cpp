#include "DSP/ScalaTuning.h"
#include "DSP/RMFEngine.h"
#include <cmath>
#include <iostream>

namespace
{
int failures = 0;
void check (bool condition, const char* message)
{
    if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main (int argc, char** argv)
{
    const juce::File tuningDir (argc > 1 ? argv[1] : "Tunings");
    rmf::ScalaTuning tuning;
    check (std::abs (tuning.noteToFrequency (69) - 440.0) < 0.001, "12-EDO reference frequency");
    check (std::abs (tuning.noteToFrequency (81) - 880.0) < 0.01, "12-EDO octave");

    auto result = tuning.loadFromFile (tuningDir.getChildFile ("19edo.scl"));
    check (result.ok, "parse 19-EDO file");
    check (tuning.getDegreeCount() == 19, "19-EDO degree count");
    check (std::abs (tuning.latticeFrequency (100.0, 19) - 200.0) < 0.01, "19-EDO period");

    result = tuning.loadFromFile (tuningDir.getChildFile ("bohlen_pierce_13edt.scl"));
    check (result.ok, "parse Bohlen-Pierce file");
    check (std::abs (tuning.getPeriod() - 3.0) < 0.001, "arbitrary 3/1 period");
    check (std::abs (tuning.latticeFrequency (100.0, 13) - 300.0) < 0.01, "Bohlen-Pierce tritave");

    auto bad = tuning.loadFromString ("Bad\n3\n100.0\n50.0\n2/1\n", "bad.scl");
    check (! bad.ok, "reject descending intervals");

    rmf::Engine engine;
    rmf::Parameters p;
    engine.prepare (48000.0);
    engine.noteOn (48, 0.8f, p, tuning);
    double energy = 0.0;
    for (int i = 0; i < 48000; ++i)
    {
        auto s = engine.renderSample (p, tuning);
        check (std::isfinite (s[0]) && std::isfinite (s[1]), "DSP output remains finite");
        energy += std::abs (s[0]) + std::abs (s[1]);
    }
    check (energy > 0.1, "DSP produces audible energy");
    check (energy < 96000.0, "DSP remains bounded");

    // Worst-case automation corner: maximum voices/modes/contact density with
    // reinforcing active impedance must remain finite and limiter-bounded.
    p.polyphony = rmf::maxVoices; p.modeCount = rmf::maxModes;
    p.density = 400.0f; p.durationMs = 100.0f; p.force = 1.0f;
    p.roughness = 1.0f; p.coupling = 1.0f; p.loading = 0.0f;
    p.activeImpedance = 0.8f; p.damping = 12.0f;
    engine.reset();
    for (int note = 36; note < 48; ++note) engine.noteOn (note, 1.0f, p, tuning);
    for (int i = 0; i < 24000; ++i)
    {
        auto s = engine.renderSample (p, tuning);
        check (std::isfinite (s[0]) && std::isfinite (s[1]), "extreme DSP remains finite");
        check (std::abs (s[0]) <= 1.0001f && std::abs (s[1]) <= 1.0001f, "output limiter remains bounded");
    }

    if (failures == 0) std::cout << "All RMF tests passed\n";
    return failures == 0 ? 0 : 1;
}
