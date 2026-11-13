#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Style "Bakr Audio Plugins" : métal brossé, néons bleus, boutons chromés
//==============================================================================
class BakrLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BakrLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

class VuMeter : public juce::Component
{
public:
    void setValue (float vuDb, int meterMode);
    void paint (juce::Graphics&) override;
private:
    float value = -20.0f;
    int mode = 1;
};

// Tout le panneau est dessiné à une taille de base 1100 × 550, puis mis à l'échelle
class BakrPanel : public juce::Component
{
public:
    static constexpr int baseW = 1100, baseH = 550;

    explicit BakrPanel (BakrTubeCompAudioProcessor&);
    ~BakrPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();

private:
    struct Knob
    {
        juce::Slider slider;
        juce::String label, minTxt, maxTxt;
        juce::StringArray marks;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    void setupKnob (Knob& k, const juce::String& id, const juce::String& label,
                    const juce::String& minTxt, const juce::String& maxTxt,
                    juce::StringArray marks, const juce::String& size);
    void drawKnobText (juce::Graphics&, const Knob&, float labelGap);
    void buildBackground();
    float param (const char* id) const;

    BakrTubeCompAudioProcessor& proc;
    BakrLookAndFeel lnf;

    Knob input, output, mode, recovery, meter, mix;
    Knob scHp, detect, lp, pkF, pkG, pkQ, scGain;
    juce::ToggleButton bypassBtn, powerSwitch, eqSwitch, listenBtn, srcInt, srcExt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt, powerAtt, eqAtt, listenAtt;

    juce::ComboBox presetBox;
    VuMeter vu;
    float vuBallistic = -20.0f;
    juce::Image background;
    juce::int64 lastStateHash = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakrPanel)
};

class BakrTubeCompAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BakrTubeCompAudioProcessorEditor (BakrTubeCompAudioProcessor&);
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
    void resized() override;
private:
    void timerCallback() override { panel.tick(); }
    BakrPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakrTubeCompAudioProcessorEditor)
};
