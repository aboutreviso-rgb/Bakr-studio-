#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace bvc
{
class LnF : public juce::LookAndFeel_V4
{
public:
    LnF();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (12.5f)); }
};

// Bouton marche/arrêt d'un module
class PowerButton : public juce::Button
{
public:
    explicit PowerButton (juce::Colour c) : juce::Button ("on"), accent (c) { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool, bool) override;
private:
    juce::Colour accent;
};

// Sélecteur  < nom >  lié à un paramètre à choix
class ChoiceStepper : public juce::Component
{
public:
    ChoiceStepper (juce::RangedAudioParameter& p, juce::StringArray names);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    juce::TextButton prev { "<" }, next { ">" };
    juce::StringArray items; int index = 0;
    std::unique_ptr<juce::ParameterAttachment> att;
};

// Petit écran : courbe d'EQ d'une bande (0 = aigus, 1 = médiums, 2 = coupe-bas)
class EqView : public juce::Component
{
public:
    EqView (std::atomic<float>& v, int bandType, juce::Colour c) : value (v), band (bandType), accent (c) {}
    void paint (juce::Graphics&) override;
    void tick() { float v = value.load(); if (std::abs (v - shown) > 1.0e-3f) { shown = v; repaint(); } }
private:
    std::atomic<float>& value; int band; juce::Colour accent; float shown = -999;
};

// Écran du doubleur : position des voix dans la stéréo
class DoublerView : public juce::Component
{
public:
    DoublerView (std::atomic<float>& s, std::atomic<float>& a, juce::Colour c) : spread (s), amount (a), accent (c) {}
    void paint (juce::Graphics&) override;
    void tick() { float k = spread.load() * 1000 + amount.load(); if (std::abs (k - shown) > 1.0e-3f) { shown = k; repaint(); } }
private:
    std::atomic<float>& spread; std::atomic<float>& amount; juce::Colour accent; float shown = -1;
};

// Vumètre de réduction de gain (horizontal)
class GRMeter : public juce::Component
{
public:
    GRMeter (std::atomic<float>& gr, juce::Colour c) : value (gr), accent (c) {}
    void paint (juce::Graphics&) override;
    void tick() { float v = value.load(); if (std::abs (v - shown) > 0.05f) { shown = v; repaint(); } }
private:
    std::atomic<float>& value; juce::Colour accent; float shown = -1;
};

// Pictogramme de module (dessiné en code)
class Icon : public juce::Component
{
public:
    Icon (int type, juce::Colour c) : kind (type), accent (c) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics&) override;
    void setActive (bool a) { if (a != active) { active = a; repaint(); } }
private:
    int kind; juce::Colour accent; bool active = true;
};
} // namespace bvc

class BakrVoxChainEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BakrVoxChainEditor (BakrVoxChainProcessor&);
    ~BakrVoxChainEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    juce::Slider* knob (const char* id, const juce::String& label, juce::Colour c);
    juce::Slider* hslider (const char* id, juce::Colour c);
    juce::TextButton* toggle (const char* id, const juce::String& text, juce::Colour c);

    BakrVoxChainProcessor& proc;
    bvc::LnF lnf;

    struct Root : public juce::Component
    {
        void paint (juce::Graphics&) override;
        std::array<juce::Rectangle<int>, 5> cols;
        std::array<juce::Colour, 5> accents;
        std::array<bool, 5> on { true, true, true, true, true };
    } root;

    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label> labels;
    juce::OwnedArray<SA> sAtts;
    juce::OwnedArray<juce::TextButton> toggles;
    juce::OwnedArray<BA> bAtts;
    juce::OwnedArray<bvc::PowerButton> powers;
    juce::OwnedArray<bvc::Icon> icons;
    std::map<juce::String, juce::Slider*> sl;
    std::map<juce::String, juce::Label*> lb;
    std::map<juce::String, juce::TextButton*> tg;

    std::unique_ptr<bvc::GRMeter> grMeter;
    juce::OwnedArray<bvc::EqView> eqViews;
    std::unique_ptr<bvc::DoublerView> dblView;
    std::unique_ptr<bvc::ChoiceStepper> revType, dlyType;
    juce::ComboBox dlyTime;
    std::unique_ptr<CA> dlyTimeAtt;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakrVoxChainEditor)
};
