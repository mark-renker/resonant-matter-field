#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace IDs
{
static constexpr auto density = "contactDensity";
static constexpr auto duration = "contactDuration";
static constexpr auto force = "contactForce";
static constexpr auto roughness = "roughness";
static constexpr auto x = "contactX";
static constexpr auto y = "contactY";
static constexpr auto spread = "positionSpread";
static constexpr auto trajectoryDepth = "trajectoryDepth";
static constexpr auto trajectoryRate = "trajectoryRate";
static constexpr auto stiffness = "stiffness";
static constexpr auto anisotropy = "anisotropy";
static constexpr auto dispersion = "dispersion";
static constexpr auto damping = "damping";
static constexpr auto modes = "modeCount";
static constexpr auto coupling = "blCoupling";
static constexpr auto loading = "electricalLoading";
static constexpr auto inductance = "inductance";
static constexpr auto active = "activeImpedance";
static constexpr auto width = "radiationWidth";
static constexpr auto tuningMode = "tuningMode";
static constexpr auto tuningSpread = "tuningSpread";
static constexpr auto transpose = "transpose";
static constexpr auto polyphony = "polyphony";
static constexpr auto output = "outputDb";
static constexpr auto enabled = "enabled";
}

namespace
{
using APF = juce::AudioParameterFloat;
using API = juce::AudioParameterInt;
using APC = juce::AudioParameterChoice;
using APB = juce::AudioParameterBool;

juce::NormalisableRange<float> skewed (float min, float max, float centre)
{
    juce::NormalisableRange<float> range (min, max);
    range.setSkewForCentre (centre);
    return range;
}

float value (const juce::AudioProcessorValueTreeState& state, const char* id) noexcept
{
    return state.getRawParameterValue (id)->load (std::memory_order_relaxed);
}

struct Preset
{
    const char* name;
    std::initializer_list<std::pair<const char*, float>> values;
};

const std::array<Preset, 24> presets {{
    { "Kinetic Plate", {{IDs::density,18},{IDs::duration,8},{IDs::force,.55f},{IDs::roughness,.25f},{IDs::stiffness,1},{IDs::damping,1.8f},{IDs::active,0},{IDs::tuningMode,1},{IDs::width,.7f}} },
    { "Sparse 19-EDO Metal", {{IDs::density,2.8f},{IDs::duration,3.2f},{IDs::force,.82f},{IDs::roughness,.12f},{IDs::dispersion,.012f},{IDs::damping,5.4f},{IDs::tuningMode,2},{IDs::tuningSpread,2},{IDs::width,.92f}} },
    { "Friction Cloud", {{IDs::density,180},{IDs::duration,34},{IDs::force,.21f},{IDs::roughness,.86f},{IDs::spread,.42f},{IDs::trajectoryDepth,.38f},{IDs::trajectoryRate,.5f},{IDs::damping,.72f}} },
    { "Bohlen Pierce Constellation", {{IDs::density,7.5f},{IDs::duration,11},{IDs::force,.61f},{IDs::roughness,.31f},{IDs::tuningMode,2},{IDs::tuningSpread,1},{IDs::dispersion,0},{IDs::damping,3.2f},{IDs::width,.84f}} },
    { "Active Coil Bloom", {{IDs::density,11},{IDs::duration,14},{IDs::force,.37f},{IDs::coupling,.78f},{IDs::loading,.18f},{IDs::inductance,.64f},{IDs::active,.67f},{IDs::damping,6.5f},{IDs::output,-8}} },
    { "Ceramic Rain", {{IDs::density,48},{IDs::duration,1.4f},{IDs::force,.68f},{IDs::roughness,.55f},{IDs::spread,.7f},{IDs::anisotropy,.38f},{IDs::dispersion,.05f},{IDs::damping,1.1f},{IDs::modes,30}} },
    { "Bone Chime", {{IDs::density,4.5f},{IDs::duration,5.5f},{IDs::force,.72f},{IDs::roughness,.18f},{IDs::stiffness,1.55f},{IDs::anisotropy,-.28f},{IDs::damping,4.2f},{IDs::width,.62f}} },
    { "Hollow Beam", {{IDs::density,9},{IDs::duration,16},{IDs::force,.48f},{IDs::roughness,.22f},{IDs::stiffness,.62f},{IDs::anisotropy,.76f},{IDs::dispersion,.08f},{IDs::damping,2.7f},{IDs::modes,18}} },
    { "Shell Drops", {{IDs::density,13},{IDs::duration,7},{IDs::force,.63f},{IDs::roughness,.36f},{IDs::stiffness,.82f},{IDs::anisotropy,.22f},{IDs::dispersion,.11f},{IDs::damping,1.6f},{IDs::width,.88f}} },
    { "Adhesive Slip", {{IDs::density,32},{IDs::duration,62},{IDs::force,.31f},{IDs::roughness,.47f},{IDs::spread,.22f},{IDs::trajectoryDepth,.31f},{IDs::trajectoryRate,.12f},{IDs::damping,1.3f}} },
    { "Graphite Drag", {{IDs::density,240},{IDs::duration,18},{IDs::force,.16f},{IDs::roughness,.93f},{IDs::spread,.12f},{IDs::trajectoryDepth,.45f},{IDs::trajectoryRate,.74f},{IDs::damping,.48f},{IDs::width,.58f}} },
    { "Velvet Current", {{IDs::density,110},{IDs::duration,72},{IDs::force,.12f},{IDs::roughness,.58f},{IDs::coupling,.52f},{IDs::loading,.72f},{IDs::inductance,.78f},{IDs::damping,2.2f},{IDs::output,-4}} },
    { "Tritave Dust", {{IDs::density,310},{IDs::duration,.9f},{IDs::force,.25f},{IDs::roughness,.82f},{IDs::spread,.78f},{IDs::tuningMode,2},{IDs::tuningSpread,3},{IDs::damping,.62f},{IDs::modes,32}} },
    { "Ferrofluid Sand", {{IDs::density,205},{IDs::duration,2.2f},{IDs::force,.34f},{IDs::roughness,.74f},{IDs::coupling,.66f},{IDs::loading,.32f},{IDs::inductance,.55f},{IDs::active,.21f},{IDs::damping,1.4f}} },
    { "Static Flakes", {{IDs::density,390},{IDs::duration,.35f},{IDs::force,.18f},{IDs::roughness,1.0f},{IDs::spread,.85f},{IDs::dispersion,.13f},{IDs::damping,.24f},{IDs::width,1.0f},{IDs::output,-2}} },
    { "Cold Ash", {{IDs::density,150},{IDs::duration,4.5f},{IDs::force,.23f},{IDs::roughness,.69f},{IDs::stiffness,.42f},{IDs::loading,.88f},{IDs::inductance,.82f},{IDs::damping,.55f},{IDs::width,.35f}} },
    { "Near Instability", {{IDs::density,6},{IDs::duration,28},{IDs::force,.27f},{IDs::coupling,.91f},{IDs::loading,.08f},{IDs::inductance,.42f},{IDs::active,.76f},{IDs::damping,8.2f},{IDs::output,-12}} },
    { "Magnetic Breather", {{IDs::density,22},{IDs::duration,46},{IDs::force,.22f},{IDs::roughness,.34f},{IDs::trajectoryDepth,.18f},{IDs::trajectoryRate,.07f},{IDs::coupling,.82f},{IDs::active,.54f},{IDs::damping,5.1f}} },
    { "Voltage Flower", {{IDs::density,15},{IDs::duration,21},{IDs::force,.36f},{IDs::coupling,.72f},{IDs::loading,.14f},{IDs::inductance,.22f},{IDs::active,.62f},{IDs::tuningMode,2},{IDs::tuningSpread,4},{IDs::width,.9f}} },
    { "Nodal Orbit", {{IDs::density,24},{IDs::duration,9},{IDs::force,.52f},{IDs::x,.18f},{IDs::y,.32f},{IDs::trajectoryDepth,.46f},{IDs::trajectoryRate,.23f},{IDs::spread,.08f},{IDs::width,.86f}} },
    { "Edge Pickups", {{IDs::density,8},{IDs::duration,6},{IDs::force,.7f},{IDs::x,.12f},{IDs::y,.78f},{IDs::spread,.18f},{IDs::width,1.0f},{IDs::damping,3.8f},{IDs::anisotropy,.44f}} },
    { "Center Null", {{IDs::density,38},{IDs::duration,12},{IDs::force,.43f},{IDs::x,.5f},{IDs::y,.5f},{IDs::trajectoryDepth,.34f},{IDs::trajectoryRate,.41f},{IDs::width,.72f},{IDs::modes,28}} },
    { "Wide Fracture", {{IDs::density,3.1f},{IDs::duration,2.1f},{IDs::force,.93f},{IDs::roughness,.08f},{IDs::spread,.64f},{IDs::dispersion,.16f},{IDs::damping,7.2f},{IDs::width,1.0f},{IDs::output,-10}} },
    { "Just Bronze", {{IDs::density,10},{IDs::duration,8},{IDs::force,.58f},{IDs::roughness,.2f},{IDs::tuningMode,2},{IDs::tuningSpread,2},{IDs::stiffness,.88f},{IDs::damping,4.6f},{IDs::width,.8f}} }
}};
}

