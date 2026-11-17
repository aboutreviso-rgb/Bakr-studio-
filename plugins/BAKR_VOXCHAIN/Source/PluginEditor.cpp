#include "PluginEditor.h"

using namespace bvc;
using juce::Colour; using juce::Graphics; using juce::Rectangle; using juce::Path;

namespace
{
constexpr int kW = 960, kH = 600;
const Colour cBg   { 0xff121212 };
const Colour cEdge { 0xff38332c };
const Colour cText { 0xffebe4d6 };
const Colour cDim  { 0xff958d80 };
const Colour cGold { 0xffd4aa63 };
const std::array<Colour, 5> accents { Colour (0xffe0b25c), Colour (0xffe58a4e), Colour (0xff7fcf8a), Colour (0xff5cc8c8), Colour (0xffb08ee8) };
const char* colNames[5] = { "COMPRESSION", "AMELIORATION", "EQ", "DOUBLEUR", "ESPACE" };
}

//==============================================================================
LnF::LnF()
{
    setColour (juce::Label::textColourId, cText);
    setColour (juce::ComboBox::textColourId, cText);
    setColour (juce::ComboBox::arrowColourId, cGold);
    setColour (juce::PopupMenu::backgroundColourId, Colour (0xff1d1b19));
    setColour (juce::PopupMenu::textColourId, cText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colour (0xff7a6440));
}

juce::Label* LnF::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::FontOptions (11.0f));
    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::textColourId, cDim);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return l;
}

static Colour accentOf (juce::Component& c)
{
    auto v = c.getProperties()["accent"];
    return v.isVoid() ? cGold : Colour ((juce::uint32) (juce::int64) v);
}

void LnF::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    const Colour ac = accentOf (s).withMultipliedAlpha (s.isEnabled() ? 1.0f : 0.35f);
    auto r = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4);
    float d = juce::jmin (r.getWidth(), r.getHeight());
    auto c = r.getCentre();
    float ang = a0 + pos * (a1 - a0);
    Path track; track.addCentredArc (c.x, c.y, d * 0.5f, d * 0.5f, 0, a0, a1, true);
    g.setColour (Colour (0xff2a2724)); g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    Path val; val.addCentredArc (c.x, c.y, d * 0.5f, d * 0.5f, 0, a0, ang, true);
    g.setColour (ac.withAlpha (0.25f)); g.strokePath (val, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (ac); g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    auto body = Rectangle<float> (d * 0.68f, d * 0.68f).withCentre (c);
    juce::ColourGradient gr (Colour (0xff3a3632), body.getX(), body.getY(), Colour (0xff111010), body.getRight(), body.getBottom(), false);
    g.setGradientFill (gr); g.fillEllipse (body);
    g.setColour (Colour (0xff4c4740)); g.drawEllipse (body, 1.0f);
    float rr = body.getWidth() * 0.5f;
    g.setColour (cText);
    g.drawLine (c.x + std::sin (ang) * rr * 0.35f, c.y - std::cos (ang) * rr * 0.35f,
                c.x + std::sin (ang) * rr * 0.85f, c.y - std::cos (ang) * rr * 0.85f, 2.0f);
}

void LnF::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const Colour ac = accentOf (s).withMultipliedAlpha (s.isEnabled() ? 1.0f : 0.35f);
    float cy = (float) y + (float) h * 0.5f;
    g.setColour (Colour (0xff2a2724)); g.fillRoundedRectangle ((float) x, cy - 2, (float) w, 4, 2);
    g.setColour (ac); g.fillRoundedRectangle ((float) x, cy - 2, pos - (float) x, 4, 2);
    g.setColour (ac.withAlpha (0.3f)); g.fillEllipse (pos - 9, cy - 9, 18, 18);
    g.setColour (ac.brighter (0.4f)); g.fillEllipse (pos - 6, cy - 6, 12, 12);
}

void LnF::drawButtonBackground (Graphics& g, juce::Button& b, const Colour&, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const Colour ac = accentOf (b);
    const bool on = b.getToggleState();
    g.setColour (on ? ac.withAlpha (0.22f) : (over ? Colour (0xff252220) : Colour (0xff1b1a18)));
    g.fillRoundedRectangle (r, 4);
    g.setColour (on ? ac : cEdge); g.drawRoundedRectangle (r, 4, 1);
}

