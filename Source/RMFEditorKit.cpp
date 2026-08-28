#include "RMFEditorKit.h"

namespace rmf::ui
{
namespace
{
constexpr float pi = juce::MathConstants<float>::pi;

void drawRoundedPanel (juce::Graphics& g, juce::Rectangle<float> bounds,
                       juce::Colour fill = Theme::panel())
{
    g.setColour (fill);
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (Theme::line().withAlpha (0.8f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.0f);
}

void configureLabel (juce::Label& label, float size, juce::Colour colour,
                     juce::Justification justification = juce::Justification::centredLeft)
{
    label.setFont (juce::FontOptions (size, juce::Font::plain));
    label.setColour (juce::Label::textColourId, colour);
    label.setJustificationType (justification);
    label.setInterceptsMouseClicks (false, false);
}
}

RMFLookAndFeel::RMFLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, Theme::text());
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::textColourId, Theme::text());
    setColour (juce::ComboBox::backgroundColourId, Theme::panelRaised());
    setColour (juce::ComboBox::outlineColourId, Theme::line());
    setColour (juce::ComboBox::arrowColourId, Theme::cyan());
    setColour (juce::PopupMenu::backgroundColourId, Theme::panelRaised());
    setColour (juce::PopupMenu::textColourId, Theme::text());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::cyan().withAlpha (0.20f));
    setColour (juce::PopupMenu::highlightedTextColourId, Theme::text());
    setColour (juce::TextButton::textColourOffId, Theme::textDim());
    setColour (juce::TextButton::textColourOnId, Theme::canvas());
    setColour (juce::ToggleButton::textColourId, Theme::textDim());
}

void RMFLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float pos, float startAngle, float endAngle,
                                       juce::Slider& slider)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height)
                 .reduced (juce::jmin ((float) width, (float) height) * 0.13f);
    const auto radius = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto centre = b.getCentre();
    const auto angle = startAngle + pos * (endAngle - startAngle);
    const auto stroke = juce::jmax (2.5f, radius * 0.095f);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                         startAngle, endAngle, true);
    g.setColour (Theme::line());
    g.strokePath (track, juce::PathStrokeType (stroke, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                         startAngle, angle, true);
    auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    if (accent.isTransparent())
        accent = Theme::cyan();
    g.setGradientFill (juce::ColourGradient (accent.darker (0.15f), b.getBottomLeft(),
                                              accent.brighter (0.10f), b.getTopRight(), false));
    g.strokePath (value, juce::PathStrokeType (stroke, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    g.setColour (Theme::panelRaised().brighter (0.04f));
    g.fillEllipse (b.reduced (stroke * 1.6f));
    g.setColour (Theme::line().brighter (0.16f));
    g.drawEllipse (b.reduced (stroke * 1.6f), 1.0f);

    const auto pointerLength = radius * 0.46f;
    const auto pointer = centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * pointerLength;
    g.setColour (accent);
    g.drawLine ({ centre, pointer }, juce::jmax (2.0f, radius * 0.075f));
    g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (centre));
}

void RMFLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                           const juce::Colour&, bool highlighted, bool down)
{
    const auto active = button.getToggleState();
    auto fill = active ? Theme::cyan() : Theme::panelRaised();
    if (down) fill = fill.darker (0.18f);
    else if (highlighted) fill = fill.brighter (0.08f);
    g.setColour (fill);
    g.fillRoundedRectangle (button.getLocalBounds().toFloat(), 7.0f);
    g.setColour (active ? Theme::cyan().brighter (0.35f) : Theme::line());
    g.drawRoundedRectangle (button.getLocalBounds().toFloat().reduced (0.5f), 7.0f, 1.0f);
}

void RMFLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                   int, int, int, int, juce::ComboBox&)
{
    auto b = juce::Rectangle<float> (0, 0, (float) width, (float) height);
    g.setColour (Theme::panelRaised());
    g.fillRoundedRectangle (b, 7.0f);
    g.setColour (Theme::line());
    g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.0f);
    juce::Path arrow;
    const auto cx = (float) width - 17.0f;
    const auto cy = (float) height * 0.5f;
    arrow.startNewSubPath (cx - 4.0f, cy - 2.0f);
    arrow.lineTo (cx, cy + 2.0f);
    arrow.lineTo (cx + 4.0f, cy - 2.0f);
    g.setColour (Theme::cyan());
    g.strokePath (arrow, juce::PathStrokeType (1.6f));
}

juce::Font RMFLookAndFeel::getComboBoxFont (juce::ComboBox&) { return juce::FontOptions (13.0f); }
juce::Font RMFLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return juce::FontOptions (11.5f, juce::Font::bold); }

Dial::Dial (juce::String parameterName, juce::String unitSuffix,
            double minimum, double maximum, double defaultValue, int decimalPlaces)
    : name (std::move (parameterName)), suffix (std::move (unitSuffix)), decimals (decimalPlaces)
{
    control.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    control.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 20);
    control.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    control.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    control.setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    control.setRange (minimum, maximum);
    control.setValue (defaultValue);
    control.setNumDecimalPlacesToDisplay (decimals);
    control.setTextValueSuffix (suffix);
    control.setDoubleClickReturnValue (true, defaultValue);
    control.setMouseDragSensitivity (160);
    control.setVelocityBasedMode (true);
    control.setVelocityModeParameters (0.72, 1, 0.09, true);
    control.setColour (juce::Slider::rotarySliderFillColourId, Theme::cyan());
    control.setTooltip (name.toLowerCase().replaceCharacter ('_', ' '));
    addAndMakeVisible (control);
}

void Dial::resized() { control.setBounds (getLocalBounds().withTrimmedTop (15)); }

void Dial::paint (juce::Graphics& g)
{
    g.setColour (Theme::textDim());
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawFittedText (name, getLocalBounds().removeFromTop (17), juce::Justification::centred, 1);
}

void Dial::setValue (double value, juce::NotificationType notification) { control.setValue (value, notification); }
double Dial::getValue() const { return control.getValue(); }

SegmentedControl::SegmentedControl (juce::String label, juce::StringArray choices)
    : groupLabel (std::move (label))
{
    for (int i = 0; i < choices.size(); ++i)
    {
        auto* button = buttons.add (new juce::TextButton (choices[i]));
        button->setClickingTogglesState (false);
        button->setTooltip ("Select " + choices[i].toLowerCase() + " " + groupLabel.toLowerCase());
        button->onClick = [this, i] { setSelected (i); };
        addAndMakeVisible (button);
    }
    setSelected (0);
}

void SegmentedControl::paint (juce::Graphics& g)
{
    g.setColour (Theme::textDim());
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText (groupLabel, getLocalBounds().removeFromTop (16), juce::Justification::centredLeft);
}

void SegmentedControl::resized()
{
    auto r = getLocalBounds().withTrimmedTop (19);
    const auto w = r.getWidth() / juce::jmax (1, buttons.size());
    for (auto* button : buttons)
        button->setBounds (r.removeFromLeft (w).reduced (1, 0));
}

void SegmentedControl::setSelected (int index)
{
    selected = juce::jlimit (0, buttons.size() - 1, index);
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState (i == selected, juce::dontSendNotification);
    if (onChange) onChange (selected);
}

MaterialField::MaterialField()
{
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
    startTimerHz (30);
}

juce::Rectangle<float> MaterialField::fieldBounds() const
{
    return getLocalBounds().toFloat().reduced (18.0f).withTrimmedTop (24.0f).withTrimmedBottom (22.0f);
}

