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
    // ombre douce (plusieurs couches, lumière venant d'en haut à gauche)
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        const float e = (float) i * 1.6f;
        g.fillEllipse (c.x - kr - e * 0.4f + 2.0f, c.y - kr - e * 0.2f + 4.0f, (kr + e * 0.4f) * 2, (kr + e * 0.5f) * 2);
    }

    juce::Colour top, bottom, line = juce::Colours::white;
    bool skirt = true, metal = false;
    if      (look == "red")      { top = juce::Colour (0xffd9372d); bottom = juce::Colour (0xff6e0f0b); }
    else if (look == "blue")     { top = juce::Colour (0xff4f94ea); bottom = juce::Colour (0xff12346b); }
    else if (look == "teal")     { top = juce::Colour (0xff45bdb6); bottom = juce::Colour (0xff0b4b48); }
    else if (look == "cream")    { top = juce::Colour (0xfff4ead2); bottom = juce::Colour (0xffa99773); line = juce::Colour (0xff222222); skirt = false; }
    else if (look == "grey")     { top = juce::Colour (0xffa3a9b1); bottom = juce::Colour (0xff474c53); line = juce::Colours::white; skirt = false; }
    else if (look == "chrome" || look == "silver") { top = juce::Colour (0xffe6eaef); bottom = juce::Colour (0xff474c54); metal = true; line = look == "silver" ? juce::Colour (0xff1a1a1a) : juce::Colours::white; }
    else if (look == "bakelite") { top = juce::Colour (0xff4d3c30); bottom = juce::Colour (0xff0e0906); }
    else                         { top = juce::Colour (0xff45494f); bottom = juce::Colour (0xff050506); }

    if (skirt)
    {
        // jupe noire moletée avec biseau
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4d53), c.x - kr * 0.6f, c.y - kr, juce::Colour (0xff030304), c.x + kr * 0.4f, c.y + kr, false));
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
        for (int i = 0; i < 60; ++i)
        {
            const float a = (float) i / 60.0f * juce::MathConstants<float>::twoPi;
            const float lightness = 0.5f + 0.5f * std::cos (a + 0.8f); // côté éclairé en haut à gauche
            g.setColour ((i % 2 ? juce::Colours::white : juce::Colours::black).withAlpha (i % 2 ? 0.05f + 0.13f * lightness : 0.35f));
            g.drawLine (c.x + std::sin (a) * kr * 0.84f, c.y - std::cos (a) * kr * 0.84f,
                        c.x + std::sin (a) * kr * 0.985f, c.y - std::cos (a) * kr * 0.985f, 0.9f);
        }
        juce::Path rimHi; rimHi.addCentredArc (c.x, c.y, kr - 0.6f, kr - 0.6f, 0.0f, -2.2f, 0.3f, true);
        g.setColour (juce::Colours::white.withAlpha (0.22f));
        g.strokePath (rimHi, juce::PathStrokeType (1.0f));
    }
    const float tr = skirt ? kr * 0.74f : kr;

    // corps du capuchon
    g.setGradientFill (juce::ColourGradient (top.brighter (0.3f), c.x - tr * 0.6f, c.y - tr, bottom, c.x + tr * 0.4f, c.y + tr, false));
    g.fillEllipse (c.x - tr, c.y - tr, tr * 2, tr * 2);
    if (metal)
    {
        // aluminium usiné : reflets coniques
        for (int i = 0; i < 24; ++i)
        {
            const float a = (float) i * juce::MathConstants<float>::twoPi / 24.0f;
            juce::Path wedge; wedge.startNewSubPath (c);
            wedge.addCentredArc (c.x, c.y, tr, tr, 0.0f, a, a + 0.13f, false); wedge.closeSubPath();
            const float k = std::pow (std::abs (std::cos (a - 0.7f)), 6.0f);
            g.setColour (juce::Colours::white.withAlpha (0.04f + 0.30f * k));
            g.fillPath (wedge);
        }
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        for (float rr = tr * 0.15f; rr < tr; rr += tr * 0.09f)
            g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 0.5f);
    }
    else
    {
        // plastique : point lumineux spéculaire
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.45f), c.x - tr * 0.35f, c.y - tr * 0.45f,
                                                 juce::Colours::transparentWhite, c.x - tr * 0.35f + tr * 0.9f, c.y - tr * 0.45f, true));
        g.fillEllipse (c.x - tr, c.y - tr, tr * 2, tr * 2);
        // lumière d'appoint en bas à droite (rebond)
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentWhite, c.x, c.y,
                                                 juce::Colours::white.withAlpha (0.10f), c.x + tr * 0.7f, c.y + tr * 0.7f, true));
        g.fillEllipse (c.x - tr, c.y - tr, tr * 2, tr * 2);
    }
    // biseau du capuchon
    juce::Path hi; hi.addCentredArc (c.x, c.y, tr - 0.7f, tr - 0.7f, 0.0f, -2.4f, 0.5f, true);
    g.setColour (juce::Colours::white.withAlpha (0.35f)); g.strokePath (hi, juce::PathStrokeType (1.1f));
    juce::Path lo; lo.addCentredArc (c.x, c.y, tr - 0.7f, tr - 0.7f, 0.0f, 0.8f, 3.6f, true);
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.strokePath (lo, juce::PathStrokeType (1.2f));

    // index gravé
    juce::Path idx;
    idx.addRoundedRectangle (-1.6f, -kr * 0.98f, 3.2f, kr * 0.50f, 1.3f);
    const auto tf = juce::AffineTransform::rotation (ang).translated (c.x, c.y);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillPath (idx, tf.translated (0.6f, 0.9f));
    g.setColour (line);
    g.fillPath (idx, tf);
}

void LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                    juce::Slider::SliderStyle, juce::Slider& s)
{
    const auto tick = juce::Colour::fromString (s.getProperties()["tick"].toString());
    const F b ((float) x, (float) y, (float) w, (float) h);
    const float cx = b.getCentreX();
    // fente
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (F (cx - 3.0f, b.getY(), 6.0f, b.getHeight()), 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawRoundedRectangle (F (cx - 3.0f, b.getY(), 6.0f, b.getHeight()), 3.0f, 0.8f);
    // graduations
    g.setColour (tick.withAlpha (0.7f));
    for (int i = 0; i <= 10; ++i)
    {
        const float yy = b.getY() + b.getHeight() * (float) i / 10.0f;
        const float len = i % 5 == 0 ? 10.0f : 6.0f;
        g.drawLine (cx - 14.0f - len, yy, cx - 14.0f, yy, 1.0f);
        g.drawLine (cx + 14.0f, yy, cx + 14.0f + len, yy, 1.0f);
    }
    // capuchon de fader
    const float capH = 34.0f, capW = 30.0f;
    const F cap = F (capW, capH).withCentre ({ cx, pos });
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (cap.translated (2, 3), 3.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9ecef), cap.getX(), cap.getY(), juce::Colour (0xff6b7077), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 3.0f);
    for (int i = 1; i < 6; ++i)
    {
        const float yy = cap.getY() + cap.getHeight() * (float) i / 6.0f;
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawLine (cap.getX() + 3, yy, cap.getRight() - 3, yy, 0.8f);
    }
    g.setColour (juce::Colours::black);
    g.drawLine (cap.getX() + 2, cap.getCentreY(), cap.getRight() - 2, cap.getCentreY(), 2.0f);
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
        case Src::Extra0: return proc.extra[0].load();
        case Src::Extra1: return proc.extra[1].load();
        case Src::Extra2: return proc.extra[2].load();
        case Src::Extra3: return proc.extra[3].load();
        default:          return 0.0f;
    }
}

