#include "PluginProcessor.h"
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
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    const juce::File tuningDir (argc > 1 ? argv[1] : "Tunings");
    auto temporaryScale = tuningDir.getNonexistentChildFile ("rmf-state", ".scl", false);
    check (tuningDir.getChildFile ("bohlen_pierce_13edt.scl").copyFileTo (temporaryScale),
           "create temporary tuning source");

    ResonantMatterFieldAudioProcessor source;
    source.setCurrentProgram (3);
    check (source.loadScalaFile (temporaryScale).ok, "load temporary Bohlen-Pierce tuning");
    if (auto* p = source.getValueTreeState().getParameter ("roughness"))
        p->setValueNotifyingHost (p->convertTo0to1 (0.77f));

    juce::MemoryBlock state;
    source.getStateInformation (state);
    check (state.getSize() > 0, "serialize plugin state");
    check (temporaryScale.deleteFile(), "remove original tuning file");

    ResonantMatterFieldAudioProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    check (restored.getTuning() != nullptr, "restore embedded tuning object");
    check (std::abs (restored.getTuning()->getPeriod() - 3.0) < 0.001, "restore non-octave period without source file");
    check (restored.getTuning()->getDegreeCount() == 13, "restore embedded scale degrees");
    check (restored.getCurrentProgram() == 3, "restore factory program selection");
    check (std::abs (restored.getValueTreeState().getRawParameterValue ("roughness")->load() - 0.77f) < 0.002f,
           "restore audible parameter state");

    if (failures == 0) std::cout << "All RMF state-recall tests passed\n";
    return failures == 0 ? 0 : 1;
}