void LnF::drawButtonText (Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (juce::FontOptions (11.5f));
    g.setColour (b.getToggleState() ? accentOf (b).brighter (0.3f) : cDim);
    g.drawFittedText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1);
}

void LnF::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (Colour (0xff0d0d0c)); g.fillRoundedRectangle (r, 4);
    g.setColour (cEdge); g.drawRoundedRectangle (r, 4, 1);
    Path a; float ax = (float) w - 12, ay = (float) h * 0.5f;
    a.startNewSubPath (ax - 4, ay - 2); a.lineTo (ax, ay + 2); a.lineTo (ax + 4, ay - 2);
    g.setColour (cGold); g.strokePath (a, juce::PathStrokeType (1.5f));
}

//==============================================================================
void PowerButton::paintButton (Graphics& g, bool over, bool)
{
    auto r = getLocalBounds().toFloat().reduced (3);
    const bool on = getToggleState();
    Colour c = on ? accent : (over ? cDim : cDim.withAlpha (0.5f));
    Path p; p.addCentredArc (r.getCentreX(), r.getCentreY(), r.getWidth() * 0.42f, r.getHeight() * 0.42f, 0, 0.6f, 6.2831853f - 0.6f, true);
    g.setColour (c); g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.drawLine (r.getCentreX(), r.getY(), r.getCentreX(), r.getCentreY(), 1.8f);
}

ChoiceStepper::ChoiceStepper (juce::RangedAudioParameter& p, juce::StringArray names) : items (std::move (names))
{
    att = std::make_unique<juce::ParameterAttachment> (p, [this] (float v) { index = juce::roundToInt (v); repaint(); });
    att->sendInitialUpdate();
    prev.onClick = [this] { att->setValueAsCompleteGesture ((float) ((index + items.size() - 1) % items.size())); };
    next.onClick = [this] { att->setValueAsCompleteGesture ((float) ((index + 1) % items.size())); };
    for (auto* b : { &prev, &next }) { b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack); addAndMakeVisible (b); }
}
void ChoiceStepper::resized() { prev.setBounds (0, 0, 24, getHeight()); next.setBounds (getWidth() - 24, 0, 24, getHeight()); }
void ChoiceStepper::paint (Graphics& g)
{
    g.setColour (cText); g.setFont (juce::FontOptions (12.5f));
    g.drawText (juce::isPositiveAndBelow (index, items.size()) ? items[index] : juce::String(), getLocalBounds().reduced (24, 0), juce::Justification::centred);
}

void EqView::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0b0a)); g.fillRoundedRectangle (r, 4);
    g.setColour (cEdge); g.drawRoundedRectangle (r.reduced (0.5f), 4, 1);
    auto p = r.reduced (6, 6);
    g.setColour (cDim.withAlpha (0.2f)); g.drawHorizontalLine ((int) p.getCentreY(), p.getX(), p.getRight());
    Path path;
    const float v = value.load();
    for (int i = 0; i <= 60; ++i)
    {
        float t = (float) i / 60.0f;                // 20 Hz -> 20 kHz (log)
        float f = 20.0f * std::pow (1000.0f, t);
        float db = 0;
        if (band == 0) db = v / (1.0f + std::pow (8000.0f / f, 2.0f));                       // étagère haute
        if (band == 1) db = v * std::exp (-std::pow (std::log2 (f / 1500.0f) / 1.1f, 2.0f)); // cloche
        if (band == 2) { float fc = 20.0f * std::pow (15.0f, v * 0.01f); db = -40.0f / (1.0f + std::pow (f / fc, 4.0f)); }
        float y = p.getCentreY() - juce::jlimit (-1.0f, 1.0f, db / (band == 2 ? 24.0f : 12.0f)) * p.getHeight() * 0.48f;
        if (band == 2) y = p.getY() + 4 - db / 40.0f * (p.getHeight() - 6);
        float x = p.getX() + t * p.getWidth();
        if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
    }
    g.setColour (accent.withAlpha (0.25f)); g.strokePath (path, juce::PathStrokeType (5.0f));
    g.setColour (accent); g.strokePath (path, juce::PathStrokeType (1.8f));
}

