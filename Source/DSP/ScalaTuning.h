#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace rmf
{
class ScalaTuning
{
public:
    struct Result
    {
        bool ok = false;
        juce::String message;
    };

    ScalaTuning();

    Result loadFromFile (const juce::File& file);
    Result loadFromString (const juce::String& text, const juce::String& sourceName);
    void resetTo12Edo();

    double noteToFrequency (int midiNote, double referenceHz = 440.0, int referenceNote = 69) const noexcept;
    double latticeFrequency (double fundamental, int degree) const noexcept;
    double nearestLatticeFrequency (double target, double fundamental) const noexcept;

    const std::vector<double>& getRatios() const noexcept { return ratios; }
    double getPeriod() const noexcept { return period; }
    int getDegreeCount() const noexcept { return static_cast<int> (ratios.size()); }
    const juce::String& getName() const noexcept { return name; }
    const juce::String& getSourceText() const noexcept { return sourceText; }

private:
    static bool parseInterval (const juce::String& token, double& ratio);

    juce::String name { "12-tone equal temperament" };
    juce::String sourceText;
    std::vector<double> ratios;
    double period = 2.0;
};
}
