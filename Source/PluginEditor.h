#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "RMFEditorKit.h"

class ResonantMatterFieldAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                       private juce::Timer
{
public:
    explicit ResonantMatterFieldAudioProcessorEditor (ResonantMatterFieldAudioProcessor&);
    ~ResonantMatterFieldAudioProcessorEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ResonantMatterFieldAudioProcessor& rmfProcessor;
    rmf::ui::EditorShell shell;
    juce::TooltipWindow tooltips { this, 550 };
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAttachment;

    void timerCallback() override;
    void chooseTuning();
    void loadTuning (const juce::File&);
    void savePreset();
    void setParameter (const char* id, float value);
    int getChoice (const char* id) const;
    void refreshTuningVisuals();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonantMatterFieldAudioProcessorEditor)
};