void MaterialField::paint (juce::Graphics& g)
{
    auto outer = getLocalBounds().toFloat();
    drawRoundedPanel (g, outer, Theme::panel());
    auto b = fieldBounds();

    juce::ColourGradient background (juce::Colour (0xff1b2c31), b.getTopLeft(),
                                     juce::Colour (0xff0d1418), b.getBottomRight(), false);
    background.addColour (0.46, juce::Colour (0xff102026));
    g.setGradientFill (background);
    g.fillRoundedRectangle (b, 10.0f);

    g.saveState();
    g.reduceClipRegion (b.toNearestInt());

    for (int i = 1; i < 8; ++i)
    {
        const auto t = (float) i / 8.0f;
        g.setColour (Theme::line().withAlpha (0.27f + 0.1f * std::sin (t * pi)));
        g.drawVerticalLine ((int) (b.getX() + b.getWidth() * t), b.getY(), b.getBottom());
    }
    for (int i = 1; i < 5; ++i)
    {
        const auto t = (float) i / 5.0f;
        g.setColour (Theme::line().withAlpha (0.24f));
        g.drawHorizontalLine ((int) (b.getY() + b.getHeight() * t), b.getX(), b.getRight());
    }

    const auto contact = juce::Point<float> (b.getX() + state.contactX * b.getWidth(),
                                              b.getY() + state.contactY * b.getHeight());
    const auto maxR = juce::jmin (b.getWidth(), b.getHeight()) * (0.11f + 0.34f * state.spread);

    for (int ring = 5; ring >= 0; --ring)
    {
        const auto phase = std::fmod (state.motionPhase + (float) ring / 6.0f, 1.0f);
        const auto radius = maxR * (0.18f + phase);
        const auto alpha = (1.0f - phase) * (0.08f + 0.16f * state.energy);
        g.setColour (Theme::amber().withAlpha (alpha));
        g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (contact),
                       1.1f + state.energy * 1.3f);
    }

    for (int i = 0; i < (int) modes.size(); ++i)
    {
        const auto normalized = juce::jlimit (0.0f, 1.0f, std::log2 (modes[(size_t) i] / 55.0f) / 5.3f);
        const auto x = b.getX() + b.getWidth() * (0.08f + 0.84f * normalized);
        const auto waviness = std::sin (state.motionPhase * 2.0f * pi + (float) i * 1.73f);
        const auto y = b.getCentreY() + waviness * b.getHeight() * (0.10f + 0.26f * state.roughness);
        const auto distance = juce::jlimit (0.0f, 1.0f, contact.getDistanceFrom ({ x, y }) / b.getWidth());
        const auto glow = (1.0f - distance) * (0.35f + 0.65f * smoothedLevel);
        const auto radius = 2.5f + glow * 5.5f;
        g.setColour (Theme::cyan().withAlpha (0.18f + glow * 0.48f));
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre ({ x, y }));
        g.setColour (Theme::focus().withAlpha (0.35f + glow * 0.5f));
        g.fillEllipse (juce::Rectangle<float> (3.2f, 3.2f).withCentre ({ x, y }));
    }

    const auto pickupY = b.getBottom() - 18.0f;
    const auto pickupSpan = b.getWidth() * (0.08f + 0.36f * state.pickupWidth);
    for (auto side : { -1.0f, 1.0f })
    {
        const auto p = juce::Point<float> (b.getCentreX() + side * pickupSpan, pickupY);
        g.setColour (Theme::coral().withAlpha (0.22f));
        g.fillEllipse (juce::Rectangle<float> (17, 17).withCentre (p));
        g.setColour (Theme::coral());
        g.fillEllipse (juce::Rectangle<float> (6, 6).withCentre (p));
    }

    g.setColour (Theme::amber().withAlpha (0.17f));
    g.fillEllipse (juce::Rectangle<float> (26, 26).withCentre (contact));
    g.setColour (Theme::amber());
    g.drawEllipse (juce::Rectangle<float> (13, 13).withCentre (contact), 1.8f);
    g.drawLine (contact.x - 8, contact.y, contact.x + 8, contact.y, 1.2f);
    g.drawLine (contact.x, contact.y - 8, contact.x, contact.y + 8, 1.2f);

    g.restoreState();
    g.setColour (Theme::line());
    g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);

    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.setColour (Theme::amber());
    g.drawText ("CONTACT", (int) b.getX(), (int) b.getY() - 20, 80, 15, juce::Justification::centredLeft);
    g.setColour (Theme::textDim());
    g.drawText ("drag: position   wheel: spread", (int) b.getRight() - 190, (int) b.getY() - 20,
                190, 15, juce::Justification::centredRight);
    g.setColour (Theme::cyan());
    g.drawText (juce::String (modes.size()) + " TUNED MODES", (int) b.getX(), (int) b.getBottom() + 5,
                110, 15, juce::Justification::centredLeft);
    g.setColour (Theme::coral());
    g.drawText ("L / R PICKUPS", (int) b.getRight() - 110, (int) b.getBottom() + 5,
                110, 15, juce::Justification::centredRight);
}

