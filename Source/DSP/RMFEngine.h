#pragma once

#include "ScalaTuning.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>

namespace rmf
{
constexpr int maxModes = 32;
constexpr int maxVoices = 12;
constexpr int grainSlots = 4;

struct Parameters
{
    float density = 18.0f;
    float durationMs = 8.0f;
    float force = 0.55f;
    float roughness = 0.25f;
    float contactX = 0.37f;
    float contactY = 0.61f;
    float positionSpread = 0.15f;
    float trajectoryDepth = 0.2f;
    float trajectoryRate = 0.17f;
    float stiffness = 1.0f;
    float anisotropy = 0.0f;
    float dispersion = 0.018f;
    float damping = 1.8f;
    float coupling = 0.35f;
    float loading = 0.42f;
    float inductance = 0.25f;
    float activeImpedance = 0.0f;
    float radiationWidth = 0.7f;
    float outputGain = 0.5f;
    int modeCount = 24;
    int tuningMode = 1;
    int tuningSpread = 1;
    int transpose = 0;
    int polyphony = 8;
};

class Random
{
public:
    explicit Random (uint32_t seed = 0x1234567u) : state (seed != 0 ? seed : 1u) {}
    float uniform() noexcept
    {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        return static_cast<float> (state) * (1.0f / 4294967296.0f);
    }
    float bipolar() noexcept { return uniform() * 2.0f - 1.0f; }
private:
    uint32_t state;
};

class Voice
{
public:
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        reset();
    }

    void reset() noexcept
    {
        active = false; releasing = false; sustainHeld = false;
        ageSamples = 0; quietSamples = 0; coilCurrent = 0.0f;
        nextEventSamples = 0.0f; trajectoryPhase = 0.0f;
        for (auto& m : modes) m = {};
        for (auto& g : grains) g = {};
    }

    void start (int newNote, float newVelocity, uint64_t stamp, const ScalaTuning& tuning,
                const Parameters& p, float pitchRatio) noexcept
    {
        reset();
        active = true; note = newNote; velocity = juce::jlimit (0.01f, 1.0f, newVelocity); startStamp = stamp;
        fundamental = tuning.noteToFrequency (newNote + p.transpose);
        initialiseModes (tuning, p, pitchRatio);
        triggerGrain (p, 1.35f);
        scheduleNextEvent (p.density);
    }

    void stop (bool allowTail) noexcept
    {
        if (! active) return;
        if (sustainHeld) { releasing = true; return; }
        if (allowTail) releasing = true;
        else reset();
    }

    void setSustain (bool down) noexcept
    {
        sustainHeld = down;
        if (! down && releasing) sustainHeld = false;
    }

    bool isActive() const noexcept { return active; }
    int getNote() const noexcept { return note; }
    uint64_t getStartStamp() const noexcept { return startStamp; }