void DoublerView::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b0b0a)); g.fillRoundedRectangle (r, 4);
    g.setColour (cEdge); g.drawRoundedRectangle (r.reduced (0.5f), 4, 1);
    auto p = r.reduced (10, 8);
    const float s = spread.load() * 0.01f, a = amount.load() * 0.01f;
    const float cx = p.getCentreX();
    g.setColour (accent); g.fillRect (cx - 1.5f, p.getY(), 3.0f, p.getHeight());   // voix principale
    static const float pan[4] = { -1.0f, -0.45f, 0.45f, 1.0f };
    for (int v = 0; v < 4; ++v)
    {
        float x = cx + pan[v] * s * p.getWidth() * 0.46f;
        float h = p.getHeight() * (0.15f + 0.6f * a) * (std::abs (pan[v]) > 0.9f ? 0.8f : 1.0f);
        g.setColour (accent.withAlpha (0.85f)); g.fillRect (x - 1.5f, p.getBottom() - h, 3.0f, h);
        for (int k = 1; k <= 2; ++k)
        {
            float hx = x + (pan[v] < 0 ? -1.0f : 1.0f) * (float) k * 6.0f * s;
            g.setColour (accent.withAlpha (0.35f)); g.fillRect (hx - 1, p.getBottom() - h * 0.5f, 2.0f, h * 0.5f);
        }
    }
    g.setColour (cDim.withAlpha (0.4f)); g.drawHorizontalLine ((int) p.getBottom(), p.getX(), p.getRight());
}

void GRMeter::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto bar = r.removeFromTop (8).reduced (12, 0);
    g.setColour (Colour (0xff0b0b0a)); g.fillRoundedRectangle (bar, 3);
    float v = juce::jlimit (0.0f, 18.0f, value.load());
    float w = bar.getWidth() * v / 18.0f;
    g.setColour (accent); g.fillRoundedRectangle (bar.withX (bar.getRight() - w).withWidth (w), 3);
    g.setColour (cDim); g.setFont (juce::FontOptions (9.5f));
    const char* marks[] = { "-18", "-12", "-6", "0" };
    for (int i = 0; i < 4; ++i)
        g.drawText (marks[i], (int) (bar.getX() + bar.getWidth() * (float) i / 3.0f) - 12, (int) r.getY() + 2, 24, 12, juce::Justification::centred);
}