ResonantMatterFieldAudioProcessor::ResonantMatterFieldAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "RMFState", createParameterLayout())
{
    tuningHistory.push_back (std::make_unique<const rmf::ScalaTuning>());
    tuning.store (tuningHistory.back().get(), std::memory_order_release);
}

juce::AudioProcessorValueTreeState::ParameterLayout ResonantMatterFieldAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto addF = [&p] (const char* id, const char* name, juce::NormalisableRange<float> range, float initial, const char* suffix = "")
    {
        p.add (std::make_unique<APF> (juce::ParameterID { id, 1 }, name, range, initial,
                                      juce::AudioParameterFloatAttributes().withLabel (suffix)));
    };

    addF (IDs::density, "Contact Density", skewed (0.05f, 400.0f, 24.0f), 18.0f, " /s");
    addF (IDs::duration, "Contact Duration", skewed (0.25f, 100.0f, 8.0f), 8.0f, " ms");
    addF (IDs::force, "Contact Force", { 0.0f, 1.0f, 0.001f }, 0.55f);
    addF (IDs::roughness, "Roughness", { 0.0f, 1.0f, 0.001f }, 0.25f);
    addF (IDs::x, "Contact X", { 0.0f, 1.0f, 0.001f }, 0.37f);
    addF (IDs::y, "Contact Y", { 0.0f, 1.0f, 0.001f }, 0.61f);
    addF (IDs::spread, "Position Spread", { 0.0f, 0.9f, 0.001f }, 0.15f);
    addF (IDs::trajectoryDepth, "Trajectory Depth", { 0.0f, 0.48f, 0.001f }, 0.20f);
    addF (IDs::trajectoryRate, "Trajectory Rate", skewed (0.01f, 12.0f, 0.3f), 0.17f, " Hz");
    addF (IDs::stiffness, "Stiffness", skewed (0.25f, 4.0f, 1.0f), 1.0f);
    addF (IDs::anisotropy, "Anisotropy", { -0.95f, 0.95f, 0.001f }, 0.0f);
    addF (IDs::dispersion, "Dispersion", skewed (0.0f, 0.22f, 0.025f), 0.018f);
    addF (IDs::damping, "Decay", skewed (0.05f, 12.0f, 1.8f), 1.8f, " s");
    p.add (std::make_unique<API> (juce::ParameterID { IDs::modes, 1 }, "Mode Count", 8, rmf::maxModes, 24));
    addF (IDs::coupling, "BL Coupling", { 0.0f, 1.0f, 0.001f }, 0.35f);
    addF (IDs::loading, "Electrical Loading", { 0.0f, 1.0f, 0.001f }, 0.42f);
    addF (IDs::inductance, "Inductance", { 0.0f, 1.0f, 0.001f }, 0.25f);
    addF (IDs::active, "Active Impedance", { -0.8f, 0.8f, 0.001f }, 0.0f);
    addF (IDs::width, "Radiation Width", { 0.0f, 1.0f, 0.001f }, 0.7f);
    p.add (std::make_unique<APC> (juce::ParameterID { IDs::tuningMode, 1 }, "Resonance Mapping",
                                  juce::StringArray { "Natural", "Nearest", "Ascending" }, 1));
    p.add (std::make_unique<API> (juce::ParameterID { IDs::tuningSpread, 1 }, "Scale Degree Spread", 1, 8, 1));
    p.add (std::make_unique<API> (juce::ParameterID { IDs::transpose, 1 }, "Transpose", -36, 36, 0));
    p.add (std::make_unique<API> (juce::ParameterID { IDs::polyphony, 1 }, "Polyphony", 1, rmf::maxVoices, 8));
    addF (IDs::output, "Output", { -30.0f, 6.0f, 0.1f }, -6.0f, " dB");
    p.add (std::make_unique<APB> (juce::ParameterID { IDs::enabled, 1 }, "Active", true));
    return p;
}