void Meter::tick()
{
    Src s = spec.src;
    if (s == Src::Selectable && spec.selectParam != nullptr)
    {
        sel = (int) proc.p (spec.selectParam);
        s = sel == 0 ? Src::GrMax : (sel == 1 ? Src::InMax : Src::OutMax);
    }
    const bool gr = (s == Src::GrMax || s == Src::GrL || s == Src::GrR);
    mode = gr ? 0 : 1;
    const float raw = read (s);

    if (spec.face == "readout")
    {
        if (std::abs (raw - val2) > 0.04f) { val2 = raw; repaint(); }
        return;
    }
    if (spec.face == "bar")
    {
        val2 = raw > val2 ? raw : val2 + 0.15f * (raw - val2);
        peak = juce::jmax (val2, peak - 0.08f);
        repaint();
        return;
    }

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

    if (spec.face == "readout")
    {
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (b, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawRoundedRectangle (b.reduced (0.5f), 4.0f, 1.0f);
        const bool none = val2 <= -59.0f;
        g.setColour (juce::Colour (0xffffb347));
        g.setFont (juce::FontOptions (b.getHeight() * 0.55f, juce::Font::bold));
        g.drawText (none ? juce::String ("--.-") : juce::String (val2, 1), b.withTrimmedRight (b.getWidth() * 0.3f), juce::Justification::centredRight);
        g.setColour (juce::Colour (0xffb0b4ba));
        g.setFont (juce::FontOptions (b.getHeight() * 0.30f, juce::Font::bold));
        g.drawText (spec.unit, b.withTrimmedLeft (b.getWidth() * 0.72f), juce::Justification::centredLeft);
        g.drawText (spec.caption, b.withHeight (b.getHeight() * 0.32f).withTrimmedLeft (6.0f), juce::Justification::centredLeft);
        return;
    }
    if (spec.face == "bar")
    {
        // barre de réduction de gain (0 à 20 dB) de gauche à droite, LED orange
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (b, 3.0f);
        const int nSeg = 20;
        auto area = b.reduced (3.0f, 3.0f);
        const float sw = area.getWidth() / (float) nSeg;
        for (int i = 0; i < nSeg; ++i)
        {
            const bool lit = val2 >= (float) i + 0.5f || (peak >= (float) i + 0.5f && peak < (float) i + 1.5f);
            const auto colour = juce::Colour (0xffffa030);
            g.setColour (lit ? colour : colour.withAlpha (0.12f));
            g.fillRoundedRectangle (area.getX() + (float) i * sw + 1.0f, area.getY(), sw - 2.0f, area.getHeight(), 1.5f);
        }
        return;
    }
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

    // boîtier : cadre noir chanfreiné
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (b.translated (2.0f, 4.0f), 7.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5d6168), b.getX(), b.getY(), juce::Colour (0xff08090a), b.getRight(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 7.0f);
    auto bez = b.reduced (b.getWidth() * 0.025f, b.getHeight() * 0.035f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff050506), bez.getX(), bez.getY(), juce::Colour (0xff2c2f34), bez.getRight(), bez.getBottom(), false));
    g.fillRoundedRectangle (bez, 5.0f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawRoundedRectangle (b.reduced (0.6f), 7.0f, 0.8f);
    auto face = b.reduced (b.getWidth() * 0.05f, b.getHeight() * 0.07f);
    juce::Colour f1 (0xfffff4d8), f2 (0xffcdb98a);
    if (spec.face == "amber") { f1 = juce::Colour (0xfffff2c2); f2 = juce::Colour (0xffd88a22); }
    if (spec.face == "white") { f1 = juce::Colour (0xfffdfcf6); f2 = juce::Colour (0xffcfd2d4); }
    // rétroéclairage chaud venant du bas
    g.setGradientFill (juce::ColourGradient (f1, face.getCentreX(), face.getBottom() - face.getHeight() * 0.15f, f2, face.getX(), face.getY(), true));
    g.fillRoundedRectangle (face, 3.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0x40ffb040), face.getCentreX(), face.getBottom(),
                                             juce::Colour (0x00ffb040), face.getCentreX(), face.getY(), true));
    g.fillRoundedRectangle (face, 3.0f);
    // ombre intérieure sous le cadre
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.35f), 0, face.getY(), juce::Colours::transparentBlack, 0, face.getY() + face.getHeight() * 0.18f, false));
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
    if (spec.src == Src::Selectable && sel >= 0) cap = sel == 0 ? "GAIN REDUCTION" : (sel == 1 ? "INPUT LEVEL" : "OUTPUT LEVEL");
    g.drawText (cap, face.withTrimmedTop (face.getHeight() * 0.74f).withHeight (fs * 1.2f), juce::Justification::centred);

    const float a = angleFor (juce::jlimit (-22.0f, 4.0f, val));
    g.setColour (juce::Colours::black.withAlpha (0.2f));
    g.drawLine (pivot.x + std::sin (a) * R * 0.42f + 2, pivot.y - std::cos (a) * R * 0.42f + 2,
                pivot.x + std::sin (a) * R * 0.92f + 2, pivot.y - std::cos (a) * R * 0.92f + 2, 1.6f);
    g.setColour (juce::Colour (0xff101010));
    g.drawLine (pivot.x + std::sin (a) * R * 0.42f, pivot.y - std::cos (a) * R * 0.42f,
                pivot.x + std::sin (a) * R * 0.92f, pivot.y - std::cos (a) * R * 0.92f, 1.6f);
    // reflet du verre en diagonale
    juce::Path glare;
    glare.startNewSubPath (face.getX(), face.getY());
    glare.lineTo (face.getX() + face.getWidth() * 0.62f, face.getY());
    glare.lineTo (face.getX() + face.getWidth() * 0.30f, face.getBottom());
    glare.lineTo (face.getX(), face.getBottom());
    glare.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.20f), face.getX(), face.getY(),
                                             juce::Colours::white.withAlpha (0.02f), face.getX() + face.getWidth() * 0.4f, face.getBottom(), false));
    g.fillPath (glare);
}