void Icon::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (8);
    Colour c = active ? accent : cDim.withAlpha (0.4f);
    // halo doux derrière le pictogramme
    if (active)
    {
        juce::ColourGradient halo (accent.withAlpha (0.18f), r.getCentreX(), r.getCentreY(), accent.withAlpha (0.0f), r.getCentreX() + r.getWidth() * 0.5f, r.getCentreY(), true);
        g.setGradientFill (halo); g.fillEllipse (r.expanded (6));
    }
    g.setColour (c);
    juce::PathStrokeType st (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    const float cx = r.getCentreX(), cy = r.getCentreY(), s = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    Path p;
    switch (kind)
    {
        case 0: // compression : onde serrée entre deux barres
            g.drawLine (cx - s, cy - s * 0.6f, cx + s, cy - s * 0.6f, 2.4f);
            g.drawLine (cx - s, cy + s * 0.6f, cx + s, cy + s * 0.6f, 2.4f);
            p.startNewSubPath (cx - s * 0.9f, cy);
            for (float t = 0; t <= 1.0f; t += 0.02f)
            {
                float env = 0.15f + 0.35f * std::abs (std::sin (t * 3.14159f));
                p.lineTo (cx - s * 0.9f + t * s * 1.8f, cy + std::sin (t * 40.0f) * s * env);
            }
            g.strokePath (p, st); break;
        case 1: // amélioration : étincelle + micro
        {
            auto mic = Rectangle<float> (cx - s * 0.28f, cy - s * 0.85f, s * 0.56f, s * 1.0f);
            g.drawRoundedRectangle (mic, s * 0.28f, 2.4f);
            p.addCentredArc (cx, cy - s * 0.15f, s * 0.48f, s * 0.48f, 0, 1.9f, 4.38f, true);
            g.strokePath (p, st);
            g.drawLine (cx, cy + s * 0.33f, cx, cy + s * 0.75f, 2.4f);
            g.drawLine (cx - s * 0.3f, cy + s * 0.75f, cx + s * 0.3f, cy + s * 0.75f, 2.4f);
            for (int k = 0; k < 3; ++k)
            {
                float sx = cx + s * (0.65f + 0.2f * (float) k), sy = cy - s * (0.8f - 0.35f * (float) k), z = s * (0.14f - 0.03f * (float) k);
                g.drawLine (sx - z, sy, sx + z, sy, 1.8f); g.drawLine (sx, sy - z, sx, sy + z, 1.8f);
            }
            break;
        }
        case 2: // EQ : trois courbes empilées
            for (int k = 0; k < 3; ++k)
            {
                Path q; float y0 = cy - s * 0.6f + (float) k * s * 0.6f;
                q.startNewSubPath (cx - s, y0);
                float px = cx - s * 0.5f + (float) k * s * 0.5f;
                q.lineTo (px - s * 0.25f, y0);
                q.quadraticTo (px, y0 - s * 0.45f, px + s * 0.25f, y0);
                q.lineTo (cx + s, y0);
                g.strokePath (q, st);
            }
            break;
        case 3: // doubleur : trois ondes superposées décalées
            for (int k = -1; k <= 1; ++k)
            {
                Path q; float ox = (float) k * s * 0.18f, oy = (float) k * s * 0.35f;
                q.startNewSubPath (cx - s + ox, cy + oy);
                for (float t = 0; t <= 1.0f; t += 0.02f) q.lineTo (cx - s + ox + t * s * 2.0f * 0.9f, cy + oy + std::sin (t * 12.0f) * s * 0.2f);
                g.setColour (c.withAlpha (k == 0 ? 1.0f : 0.55f)); g.strokePath (q, st);
            }
            break;
        default: // espace : ondes qui s'éloignent
            g.fillEllipse (cx - s * 0.12f - s * 0.6f, cy - s * 0.12f, s * 0.24f, s * 0.24f);
            for (int k = 1; k <= 4; ++k)
            {
                Path q; float rad = s * 0.35f * (float) k;
                q.addCentredArc (cx - s * 0.6f, cy, rad, rad, 0, 0.6f, 2.55f, true);
                g.setColour (c.withAlpha (1.0f - 0.18f * (float) k)); g.strokePath (q, st);
            }
    }
}

//==============================================================================
void BakrVoxChainEditor::Root::paint (Graphics& g)
{
    g.fillAll (cBg);
    juce::ColourGradient hg (Colour (0xff1f1d1a), 0, 0, Colour (0xff141312), 0, 56, false);
    g.setGradientFill (hg); g.fillRect (0, 0, kW, 56);
    g.setColour (cEdge); g.drawHorizontalLine (55, 0, (float) kW);
    g.setColour (cGold); g.drawRoundedRectangle (16, 12, 32, 32, 6, 1.5f);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold)); g.drawText ("B", 16, 12, 32, 32, juce::Justification::centred);
    g.setColour (cText); g.setFont (juce::FontOptions (20.0f, juce::Font::bold)); g.drawText ("BAKR", 58, 8, 70, 24, juce::Justification::centredLeft);
    g.setColour (cGold); g.setFont (juce::FontOptions (20.0f)); g.drawText ("VOXCHAIN", 116, 8, 130, 24, juce::Justification::centredLeft);
    g.setColour (cDim); g.setFont (juce::FontOptions (9.5f));
    g.drawText ("CHAINE VOCALE COMPLETE EN 5 MODULES", 58, 32, 260, 14, juce::Justification::centredLeft);

    for (int i = 0; i < 5; ++i)
    {
        auto rf = cols[(size_t) i].toFloat();
        const Colour ac = accents[(size_t) i];
        juce::ColourGradient pg (ac.withAlpha (on[(size_t) i] ? 0.10f : 0.03f).overlaidWith (Colour (0xff1b1a18)), rf.getX(), rf.getY(),
                                 Colour (0xff151413), rf.getX(), rf.getBottom(), false);
        g.setGradientFill (pg); g.fillRoundedRectangle (rf, 8);
        g.setColour (cEdge); g.drawRoundedRectangle (rf.reduced (0.5f), 8, 1);
        g.setColour (on[(size_t) i] ? ac : cDim); g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (colNames[i], cols[(size_t) i].withHeight (34).withTrimmedLeft (34), juce::Justification::centredLeft);
        g.setColour (cEdge); g.drawHorizontalLine ((int) rf.getY() + 158, rf.getX() + 16, rf.getRight() - 16);
    }
}