void MaterialField::resized() {}

void MaterialField::updateContactFromPoint (juce::Point<float> p)
{
    auto b = fieldBounds();
    state.contactX = juce::jlimit (0.0f, 1.0f, (p.x - b.getX()) / b.getWidth());
    state.contactY = juce::jlimit (0.0f, 1.0f, (p.y - b.getY()) / b.getHeight());
    state.energy = juce::jlimit (0.0f, 1.0f, state.energy + 0.05f);
    if (onContactMoved) onContactMoved (state.contactX, state.contactY);
    repaint();
}

void MaterialField::mouseDown (const juce::MouseEvent& e) { grabKeyboardFocus(); updateContactFromPoint (e.position); }
void MaterialField::mouseDrag (const juce::MouseEvent& e) { updateContactFromPoint (e.position); }

void MaterialField::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    state.spread = juce::jlimit (0.0f, 1.0f, state.spread + wheel.deltaY * 0.16f);
    if (onSpreadChanged) onSpreadChanged (state.spread);
    repaint();
}

bool MaterialField::keyPressed (const juce::KeyPress& key)
{
    constexpr float step = 0.0125f;
    if (key == juce::KeyPress::leftKey) state.contactX -= step;
    else if (key == juce::KeyPress::rightKey) state.contactX += step;
    else if (key == juce::KeyPress::upKey) state.contactY -= step;
    else if (key == juce::KeyPress::downKey) state.contactY += step;
    else return false;
    state.contactX = juce::jlimit (0.0f, 1.0f, state.contactX);
    state.contactY = juce::jlimit (0.0f, 1.0f, state.contactY);
    if (onContactMoved) onContactMoved (state.contactX, state.contactY);
    repaint();
    return true;
}

juce::String MaterialField::getTooltip()
{
    return "Contact field: drag to move the excitation, use the wheel to change its spread, or use arrow keys for precision.";
}

void MaterialField::setState (FieldState newState) { state = newState; repaint(); }
void MaterialField::setModeFrequencies (std::vector<float> f) { modes = std::move (f); repaint(); }
void MaterialField::setLevel (float newLevel) noexcept { level = juce::jlimit (0.0f, 1.0f, newLevel); }

void MaterialField::timerCallback()
{
    state.motionPhase = std::fmod (state.motionPhase + 0.006f + level * 0.02f, 1.0f);
    smoothedLevel += (level - smoothedLevel) * (level > smoothedLevel ? 0.32f : 0.06f);
    state.energy = juce::jmax (0.15f, state.energy * 0.994f);
    repaint();
}

TuningCard::TuningCard()
{
    loadButton.setButtonText ("LOAD .SCL");
    loadButton.setTooltip ("Load a Scala scale (.scl) as the resonant-mode lattice");
    loadButton.onClick = [this] { if (onChooseFile) onChooseFile(); };
    addAndMakeVisible (loadButton);
}

