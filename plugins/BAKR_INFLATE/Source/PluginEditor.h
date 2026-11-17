#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace binf
{
class LnF : public juce::LookAndFeel_V4
{
public:
    LnF();
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    int getSliderThumbRadius (juce::Slider&) override { return 24; }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (13.0f)); }
};

// Vumètre stéréo vertical avec graduations
class Meter : public juce::Component
{
public:
    Meter (std::atomic<float>& l, std::atomic<float>& r, bool scaleRight) : L (l), R (r), right (scaleRight) {}
    void paint (juce::Graphics&) override;
    void tick() { float a = L.load(), b = R.load(); if (std::abs (a - shownL) > 1.0e-4f || std::abs (b - shownR) > 1.0e-4f) { shownL = a; shownR = b; repaint(); } }
private:
    std::atomic<float>& L; std::atomic<float>& R; bool right;
    float shownL = -1, shownR = -1;
};

// Vumètre du gonflement (dB ajoutés)
class EffectMeter : public juce::Component
{
public:
    explicit EffectMeter (std::atomic<float>& v) : val (v) {}
    void paint (juce::Graphics&) override;
    void tick() { float v = val.load(); if (std::abs (v - shown) > 0.02f) { shown = v; repaint(); } }
private:
    std::atomic<float>& val; float shown = -1;
};

// Courbe de transfert en direct
class CurveView : public juce::Component
{
public:
    explicit CurveView (BakrInflateProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void tick();
private:
    BakrInflateProcessor& proc;
    float lastC = 999, lastE = -1; bool lastClip = false, lastOn = true;
};
} // namespace binf

class BakrInflateEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BakrInflateEditor (BakrInflateProcessor&);
    ~BakrInflateEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    BakrInflateProcessor& proc;
    binf::LnF lnf;

    struct Root : public juce::Component
    {
        void paint (juce::Graphics&) override;
        juce::Rectangle<int> inRect, fxRect, outRect;
    } root;

    juce::Slider inFader, effectFader, curveFader, outFader;
    std::unique_ptr<SA> inAtt, effectAtt, curveAtt, outAtt;
    juce::TextButton clipBtn { "CLIP 0 dB" }, bandBtn { "3 BANDES" }, effectBtn { "EFFET ON" };
    std::unique_ptr<BA> clipAtt, bandAtt, effectInAtt;
    juce::TextButton aBtn { "A" }, bBtn { "B" }, prevBtn { "<" }, nextBtn { ">" };
    juce::ComboBox presetBox;
    juce::Label clipLed;

    binf::Meter inMeter, outMeter;
    binf::EffectMeter fxMeter;
    binf::CurveView curve;
    int clipHold = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakrInflateEditor)
};