//==============================================================================
Panel::Panel (BaseProcessor& p, UiSpec s) : proc (p), spec (std::move (s))
{
    setLookAndFeel (&lnf);
    for (auto& cs : spec.controls)
    {
        auto it = std::make_unique<Item>();
        it->spec = cs;
        if (cs.type == Ctl::Fader)
        {
            it->slider = std::make_unique<juce::Slider> (juce::Slider::LinearVertical, juce::Slider::NoTextBox);
            auto& sl = *it->slider;
            sl.getProperties().set ("tick", (cs.labelColour.getAlpha() > 0 ? cs.labelColour : spec.theme.text).toString());
            sl.setPopupDisplayEnabled (true, true, this);
            sl.setBounds (juce::Rectangle<float> (70.0f, cs.size).withCentre ({ cs.x, cs.y }).toNearestInt());
            if (auto* prm = proc.apvts.getParameter (cs.id))
                sl.setDoubleClickReturnValue (true, proc.apvts.getParameterRange (cs.id).convertFrom0to1 (prm->getDefaultValue()));
            addAndMakeVisible (sl);
            it->sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, cs.id, sl);
        }
        else if (cs.type == Ctl::Knob || cs.type == Ctl::Selector)
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
        else if (cs.type == Ctl::Radio)
        {
            it->button = std::make_unique<juce::ToggleButton> (cs.label);
            auto& bt = *it->button;
            bt.getProperties().set ("kind", "button");
            bt.getProperties().set ("light", cs.light.toString());
            bt.setClickingTogglesState (false);
            bt.setBounds (juce::Rectangle<float> (cs.size, cs.size * 0.55f).withCentre ({ cs.x, cs.y }).toNearestInt());
            const juce::String pid (cs.id);
            const int v = cs.radioValue;
            bt.onClick = [this, pid, v]
            {
                if (auto* prm = proc.apvts.getParameter (pid))
                {
                    prm->beginChangeGesture();
                    prm->setValueNotifyingHost (prm->convertTo0to1 ((float) v));
                    prm->endChangeGesture();
                }
            };
            addAndMakeVisible (bt);
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
            // oreille de rack en acier brossé
            g.setGradientFill (juce::ColourGradient (t.earColour.brighter (0.35f), ear.getX(), 0, t.earColour.darker (0.6f), ear.getRight(), 0, false));
            g.fillRect (ear);
            juce::Random rr (7);
            for (float y = 0; y < ear.getHeight(); y += 0.8f)
            {
                g.setColour (juce::Colours::white.withAlpha (rr.nextFloat() * 0.05f));
                g.drawHorizontalLine ((int) y, ear.getX(), ear.getRight());
            }
            for (float yy : { 26.0f, all.getHeight() - 26.0f })
            {
                auto slot = F (13, 22).withCentre ({ ear.getCentreX(), yy });
                g.setColour (juce::Colours::black);
                g.fillRoundedRectangle (slot, 6.5f);
                g.setColour (juce::Colours::white.withAlpha (0.18f));
                g.drawRoundedRectangle (slot.translated (0, 0.8f), 6.5f, 0.8f);
                // vis cruciforme
                const P sc2 = slot.getCentre();
                g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe2e5e9), sc2.x - 5, sc2.y - 5, juce::Colour (0xff3b3f45), sc2.x + 5, sc2.y + 5, false));
                g.fillEllipse (sc2.x - 5.5f, sc2.y - 5.5f, 11, 11);
                g.setColour (juce::Colour (0xff1e2124));
                g.drawLine (sc2.x - 3.2f, sc2.y, sc2.x + 3.2f, sc2.y, 1.5f);
                g.drawLine (sc2.x, sc2.y - 3.2f, sc2.x, sc2.y + 3.2f, 1.5f);
            }
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawVerticalLine ((int) ear.getRight() - 1, 0, ear.getHeight());
        }
        plate = all.reduced (34.0f, 0.0f);
    }
    g.setGradientFill (juce::ColourGradient (t.plateTop, 0, plate.getY(), t.plateBottom, 0, plate.getBottom(), false));
    g.fillRect (plate);

    juce::Random rnd (42);
    const int texture = t.texture != 0 ? t.texture : (t.brushed ? 0 : 2);
    if (texture == 0)
    {
        for (float y = plate.getY(); y < plate.getBottom(); y += 0.75f)
        {
            g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (rnd.nextFloat() * 0.035f));
            g.drawHorizontalLine ((int) y, plate.getX(), plate.getRight());
        }
    }
    else
    {
        // peinture martelée / poudre : taches douces + grain fin
        const int blotches = texture == 1 ? 900 : 120;
        for (int i = 0; i < blotches; ++i)
        {
            const float x = plate.getX() + rnd.nextFloat() * plate.getWidth();
            const float y = plate.getY() + rnd.nextFloat() * plate.getHeight();
            const float r = texture == 1 ? 3.0f + rnd.nextFloat() * 9.0f : 20.0f + rnd.nextFloat() * 60.0f;
            const bool light = rnd.nextBool();
            const auto c0 = (light ? juce::Colours::white : juce::Colours::black).withAlpha (texture == 1 ? 0.07f : 0.03f);
            g.setGradientFill (juce::ColourGradient (c0, x, y, c0.withAlpha (0.0f), x + r, y, true));
            g.fillEllipse (x - r, y - r, r * 2, r * 2);
        }
        const int grains = (int) (plate.getWidth() * plate.getHeight() / (texture == 1 ? 9.0f : 14.0f));
        for (int i = 0; i < grains; ++i)
        {
            g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.02f + rnd.nextFloat() * 0.05f));
            g.fillRect (plate.getX() + rnd.nextFloat() * plate.getWidth(), plate.getY() + rnd.nextFloat() * plate.getHeight(), 0.7f, 0.7f);
        }
    }
    // lumière de studio : reflet en haut, vignettage sur les bords
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.12f), 0, plate.getY(), juce::Colours::transparentWhite, 0, plate.getY() + plate.getHeight() * 0.35f, false));
    g.fillRect (plate);
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, plate.getCentreX(), plate.getCentreY() - plate.getHeight() * 0.1f,
                                             juce::Colours::black.withAlpha (0.28f), plate.getX(), plate.getBottom(), true));
    g.fillRect (plate);
    // biseau de la façade
    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.drawHorizontalLine ((int) plate.getY() + 1, plate.getX(), plate.getRight());
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawVerticalLine ((int) plate.getX() + 1, plate.getY(), plate.getBottom());
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawHorizontalLine ((int) plate.getBottom() - 1, plate.getX(), plate.getRight());
    g.drawVerticalLine ((int) plate.getRight() - 1, plate.getY(), plate.getBottom());

    for (auto& bx : spec.boxes)
    {
        const F r (bx.x, bx.y, bx.w, bx.h);
        g.setColour (bx.fill); g.fillRoundedRectangle (r, bx.radius);
        if (bx.deco == 1)
        {
            juce::Graphics::ScopedSaveState clip (g);
            g.reduceClipRegion (r.reduced (2.0f).toNearestInt());
            // lampes en verre avec filament orange
            const int nT = juce::jmax (2, (int) (r.getWidth() / 62.0f));
            const float tw = r.getWidth() / (float) nT;
            for (int i = 0; i < nT; ++i)
            {
                auto tb = F (r.getX() + tw * (float) i, r.getY(), tw, r.getHeight()).reduced (tw * 0.18f, 6.0f);
                g.setGradientFill (juce::ColourGradient (juce::Colour (0x88ff8a2a), tb.getCentreX(), tb.getBottom() - tb.getHeight() * 0.25f,
                                                         juce::Colour (0x00ff8a2a), tb.getCentreX(), tb.getY() - 10, true));
                g.fillEllipse (tb.expanded (tw * 0.25f, 0.0f));
                g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.28f), tb.getX(), tb.getY(),
                                                         juce::Colours::white.withAlpha (0.04f), tb.getRight(), tb.getY(), false));
                g.fillRoundedRectangle (tb, tb.getWidth() * 0.45f);
                auto plate = tb.reduced (tb.getWidth() * 0.28f, tb.getHeight() * 0.22f);
                g.setColour (juce::Colour (0xff5d5a55)); g.fillRect (plate);
                g.setColour (juce::Colour (0xff2b2926)); g.drawRect (plate, 1.0f);
                g.setColour (juce::Colour (0xffffa040));
                g.fillRect (plate.withSizeKeepingCentre (2.5f, plate.getHeight() * 0.8f));
                g.setColour (juce::Colours::white.withAlpha (0.35f));
                g.drawRoundedRectangle (tb, tb.getWidth() * 0.45f, 1.0f);
                g.setColour (juce::Colour (0xff1c1a17));
                g.fillRect (tb.withTop (tb.getBottom() - 8.0f).expanded (3.0f, 0.0f));
            }
        }
        if (bx.deco == 2)
        {
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1f5fc8), r.getX(), r.getY(), juce::Colour (0xff0d2f74), r.getX(), r.getBottom(), false));
            g.fillRoundedRectangle (r.reduced (2.0f), bx.radius);
        }
        g.setColour (bx.line); g.drawRoundedRectangle (r, bx.radius, 1.2f);
        // effet gravé : ombre en haut, lumière en bas
        g.setColour (juce::Colours::black.withAlpha (0.30f));
        g.drawLine (r.getX() + bx.radius, r.getY() + 1.0f, r.getRight() - bx.radius, r.getY() + 1.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawLine (r.getX() + bx.radius, r.getBottom() + 1.0f, r.getRight() - bx.radius, r.getBottom() + 1.0f, 1.0f);
    }
    for (auto& tx : spec.texts)
    {
        if (tx.vertical)
        {
            juce::Graphics::ScopedSaveState ss (g);
            const P c (tx.x + tx.w * 0.5f, tx.y + tx.h * 0.5f);
            g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, c.x, c.y));
            shadowText (g, tx.text, F (tx.h, tx.w).withCentre (c), tx.size, tx.colour, tx.bold, tx.italic, tx.just);
        }
        else
            shadowText (g, tx.text, { tx.x, tx.y, tx.w, tx.h }, tx.size, tx.colour, tx.bold, tx.italic, tx.just);
    }
}