void TuningCard::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    drawRoundedPanel (g, b, dragOver ? Theme::cyan().withAlpha (0.09f) : Theme::panel());
    g.setColour (Theme::textDim());
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("RESONANCE TUNING", 16, 12, getWidth() - 32, 16, juce::Justification::centredLeft);
    g.setColour (Theme::text());
    g.setFont (juce::FontOptions (17.0f, juce::Font::bold));
    g.drawFittedText (name, 16, 35, getWidth() - 32, 24, juce::Justification::centredLeft, 1);
    g.setColour (Theme::cyan());
    g.setFont (juce::FontOptions (11.5f));
    g.drawText (juce::String (degrees) + " degrees  ·  " + periodText, 16, 62,
                getWidth() - 32, 18, juce::Justification::centredLeft);
    g.setColour (Theme::textDim());
    g.drawFittedText (mapping, 16, 84, getWidth() - 32, 18, juce::Justification::centredLeft, 1);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (dragOver ? "Release to load tuning" : "or drop a .scl file here", 16, getHeight() - 27,
                getWidth() - 32, 15, juce::Justification::centredLeft);
}

void TuningCard::resized() { loadButton.setBounds (getWidth() - 132, getHeight() - 42, 116, 27); }

bool TuningCard::isInterestedInFileDrag (const juce::StringArray& files)
{
    return std::any_of (files.begin(), files.end(), [] (const auto& p)
    {
        const auto ext = juce::File (p).getFileExtension().toLowerCase();
        return ext == ".scl";
    });
}

void TuningCard::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    for (const auto& path : files)
    {
        juce::File f (path);
        if (isInterestedInFileDrag ({ path }))
        {
            if (onFileDropped) onFileDropped (f);
            break;
        }
    }
    repaint();
}

void TuningCard::fileDragEnter (const juce::StringArray&, int, int) { dragOver = true; repaint(); }
void TuningCard::fileDragExit (const juce::StringArray&) { dragOver = false; repaint(); }

void TuningCard::setTuningSummary (juce::String scaleName, juce::String period,
                                   int degreeCount, juce::String mappingName)
{
    name = std::move (scaleName);
    periodText = std::move (period);
    degrees = degreeCount;
    mapping = std::move (mappingName);
    repaint();
}

void LevelPill::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    drawRoundedPanel (g, b, Theme::panelRaised());
    auto meter = b.reduced (8, 7);
    const auto half = (meter.getWidth() - 4.0f) * 0.5f;
    for (int i = 0; i < 2; ++i)
    {
        auto channel = juce::Rectangle<float> (meter.getX() + (float) i * (half + 4.0f), meter.getY(), half, meter.getHeight());
        g.setColour (Theme::line());
        g.fillRoundedRectangle (channel, 2.0f);
        const auto value = i == 0 ? l : r;
        auto fill = channel.withWidth (channel.getWidth() * value);
        g.setGradientFill (juce::ColourGradient (Theme::cyan(), fill.getTopLeft(),
                                                  value > 0.82f ? Theme::coral() : Theme::amber(),
                                                  fill.getTopRight(), false));
        g.fillRoundedRectangle (fill, 2.0f);
    }
}

void LevelPill::setLevel (float left, float right)
{
    l = juce::jlimit (0.0f, 1.0f, left);
    r = juce::jlimit (0.0f, 1.0f, right);
    repaint();
}