void ResonantMatterFieldAudioProcessor::Smoothers::reset (double sr)
{
    for (auto* s : std::array<Smooth<float>*, 19> { &density,&duration,&force,&roughness,&x,&y,&spread,&trajectoryDepth,
        &trajectoryRate,&stiffness,&anisotropy,&dispersion,&damping,&coupling,&loading,&inductance,&active,&width,&output })
        s->reset (sr, 0.025);
}

void ResonantMatterFieldAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    modWheel = 0.0f;
    channelPressure = 0.0f;
    smooth.reset (sampleRate);
    initialiseSmoothers (parameterTargets());
    smoothersInitialised = true;
}

bool ResonantMatterFieldAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet().isDisabled();
}

rmf::Parameters ResonantMatterFieldAudioProcessor::parameterTargets() const noexcept
{
    rmf::Parameters p;
    p.density = value (state, IDs::density); p.durationMs = value (state, IDs::duration);
    p.force = value (state, IDs::force); p.roughness = value (state, IDs::roughness);
    p.contactX = value (state, IDs::x); p.contactY = value (state, IDs::y);
    p.positionSpread = value (state, IDs::spread); p.trajectoryDepth = value (state, IDs::trajectoryDepth);
    p.trajectoryRate = value (state, IDs::trajectoryRate); p.stiffness = value (state, IDs::stiffness);
    p.anisotropy = value (state, IDs::anisotropy); p.dispersion = value (state, IDs::dispersion);
    p.damping = value (state, IDs::damping); p.modeCount = static_cast<int> (value (state, IDs::modes));
    p.coupling = value (state, IDs::coupling); p.loading = value (state, IDs::loading);
    p.inductance = value (state, IDs::inductance); p.activeImpedance = value (state, IDs::active);
    p.radiationWidth = value (state, IDs::width); p.tuningMode = static_cast<int> (value (state, IDs::tuningMode));
    p.tuningSpread = static_cast<int> (value (state, IDs::tuningSpread)); p.transpose = static_cast<int> (value (state, IDs::transpose));
    p.polyphony = static_cast<int> (value (state, IDs::polyphony));
    p.outputGain = juce::Decibels::decibelsToGain (value (state, IDs::output));
    return p;
}