juce::Slider* BakrVoxChainEditor::knob (const char* id, const juce::String& label, Colour c)
{
    auto* s = sliders.add (new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow));
    s->getProperties().set ("accent", (juce::int64) c.getARGB());
    s->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    root.addAndMakeVisible (s);
    s->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 70, 14);
    sAtts.add (new SA (proc.apvts, id, *s));
    auto* l = labels.add (new juce::Label ({}, label));
    l->setJustificationType (juce::Justification::centred); l->setFont (juce::FontOptions (12.5f));
    root.addAndMakeVisible (l);
    sl[id] = s; lb[id] = l;
    return s;
}

juce::Slider* BakrVoxChainEditor::hslider (const char* id, Colour c)
{
    auto* s = sliders.add (new juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox));
    s->getProperties().set ("accent", (juce::int64) c.getARGB());
    root.addAndMakeVisible (s);
    sAtts.add (new SA (proc.apvts, id, *s));
    sl[id] = s;
    return s;
}

juce::TextButton* BakrVoxChainEditor::toggle (const char* id, const juce::String& text, Colour c)
{
    auto* b = toggles.add (new juce::TextButton (text));
    b->setClickingTogglesState (true);
    b->getProperties().set ("accent", (juce::int64) c.getARGB());
    root.addAndMakeVisible (b);
    bAtts.add (new BA (proc.apvts, id, *b));
    tg[id] = b;
    return b;
}