EditorShell::EditorShell()
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);

    brand.setText ("RESONANT MATTER FIELD", juce::dontSendNotification);
    subtitle.setText ("spatial contact instrument", juce::dontSendNotification);
    configureLabel (brand, 20.0f, Theme::text());
    brand.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    configureLabel (subtitle, 11.5f, Theme::textDim());
    addAndMakeVisible (brand);
    addAndMakeVisible (subtitle);

    for (auto* label : { &fieldTitle, &bodyTitle, &contactTitle, &spatialTitle })
    {
        configureLabel (*label, 10.5f, Theme::textDim());
        label->setFont (juce::FontOptions (10.5f, juce::Font::bold));
        addAndMakeVisible (*label);
    }
    fieldTitle.setText ("FIELD", juce::dontSendNotification);
    bodyTitle.setText ("MATERIAL BODY", juce::dontSendNotification);
    contactTitle.setText ("MICROCONTACTS", juce::dontSendNotification);
    spatialTitle.setText ("TRANSDUCER / OUTPUT", juce::dontSendNotification);

    presetBox.addItemList ({ "Brittle Constellation", "Adhesive Bloom", "Tritave Dust",
                             "Copper Skin", "Almost Unstable" }, 1);
    presetBox.setSelectedId (1);
    presetBox.setTooltip ("Browse factory and user presets");
    saveButton.setTooltip ("Save the current state as a user preset");
    powerButton.setToggleState (true, juce::dontSendNotification);
    powerButton.setTooltip ("Bypass or activate RMF");

    std::array<juce::Component*, 17> components { &field, &tuning, &density, &impulse,
                                                  &roughness, &stiffness, &damping, &coupling,
                                                  &drive, &width, &eventType, &bodyType,
                                                  &tuneMode, &presetBox, &saveButton,
                                                  &powerButton, &outputMeter };
    for (auto* component : components)
        addAndMakeVisible (component);

    roughness.slider().setColour (juce::Slider::rotarySliderFillColourId, Theme::amber());
    density.slider().setColour (juce::Slider::rotarySliderFillColourId, Theme::amber());
    impulse.slider().setColour (juce::Slider::rotarySliderFillColourId, Theme::amber());
    coupling.slider().setColour (juce::Slider::rotarySliderFillColourId, Theme::coral());
    drive.slider().setColour (juce::Slider::rotarySliderFillColourId, Theme::coral());
    width.slider().setColour (juce::Slider::rotarySliderFillColourId, Theme::coral());

    density.slider().setTooltip ("How many microscopic contact events occur each second.");
    impulse.slider().setTooltip ("Force delivered by each contact into the modeled body.");
    roughness.slider().setTooltip ("Irregularity of the surface and variation between micro-slips.");
    stiffness.slider().setTooltip ("Rigidity and dispersive spacing of the resonant modes.");
    damping.slider().setTooltip ("How quickly energy leaves the resonant body.");
    coupling.slider().setTooltip ("Electromechanical loading; negative values approach active reinforcement.");
    drive.slider().setTooltip ("Output gain into the soft saturation stage.");
    width.slider().setTooltip ("Distance between the modeled left and right pickup positions.");

    tuneMode.onChange = [this] (int mode)
    {
        tuning.setTuningSummary ("19 equal divisions", "2:1 period", 19,
                                 mode == 0 ? "Nearest modal mapping" : "Ascending degree mapping");
    };

    configureLabel (compactHint, 10.0f, Theme::textDim(), juce::Justification::centredRight);
    compactHint.setText ("Double-click a dial to reset  |  Shift-drag for fine control", juce::dontSendNotification);
    addAndMakeVisible (compactHint);
    addAndMakeVisible (headerRule);
}

EditorShell::~EditorShell() { setLookAndFeel (nullptr); }

void EditorShell::paint (juce::Graphics& g)
{
    g.fillAll (Theme::canvas());
    auto glow = getLocalBounds().toFloat().withWidth ((float) getWidth() * 0.55f).expanded (80.0f);
    juce::ColourGradient gradient (Theme::cyan().withAlpha (0.045f), glow.getTopLeft(),
                                   juce::Colours::transparentBlack, glow.getBottomRight(), true);
    g.setGradientFill (gradient);
    g.fillRect (getLocalBounds());

    g.setColour (Theme::line().withAlpha (0.65f));
    g.fillRect (0, 65, getWidth(), 1);
}