void Panel::drawItemText (juce::Graphics& g, const Item& it)
{
    const auto& cs = it.spec;
    const auto& t = spec.theme;
    if (cs.type == Ctl::Button || cs.type == Ctl::Radio) return;
    const bool custom = cs.labelColour.getAlpha() > 0;
    const juce::Colour txt = custom ? cs.labelColour : t.text;
    const juce::Colour dim = custom ? cs.labelColour.withAlpha (0.75f) : t.dim;
    const juce::Colour acc = custom ? cs.labelColour.contrasting (0.0f).interpolatedWith (juce::Colour (0xffb01c10), 0.85f) : t.accent;

    if (cs.type == Ctl::Fader)
    {
        shadowText (g, cs.label, F (cs.x - 60, cs.y - cs.size * 0.5f - 34, 120, 16), 13.0f, txt);
        shadowText (g, cs.maxTxt, F (cs.x + 24, cs.y - cs.size * 0.5f + 8, 40, 12), 10.0f, dim, true, false, juce::Justification::centredLeft);
        shadowText (g, cs.minTxt, F (cs.x + 24, cs.y + cs.size * 0.5f - 22, 40, 12), 10.0f, dim, true, false, juce::Justification::centredLeft);
        return;
    }
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
    juce::int64 h = 0;
    for (auto& it : items)
    {
        if (it->spec.type == Ctl::Selector) h = h * 13 + (juce::int64) std::round (it->slider->getValue());
        if (it->spec.type == Ctl::Radio)
            it->button->setToggleState ((int) std::round (proc.p (it->spec.id)) == it->spec.radioValue, juce::dontSendNotification);
    }
    if (h != lastHash) { lastHash = h; repaint(); }
}