void ResonantMatterFieldAudioProcessor::setSmootherTargets (const rmf::Parameters& p) noexcept
{
    smooth.density.setTargetValue (p.density); smooth.duration.setTargetValue (p.durationMs);
    smooth.force.setTargetValue (p.force); smooth.roughness.setTargetValue (p.roughness);
    smooth.x.setTargetValue (p.contactX); smooth.y.setTargetValue (p.contactY);
    smooth.spread.setTargetValue (p.positionSpread); smooth.trajectoryDepth.setTargetValue (p.trajectoryDepth);
    smooth.trajectoryRate.setTargetValue (p.trajectoryRate); smooth.stiffness.setTargetValue (p.stiffness);
    smooth.anisotropy.setTargetValue (p.anisotropy); smooth.dispersion.setTargetValue (p.dispersion);
    smooth.damping.setTargetValue (p.damping); smooth.coupling.setTargetValue (p.coupling);
    smooth.loading.setTargetValue (p.loading); smooth.inductance.setTargetValue (p.inductance);
    smooth.active.setTargetValue (p.activeImpedance); smooth.width.setTargetValue (p.radiationWidth);
    smooth.output.setTargetValue (p.outputGain);
}

void ResonantMatterFieldAudioProcessor::initialiseSmoothers (const rmf::Parameters& p) noexcept
{
    setSmootherTargets (p);
    for (auto* s : std::array<Smooth<float>*, 19> { &smooth.density,&smooth.duration,&smooth.force,&smooth.roughness,&smooth.x,&smooth.y,&smooth.spread,&smooth.trajectoryDepth,
        &smooth.trajectoryRate,&smooth.stiffness,&smooth.anisotropy,&smooth.dispersion,&smooth.damping,&smooth.coupling,&smooth.loading,&smooth.inductance,&smooth.active,&smooth.width,&smooth.output })
        s->setCurrentAndTargetValue (s->getTargetValue());
}