void EditorShell::resized()
{
    auto all = getLocalBounds();
    auto header = all.removeFromTop (66).reduced (18, 12);
    auto titleArea = header.removeFromLeft (juce::jmin (330, header.getWidth() / 3));
    brand.setBounds (titleArea.removeFromTop (25));
    subtitle.setBounds (titleArea);

    powerButton.setBounds (header.removeFromRight (76).reduced (0, 3));
    outputMeter.setBounds (header.removeFromRight (102).reduced (8, 8));
    saveButton.setBounds (header.removeFromRight (62).reduced (4, 3));
    presetBox.setBounds (header.removeFromRight (juce::jmin (230, header.getWidth())).reduced (5, 3));

    all.reduce (18, 14);
    const auto gap = 12;
    if (getWidth() >= 920)
    {
        auto left = all.removeFromLeft ((int) ((float) all.getWidth() * 0.54f));
        all.removeFromLeft (gap);
        auto right = all;
        field.setBounds (left);

        auto tuningRow = right.removeFromTop (142);
        tuning.setBounds (tuningRow.removeFromLeft (juce::roundToInt ((float) tuningRow.getWidth() * 0.63f)));
        tuningRow.removeFromLeft (gap);
        tuneMode.setBounds (tuningRow.reduced (0, 2));
        right.removeFromTop (gap);

        const auto sectionGap = 10;
        auto segmentRow = right.removeFromTop (57);
        eventType.setBounds (segmentRow.removeFromLeft (segmentRow.getWidth() / 2).reduced (0, 1));
        segmentRow.removeFromLeft (sectionGap);
        bodyType.setBounds (segmentRow.reduced (0, 1));
        right.removeFromTop (sectionGap);

        auto dialArea = right;
        auto topRow = dialArea.removeFromTop (dialArea.getHeight() / 2);
        auto bottomRow = dialArea;
        auto place = [] (juce::Rectangle<int> row, std::initializer_list<juce::Component*> items)
        {
            const int each = row.getWidth() / (int) items.size();
            for (auto* item : items) item->setBounds (row.removeFromLeft (each).reduced (2, 0));
        };
        place (topRow, { &density, &impulse, &roughness, &stiffness });
        place (bottomRow, { &damping, &coupling, &drive, &width });
    }
    else
    {
        // Compact layout keeps every performance control visible without miniature dials.
        auto fieldArea = all.removeFromTop (juce::roundToInt ((float) all.getHeight() * 0.43f));
        field.setBounds (fieldArea);
        all.removeFromTop (gap);
        auto tuningArea = all.removeFromLeft (juce::roundToInt ((float) all.getWidth() * 0.38f));
        tuning.setBounds (tuningArea.removeFromTop (130));
        tuneMode.setBounds (tuningArea.removeFromTop (56).reduced (0, 4));
        eventType.setBounds (tuningArea.removeFromTop (56).reduced (0, 4));
        bodyType.setBounds (tuningArea.removeFromTop (56).reduced (0, 4));
        all.removeFromLeft (gap);
        auto topRow = all.removeFromTop (all.getHeight() / 2);
        auto place = [] (juce::Rectangle<int> row, std::initializer_list<juce::Component*> items)
        {
            const int each = row.getWidth() / (int) items.size();
            for (auto* item : items) item->setBounds (row.removeFromLeft (each).reduced (1, 0));
        };
        place (topRow, { &density, &impulse, &roughness, &stiffness });
        place (all, { &damping, &coupling, &drive, &width });
    }

    compactHint.setBounds (getWidth() - 390, getHeight() - 18, 370, 14);
    for (auto* label : { &fieldTitle, &bodyTitle, &contactTitle, &spatialTitle })
        label->setBounds ({ 0, 0, 0, 0 }); // Section meaning is encoded by colour and grouping in this shell.
}
} // namespace rmf::ui
