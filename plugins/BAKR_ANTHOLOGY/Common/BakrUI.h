#pragma once
#include <JuceHeader.h>
#include "BakrBase.h"

namespace bakr
{
    enum class Ctl { Knob, Selector, Button, Lamp, Lever };
    enum class Src { GrMax, GrL, GrR, InMax, OutMax, InL, InR, OutL, OutR, Selectable };

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
    };

    struct MeterSpec
    {
        float x, y, w, h;
        Src src;
        juce::String caption;
        juce::String face = "cream";      // cream, amber, white, led
        const char* selectParam = nullptr; // pour Src::Selectable (0=GR,1=IN,2=OUT)
    };

    struct TextSpec
    {
        juce::String text;
        float x, y, w, h, size;
        juce::Colour colour;
        bool bold = true, italic = false;
        juce::Justification just = juce::Justification::centred;
    };

    struct BoxSpec
    {
        float x, y, w, h;
        juce::Colour fill, line;
        float radius = 6.0f;
    };

    struct Theme
    {
        juce::Colour plateTop, plateBottom, text, dim, accent;
        bool brushed = true;
        bool rackEars = true;
        juce::Colour earColour { 0xff2a2c30 };
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
        int mode = 0;
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
        juce::ComboBox presets;
        juce::Image bg;
        juce::int64 lastHash = -1;
    };

    class Editor : public juce::AudioProcessorEditor, private juce::Timer
    {
    public:
        Editor (BaseProcessor&, UiSpec);
        void paint (juce::Graphics& g) override { g.fillAll (juce::Colours::black); }
        void resized() override;
    private:
        void timerCallback() override { panel.tick(); }
        Panel panel;
    };
}
