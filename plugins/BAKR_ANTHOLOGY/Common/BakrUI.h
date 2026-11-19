#pragma once
#include <JuceHeader.h>
#include "BakrBase.h"

namespace bakr
{
    enum class Ctl { Knob, Selector, Button, Lamp, Lever, Fader, Radio };
    enum class Src { GrMax, GrL, GrR, InMax, OutMax, InL, InR, OutL, OutR, Selectable, Extra0, Extra1, Extra2, Extra3 };

    struct ControlSpec
    {
        Ctl type;
        const char* id;
        juce::String label;
        float x, y, size;                 // centre + diamètre (boutons : largeur)
        juce::String look = "black";      // black, chrome, red, blue, cream, grey, bakelite, silver
        juce::StringArray marks {};       // positions des sélecteurs
        juce::String minTxt {}, maxTxt {};
        juce::Colour light { 0xffff3b2f };// couleur des boutons lumineux
        float labelGap = 0.0f;            // décalage du titre (0 = auto)
        juce::Colour labelColour { 0x00000000 }; // 0 = couleur du thème
        int radioValue = 0;               // Ctl::Radio : valeur (index) du choix
    };

    struct MeterSpec
    {
        float x, y, w, h;
        Src src;
        juce::String caption;
        juce::String face = "cream";      // cream, amber, white, led, readout (chiffres), bar (barre GR)
        const char* selectParam = nullptr; // pour Src::Selectable (0=GR,1=IN,2=OUT)
        juce::String unit {};             // readout : unité affichée
    };

    struct TextSpec
    {
        juce::String text;
        float x, y, w, h, size;
        juce::Colour colour;
        bool bold = true, italic = false;
        juce::Justification just = juce::Justification::centred;
        bool vertical = false;
    };

    struct BoxSpec
    {
        float x, y, w, h;
        juce::Colour fill, line;
        float radius = 6.0f;
        int deco = 0;   // 1 = lampes en verre, 2 = afficheur LCD bleu
    };

    struct Theme
    {
        juce::Colour plateTop, plateBottom, text, dim, accent;
        bool brushed = true;
        bool rackEars = true;
        juce::Colour earColour { 0xff2a2c30 };
        int texture = 0;   // 0 = brossé (si brushed), 1 = peinture martelée, 2 = peinture poudre lisse
    };

    struct UiSpec
    {
        int w = 1000, h = 400;
        Theme theme;
        std::vector<BoxSpec> boxes;
        std::vector<TextSpec> texts;
        std::vector<ControlSpec> controls;
        std::vector<MeterSpec> meters;
        juce::Rectangle<int> presetArea { 0, 0, 0, 0 };
    };

    //==========================================================================
    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
        void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float,
                               juce::Slider::SliderStyle, juce::Slider&) override;
    };

    class Meter : public juce::Component
    {
    public:
        Meter (BaseProcessor& p, MeterSpec s) : proc (p), spec (std::move (s)) {}
        void paint (juce::Graphics&) override;
        void tick();
    private:
        float read (Src s) const;
        BaseProcessor& proc;
        MeterSpec spec;
        float val = -20.0f, val2 = -60.0f, peak = -60.0f;
        int mode = 0, sel = -1;
    };

    class Panel : public juce::Component
    {
    public:
        Panel (BaseProcessor&, UiSpec);
        ~Panel() override;
        void paint (juce::Graphics&) override;
        void tick();
        const UiSpec& getSpec() const { return spec; }
    private:
        struct Item
        {
            ControlSpec spec;
            std::unique_ptr<juce::Slider> slider;
            std::unique_ptr<juce::ToggleButton> button;
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
            std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
        };
        void drawItemText (juce::Graphics&, const Item&);
        void buildBackground();

        BaseProcessor& proc;
        UiSpec spec;
        LookAndFeel lnf;
        std::vector<std::unique_ptr<Item>> items;
        std::vector<std::unique_ptr<Meter>> meters;
        juce::Image bg;
        juce::int64 lastHash = -1;
    };

    // Fenêtre du plugin : façade + barre d'outils (presets, comparaison A/B)
    class Editor : public juce::AudioProcessorEditor, private juce::Timer
    {
    public:
        static constexpr int barH = 40;
        Editor (BaseProcessor&, UiSpec);
        ~Editor() override { setLookAndFeel (nullptr); }
        void paint (juce::Graphics& g) override;
        void resized() override;
    private:
        void timerCallback() override;
        void selectSlot (int slot);
        BaseProcessor& proc;
        Panel panel;
        LookAndFeel lnf;
        juce::ComboBox presets;
        juce::TextButton aBtn { "A" }, bBtn { "B" }, copyBtn { "A > B" }, prevBtn { "<" }, nextBtn { ">" };
        juce::ValueTree slots[2];
        int cur = 0;
    };
}
