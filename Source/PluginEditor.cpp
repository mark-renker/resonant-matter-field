#include "PluginEditor.h"
#include <cmath>

ResonantMatterFieldAudioProcessorEditor::ResonantMatterFieldAudioProcessorEditor (ResonantMatterFieldAudioProcessor& p)
    : AudioProcessorEditor (&p), rmfProcessor (p)
{
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (820, 620, 1680, 1040);
    setSize (1160, 720);
    addAndMakeVisible (shell);

    auto attach = [this] (const char* id, juce::Slider& slider)
    {
        sliderAttachments.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (rmfProcessor.getValueTreeState(), id, slider));
    };
    attach ("contactDensity", shell.density.slider());
    attach ("contactForce", shell.impulse.slider());
    attach ("roughness", shell.roughness.slider());
    attach ("stiffness", shell.stiffness.slider());
    attach ("damping", shell.damping.slider());
    attach ("blCoupling", shell.coupling.slider());
    attach ("outputDb", shell.drive.slider());
    attach ("radiationWidth", shell.width.slider());
    auto oneDecimal = [] (double v) { return juce::String (v, 1); };
    auto twoDecimals = [] (double v) { return juce::String (v, 2); };
    shell.density.slider().textFromValueFunction = oneDecimal;
    shell.impulse.slider().textFromValueFunction = twoDecimals;
    shell.roughness.slider().textFromValueFunction = twoDecimals;
    shell.stiffness.slider().textFromValueFunction = twoDecimals;
    shell.damping.slider().textFromValueFunction = twoDecimals;
    shell.coupling.slider().textFromValueFunction = twoDecimals;
    shell.drive.slider().textFromValueFunction = oneDecimal;
    shell.width.slider().textFromValueFunction = twoDecimals;
    for (auto* slider : std::array<juce::Slider*, 8> { &shell.density.slider(), &shell.impulse.slider(),
            &shell.roughness.slider(), &shell.stiffness.slider(), &shell.damping.slider(),
            &shell.coupling.slider(), &shell.drive.slider(), &shell.width.slider() })
        slider->updateText();
    powerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (rmfProcessor.getValueTreeState(), "enabled", shell.powerButton);

    shell.field.onContactMoved = [this] (float x, float y) { setParameter ("contactX", x); setParameter ("contactY", y); };
    shell.field.onSpreadChanged = [this] (float spread) { setParameter ("positionSpread", spread * 0.9f); };
    shell.eventType.onChange = [this] (int value)
    {
        if (value == 0) { setParameter ("contactDensity", 18.0f); setParameter ("contactDuration", 8.0f); setParameter ("roughness", 0.25f); }
        else if (value == 1) { setParameter ("contactDensity", 180.0f); setParameter ("contactDuration", 34.0f); setParameter ("roughness", 0.86f); }
        else if (value == 2) { setParameter ("contactDensity", 30.0f); setParameter ("contactDuration", 62.0f); setParameter ("roughness", 0.48f); }
        else { setParameter ("contactDensity", 260.0f); setParameter ("contactDuration", 1.4f); setParameter ("roughness", 0.68f); }
    };
    shell.bodyType.onChange = [this] (int value)
    {
        if (value == 0) { setParameter ("stiffness", 1.0f); setParameter ("anisotropy", 0.0f); setParameter ("dispersion", 0.018f); }
        else if (value == 1) { setParameter ("stiffness", 0.78f); setParameter ("anisotropy", 0.25f); setParameter ("dispersion", 0.070f); }
        else if (value == 2) { setParameter ("stiffness", 1.40f); setParameter ("anisotropy", 0.80f); setParameter ("dispersion", 0.035f); }
        else { setParameter ("stiffness", 0.65f); setParameter ("anisotropy", 0.0f); setParameter ("dispersion", 0.0f); }
    };
    shell.tuneMode.onChange = [this] (int value) { setParameter ("tuningMode", (float) (value + 1)); refreshTuningVisuals(); };
    shell.eventType.setSelected (0);
    shell.bodyType.setSelected (0);
    shell.tuneMode.setSelected (juce::jlimit (0, 1, getChoice ("tuningMode") - 1));

    shell.tuning.onChooseFile = [this] { chooseTuning(); };
    shell.tuning.onFileDropped = [this] (const juce::File& file) { loadTuning (file); };
    shell.presetBox.clear();
    juce::StringArray programNames;
    for (int i = 0; i < rmfProcessor.getNumPrograms(); ++i) programNames.add (rmfProcessor.getProgramName (i));
    shell.presetBox.addItemList (programNames, 1);
    shell.presetBox.setSelectedId (1, juce::dontSendNotification);
    shell.presetBox.onChange = [this] { rmfProcessor.setCurrentProgram (shell.presetBox.getSelectedItemIndex()); };
    shell.saveButton.onClick = [this] { savePreset(); };
    shell.saveButton.setTooltip ("Save a portable RMF preset with the Scala tuning embedded.");
    refreshTuningVisuals();
    startTimerHz (30);
}

void ResonantMatterFieldAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll (rmf::ui::Theme::canvas()); }
void ResonantMatterFieldAudioProcessorEditor::resized() { shell.setBounds (getLocalBounds()); }

void ResonantMatterFieldAudioProcessorEditor::setParameter (const char* id, float value)
{
    if (auto* parameter = rmfProcessor.getValueTreeState().getParameter (id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        parameter->endChangeGesture();
    }
}

int ResonantMatterFieldAudioProcessorEditor::getChoice (const char* id) const
{
    if (auto* value = rmfProcessor.getValueTreeState().getRawParameterValue (id)) return juce::roundToInt (value->load());
    return 0;
}

void ResonantMatterFieldAudioProcessorEditor::chooseTuning()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Load a Scala resonance tuning", juce::File(), "*.scl");
    fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& chooser)
        {
            const auto file = chooser.getResult();
            if (file != juce::File()) loadTuning (file);
        });
}

void ResonantMatterFieldAudioProcessorEditor::loadTuning (const juce::File& file)
{
    juce::String error;
    if (file.getFileExtension().toLowerCase() != ".scl")
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Unsupported tuning file",
                                                "RMF modal tuning currently loads Scala .scl files. The previous tuning remains active.");
        return;
    }
    const auto result = rmfProcessor.loadScalaFile (file);
    if (! result.ok)
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not load tuning", result.message);
    else
        refreshTuningVisuals();
}

void ResonantMatterFieldAudioProcessorEditor::savePreset()
{
    fileChooser = std::make_unique<juce::FileChooser> ("Save RMF preset", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("RMF Preset.rmfpreset"), "*.rmfpreset");
    fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
        [this] (const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult(); if (file == juce::File()) return;
            if (! file.hasFileExtension ("rmfpreset")) file = file.withFileExtension ("rmfpreset");
            juce::MemoryBlock data; rmfProcessor.getStateInformation (data);
            if (! file.replaceWithData (data.getData(), data.getSize()))
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Preset not saved", "The selected file could not be written.");
        });
}

void ResonantMatterFieldAudioProcessorEditor::refreshTuningVisuals()
{
    const auto* t = rmfProcessor.getTuning();
    if (t == nullptr) return;
    shell.tuning.setTuningSummary (t->getName(),
                                   juce::String (t->getPeriod(), t->getPeriod() == std::floor (t->getPeriod()) ? 0 : 3) + ":1 period  |  A4 440 Hz",
                                   t->getDegreeCount(),
                                   getChoice ("tuningMode") == 1 ? "Nearest modal mapping" : "Ascending degree mapping");
    std::vector<float> modes;
    modes.reserve (12);
    const int spread = juce::jlimit (1, 8, juce::roundToInt (rmfProcessor.getValueTreeState().getRawParameterValue ("tuningSpread")->load()));
    for (int i = 0; i < 12; ++i) modes.push_back ((float) t->latticeFrequency (55.0, i * spread));
    shell.field.setModeFrequencies (std::move (modes));
}

void ResonantMatterFieldAudioProcessorEditor::timerCallback()
{
    auto state = shell.field.getState();
    state.contactX = rmfProcessor.visualX.load();
    state.contactY = rmfProcessor.visualY.load();
    state.spread = rmfProcessor.getValueTreeState().getRawParameterValue ("positionSpread")->load() / 0.9f;
    state.roughness = rmfProcessor.getValueTreeState().getRawParameterValue ("roughness")->load();
    state.pickupWidth = rmfProcessor.getValueTreeState().getRawParameterValue ("radiationWidth")->load();
    state.energy = juce::jlimit (0.0f, 1.0f, rmfProcessor.visualEnergy.load() * 4.0f);
    shell.field.setState (state);
    shell.field.setLevel (state.energy);
    shell.outputMeter.setLevel (rmfProcessor.meterL.load(), rmfProcessor.meterR.load());
}