BakrVoxChainEditor::BakrVoxChainEditor (BakrVoxChainProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (root);
    for (int i = 0; i < 5; ++i) root.accents[(size_t) i] = accents[(size_t) i];

    const char* onIds[5] = { "comp_on", "enh_on", "eq_on", "mul_on", "spc_on" };
    for (int i = 0; i < 5; ++i)
    {
        auto* pb = powers.add (new PowerButton (accents[(size_t) i]));
        root.addAndMakeVisible (pb);
        bAtts.add (new BA (proc.apvts, onIds[i], *pb));
        auto* ic = icons.add (new Icon (i, accents[(size_t) i]));
        root.addAndMakeVisible (ic);
    }

    const auto a0 = accents[0], a1 = accents[1], a2 = accents[2], a3 = accents[3], a4 = accents[4];
    grMeter = std::make_unique<GRMeter> (proc.gainReduction, a0); root.addAndMakeVisible (*grMeter);
    knob ("comp_in", juce::String::fromUTF8 ("Entr\xc3\xa9" "e"), a0);
    knob ("comp_amt", "Compression", a0);
    knob ("comp_out", "Sortie", a0);
    toggle ("comp_slow", "Lent", a0);
    toggle ("comp_auto", "Auto", a0);

    knob ("deess", "De-Ess", a1);
    knob ("denoise", "De-Noise", a1);
    knob ("air", "Air", a1);
    toggle ("enh_pre", "Avant comp", a1);

    const char* eqIds[3] = { "eq_high", "eq_mid", "eq_low" };
    const char* eqNames[3] = { "Aigus", "M\xc3\xa9" "diums", "Coupe-bas" };
    for (int i = 0; i < 3; ++i)
    {
        auto* v = eqViews.add (new EqView (*proc.apvts.getRawParameterValue (eqIds[i]), i, a2));
        root.addAndMakeVisible (v);
        hslider (eqIds[i], a2);
        auto* l = labels.add (new juce::Label ({}, juce::String::fromUTF8 (eqNames[i])));
        l->setJustificationType (juce::Justification::centred); l->setFont (juce::FontOptions (12.0f));
        root.addAndMakeVisible (l); lb[eqIds[i]] = l;
    }
    toggle ("eq_pre", juce::String::fromUTF8 ("Pr\xc3\xa9"), a2);
    toggle ("eq_adlib", "Ad-lib", a2);

    dblView = std::make_unique<DoublerView> (*proc.apvts.getRawParameterValue ("mul_spread"), *proc.apvts.getRawParameterValue ("mul_amount"), a3);
    root.addAndMakeVisible (*dblView);
    hslider ("mul_spread", a3);
    {
        auto* l = labels.add (new juce::Label ({}, "Largeur"));
        l->setJustificationType (juce::Justification::centred); l->setFont (juce::FontOptions (12.0f));
        root.addAndMakeVisible (l); lb["mul_spread"] = l;
    }
    knob ("mul_offset", juce::String::fromUTF8 ("D\xc3\xa9" "calage"), a3);
    knob ("mul_amount", juce::String::fromUTF8 ("Quantit\xc3\xa9"), a3);
    knob ("mul_low", "Graves au centre", a3);

    knob ("rev_time", "Taille", a4);
    knob ("rev_mix", "Reverb", a4);
    knob ("dly_mix", "Delay", a4);
    revType = std::make_unique<ChoiceStepper> (*proc.apvts.getParameter ("rev_type"), BakrVoxChainProcessor::reverbTypes());
    dlyType = std::make_unique<ChoiceStepper> (*proc.apvts.getParameter ("dly_type"), BakrVoxChainProcessor::delayTypes());
    root.addAndMakeVisible (*revType); root.addAndMakeVisible (*dlyType);
    juce::StringArray dt; for (auto& s : BakrVoxChainProcessor::delayTimes()) dt.add (s.replace ("pointee", juce::String::fromUTF8 ("point\xc3\xa9" "e")));
    dlyTime.addItemList (dt, 1);
    dlyTime.setJustificationType (juce::Justification::centred);
    root.addAndMakeVisible (dlyTime);
    dlyTimeAtt = std::make_unique<CA> (proc.apvts, "dly_time", dlyTime);

    presetBox.addItemList (BakrVoxChainProcessor::getPresetNames(), 1);
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.setSelectedItemIndex (proc.currentPreset.load(), juce::dontSendNotification);
    presetBox.onChange = [this] { int i = presetBox.getSelectedItemIndex(); if (i >= 0 && i != proc.currentPreset.load()) proc.applyPreset (i); };
    prevBtn.onClick = [this] { int n = proc.getNumPrograms(); proc.applyPreset ((proc.currentPreset.load() + n - 1) % n); };
    nextBtn.onClick = [this] { int n = proc.getNumPrograms(); proc.applyPreset ((proc.currentPreset.load() + 1) % n); };
    root.addAndMakeVisible (presetBox); root.addAndMakeVisible (prevBtn); root.addAndMakeVisible (nextBtn);

    sendLookAndFeelChange();
    setResizable (true, true);
    setResizeLimits (kW * 6 / 10, kH * 6 / 10, kW * 2, kH * 2);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize (kW, kH);
    startTimerHz (30);
}

BakrVoxChainEditor::~BakrVoxChainEditor() { stopTimer(); setLookAndFeel (nullptr); }
void BakrVoxChainEditor::paint (Graphics& g) { g.fillAll (cBg); }