rmf::Parameters ResonantMatterFieldAudioProcessor::nextSmoothedParameters (const rmf::Parameters& discrete) noexcept
{
    auto p = discrete;
    p.density=smooth.density.getNextValue(); p.durationMs=smooth.duration.getNextValue(); p.force=smooth.force.getNextValue();
    p.roughness=smooth.roughness.getNextValue(); p.contactX=smooth.x.getNextValue(); p.contactY=smooth.y.getNextValue();
    p.positionSpread=smooth.spread.getNextValue(); p.trajectoryDepth=smooth.trajectoryDepth.getNextValue();
    p.trajectoryRate=smooth.trajectoryRate.getNextValue(); p.stiffness=smooth.stiffness.getNextValue();
    p.anisotropy=smooth.anisotropy.getNextValue(); p.dispersion=smooth.dispersion.getNextValue(); p.damping=smooth.damping.getNextValue();
    p.coupling=smooth.coupling.getNextValue(); p.loading=smooth.loading.getNextValue(); p.inductance=smooth.inductance.getNextValue();
    p.activeImpedance=smooth.active.getNextValue(); p.radiationWidth=smooth.width.getNextValue(); p.outputGain=smooth.output.getNextValue();
    return p;
}

void ResonantMatterFieldAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    auto target = parameterTargets();
    if (! smoothersInitialised) initialiseSmoothers (target);
    setSmootherTargets (target);
    auto* currentTuning = tuning.load (std::memory_order_acquire);
    jassert (currentTuning != nullptr);

    auto event = midi.cbegin();
    const auto eventEnd = midi.cend();
    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    const bool enabled = value (state, IDs::enabled) >= 0.5f;
    float peakL = 0.0f, peakR = 0.0f, sumSquares = 0.0f;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        while (event != eventEnd && (*event).samplePosition <= sample)
        {
            const auto message = (*event).getMessage();
            if (message.isNoteOn()) engine.noteOn (message.getNoteNumber(), message.getFloatVelocity(), target, *currentTuning);
            else if (message.isNoteOff()) engine.noteOff (message.getNoteNumber());
            else if (message.isAllNotesOff() || message.isAllSoundOff()) engine.allNotesOff (! message.isAllSoundOff());
            else if (message.isSustainPedalOn()) engine.sustain (true);
            else if (message.isSustainPedalOff()) engine.sustain (false);
            else if (message.isPitchWheel()) engine.setPitchWheel (message.getPitchWheelValue());
            else if (message.isController() && message.getControllerNumber() == 1)
                modWheel = static_cast<float> (message.getControllerValue()) / 127.0f;
            else if (message.isChannelPressure())
                channelPressure = static_cast<float> (message.getChannelPressureValue()) / 127.0f;
            else if (message.isAftertouch())
                channelPressure = static_cast<float> (message.getAfterTouchValue()) / 127.0f;
            ++event;
        }
        auto p = nextSmoothedParameters (target);
        // A compact performance layer that stays inside safe parameter ranges:
        // wheel increases texture/motion, pressure increases force/coupling.
        p.roughness = juce::jlimit (0.0f, 1.0f, p.roughness + 0.45f * modWheel);
        p.trajectoryDepth = juce::jlimit (0.0f, 0.48f, p.trajectoryDepth + 0.24f * modWheel);
        p.force = juce::jlimit (0.0f, 1.0f, p.force + 0.35f * channelPressure);
        p.coupling = juce::jlimit (0.0f, 1.0f, p.coupling + 0.25f * channelPressure);
        const auto output = engine.renderSample (p, *currentTuning);
        left[sample] = enabled ? output[0] : 0.0f;
        right[sample] = enabled ? output[1] : 0.0f;
        peakL = juce::jmax (peakL, std::abs (left[sample]));
        peakR = juce::jmax (peakR, std::abs (right[sample]));
        sumSquares += 0.5f * (left[sample] * left[sample] + right[sample] * right[sample]);
    }
    visualX.store (target.contactX, std::memory_order_relaxed);
    visualY.store (target.contactY, std::memory_order_relaxed);
    visualEnergy.store (std::sqrt (sumSquares / (float) juce::jmax (1, buffer.getNumSamples())), std::memory_order_relaxed);
    meterL.store (juce::jmax (peakL, meterL.load() * 0.86f), std::memory_order_relaxed);
    meterR.store (juce::jmax (peakR, meterR.load() * 0.86f), std::memory_order_relaxed);
}

