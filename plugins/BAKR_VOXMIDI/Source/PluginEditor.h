#pragma once
#include "PluginProcessor.h"

namespace vui
{
namespace col
{
    const juce::Colour bg     { 0xff0a101c };
    const juce::Colour panel  { 0xff101828 };
    const juce::Colour module { 0xff131c2e };
    const juce::Colour border { 0xff2a3650 };
    const juce::Colour accent { 0xff2f9bff };
    const juce::Colour accLo  { 0xff1d5fa8 };
    const juce::Colour text   { 0xffe9eef8 };
    const juce::Colour dim    { 0xff8a97b0 };
    const juce::Colour track  { 0xff2a3348 };
    const juce::Colour amber  { 0xffffa53a };
    const juce::Colour rec    { 0xffff4d5e };
    const juce::Colour green  { 0xff3ddc84 };
}
juce::Font font (float h, bool bold = false);

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (14.0f); }
    juce::Font getPopupMenuFont() override { return font (14.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (14.0f, true); }
    juce::Font getSliderPopupFont (juce::Slider&) override { return font (13.0f, true); }
};

class PillSwitch : public juce::ToggleButton
{
public:
    using juce::ToggleButton::ToggleButton;
    void paintButton (juce::Graphics&, bool, bool) override;
};

class PitchRoll : public juce::Component
{
public:
    explicit PitchRoll (VoxMidiProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    VoxMidiProcessor& proc;
    float centre = 60.0f;
};

class NoteDisplay : public juce::Component
{
public:
    explicit NoteDisplay (VoxMidiProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    VoxMidiProcessor& proc;
    float shownCents = 0, level = 0;
};

class DragMidi : public juce::Component
{
public:
    explicit DragMidi (VoxMidiProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
    std::function<void (const juce::String&)> onInfo;
private:
    VoxMidiProcessor& proc;
    bool dragging = false;
};
} // namespace vui

class VoxMidiEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VoxMidiEditor (VoxMidiProcessor&);
    ~VoxMidiEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void knob (juce::Slider& s, const juce::String& id, const juce::String& label, bool bipolar = false);
    void updateInstButtons();

    VoxMidiProcessor& proc;
    vui::LookAndFeel laf;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::vector<std::unique_ptr<SA>> sa; std::vector<std::unique_ptr<BA>> ba; std::vector<std::unique_ptr<CA>> ca;
    std::vector<std::pair<juce::Slider*, juce::String>> knobLabels;

    vui::NoteDisplay noteDisp;
    vui::PitchRoll roll;
    vui::DragMidi dragMidi;
    juce::ComboBox presets, keyBox, scaleBox;
    vui::PillSwitch midiOut { "MIDI OUT" }, synthOn { "SYNTH" }, follow { "Suivre la voix" }, bend { "Pitch Bend MIDI" };
    juce::OwnedArray<juce::TextButton> instButtons;
    std::unique_ptr<juce::ParameterAttachment> instAtt;
    juce::Slider octave, gate, stab, glide, tone, velSens, voice, instLvl, out;
    juce::TextButton capBtn { juce::String::fromUTF8 ("● Capture") }, clearBtn { "Effacer" }, saveBtn { juce::String::fromUTF8 ("Enregistrer .mid") };
    juce::Label capInfo;
    int instIndex = 0;

    juce::Rectangle<int> rLeft, rRoll, rInst, rKnobs;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxMidiEditor)
};
