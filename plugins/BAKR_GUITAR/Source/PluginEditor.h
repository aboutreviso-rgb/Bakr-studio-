#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace bgui
{
const juce::Colour cBg       { 0xff121212 };
const juce::Colour panel    { 0xff1b1a19 };
const juce::Colour panel2   { 0xff242220 };
const juce::Colour edge     { 0xff3a342c };
const juce::Colour gold     { 0xffd2a865 };
const juce::Colour goldDim  { 0xff8a7048 };
const juce::Colour cText     { 0xffe9e2d6 };
const juce::Colour textDim  { 0xff9a9286 };

class LnF : public juce::LookAndFeel_V4
{
public:
    LnF();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (13.0f)); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::Font (juce::FontOptions (12.0f)); }
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

// Molette (pitch bend qui revient au centre / modulation)
class Wheel : public juce::Component
{
public:
    Wheel (std::atomic<float>& v, bool bipolar, std::atomic<bool>* active, juce::String name)
        : value (v), centre (bipolar), activeFlag (active), label (std::move (name)) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { if (activeFlag) *activeFlag = true; mouseDrag (e); }
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent&) override { if (centre) value = 0.0f; if (activeFlag) *activeFlag = false; repaint(); }
private:
    std::atomic<float>& value; bool centre; std::atomic<bool>* activeFlag; juce::String label;
};

// Clavier avec zones colorées (rouge = articulations, vert = bruits)
class Keyboard : public juce::MidiKeyboardComponent
{
public:
    Keyboard (juce::MidiKeyboardState& s, BakrGuitarProcessor& p)
        : juce::MidiKeyboardComponent (s, juce::MidiKeyboardComponent::horizontalKeyboard), proc (p) {}
    int selectedArtic = 0;
protected:
    void drawWhiteNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour lineColour, juce::Colour textColour) override;
    void drawBlackNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour noteFillColour) override;
private:
    juce::Colour zoneColour (int note) const;
    BakrGuitarProcessor& proc;
};

// Bouton d'effet avec pictogramme
class FxButton : public juce::Button
{
public:
    FxButton (const juce::String& name, int iconId) : juce::Button (name), icon (iconId) { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    int icon;
};

// Élément de la bibliothèque (AC / CN / DB / SB)
class LibraryItem : public juce::Button
{
public:
    LibraryItem (const juce::String& code, const juce::String& name) : juce::Button (name), codeText (code) {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    juce::String codeText;
};

// Illustration de la guitare (change selon le modèle choisi)
class GuitarArt : public juce::Component
{
public:
    void setGuitar (int g) { if (g != guitar) { guitar = g; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    int guitar = 2;
};
} // namespace bgui

//==============================================================================
class BakrGuitarEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit BakrGuitarEditor (BakrGuitarProcessor&);
    ~BakrGuitarEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void layoutRoot();
    void showFileMenu();
    void refreshPresetBox();

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Root : public juce::Component
    {
        void paint (juce::Graphics&) override;
        juce::Rectangle<int> artRect, knobRow, articRect, fxRect, perfRect, keysRect;
    };

    BakrGuitarProcessor& proc;
    bgui::LnF lnf;
    Root root;
    bgui::GuitarArt art;

    // barre du haut
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, fileBtn { "..." }, gearBtn { "" };
    juce::ComboBox presetBox;
    juce::Label bankLabel;

    // bibliothèque
    juce::OwnedArray<bgui::LibraryItem> library;
    std::unique_ptr<juce::ParameterAttachment> guitarAtt;

    // potards
    juce::OwnedArray<juce::Slider> knobs;
    juce::OwnedArray<juce::Label> knobLabels;
    juce::OwnedArray<SA> knobAtts;

    // articulations
    juce::OwnedArray<juce::TextButton> articBtns;
    std::unique_ptr<juce::ParameterAttachment> articAtt;

    // effets
    juce::OwnedArray<bgui::FxButton> fxBtns;
    juce::OwnedArray<BA> fxAtts;

    // performance
    juce::TextButton noiseBtn { "Bruits de main" }, strumBtn { "Strum" };
    juce::Label chordLabel, dirLabel, spaceLabel;
    juce::ComboBox dirBox;
    juce::Slider spaceSlider;
    std::unique_ptr<BA> noiseAtt, strumAtt;
    std::unique_ptr<CA> dirAtt;
    std::unique_ptr<SA> spaceAtt;

    // réglages avancés (roue dentée)
    struct AdvPanel : public juce::Component { void paint (juce::Graphics&) override; } advPanel;
    juce::OwnedArray<juce::Slider> advKnobs;
    juce::OwnedArray<juce::Label> advLabels;
    juce::OwnedArray<SA> advAtts;
    juce::ComboBox delayTimeBox;
    std::unique_ptr<CA> delayTimeAtt;
    juce::TextButton advClose { "Fermer" };

    // clavier
    bgui::Keyboard keyboard;
    bgui::Wheel pitch, mod;

    std::unique_ptr<juce::FileChooser> chooser;
    int shownPreset = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakrGuitarEditor)
};