    std::array<float, 2> render (const Parameters& p, const ScalaTuning& tuning, float pitchRatio) noexcept
    {
        if (! active) return {};

        if ((ageSamples & 63u) == 0u)
            updateModes (tuning, p, pitchRatio);

        trajectoryPhase += static_cast<float> (juce::MathConstants<double>::twoPi * p.trajectoryRate / sampleRate);
        if (trajectoryPhase > juce::MathConstants<float>::twoPi) trajectoryPhase -= juce::MathConstants<float>::twoPi;

        nextEventSamples -= 1.0f;
        if (nextEventSamples <= 0.0f && ! releasing)
        {
            triggerGrain (p, 1.0f);
            scheduleNextEvent (p.density);
        }

        float forceSignal = 0.0f;
        for (auto& grain : grains)
        {
            if (! grain.running) continue;
            const auto phase = static_cast<float> (grain.age) / static_cast<float> (juce::jmax (grain.length, 1));
            if (phase >= 1.0f) { grain.running = false; continue; }
            const auto smoothPulse = std::sin (juce::MathConstants<float>::pi * phase);
            const auto crackle = random.bipolar();
            grain.value = grain.amplitude * smoothPulse * ((1.0f - p.roughness) + p.roughness * crackle);
            forceSignal += grain.value;
            ++grain.age;
        }

        // An electrodynamic equivalent circuit: applied force drives current, while
        // modal velocity creates opposing back-EMF. Bounded active impedance can
        // reinforce the motion without allowing a numerical runaway.
        float bodyVelocity = 0.0f;
        for (int i = 0; i < p.modeCount; ++i)
            bodyVelocity += modes[static_cast<size_t> (i)].y1 - modes[static_cast<size_t> (i)].y2;
        bodyVelocity *= 0.06f;

        const auto bl = 0.08f + 1.4f * p.coupling;
        const auto resistance = 0.35f + 10.0f * p.loading;
        const auto tau = 0.00004f + 0.003f * p.inductance;
        const auto circuitStep = juce::jmin (0.5f, static_cast<float> (1.0 / sampleRate) / tau);
        const auto appliedVoltage = forceSignal * (0.5f + p.coupling);
        const auto backEmf = bl * bodyVelocity;
        coilCurrent += circuitStep * (appliedVoltage - resistance * coilCurrent - backEmf);
        coilCurrent = juce::jlimit (-3.0f, 3.0f, coilCurrent);
        const auto activeForce = bl * coilCurrent * (1.0f + 0.8f * p.activeImpedance);

        float left = 0.0f, right = 0.0f;
        for (int i = 0; i < p.modeCount; ++i)
        {
            auto& m = modes[static_cast<size_t> (i)];
            float modalForce = activeForce * 0.015f;
            for (const auto& grain : grains)
                if (grain.running)
                    modalForce += grain.value * grain.shapes[static_cast<size_t> (i)] * 0.0075f;

            const auto y = m.a1 * m.y1 - m.a2 * m.y2 + modalForce * m.gain;
            m.y2 = m.y1;
            m.y1 = std::isfinite (y) ? juce::jlimit (-8.0f, 8.0f, y) : 0.0f;
            left  += m.y1 * m.pickupL;
            right += m.y1 * m.pickupR;
        }

        const auto norm = 0.8f / std::sqrt (static_cast<float> (juce::jmax (1, p.modeCount)));
        left *= norm * velocity;
        right *= norm * velocity;

        ++ageSamples;
        const auto magnitude = std::abs (left) + std::abs (right) + std::abs (coilCurrent) * 0.01f;
        if (releasing && magnitude < 1.0e-5f) ++quietSamples; else quietSamples = 0;
        if ((releasing && quietSamples > static_cast<uint64_t> (sampleRate * 0.25))
            || ageSamples > static_cast<uint64_t> (sampleRate * 90.0))
            reset();

        return { left, right };
    }

private:
    struct Mode
    {
        float y1 = 0.0f, y2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f, gain = 1.0f, pickupL = 0.0f, pickupR = 0.0f;
        int nx = 1, ny = 1;
    };
    struct Grain
    {
        bool running = false;
        int age = 0, length = 1;
        float amplitude = 0.0f, x = 0.5f, y = 0.5f, value = 0.0f;
        std::array<float, maxModes> shapes {};
    };

    static float modeShape (int nx, int ny, float x, float y) noexcept
    {
        return std::sin (juce::MathConstants<float>::pi * static_cast<float> (nx) * x)
             * std::sin (juce::MathConstants<float>::pi * static_cast<float> (ny) * y);
    }

    void initialiseModes (const ScalaTuning& tuning, const Parameters& p, float pitchRatio) noexcept
    {
        static constexpr std::array<std::array<int, 2>, maxModes> topology {{
            {{1,1}},{{2,1}},{{1,2}},{{2,2}},{{3,1}},{{1,3}},{{3,2}},{{2,3}},
            {{4,1}},{{1,4}},{{3,3}},{{4,2}},{{2,4}},{{5,1}},{{1,5}},{{4,3}},
            {{3,4}},{{5,2}},{{2,5}},{{4,4}},{{6,1}},{{1,6}},{{5,3}},{{3,5}},
            {{6,2}},{{2,6}},{{5,4}},{{4,5}},{{6,3}},{{3,6}},{{5,5}},{{6,4}}
        }};
        for (int i = 0; i < maxModes; ++i)
        {
            modes[static_cast<size_t> (i)].nx = topology[static_cast<size_t> (i)][0];
            modes[static_cast<size_t> (i)].ny = topology[static_cast<size_t> (i)][1];
            modes[static_cast<size_t> (i)].gain = 1.0f / (1.0f + 0.11f * static_cast<float> (i));
        }
        updateModes (tuning, p, pitchRatio);
    }

