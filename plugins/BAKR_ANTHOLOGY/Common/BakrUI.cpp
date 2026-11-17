#include "BakrUI.h"

namespace bakr
{
using F = juce::Rectangle<float>;
using P = juce::Point<float>;
static constexpr float kA0 = -0.75f * juce::MathConstants<float>::pi;
static constexpr float kA1 =  0.75f * juce::MathConstants<float>::pi;

static void shadowText (juce::Graphics& g, const juce::String& s, F r, float size, juce::Colour c,
                        bool bold = true, bool italic = false, juce::Justification j = juce::Justification::centred)
{
    int style = juce::Font::plain;
    if (bold) style |= juce::Font::bold;
    if (italic) style |= juce::Font::italic;
    g.setFont (juce::FontOptions (size, style));
    const bool darkText = c.getPerceivedBrightness() < 0.45f;
    g.setColour (darkText ? juce::Colours::white.withAlpha (0.25f) : juce::Colours::black.withAlpha (0.55f));
    g.drawText (s, r.translated (0.7f, 0.9f), j);
    g.setColour (c);
    g.drawText (s, r, j);
}

static void screw (juce::Graphics& g, P c, float r)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd5d9de), c.x - r, c.y - r, juce::Colour (0xff3a3e44), c.x + r, c.y + r, false));
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.0f);
    g.drawLine (c.x - r * 0.6f, c.y - r * 0.2f, c.x + r * 0.6f, c.y + r * 0.2f, 1.6f);
}

