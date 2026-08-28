#pragma once

#include <JuceHeader.h>

namespace rmf::ui
{
struct Theme
{
    static juce::Colour canvas()      { return juce::Colour (0xff0b1014); }
    static juce::Colour panel()       { return juce::Colour (0xff121a20); }
    static juce::Colour panelRaised() { return juce::Colour (0xff18232b); }
    static juce::Colour line()        { return juce::Colour (0xff2b3a43); }
    static juce::Colour text()        { return juce::Colour (0xffe8f0f0); }
    static juce::Colour textDim()     { return juce::Colour (0xff91a2a6); }
    static juce::Colour cyan()        { return juce::Colour (0xff63e6df); }
    static juce::Colour amber()       { return juce::Colour (0xffffbd69); }
    static juce::Colour coral()       { return juce::Colour (0xffff776d); }
    static juce::Colour focus()       { return juce::Colour (0xffa7fff9); }
};

class RMFLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    RMFLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool down,
                       int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
};

class Dial final : public juce::Component
{
public:
    Dial (juce::String parameterName, juce::String unitSuffix,
          double minimum, double maximum, double defaultValue,
          int decimalPlaces = 2);

    void resized() override;
    void paint (juce::Graphics&) override;
    void setValue (double value, juce::NotificationType notification = juce::dontSendNotification);
    double getValue() const;
    juce::Slider& slider() noexcept { return control; }

private:
    juce::String name, suffix;
    juce::Slider control;
    int decimals = 2;
};

class SegmentedControl final : public juce::Component
{
public:
    SegmentedControl (juce::String label, juce::StringArray choices);

    void paint (juce::Graphics&) override;
    void resized() override;
    void setSelected (int index);
    int getSelected() const noexcept { return selected; }
    std::function<void (int)> onChange;

private:
    juce::String groupLabel;
    juce::OwnedArray<juce::TextButton> buttons;
    int selected = 0;
};

struct FieldState
{
    float contactX = 0.38f;
    float contactY = 0.58f;
    float spread = 0.34f;
    float energy = 0.52f;
    float roughness = 0.45f;
    float pickupWidth = 0.68f;
    float motionPhase = 0.0f;
};

class MaterialField final : public juce::Component,
                            public juce::TooltipClient,
                            private juce::Timer
{
public:
    MaterialField();

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;
    juce::String getTooltip() override;

    void setState (FieldState newState);
    const FieldState& getState() const noexcept { return state; }
    void setModeFrequencies (std::vector<float> frequenciesHz);
    void setLevel (float newLevel) noexcept;

    std::function<void (float x, float y)> onContactMoved;
    std::function<void (float spread)> onSpreadChanged;

private:
    void timerCallback() override;
    void updateContactFromPoint (juce::Point<float>);
    juce::Rectangle<float> fieldBounds() const;

    FieldState state;
    std::vector<float> modes { 73.4f, 91.6f, 122.8f, 164.2f, 219.7f, 293.7f,
                               392.5f, 524.4f, 700.8f, 936.5f, 1251.8f, 1673.4f };
    float level = 0.0f;
    float smoothedLevel = 0.0f;
};

class TuningCard final : public juce::Component,
                         public juce::FileDragAndDropTarget
{
public:
    TuningCard();

    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override;
    void fileDragExit (const juce::StringArray&) override;

    void setTuningSummary (juce::String scaleName, juce::String period,
                           int degreeCount, juce::String mappingName);
    std::function<void()> onChooseFile;
    std::function<void (const juce::File&)> onFileDropped;

private:
    juce::TextButton loadButton { "LOAD SCL / KBM" };
    juce::String name { "19 equal divisions" };
    juce::String periodText { "2:1 period" };
    juce::String mapping { "Nearest modal mapping" };
    int degrees = 19;
    bool dragOver = false;
};

class LevelPill final : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void setLevel (float left, float right);

private:
    float l = 0.0f, r = 0.0f;
};

/**
    Self-contained, resizable RMF editor shell. It deliberately does not depend on
    a specific AudioProcessorValueTreeState. The host editor can attach directly
    to the public controls or use the callbacks below.
*/
class EditorShell final : public juce::Component
{
public:
    EditorShell();
    ~EditorShell() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    MaterialField field;
    TuningCard tuning;
    Dial density { "DENSITY", " /s", 0.05, 400.0, 18.0, 1 };
    Dial impulse { "IMPULSE", "", 0.0, 1.0, 0.48, 2 };
    Dial roughness { "ROUGHNESS", "", 0.0, 1.0, 0.42, 2 };
    Dial stiffness { "STIFFNESS", "", 0.25, 4.0, 1.0, 2 };
    Dial damping { "DECAY", " s", 0.05, 12.0, 1.8, 2 };
    Dial coupling { "COUPLING", "", 0.0, 1.0, 0.36, 2 };
    Dial drive { "OUTPUT", " dB", -30.0, 6.0, -6.0, 1 };
    Dial width { "PICKUP WIDTH", "", 0.0, 1.0, 0.72, 2 };
    SegmentedControl eventType { "CONTACT", { "Impact", "Scrape", "Slip", "Dust" } };
    SegmentedControl bodyType { "BODY", { "Plate", "Shell", "Beam", "Membrane" } };
    SegmentedControl tuneMode { "MODE MAP", { "Nearest", "Ascending" } };

    juce::ComboBox presetBox;
    juce::TextButton saveButton { "SAVE" };
    juce::ToggleButton powerButton { "ACTIVE" };
    LevelPill outputMeter;

private:
    RMFLookAndFeel lookAndFeel;
    juce::Label brand, subtitle, fieldTitle, bodyTitle, contactTitle, spatialTitle;
    juce::Label compactHint;
    juce::Component headerRule;
};
} // namespace rmf::ui