    void updateModes (const ScalaTuning& tuning, const Parameters& p, float pitchRatio) noexcept
    {
        const auto nyquist = 0.46 * sampleRate;
        const auto pickupOffset = 0.42f * p.radiationWidth;
        const auto pickupLX = 0.5f - pickupOffset;
        const auto pickupRX = 0.5f + pickupOffset;
        constexpr auto pickupY = 0.43f;
        for (int i = 0; i < maxModes; ++i)
        {
            auto& m = modes[static_cast<size_t> (i)];
            const auto nx = static_cast<double> (m.nx);
            const auto ny = static_cast<double> (m.ny);
            const auto ax = 1.0 + 0.75 * static_cast<double> (p.anisotropy);
            const auto ay = 1.0 - 0.75 * static_cast<double> (p.anisotropy);
            auto geometryRatio = std::sqrt (ax * nx * nx + ay * ny * ny) / std::sqrt (ax + ay);
            geometryRatio *= 1.0 + static_cast<double> (p.dispersion) * geometryRatio * geometryRatio;
            const auto bentFundamental = fundamental * static_cast<double> (pitchRatio);
            auto hz = bentFundamental * static_cast<double> (p.stiffness) * geometryRatio;
            if (p.tuningMode == 1) hz = tuning.nearestLatticeFrequency (hz, bentFundamental);
            else if (p.tuningMode == 2) hz = tuning.latticeFrequency (bentFundamental, i * p.tuningSpread);
            hz = juce::jlimit (18.0, nyquist, hz);

            auto decaySeconds = static_cast<double> (p.damping) / (1.0 + 0.055 * i);
            auto loadingLoss = 0.12 + 0.9 * static_cast<double> (p.loading * p.coupling);
            auto activeRelief = 0.82 * static_cast<double> (p.activeImpedance);
            decaySeconds *= juce::jlimit (0.18, 5.0, 1.0 / juce::jmax (0.12, loadingLoss - activeRelief));
            decaySeconds = juce::jlimit (0.03, 24.0, decaySeconds);
            const auto radius = std::exp (-1.0 / (decaySeconds * sampleRate));
            const auto omega = juce::MathConstants<double>::twoPi * hz / sampleRate;
            m.a1 = static_cast<float> (2.0 * radius * std::cos (omega));
            m.a2 = static_cast<float> (radius * radius);
            m.pickupL = modeShape (m.nx, m.ny, pickupLX, pickupY);
            m.pickupR = modeShape (m.nx, m.ny, pickupRX, 1.0f - pickupY);
        }
    }

    void triggerGrain (const Parameters& p, float accent) noexcept
    {
        Grain* slot = &grains[0];
        for (auto& candidate : grains)
            if (! candidate.running || candidate.age > slot->age) slot = &candidate;

        const auto motionX = std::sin (trajectoryPhase) * p.trajectoryDepth;
        const auto motionY = std::cos (trajectoryPhase * 0.731f) * p.trajectoryDepth;
        slot->running = true;
        slot->age = 0;
        slot->length = juce::jmax (2, static_cast<int> (sampleRate * p.durationMs * 0.001f
                                         * (0.55f + 0.9f * random.uniform())));
        slot->amplitude = p.force * accent * (0.65f + 0.7f * random.uniform());
        slot->x = juce::jlimit (0.015f, 0.985f, p.contactX + motionX + p.positionSpread * random.bipolar());
        slot->y = juce::jlimit (0.015f, 0.985f, p.contactY + motionY + p.positionSpread * random.bipolar());
        for (int i = 0; i < maxModes; ++i)
        {
            const auto& mode = modes[static_cast<size_t> (i)];
            slot->shapes[static_cast<size_t> (i)] = modeShape (mode.nx, mode.ny, slot->x, slot->y);
        }
    }