//==============================================================================
LookAndFeel::LookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff111214));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffe8e8e8));
    setColour (juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha (0.25f));
    setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffd0d0d0));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff17181b));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffeeeeee));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff5a4a2a));
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xee111214));
    setColour (juce::BubbleComponent::outlineColourId, juce::Colours::white.withAlpha (0.4f));
    setColour (juce::TooltipWindow::textColourId, juce::Colours::white);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                    float a0, float a1, juce::Slider& s)
{
    const auto b = F ((float) x, (float) y, (float) w, (float) h);
    const P c = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float ang = a0 + pos * (a1 - a0);
    const juce::String look = s.getProperties()["look"].toString();
    const juce::Colour tick = juce::Colour::fromString (s.getProperties()["tick"].toString());
    const bool selector = (bool) s.getProperties()["selector"];

    // graduations
    if (! selector)
    {
        g.setColour (tick.withAlpha (0.8f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = a0 + (a1 - a0) * (float) i / 10.0f;
            const float r1 = R * (i % 5 == 0 ? 0.84f : 0.89f);
            g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1,
                        c.x + std::sin (a) * R * 0.98f, c.y - std::cos (a) * R * 0.98f, i % 5 == 0 ? 1.6f : 1.0f);
        }
    }

    const float kr = R * 0.76f;
    // ombre portée
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - kr + 2, c.y - kr + 4, kr * 2, kr * 2);

    juce::Colour top, bottom, line = juce::Colours::white;
    bool skirt = true, metal = false;
    if      (look == "red")      { top = juce::Colour (0xffd8352c); bottom = juce::Colour (0xff7a120e); }
    else if (look == "blue")     { top = juce::Colour (0xff3d7fd6); bottom = juce::Colour (0xff12376e); }
    else if (look == "cream")    { top = juce::Colour (0xfff1e6cc); bottom = juce::Colour (0xffb9a982); line = juce::Colour (0xff222222); skirt = false; }
    else if (look == "grey")     { top = juce::Colour (0xff9aa0a8); bottom = juce::Colour (0xff4c5158); line = juce::Colours::white; skirt = false; }
    else if (look == "chrome" || look == "silver") { top = juce::Colour (0xffe1e5ea); bottom = juce::Colour (0xff4a4f57); metal = true; line = look == "silver" ? juce::Colour (0xff1a1a1a) : juce::Colours::white; }
    else if (look == "bakelite") { top = juce::Colour (0xff4a3a2e); bottom = juce::Colour (0xff120c08); }
    else                         { top = juce::Colour (0xff3e4247); bottom = juce::Colour (0xff060708); }

    if (skirt)
    {
        // jupe noire striée
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3d42), c.x, c.y - kr, juce::Colour (0xff050506), c.x, c.y + kr, false));
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        for (int i = 0; i < 36; ++i)
        {
            const float a = (float) i / 36.0f * juce::MathConstants<float>::twoPi;
            g.drawLine (c.x + std::sin (a) * kr * 0.80f, c.y - std::cos (a) * kr * 0.80f,
                        c.x + std::sin (a) * kr * 0.98f, c.y - std::cos (a) * kr * 0.98f, 0.8f);
        }
    }
    const float tr = skirt ? kr * 0.74f : kr;
    g.setGradientFill (juce::ColourGradient (top.brighter (0.25f), c.x - tr * 0.5f, c.y - tr, bottom, c.x + tr * 0.3f, c.y + tr, false));
    g.fillEllipse (c.x - tr, c.y - tr, tr * 2, tr * 2);
    if (metal)
        for (int i = 0; i < 16; ++i)
        {
            const float a = (float) i * juce::MathConstants<float>::pi / 8.0f;
            juce::Path wedge; wedge.startNewSubPath (c);
            wedge.addCentredArc (c.x, c.y, tr, tr, 0.0f, a, a + 0.18f, false); wedge.closeSubPath();
            g.setColour (juce::Colours::white.withAlpha (i % 2 ? 0.12f : 0.04f));
            g.fillPath (wedge);
        }
    // reflet
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.28f), c.x, c.y - tr,
                                             juce::Colours::transparentWhite, c.x, c.y, false));
    g.fillEllipse (c.x - tr * 0.85f, c.y - tr * 0.95f, tr * 1.7f, tr * 1.2f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (c.x - tr, c.y - tr, tr * 2, tr * 2, 1.0f);

    // index
    juce::Path idx;
    idx.addRoundedRectangle (-1.5f, -kr * 0.97f, 3.0f, kr * 0.48f, 1.2f);
    g.setColour (line);
    g.fillPath (idx, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    const auto kind = b.getProperties()["kind"].toString();
    const auto light = juce::Colour::fromString (b.getProperties()["light"].toString());
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (2.0f);

    if (kind == "lamp")
    {
        const float d = juce::jmin (r.getWidth(), r.getHeight());
        auto e = r.withSizeKeepingCentre (d, d);
        g.setColour (juce::Colour (0xff6a6e75));
        g.fillEllipse (e);
        auto in = e.reduced (d * 0.14f);
        if (on)
        {
            g.setColour (light.withAlpha (0.35f)); g.fillEllipse (e.expanded (d * 0.15f));
            g.setGradientFill (juce::ColourGradient (light.brighter (0.8f), in.getCentreX(), in.getCentreY() - 2, light.darker (0.4f), in.getRight(), in.getBottom(), true));
        }
        else g.setColour (light.darker (2.8f));
        g.fillEllipse (in);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.fillEllipse (in.getX() + in.getWidth() * 0.25f, in.getY() + in.getHeight() * 0.12f, in.getWidth() * 0.35f, in.getHeight() * 0.25f);
        return;
    }
    if (kind == "lever")
    {
        auto c = r.withSizeKeepingCentre (16, 16);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd0d5db), c.getX(), c.getY(), juce::Colour (0xff2b2f35), c.getRight(), c.getBottom(), false));
        g.fillEllipse (c);
        const float dy = on ? -12.0f : 12.0f;
        g.setColour (juce::Colour (0xffe8ecf0));
        g.drawLine (c.getCentreX(), c.getCentreY(), c.getCentreX(), c.getCentreY() + dy, 4.5f);
        g.fillEllipse (c.getCentreX() - 4, c.getCentreY() + dy - 4, 8, 8);
        return;
    }
    // bouton poussoir lumineux (style console)
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (0, 2), 4.0f);
    if (on)
    {
        g.setColour (light.withAlpha (0.3f)); g.fillRoundedRectangle (r.expanded (3), 6.0f);
        g.setGradientFill (juce::ColourGradient (light.brighter (0.5f), r.getCentreX(), r.getY(), light.darker (0.3f), r.getCentreX(), r.getBottom(), false));
    }
    else
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd8d8d4), r.getCentreX(), r.getY(), juce::Colour (0xff8c8c88), r.getCentreX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (juce::Colours::black.withAlpha (hl ? 0.8f : 0.5f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    g.setColour (on ? juce::Colours::white : juce::Colour (0xff222222));
    g.setFont (juce::FontOptions (juce::jmin (12.0f, r.getHeight() * 0.5f), juce::Font::bold));
    g.drawFittedText (b.getButtonText(), r.toNearestInt().reduced (2, 0), juce::Justification::centred, 2);
}

//==============================================================================
float Meter::read (Src s) const
{
    auto mx = [] (const std::array<std::atomic<float>, 2>& a) { return juce::jmax (a[0].load(), a[1].load()); };
    switch (s)
    {
        case Src::GrMax:  return mx (proc.grDb);
        case Src::GrL:    return proc.grDb[0].load();
        case Src::GrR:    return proc.grDb[1].load();
        case Src::InMax:  return mx (proc.inDb);
        case Src::OutMax: return mx (proc.outDb);
        case Src::InL:    return proc.inDb[0].load();
        case Src::InR:    return proc.inDb[1].load();
        case Src::OutL:   return proc.outDb[0].load();
        case Src::OutR:   return proc.outDb[1].load();
        default:          return 0.0f;
    }
}

void Meter::tick()
{
    Src s = spec.src;
    if (s == Src::Selectable && spec.selectParam != nullptr)
    {
        mode = (int) proc.p (spec.selectParam);
        s = mode == 0 ? Src::GrMax : (mode == 1 ? Src::InMax : Src::OutMax);
    }
    const bool gr = (s == Src::GrMax || s == Src::GrL || s == Src::GrR);
    mode = gr ? 0 : 1;
    const float raw = read (s);

    if (spec.face == "led")
    {
        const float target = raw;
        val2 = target > val2 ? target : val2 - 1.2f;
        peak = juce::jmax (val2, peak - 0.25f);
        repaint();
        return;
    }
    const float target = gr ? -raw : raw + 18.0f;
    const float old = val;
    val += 0.22f * (target - val);
    if (std::abs (old - val) > 0.01f) repaint();
}

void Meter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    if (spec.face == "led")
    {
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (b, 3.0f);
        const int nSeg = 24;
        auto area = b.reduced (4.0f, 3.0f);
        const float sw = area.getWidth() / (float) nSeg;
        for (int i = 0; i < nSeg; ++i)
        {
            const float db = -42.0f + 2.0f * (float) i;
            const auto colour = db >= 0.0f ? juce::Colour (0xffff3b2f) : (db >= -8.0f ? juce::Colour (0xffffc23a) : juce::Colour (0xff3ad06a));
            const bool lit = val2 >= db || (peak >= db && peak < db + 2.0f);
            g.setColour (lit ? colour : colour.withAlpha (0.12f));
            g.fillRoundedRectangle (area.getX() + (float) i * sw + 1.0f, area.getY(), sw - 2.0f, area.getHeight(), 1.5f);
        }
        return;
    }

    // boîtier
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff55595f), b.getX(), b.getY(), juce::Colour (0xff0b0c0e), b.getRight(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 6.0f);
    auto face = b.reduced (b.getWidth() * 0.05f, b.getHeight() * 0.07f);
    juce::Colour f1 (0xfff7efd8), f2 (0xffd9c9a0);
    if (spec.face == "amber") { f1 = juce::Colour (0xfffff0c4); f2 = juce::Colour (0xffe09a2e); }
    if (spec.face == "white") { f1 = juce::Colour (0xfffafafa); f2 = juce::Colour (0xffd6d8da); }
    g.setGradientFill (juce::ColourGradient (f1, face.getCentreX(), face.getBottom() - face.getHeight() * 0.2f, f2, face.getX(), face.getY(), true));
    g.fillRoundedRectangle (face, 3.0f);

    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (face.toNearestInt());
    const P pivot (face.getCentreX(), face.getBottom() + face.getHeight() * 0.32f);
    const float R = face.getHeight() * 1.10f;
    constexpr float aMin = -0.70f, aMax = 0.70f;
    auto angleFor = [&] (float vu)
    {
        const float lin = std::pow (10.0f, vu / 20.0f);
        return aMin + juce::jlimit (0.0f, 1.0f, (lin - 0.1f) / (1.4125f - 0.1f)) * (aMax - aMin);
    };
    const juce::Colour ink (0xff1c1712), red (0xffc8261c);
    const float fs = juce::jmax (8.0f, face.getHeight() * 0.11f);

    juce::Path arc; arc.addCentredArc (pivot.x, pivot.y, R * 0.80f, R * 0.80f, 0, aMin, angleFor (0), true);
    g.setColour (ink); g.strokePath (arc, juce::PathStrokeType (1.3f));
    juce::Path ra; ra.addCentredArc (pivot.x, pivot.y, R * 0.80f, R * 0.80f, 0, angleFor (0), aMax, true);
    g.setColour (red); g.strokePath (ra, juce::PathStrokeType (3.5f));

    const float marks[] = { -20, -10, -7, -5, -3, -1, 0, 1, 3 };
    g.setFont (juce::FontOptions (fs, juce::Font::bold));
    for (float m : marks)
    {
        const float a = angleFor (m), sa = std::sin (a), ca = std::cos (a);
        g.setColour (m > 0 ? red : ink);
        g.drawLine (pivot.x + sa * R * 0.80f, pivot.y - ca * R * 0.80f, pivot.x + sa * R * 0.88f, pivot.y - ca * R * 0.88f, 1.3f);
        const juce::String s = m > 0 ? "+" + juce::String ((int) m) : juce::String ((int) m);
        g.drawText (s, F (fs * 2.6f, fs * 1.2f).withCentre ({ pivot.x + sa * R * 0.96f, pivot.y - ca * R * 0.96f }), juce::Justification::centred);
    }
    g.setColour (ink);
    g.setFont (juce::FontOptions (fs * 1.6f, juce::Font::bold));
    g.drawText ("VU", face.withTrimmedTop (face.getHeight() * 0.50f).withHeight (fs * 2.0f), juce::Justification::centred);
    g.setFont (juce::FontOptions (fs * 0.8f, juce::Font::bold));
    g.setColour (ink.withAlpha (0.75f));
    juce::String cap = spec.caption;
    if (spec.src == Src::Selectable) cap = mode == 0 ? "GAIN REDUCTION" : cap;
    g.drawText (cap, face.withTrimmedTop (face.getHeight() * 0.74f).withHeight (fs * 1.2f), juce::Justification::centred);

    const float a = angleFor (juce::jlimit (-22.0f, 4.0f, val));
    g.setColour (juce::Colours::black.withAlpha (0.2f));
    g.drawLine (pivot.x + std::sin (a) * R * 0.42f + 2, pivot.y - std::cos (a) * R * 0.42f + 2,
                pivot.x + std::sin (a) * R * 0.92f + 2, pivot.y - std::cos (a) * R * 0.92f + 2, 1.6f);
    g.setColour (juce::Colour (0xff101010));
    g.drawLine (pivot.x + std::sin (a) * R * 0.42f, pivot.y - std::cos (a) * R * 0.42f,
                pivot.x + std::sin (a) * R * 0.92f, pivot.y - std::cos (a) * R * 0.92f, 1.6f);
    g.setColour (juce::Colours::white.withAlpha (0.13f));
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.3f), 3.0f);
}