int ResonantMatterFieldAudioProcessor::getNumPrograms() { return static_cast<int> (presets.size()); }
const juce::String ResonantMatterFieldAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, getNumPrograms()) ? presets[static_cast<size_t> (index)].name : juce::String();
}

void ResonantMatterFieldAudioProcessor::setParameterActual (const juce::String& id, float actual)
{
    if (auto* parameter = state.getParameter (id))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (actual));
}

void ResonantMatterFieldAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms())) return;
    currentProgram = index;
    for (const auto& item : presets[static_cast<size_t> (index)].values) setParameterActual (item.first, item.second);
}

rmf::ScalaTuning::Result ResonantMatterFieldAudioProcessor::loadScalaFile (const juce::File& file)
{
    auto candidate = std::make_unique<rmf::ScalaTuning>();
    auto result = candidate->loadFromFile (file);
    if (result.ok)
    {
        auto* published = candidate.get();
        tuningHistory.push_back (std::move (candidate));
        tuning.store (published, std::memory_order_release);
        const juce::ScopedLock lock (tuningMetadataLock);
        tuningPath = file.getFullPathName();
    }
    return result;
}

void ResonantMatterFieldAudioProcessor::resetTuning()
{
    auto candidate = std::make_unique<const rmf::ScalaTuning>();
    auto* published = candidate.get();
    tuningHistory.push_back (std::move (candidate));
    tuning.store (published, std::memory_order_release);
    const juce::ScopedLock lock (tuningMetadataLock);
    tuningPath.clear();
}

juce::String ResonantMatterFieldAudioProcessor::getTuningName() const
{
    if (auto* loaded = tuning.load (std::memory_order_acquire)) return loaded->getName();
    return "12-tone equal temperament";
}

void ResonantMatterFieldAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    auto tree = state.copyState();
    if (auto* loaded = tuning.load (std::memory_order_acquire))
    {
        tree.setProperty ("scalaName", loaded->getName(), nullptr);
        tree.setProperty ("scalaText", loaded->getSourceText(), nullptr);
    }
    {
        const juce::ScopedLock lock (tuningMetadataLock);
        tree.setProperty ("scalaPath", tuningPath, nullptr);
    }
    tree.setProperty ("factoryProgram", currentProgram, nullptr);
    if (auto xml = tree.createXml()) copyXmlToBinary (*xml, destination);
}

void ResonantMatterFieldAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid() && tree.hasType (state.state.getType()))
        {
            auto scalaText = tree.getProperty ("scalaText").toString();
            auto scalaName = tree.getProperty ("scalaName").toString();
            auto candidate = std::make_unique<rmf::ScalaTuning>();
            if (scalaText.isNotEmpty() && candidate->loadFromString (scalaText, scalaName).ok)
            {
                auto* published = candidate.get();
                tuningHistory.push_back (std::move (candidate));
                tuning.store (published, std::memory_order_release);
            }
            {
                const juce::ScopedLock lock (tuningMetadataLock);
                tuningPath = tree.getProperty ("scalaPath").toString();
            }
            currentProgram = juce::jlimit (0, getNumPrograms() - 1, static_cast<int> (tree.getProperty ("factoryProgram", 0)));
            tree.removeProperty ("scalaName", nullptr); tree.removeProperty ("scalaText", nullptr);
            tree.removeProperty ("scalaPath", nullptr); tree.removeProperty ("factoryProgram", nullptr);
            state.replaceState (tree);
        }
    }
}

juce::AudioProcessorEditor* ResonantMatterFieldAudioProcessor::createEditor()
{
    return new ResonantMatterFieldAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ResonantMatterFieldAudioProcessor();
}