void BakrVoxChainEditor::resized()
{
    root.setBounds (0, 0, kW, kH);
    root.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) kW));

    prevBtn.setBounds (360, 14, 28, 28);
    presetBox.setBounds (392, 14, 240, 28);
    nextBtn.setBounds (636, 14, 28, 28);

    for (int i = 0; i < 5; ++i)
    {
        root.cols[(size_t) i] = { 12 + i * 188, 66, 180, 522 };
        auto c = root.cols[(size_t) i];
        powers[i]->setBounds (c.getX() + 8, c.getY() + 6, 22, 22);
        icons[i]->setBounds (c.getX() + 30, c.getY() + 40, 120, 108);
    }

    auto placeKnob = [this] (const char* id, int cx, int y)
    {
        sl[id]->setBounds (cx - 38, y, 76, 74);
        lb[id]->setBounds (cx - 60, y + 74, 120, 16);
    };

    // compression
    auto c0 = root.cols[0]; int cx0 = c0.getX() + 72;
    grMeter->setBounds (c0.getX() + 16, c0.getY() + 170, c0.getWidth() - 32, 24);
    placeKnob ("comp_in", cx0, c0.getY() + 204);
    placeKnob ("comp_amt", cx0, c0.getY() + 300);
    placeKnob ("comp_out", cx0, c0.getY() + 396);
    tg["comp_slow"]->setBounds (c0.getRight() - 56, c0.getY() + 330, 46, 22);
    tg["comp_auto"]->setBounds (c0.getRight() - 56, c0.getY() + 426, 46, 22);

    // amélioration
    auto c1 = root.cols[1]; int cx1 = c1.getCentreX();
    placeKnob ("deess", cx1, c1.getY() + 172);
    placeKnob ("denoise", cx1, c1.getY() + 266);
    placeKnob ("air", cx1, c1.getY() + 360);
    tg["enh_pre"]->setBounds (cx1 - 48, c1.getY() + 478, 96, 26);

    // EQ
    auto c2 = root.cols[2];
    const char* eqIds[3] = { "eq_high", "eq_mid", "eq_low" };
    for (int i = 0; i < 3; ++i)
    {
        int y = c2.getY() + 172 + i * 100;
        eqViews[i]->setBounds (c2.getX() + 14, y, c2.getWidth() - 28, 52);
        sl[eqIds[i]]->setBounds (c2.getX() + 14, y + 54, c2.getWidth() - 28, 20);
        lb[eqIds[i]]->setBounds (c2.getX() + 14, y + 74, c2.getWidth() - 28, 16);
    }
    tg["eq_pre"]->setBounds (c2.getX() + 16, c2.getY() + 478, 68, 26);
    tg["eq_adlib"]->setBounds (c2.getRight() - 84, c2.getY() + 478, 68, 26);

    // doubleur
    auto c3 = root.cols[3];
    dblView->setBounds (c3.getX() + 14, c3.getY() + 172, c3.getWidth() - 28, 56);
    sl["mul_spread"]->setBounds (c3.getX() + 14, c3.getY() + 230, c3.getWidth() - 28, 20);
    lb["mul_spread"]->setBounds (c3.getX() + 14, c3.getY() + 250, c3.getWidth() - 28, 16);
    placeKnob ("mul_offset", c3.getX() + 48, c3.getY() + 280);
    placeKnob ("mul_amount", c3.getRight() - 48, c3.getY() + 280);
    placeKnob ("mul_low", c3.getCentreX(), c3.getY() + 384);

    // espace
    auto c4 = root.cols[4];
    placeKnob ("rev_time", c4.getX() + 48, c4.getY() + 172);
    placeKnob ("rev_mix", c4.getRight() - 48, c4.getY() + 172);
    revType->setBounds (c4.getX() + 12, c4.getY() + 266, c4.getWidth() - 24, 24);
    dlyTime.setBounds (c4.getX() + 8, c4.getY() + 318, 96, 24);
    placeKnob ("dly_mix", c4.getRight() - 40, c4.getY() + 304);
    dlyType->setBounds (c4.getX() + 12, c4.getY() + 410, c4.getWidth() - 24, 24);
}

void BakrVoxChainEditor::timerCallback()
{
    grMeter->tick();
    for (auto* v : eqViews) v->tick();
    dblView->tick();
    const char* onIds[5] = { "comp_on", "enh_on", "eq_on", "mul_on", "spc_on" };
    bool changed = false;
    for (int i = 0; i < 5; ++i)
    {
        bool on = proc.apvts.getRawParameterValue (onIds[i])->load() > 0.5f;
        icons[i]->setActive (on);
        if (root.on[(size_t) i] != on) { root.on[(size_t) i] = on; changed = true; }
    }
    if (changed) root.repaint();
    if (presetBox.getSelectedItemIndex() != proc.currentPreset.load())
        presetBox.setSelectedItemIndex (proc.currentPreset.load(), juce::dontSendNotification);
}