//==============================================================================
Panel::Panel (BaseProcessor& p, UiSpec s) : proc (p), spec (std::move (s))
{
    setLookAndFeel (&lnf);
    for (auto& cs : spec.controls)
    {
        auto it = std::make_unique<Item>();
        it->spec = cs;
        if (cs.type == Ctl::Knob || cs.type == Ctl::Selector)
        {
            it->slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
            auto& sl = *it->slider;
            sl.setRotaryParameters (kA0, kA1, true);
            sl.getProperties().set ("look", cs.look);
            sl.getProperties().set ("tick", (cs.labelColour.getAlpha() > 0 ? cs.labelColour : spec.theme.text).toString());
            sl.getProperties().set ("selector", cs.type == Ctl::Selector);
            if (cs.type == Ctl::Knob) sl.setPopupDisplayEnabled (true, true, this);
            sl.setBounds (juce::Rectangle<float> (cs.size, cs.size).withCentre ({ cs.x, cs.y }).toNearestInt());
            if (auto* prm = proc.apvts.getParameter (cs.id))
                sl.setDoubleClickReturnValue (true, proc.apvts.getParameterRange (cs.id).convertFrom0to1 (prm->getDefaultValue()));
            addAndMakeVisible (sl);
            it->sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, cs.id, sl);
        }
        else
        {
            it->button = std::make_unique<juce::ToggleButton> (cs.type == Ctl::Button ? cs.label : juce::String());
            auto& bt = *it->button;
            bt.getProperties().set ("kind", cs.type == Ctl::Lamp ? "lamp" : (cs.type == Ctl::Lever ? "lever" : "button"));
            bt.getProperties().set ("light", cs.light.toString());
            const float hgt = cs.type == Ctl::Button ? cs.size * 0.55f : (cs.type == Ctl::Lever ? 44.0f : cs.size);
            bt.setBounds (juce::Rectangle<float> (cs.size, hgt).withCentre ({ cs.x, cs.y }).toNearestInt());
            addAndMakeVisible (bt);
            it->bAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, cs.id, bt);
        }
        items.push_back (std::move (it));
    }
    for (auto& ms : spec.meters)
    {
        auto m = std::make_unique<Meter> (proc, ms);
        m->setBounds (juce::Rectangle<float> (ms.x, ms.y, ms.w, ms.h).toNearestInt());
        addAndMakeVisible (*m);
        meters.push_back (std::move (m));
    }
    presets.addItemList (proc.getPresetNames(), 1);
    presets.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    presets.onChange = [this] { if (presets.getSelectedItemIndex() >= 0) proc.setCurrentProgram (presets.getSelectedItemIndex()); };
    presets.setBounds (spec.presetArea);
    addAndMakeVisible (presets);
    setSize (spec.w, spec.h);
}

