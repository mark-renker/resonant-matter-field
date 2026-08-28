#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include "DSP/RMFEngine.h"

class ResonantMatterFieldAudioProcessor final : public juce::AudioProcessor
{
public:
    ResonantMatterFieldAudioProcessor();
    ~ResonantMatterFieldAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 24.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return state; }
    const juce::AudioProcessorValueTreeState& getValueTreeState() const noexcept { return state; }

    rmf::ScalaTuning::Result loadScalaFile (const juce::File&);
    juce::String getTuningName() const;
    const rmf::ScalaTuning* getTuning() const noexcept { return tuning.load (std::memory_order_acquire); }
    void resetTuning();

    std::atomic<float> visualX { 0.37f }, visualY { 0.61f }, visualEnergy { 0.0f };
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f };

private:
    template <typename T> using Smooth = juce::SmoothedValue<T, juce::ValueSmoothingTypes::Linear>;
    struct Smoothers
    {
        Smooth<float> density, duration, force, roughness, x, y, spread, trajectoryDepth,
                      trajectoryRate, stiffness, anisotropy, dispersion, damping, coupling,
                      loading, inductance, active, width, output;
        void reset (double sampleRate);
    } smooth;

    rmf::Parameters parameterTargets() const noexcept;
    rmf::Parameters nextSmoothedParameters (const rmf::Parameters&) noexcept;
    void setSmootherTargets (const rmf::Parameters&) noexcept;
    void initialiseSmoothers (const rmf::Parameters&) noexcept;
    void setParameterActual (const juce::String& id, float value);

    juce::AudioProcessorValueTreeState state;
    rmf::Engine engine;
    // Tuning objects are immutable once published. Retaining every loaded table
    // for this processor's lifetime makes the audio-thread handoff a single
    // lock-free raw-pointer load, with no refcount destruction on the callback.
    std::atomic<const rmf::ScalaTuning*> tuning { nullptr };
    std::vector<std::unique_ptr<const rmf::ScalaTuning>> tuningHistory;
    juce::String tuningPath;
    mutable juce::CriticalSection tuningMetadataLock;
    int currentProgram = 0;
    bool smoothersInitialised = false;
    float modWheel = 0.0f;
    float channelPressure = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonantMatterFieldAudioProcessor)
};
