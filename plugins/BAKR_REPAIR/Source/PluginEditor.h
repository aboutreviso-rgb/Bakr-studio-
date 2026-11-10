#pragma once
#include "PluginProcessor.h"

namespace ui
{
namespace col
{
    const juce::Colour bg       { 0xff0a101c };
    const juce::Colour panel    { 0xff101828 };
    const juce::Colour module   { 0xff121b2c };
    const juce::Colour border   { 0xff2a3650 };
    const juce::Colour accent   { 0xff2f9bff };
    const juce::Colour accentLo { 0xff1d5fa8 };
    const juce::Colour text     { 0xffe9eef8 };
    const juce::Colour dim      { 0xff8a97b0 };
    const juce::Colour track    { 0xff2a3348 };
    const juce::Colour amber    { 0xffffa53a };
}

juce::Font font (float h, bool bold = false);
void drawIcon (juce::Graphics& g, const juce::String& name, juce::Rectangle<float> r, juce::Colour c);

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (14.0f); }
    juce::Font getPopupMenuFont() override { return font (14.0f); }
    juce::Font getSliderPopupFont (juce::Slider&) override { return font (13.0f, true); }
};

// Interrupteur pilule (De-hum, De-click, Lo-cut, Limiter...)
class PillSwitch : public juce::ToggleButton
{
public:
    using juce::ToggleButton::ToggleButton;
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// Bouton d'activation de module
class PowerButton : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// Oreille : écouter ce qui est retiré
class EarButton : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// Bouton icône simple (reset, réglages, aide)
class IconButton : public juce::Button
{
public:
    explicit IconButton (const juce::String& iconName) : juce::Button (iconName), icon (iconName) {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    juce::String icon;
};

// Deux segments liés à un paramètre booléen
class SegPair : public juce::Component
{
public:
    SegPair (juce::RangedAudioParameter& p, juce::String a, juce::String b, bool vertical, juce::String iconA = {}, juce::String iconB = {});
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    juce::ParameterAttachment att;
    juce::String la, lb, ia, ib;
    bool vert, value = false;
};

class Spectrum : public juce::Component
{
public:
    explicit Spectrum (BakrRepairProcessor& p);
    void update();
    void paint (juce::Graphics&) override;
private:
    static constexpr int order = 11, N = 1 << order;
    BakrRepairProcessor& proc;
    juce::dsp::FFT fft { order };
    std::vector<float> win, buf, preDb, postDb;
};

class DeclipMeter : public juce::Component
{
public:
    explicit DeclipMeter (BakrRepairProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    BakrRepairProcessor& proc;
};

class GRMeter : public juce::Component
{
public:
    explicit GRMeter (BakrRepairProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    BakrRepairProcessor& proc;
};

class LearnButton : public juce::Button
{
public:
    explicit LearnButton (BakrRepairProcessor& p) : juce::Button ("Learn"), proc (p) {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    BakrRepairProcessor& proc;
};

class HelpOverlay : public juce::Component
{
public:
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { setVisible (false); }
};
} // namespace ui

//==============================================================================
class BakrRepairEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BakrRepairEditor (BakrRepairProcessor&);
    ~BakrRepairEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider& s, const juce::String& id, const juce::String& icon, bool bipolar = false);
    void setupToggle (juce::Button& b, const juce::String& id);
    void drawModule (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title);

    BakrRepairProcessor& proc;
    ui::LookAndFeel laf;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SA>> sliderAtts;
    std::vector<std::unique_ptr<BA>> buttonAtts;

    ui::Spectrum spectrum;
    ui::LearnButton learn;
    ui::IconButton resetBtn { "reset" }, settingsBtn { "gear" }, helpBtn { "help" };
    std::unique_ptr<ui::SegPair> modeSeg, toneSeg, speedSeg;

    ui::PowerButton cuPower, tonePower, dsPower, dcPower;
    ui::EarButton cuEar, dsEar, dcEar;

    juce::Slider denoise, dereverb, tone, shaping, dsThresh, outGain, dcThresh;
    ui::PillSwitch dehum { "De-hum" }, declick { "De-click" }, locut { "Lo-cut" }, hicut { "Hi-cut" }, limiter { "Limiter" };
    ui::GRMeter grMeter;
    ui::DeclipMeter dcMeter;
    juce::Label dcValue;

    juce::TextButton bypass { "Bypass" };
    juce::ComboBox presets;
    ui::HelpOverlay help;

    juce::Rectangle<int> rCU, rTone, rDS, rDC;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakrRepairEditor)
};