Panel::~Panel() { setLookAndFeel (nullptr); }

void Panel::buildBackground()
{
    const float sc = 2.0f;
    bg = juce::Image (juce::Image::ARGB, (int) ((float) spec.w * sc), (int) ((float) spec.h * sc), true);
    juce::Graphics g (bg);
    g.addTransform (juce::AffineTransform::scale (sc));
    const auto& t = spec.theme;
    const F all (0, 0, (float) spec.w, (float) spec.h);

    g.setColour (juce::Colours::black);
    g.fillRect (all);
    F plate = all;
    if (t.rackEars)
    {
        for (auto ear : { all.withWidth (34.0f), all.withTrimmedLeft (all.getWidth() - 34.0f) })
        {
            g.setGradientFill (juce::ColourGradient (t.earColour.brighter (0.25f), ear.getX(), 0, t.earColour.darker (0.5f), ear.getRight(), 0, false));
            g.fillRect (ear);
            for (float yy : { 26.0f, all.getHeight() - 26.0f })
            {
                auto slot = F (12, 20).withCentre ({ ear.getCentreX(), yy });
                g.setColour (juce::Colours::black);
                g.fillRoundedRectangle (slot, 6.0f);
                screw (g, slot.getCentre(), 5.5f);
            }
        }
        plate = all.reduced (34.0f, 0.0f);
    }
    g.setGradientFill (juce::ColourGradient (t.plateTop, 0, plate.getY(), t.plateBottom, 0, plate.getBottom(), false));
    g.fillRect (plate);
    if (t.brushed)
    {
        juce::Random rnd (42);
        for (float y = plate.getY(); y < plate.getBottom(); y += 0.75f)
        {
            g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (rnd.nextFloat() * 0.035f));
            g.drawHorizontalLine ((int) y, plate.getX(), plate.getRight());
        }
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), 0, plate.getY(), juce::Colours::transparentWhite, 0, plate.getY() + 50, false));
    g.fillRect (plate.withHeight (50));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRect (plate, 1.5f);

    for (auto& bx : spec.boxes)
    {
        const F r (bx.x, bx.y, bx.w, bx.h);
        g.setColour (bx.fill); g.fillRoundedRectangle (r, bx.radius);
        g.setColour (bx.line); g.drawRoundedRectangle (r, bx.radius, 1.2f);
    }
    for (auto& tx : spec.texts)
        shadowText (g, tx.text, { tx.x, tx.y, tx.w, tx.h }, tx.size, tx.colour, tx.bold, tx.italic, tx.just);
}