    void scheduleNextEvent (float density) noexcept
    {
        const auto safeDensity = juce::jmax (0.05f, density);
        const auto u = juce::jmax (1.0e-6f, random.uniform());
        nextEventSamples = static_cast<float> (-std::log (u) * sampleRate / safeDensity);
    }

    double sampleRate = 48000.0;
    bool active = false, releasing = false, sustainHeld = false;
    int note = -1;
    float velocity = 0.0f;
    double fundamental = 110.0;
    uint64_t startStamp = 0, ageSamples = 0, quietSamples = 0;
    float nextEventSamples = 0.0f, trajectoryPhase = 0.0f, coilCurrent = 0.0f;
    std::array<Mode, maxModes> modes {};
    std::array<Grain, grainSlots> grains {};
    Random random { 0x2f6e2b1u };
};

class Engine
{
public:
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        pitchRatio = 1.0f;
        for (auto& voice : voices) voice.prepare (sampleRate);
        dcX1.fill (0.0f); dcY1.fill (0.0f); clock = 0;
    }

    void reset() noexcept
    {
        for (auto& voice : voices) voice.reset();
        dcX1.fill (0.0f); dcY1.fill (0.0f);
        pitchRatio = 1.0f;
    }

    void noteOn (int note, float velocity, const Parameters& p, const ScalaTuning& tuning) noexcept
    {
        Voice* selected = nullptr;
        for (int i = 0; i < juce::jlimit (1, maxVoices, p.polyphony); ++i)
            if (! voices[static_cast<size_t> (i)].isActive()) { selected = &voices[static_cast<size_t> (i)]; break; }
        if (selected == nullptr)
            selected = &*std::min_element (voices.begin(), voices.begin() + juce::jlimit (1, maxVoices, p.polyphony),
                [] (const Voice& a, const Voice& b) { return a.getStartStamp() < b.getStartStamp(); });
        selected->start (note, velocity, ++clock, tuning, p, pitchRatio);
    }

    void noteOff (int note, bool allowTail = true) noexcept
    {
        for (auto& voice : voices)
            if (voice.isActive() && voice.getNote() == note) voice.stop (allowTail);
    }

    void allNotesOff (bool allowTail) noexcept
    {
        for (auto& voice : voices) voice.stop (allowTail);
    }

    void sustain (bool down) noexcept { for (auto& voice : voices) voice.setSustain (down); }

    void setPitchWheel (int value) noexcept
    {
        const auto normalized = juce::jlimit (-1.0f, 1.0f, (static_cast<float> (value) - 8192.0f) / 8192.0f);
        pitchRatio = std::pow (2.0f, normalized * (2.0f / 12.0f));
    }

    std::array<float, 2> renderSample (const Parameters& p, const ScalaTuning& tuning) noexcept
    {
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < juce::jlimit (1, maxVoices, p.polyphony); ++i)
        {
            auto s = voices[static_cast<size_t> (i)].render (p, tuning, pitchRatio);
            l += s[0]; r += s[1];
        }
        l = dcBlock (l, 0); r = dcBlock (r, 1);
        return { std::tanh (l * p.outputGain), std::tanh (r * p.outputGain) };
    }

private:
    float dcBlock (float x, int channel) noexcept
    {
        const auto y = x - dcX1[static_cast<size_t> (channel)] + 0.995f * dcY1[static_cast<size_t> (channel)];
        dcX1[static_cast<size_t> (channel)] = x;
        dcY1[static_cast<size_t> (channel)] = y;
        return y;
    }

    double sampleRate = 48000.0;
    float pitchRatio = 1.0f;
    uint64_t clock = 0;
    std::array<Voice, maxVoices> voices {};
    std::array<float, 2> dcX1 {}, dcY1 {};
};
}
