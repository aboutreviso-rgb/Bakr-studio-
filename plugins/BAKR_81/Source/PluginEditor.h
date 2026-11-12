#pragma once
#include "PluginProcessor.h"

namespace ui81
{
constexpr int W = 1180, H = 598;

juce::Font font (float h, bool bold = false);

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (15.0f, true); }
    juce::Font getPopupMenuFont() override { return font (15.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (15.0f, true); }
    juce::Font getLabelFont (juce::Label&) override { return font (14.0f, true); }
};

// Bouton rotatif : kind = "red", "grey", "blue", "small"
class Knob : public juce::Slider
{
public:
    Knob (const juce::String& kind, double resetValue);
};

// Touche carrée crème (IN, BELL, PHASE) ou rouge (POWER)
class RackSwitch : public juce::ToggleButton
{
public:
    explicit RackSwitch (bool red = false) : isRed (red) {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    bool isRed;
};

// Courbe d'EQ avec les 4 points à glisser
class EqGraph : public juce::Component
{
public:
    explicit EqGraph (Bakr81Processor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::Rectangle<float> plot() const;
    float xForF (double f) const;
    double fForX (float x) const;
    float yForDb (float db) const;
    float dbForY (float y) const;
    juce::Point<float> nodePos (int i) const;
    int hitNode (juce::Point<float>) const;

    Bakr81Processor& proc;
    juce::RangedAudioParameter *gainP[4], *freqP[4];
    int dragging = -1, hover = -1;
};

// Vumètres IN / OUT à LED
class Meters : public juce::Component
{
public:
    explicit Meters (Bakr81Processor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void tick();
private:
    Bakr81Processor& proc;
    float lvl[4] {}, hold[4] {};
    int holdT[4] {};
};

// Contenu dessiné à la taille de base, puis mis à l'échelle
class Content : public juce::Component, private juce::Timer
{
public:
    explicit Content (Bakr81Processor&);
    ~Content() override;
    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    void timerCallback() override;
    void placeKnob (juce::Slider&, float cx, float cy, float size);
    void placeSwitch (juce::Button&, float cx, float cy, float w = 36, float h = 28);
    void savePreset();
    void loadPreset();
    void refreshPresetBox();

    Bakr81Processor& proc;
    LookAndFeel lnf;
    juce::TooltipWindow tooltipWindow { this, 600 };

    // Barre du haut
    juce::TextButton abButton { "A / B" }, prevButton { "<" }, nextButton { ">" }, saveButton { "Save" }, loadButton { "Load" };
    juce::ComboBox presetBox;
    std::unique_ptr<juce::FileChooser> chooser;

    // Rack
    Knob inputK { "red", 0 }, lfK { "grey", 0 }, lmfK { "grey", 0 }, hmfK { "grey", 0 }, hfK { "grey", 0 },
         outputK { "blue", 0 }, eqTrimK { "small", 0 };
    RackSwitch lowInS, lfBellS, highInS, hfBellS, eqInS, phaseS, powerS { true };

    // Graphe + vumètres
    EqGraph graph;
    Meters meters;

    // Bas
    Knob lfK2 { "grey", 0 }, lfF { "small", 60 }, lmfK2 { "grey", 0 }, lmfF { "small", 800 },
         hmfK2 { "grey", 0 }, hmfF { "small", 3500 }, hfK2 { "grey", 0 }, hfF { "small", 12000 }, eqTrimK2 { "grey", 0 };
    RackSwitch phaseS2, powerS2 { true };
    juce::Label values[9];
    juce::String valueIds[9] { "lfG", "lfF", "lmfG", "lmfF", "hmfG", "hmfF", "hfG", "hfF", "eqTrim" };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SA>> sliderAtt;
    std::vector<std::unique_ptr<BA>> buttonAtt;
    int lastPreset = -1;
};
} // namespace ui81

class Bakr81Editor : public juce::AudioProcessorEditor
{
public:
    explicit Bakr81Editor (Bakr81Processor&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff0b0e13)); }
private:
    ui81::Content content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Bakr81Editor)
};