//==============================================================================
Editor::Editor (BaseProcessor& p, UiSpec s) : AudioProcessorEditor (&p), proc (p), panel (p, std::move (s))
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (panel);

    presets.addItemList (proc.getPresetNames(), 1);
    presets.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    presets.onChange = [this] { if (presets.getSelectedItemIndex() >= 0) proc.setCurrentProgram (presets.getSelectedItemIndex()); };
    addAndMakeVisible (presets);

    auto styleBtn = [this] (juce::TextButton& b)
    {
        b.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff23252a));
        b.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffc99a3e));
        b.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffd8d8d8));
        b.setColour (juce::TextButton::textColourOnId, juce::Colour (0xff141414));
        addAndMakeVisible (b);
    };
    for (auto* b : { &aBtn, &bBtn, &copyBtn, &prevBtn, &nextBtn }) styleBtn (*b);
    aBtn.setClickingTogglesState (false); bBtn.setClickingTogglesState (false);
    aBtn.setToggleState (true, juce::dontSendNotification);
    aBtn.onClick = [this] { selectSlot (0); };
    bBtn.onClick = [this] { selectSlot (1); };
    copyBtn.onClick = [this] { slots[1 - cur] = proc.apvts.copyState(); };
    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + n - 1) % n); };
    nextBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % n); };
    aBtn.setTooltip ("Reglage A"); bBtn.setTooltip ("Reglage B (pour comparer)"); copyBtn.setTooltip ("Copier le reglage actuel dans l'autre");

    const auto& sp = panel.getSpec();
    const int H = sp.h + barH;
    setResizable (true, true);
    setResizeLimits (sp.w / 2, H / 2, sp.w * 2, H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) sp.w / (double) H);
    setSize (sp.w, H);
    startTimerHz (30);
}

