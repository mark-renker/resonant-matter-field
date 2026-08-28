#include "ScalaTuning.h"
#include <cmath>
#include <limits>

namespace rmf
{
ScalaTuning::ScalaTuning() { resetTo12Edo(); }

void ScalaTuning::resetTo12Edo()
{
    name = "12-tone equal temperament";
    sourceText = "! Built-in 12-EDO\n12-tone equal temperament\n12\n100.0\n200.0\n300.0\n400.0\n500.0\n600.0\n700.0\n800.0\n900.0\n1000.0\n1100.0\n2/1\n";
    ratios.clear();
    ratios.reserve (12);
    ratios.push_back (1.0);
    for (int i = 1; i < 12; ++i)
        ratios.push_back (std::pow (2.0, static_cast<double> (i) / 12.0));
    period = 2.0;
}

ScalaTuning::Result ScalaTuning::loadFromFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return { false, "Tuning file does not exist" };
    return loadFromString (file.loadFileAsString(), file.getFileName());
}

bool ScalaTuning::parseInterval (const juce::String& original, double& ratio)
{
    auto token = original.trim();
    if (token.isEmpty()) return false;

    if (token.containsChar ('/'))
    {
        auto parts = juce::StringArray::fromTokens (token, "/", "");
        if (parts.size() != 2) return false;
        auto numerator = parts[0].getDoubleValue();
        auto denominator = parts[1].getDoubleValue();
        if (numerator <= 0.0 || denominator <= 0.0) return false;
        ratio = numerator / denominator;
    }
    else if (token.containsChar ('.'))
    {
        ratio = std::pow (2.0, token.getDoubleValue() / 1200.0);
    }
    else
    {
        auto integerRatio = token.getDoubleValue();
        if (integerRatio <= 0.0) return false;
        ratio = integerRatio;
    }

    return std::isfinite (ratio) && ratio > 0.0;
}

ScalaTuning::Result ScalaTuning::loadFromString (const juce::String& text, const juce::String& sourceName)
{
    juce::StringArray logicalLines;
    logicalLines.addLines (text);
    for (int i = logicalLines.size(); --i >= 0;)
    {
        auto line = logicalLines[i].trim();
        if (line.isEmpty() || line.startsWithChar ('!')) logicalLines.remove (i);
        else logicalLines.set (i, line.upToFirstOccurrenceOf ("!", false, false).trim());
    }

    if (logicalLines.size() < 3)
        return { false, "Scala file is missing its name, count, or intervals" };

    auto declaredCount = logicalLines[1].getIntValue();
    if (declaredCount < 1 || declaredCount > 256)
        return { false, "Scala degree count must be between 1 and 256" };
    if (logicalLines.size() < declaredCount + 2)
        return { false, "Scala file declares more intervals than it contains" };

    std::vector<double> parsed;
    parsed.reserve (static_cast<size_t> (declaredCount));
    parsed.push_back (1.0);
    double last = 1.0;
    for (int i = 0; i < declaredCount; ++i)
    {
        double value = 0.0;
        if (! parseInterval (logicalLines[i + 2], value))
            return { false, "Invalid Scala interval on logical line " + juce::String (i + 3) };
        if (value <= last)
            return { false, "Scala intervals must be strictly ascending" };
        if (i + 1 < declaredCount) parsed.push_back (value);
        last = value;
    }

    if (last <= 1.0 || last > 16.0)
        return { false, "Scala period must be greater than 1/1 and no larger than 16/1" };

    ratios = std::move (parsed);
    period = last;
    name = logicalLines[0].isNotEmpty() ? logicalLines[0] : sourceName;
    sourceText = text;
    return { true, "Loaded " + name + " (" + juce::String (getDegreeCount()) + " degrees)" };
}

double ScalaTuning::latticeFrequency (double fundamental, int degree) const noexcept
{
    if (ratios.empty() || fundamental <= 0.0) return fundamental;
    const auto n = static_cast<int> (ratios.size());
    auto cycle = degree / n;
    auto index = degree % n;
    if (index < 0) { index += n; --cycle; }
    return fundamental * std::pow (period, static_cast<double> (cycle)) * ratios[static_cast<size_t> (index)];
}

double ScalaTuning::noteToFrequency (int midiNote, double referenceHz, int referenceNote) const noexcept
{
    return latticeFrequency (referenceHz, midiNote - referenceNote);
}

double ScalaTuning::nearestLatticeFrequency (double target, double fundamental) const noexcept
{
    if (target <= 0.0 || fundamental <= 0.0 || ratios.empty()) return target;
    const auto approxCycle = static_cast<int> (std::floor (std::log (target / fundamental) / std::log (period)));
    double best = target;
    double bestDistance = std::numeric_limits<double>::max();
    for (int cycle = approxCycle - 1; cycle <= approxCycle + 1; ++cycle)
        for (auto ratio : ratios)
        {
            const auto candidate = fundamental * std::pow (period, static_cast<double> (cycle)) * ratio;
            const auto distance = std::abs (std::log (candidate / target));
            if (distance < bestDistance) { bestDistance = distance; best = candidate; }
        }
    return best;
}
}
