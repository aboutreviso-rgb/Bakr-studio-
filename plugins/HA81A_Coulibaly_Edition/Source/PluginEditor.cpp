#include "PluginEditor.h"

namespace
{
    class DarkMetalLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        DarkMetalLookAndFeel()
        {
            setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff258bd0));
            setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff080b10));
            setColour(juce::Slider::thumbColourId, juce::Colour(0xffe4e7eb));
        }
        void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                              float start, float end, juce::Slider&) override
        {
            auto r = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h)).reduced(9.0f);
            const float diameter = juce::jmin(r.getWidth(), r.getHeight());
            r = r.withSizeKeepingCentre(diameter, diameter);
            g.setColour(juce::Colour(0xff080b10)); g.fillEllipse(r.expanded(2.0f));
            g.setColour(juce::Colour(0xff505862)); g.drawEllipse(r, 2.0f);
            auto inner = r.reduced(5.0f);
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xffe3e6e9), inner.getX(), inner.getY(), juce::Colour(0xff626a73), inner.getRight(), inner.getBottom(), false));
            g.fillEllipse(inner);
            g.setColour(juce::Colour(0xff343b44)); g.drawEllipse(inner.reduced(4.0f), 1.0f);
            const float angle = start + pos * (end - start);
            juce::Path pointer;
            pointer.addRoundedRectangle(-2.2f, -inner.getHeight() * 0.36f, 4.4f, inner.getHeight() * 0.22f, 1.8f);
            g.setColour(juce::Colour(0xfff5f7fa));
            g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(inner.getCentreX(), inner.getCentreY()));
        }
    };
    DarkMetalLookAndFeel metalLook;
}

HA81ACoulibalyAudioProcessorEditor::HA81ACoulibalyAudioProcessorEditor(HA81ACoulibalyAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&metalLook);
    setSize(1000, 570);
    title.setText("HERITAGE-STYLE  |  HA-81A EQ  |  COULIBALY EDITION", juce::dontSendNotification);
    title.setFont(juce::Font(juce::FontOptions(22.0f).withStyle("Bold")));
    title.setColour(juce::Label::textColourId, juce::Colour(0xffe7e9ec));
    title.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(title);

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& s = knobs[i];
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 22);
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff111820));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff59616b));
        addAndMakeVisible(s);
        labels[i].setText(names[i], juce::dontSendNotification);
        labels[i].setJustificationType(juce::Justification::centred);
        labels[i].setColour(juce::Label::textColourId, juce::Colour(0xffd8dce1));
        labels[i].setFont(juce::Font(juce::FontOptions(13.0f).withStyle("Bold")));
        addAndMakeVisible(labels[i]);
        sliderAttachments[i] = std::make_unique<Attachment>(processor.parameters, ids[i], s);
    }

    const std::array<const char*, 3> switchNames { "BYPASS", "PHASE", "POWER" };
    const std::array<const char*, 3> switchIds { "bypass", "phase", "power" };
    for (size_t i = 0; i < switches.size(); ++i)
    {
        switches[i].setButtonText(switchNames[i]);
        switches[i].setClickingTogglesState(true);
        switches[i].setColour(juce::TextButton::buttonColourId, juce::Colour(0xff303943));
        switches[i].setColour(juce::TextButton::buttonOnColourId, i == 2 ? juce::Colour(0xffa51f2c) : juce::Colour(0xff176ca6));
        switches[i].setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        switches[i].setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        addAndMakeVisible(switches[i]);
        buttonAttachments[i] = std::make_unique<ButtonAttachment>(processor.parameters, switchIds[i], switches[i]);
    }
}

void HA81ACoulibalyAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff10151c));
    auto panel = getLocalBounds().toFloat().reduced(12.0f);
    g.setColour(juce::Colour(0xff202832)); g.fillRoundedRectangle(panel, 12.0f);
    g.setColour(juce::Colour(0xff56616d)); g.drawRoundedRectangle(panel, 12.0f, 1.2f);
    auto header = juce::Rectangle<float>(24.0f, 18.0f, 952.0f, 62.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff303943), header.getX(), header.getY(), juce::Colour(0xff151b22), header.getX(), header.getBottom(), false));
    g.fillRoundedRectangle(header, 7.0f);
    g.setColour(juce::Colour(0xffaab3bd));
    g.drawLine(26.0f, 91.0f, 974.0f, 91.0f, 1.0f);
    auto graph = juce::Rectangle<float>(34.0f, 112.0f, 932.0f, 175.0f);
    g.setColour(juce::Colour(0xff0b1118)); g.fillRoundedRectangle(graph, 6.0f);
    g.setColour(juce::Colour(0xff303b47));
    for (int i = 1; i < 8; ++i) g.drawVerticalLine(static_cast<int>(graph.getX() + graph.getWidth() * i / 8.0f), graph.getY(), graph.getBottom());
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine(static_cast<int>(graph.getY() + graph.getHeight() * i / 4.0f), graph.getX(), graph.getRight());
    g.setColour(juce::Colour(0xff8fa2b5)); g.drawHorizontalLine(static_cast<int>(graph.getCentreY()), graph.getX(), graph.getRight());
    juce::Path curve;
    curve.startNewSubPath(graph.getX(), graph.getCentreY());
    curve.cubicTo(graph.getX() + 170, graph.getCentreY() - 22, graph.getX() + 260, graph.getCentreY() + 8, graph.getX() + 390, graph.getCentreY());
    curve.cubicTo(graph.getX() + 560, graph.getCentreY() + 8, graph.getX() + 680, graph.getCentreY() - 60, graph.getX() + 790, graph.getCentreY() - 45);
    curve.cubicTo(graph.getX() + 850, graph.getCentreY() - 36, graph.getX() + 900, graph.getCentreY() - 8, graph.getRight(), graph.getCentreY() - 4);
    g.setColour(juce::Colour(0xff48a9e8)); g.strokePath(curve, juce::PathStrokeType(2.2f));
    g.setColour(juce::Colour(0xffaeb8c2)); g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("20 Hz", 34, 290, 60, 18, juce::Justification::left);
    g.drawText("100 Hz", 210, 290, 70, 18, juce::Justification::centred);
    g.drawText("1 kHz", 470, 290, 70, 18, juce::Justification::centred);
    g.drawText("10 kHz", 760, 290, 70, 18, juce::Justification::centred);
    g.drawText("20 kHz", 906, 290, 60, 18, juce::Justification::right);
    g.setColour(juce::Colour(0xff121820)); g.fillRoundedRectangle(26.0f, 320.0f, 948.0f, 225.0f, 8.0f);
    g.setColour(juce::Colour(0xff58636e)); g.drawRoundedRectangle(26.0f, 320.0f, 948.0f, 225.0f, 8.0f, 1.0f);
}

void HA81ACoulibalyAudioProcessorEditor::resized()
{
    title.setBounds(42, 28, 700, 42);
    const int startX = 34, colW = 86, gap = 7;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int x = startX + static_cast<int>(i) * (colW + gap);
        labels[i].setBounds(x, 337, colW, 22);
        knobs[i].setBounds(x, 358, colW, 166);
    }
    switches[0].setBounds(755, 30, 66, 36);
    switches[1].setBounds(830, 30, 66, 36);
    switches[2].setBounds(905, 30, 66, 36);
}