void Panel::drawItemText (juce::Graphics& g, const Item& it)
{
    const auto& cs = it.spec;
    const auto& t = spec.theme;
    if (cs.type == Ctl::Button) return;
    const bool custom = cs.labelColour.getAlpha() > 0;
    const juce::Colour txt = custom ? cs.labelColour : t.text;
    const juce::Colour dim = custom ? cs.labelColour.withAlpha (0.75f) : t.dim;
    const juce::Colour acc = custom ? cs.labelColour.contrasting (0.0f).interpolatedWith (juce::Colour (0xffb01c10), 0.85f) : t.accent;

    const float R = cs.size * 0.5f;
    const float gap = cs.labelGap != 0.0f ? cs.labelGap : (cs.type == Ctl::Selector && cs.marks.size() > 2 ? R + 34.0f : R + 16.0f);
    if (cs.label.isNotEmpty())
        shadowText (g, cs.label, F (cs.x - 90, cs.y - gap - 8, 180, 16), 13.0f, txt);

    if (cs.type == Ctl::Knob)
    {
        auto at = [&] (float a) { return P (cs.x + std::sin (a) * R * 1.02f, cs.y - std::cos (a) * R * 1.02f + 9.0f); };
        if (cs.minTxt.isNotEmpty()) shadowText (g, cs.minTxt, F (56, 14).withCentre (at (kA0)), 10.5f, dim);
        if (cs.maxTxt.isNotEmpty()) shadowText (g, cs.maxTxt, F (56, 14).withCentre (at (kA1)), 10.5f, dim);
        return;
    }
    if (cs.type == Ctl::Selector && it.slider != nullptr)
    {
        const int n = cs.marks.size();
        const int sel = (int) std::round (it.slider->getValue());
        const float span = n <= 2 ? 0.55f : (n <= 4 ? 0.8f : 1.0f);
        for (int i = 0; i < n; ++i)
        {
            const float a = n == 1 ? 0.0f : kA0 * span + (kA1 - kA0) * span * (float) i / (float) (n - 1);
            const float rr = R + 10.0f;
            const P pnt (cs.x + std::sin (a) * rr, cs.y - std::cos (a) * rr);
            const float sa = std::sin (a);
            const float w = 70.0f;
            F area = F (w, 13).withCentre (pnt);
            auto j = juce::Justification::centred;
            if (sa < -0.3f) { area = area.withX (pnt.x - w + 6.0f); j = juce::Justification::centredRight; }
            else if (sa > 0.3f) { area = area.withX (pnt.x - 6.0f); j = juce::Justification::centredLeft; }
            shadowText (g, cs.marks[i], area, 10.5f, i == sel ? acc : dim, true, false, j);
            // petit point de position
            g.setColour (i == sel ? acc : dim.withAlpha (0.6f));
            const P d (cs.x + std::sin (a) * (R + 1.0f), cs.y - std::cos (a) * (R + 1.0f));
            g.fillEllipse (d.x - 1.5f, d.y - 1.5f, 3.0f, 3.0f);
        }
    }
    if (cs.type == Ctl::Lamp || cs.type == Ctl::Lever)
    {
        if (cs.label.isEmpty()) return;
    }
}

void Panel::paint (juce::Graphics& g)
{
    if (bg.isNull()) buildBackground();
    g.drawImage (bg, getLocalBounds().toFloat());
    for (auto& it : items) drawItemText (g, *it);
}

void Panel::tick()
{
    for (auto& m : meters) m->tick();
    if (presets.getSelectedItemIndex() != proc.getCurrentProgram())
        presets.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    juce::int64 h = 0;
    for (auto& it : items)
        if (it->spec.type == Ctl::Selector) h = h * 13 + (juce::int64) std::round (it->slider->getValue());
    if (h != lastHash) { lastHash = h; repaint(); }
}

//==============================================================================
Editor::Editor (BaseProcessor& p, UiSpec s) : AudioProcessorEditor (&p), panel (p, std::move (s))
{
    addAndMakeVisible (panel);
    const auto& sp = panel.getSpec();
    setResizable (true, true);
    setResizeLimits (sp.w / 2, sp.h / 2, sp.w * 2, sp.h * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) sp.w / (double) sp.h);
    setSize (sp.w, sp.h);
    startTimerHz (30);
}

void Editor::resized()
{
    const auto& sp = panel.getSpec();
    panel.setBounds (0, 0, sp.w, sp.h);
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) sp.w));
}
}