void Editor::selectSlot (int slot)
{
    if (slot == cur) return;
    slots[cur] = proc.apvts.copyState();
    cur = slot;
    if (slots[cur].isValid()) proc.apvts.replaceState (slots[cur].createCopy());
    else slots[cur] = proc.apvts.copyState();
    copyBtn.setButtonText (cur == 0 ? "A > B" : "B > A");
    aBtn.setToggleState (cur == 0, juce::dontSendNotification);
    bBtn.setToggleState (cur == 1, juce::dontSendNotification);
}

void Editor::timerCallback()
{
    panel.tick();
    if (presets.getSelectedItemIndex() != proc.getCurrentProgram())
        presets.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
}

void Editor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    const float s = (float) getWidth() / (float) panel.getSpec().w;
    const auto bar = getLocalBounds().toFloat().withTop ((float) panel.getSpec().h * s);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2c31), 0, bar.getY(), juce::Colour (0xff111214), 0, bar.getBottom(), false));
    g.fillRect (bar);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawHorizontalLine ((int) bar.getY(), 0, (float) getWidth());
    g.setColour (juce::Colour (0xffc99a3e));
    g.setFont (juce::FontOptions (17.0f * s, juce::Font::bold | juce::Font::italic));
    g.drawText ("BAKR", bar.withX (14.0f * s).withWidth (80.0f * s), juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff9a9ca2));
    g.setFont (juce::FontOptions (10.5f * s, juce::Font::bold));
    g.drawText (proc.getName().replace ("BAKR ", ""), bar.withX (70.0f * s).withWidth (200.0f * s), juce::Justification::centredLeft);
}

void Editor::resized()
{
    const auto& sp = panel.getSpec();
    const float s = (float) getWidth() / (float) sp.w;
    panel.setBounds (0, 0, sp.w, sp.h);
    panel.setTransform (juce::AffineTransform::scale (s));
    const int barY = (int) std::round ((float) sp.h * s);
    const int bh = getHeight() - barY;
    auto row = juce::Rectangle<int> (0, barY, getWidth(), bh).reduced ((int) (8 * s), (int) (7 * s));
    const int cx = getWidth() / 2;
    const int pw = (int) (300 * s), bw = (int) (26 * s);
    prevBtn.setBounds (cx - pw / 2 - bw - 4, row.getY(), bw, row.getHeight());
    presets.setBounds (cx - pw / 2, row.getY(), pw, row.getHeight());
    nextBtn.setBounds (cx + pw / 2 + 4, row.getY(), bw, row.getHeight());
    const int ab = (int) (34 * s), cw = (int) (58 * s);
    copyBtn.setBounds (row.getRight() - cw, row.getY(), cw, row.getHeight());
    bBtn.setBounds (copyBtn.getX() - ab - 4, row.getY(), ab, row.getHeight());
    aBtn.setBounds (bBtn.getX() - ab - 4, row.getY(), ab, row.getHeight());
}
}
