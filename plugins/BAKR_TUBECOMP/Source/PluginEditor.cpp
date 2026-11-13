#include "PluginEditor.h"

namespace col
{
    const juce::Colour neon    (0xff1e8cff);
    const juce::Colour neonHi  (0xff7fd0ff);
    const juce::Colour text    (0xffeef2f7);
    const juce::Colour dim     (0xffa9b3c0);
    const juce::Colour gold    (0xffe0a63a);
    const juce::Colour amber   (0xffffb347);
    const juce::Colour orange  (0xffff8a1c);
}

namespace
{
    using F = juce::Rectangle<float>;

    void glowArc (juce::Graphics& g, juce::Point<float> c, float r, float a0, float a1, float width, juce::Colour colour)
    {
        juce::Path p;
        p.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a1, true);
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (colour.withAlpha (0.10f));
            g.strokePath (p, juce::PathStrokeType (width + (float) i * 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (colour);
        g.strokePath (p, juce::PathStrokeType (width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (col::neonHi.withAlpha (0.8f));
        g.strokePath (p, juce::PathStrokeType (width * 0.35f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void glowLine (juce::Graphics& g, juce::Line<float> l, float width, juce::Colour colour)
    {
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (colour.withAlpha (0.09f));
            g.drawLine (l, width + (float) i * 3.5f);
        }
        g.setColour (colour);
        g.drawLine (l, width);
        g.setColour (col::neonHi.withAlpha (0.7f));
        g.drawLine (l, width * 0.4f);
    }

    void screw (juce::Graphics& g, juce::Point<float> c, float r)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd8dde3), c.x - r, c.y - r,
                                                 juce::Colour (0xff3a3f46), c.x + r, c.y + r, false));
        g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.0f);
        g.setColour (juce::Colour (0xff22262b));
        g.drawLine (c.x - r * 0.6f, c.y, c.x + r * 0.6f, c.y, 1.6f);
        g.drawLine (c.x, c.y - r * 0.6f, c.x, c.y + r * 0.6f, 1.6f);
    }

    void chromeKnob (juce::Graphics& g, juce::Point<float> c, float r, float angle)
    {
        // ombre
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (c.x - r * 1.05f, c.y - r * 0.95f, r * 2.1f, r * 2.15f);
        // jupe
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a6068), c.x, c.y - r,
                                                 juce::Colour (0xff0d0f12), c.x, c.y + r, false));
        g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        // dessus brossé (effet conique)
        const float t = r * 0.80f;
        juce::ColourGradient cg (juce::Colour (0xffe9edf1), c.x, c.y, juce::Colour (0xff3b4048), c.x + t, c.y, true);
        cg.addColour (0.5, juce::Colour (0xff9aa1aa));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffcfd5dc), c.x - t, c.y - t,
                                                 juce::Colour (0xff2c3036), c.x + t, c.y + t, false));
        g.fillEllipse (c.x - t, c.y - t, t * 2, t * 2);
        for (int i = 0; i < 12; ++i)
        {
            const float a = (float) i * juce::MathConstants<float>::pi / 6.0f;
            juce::Path wedge;
            wedge.startNewSubPath (c);
            wedge.addCentredArc (c.x, c.y, t, t, 0.0f, a, a + 0.26f, false);
            wedge.closeSubPath();
            g.setColour (juce::Colours::white.withAlpha (i % 2 ? 0.10f : 0.03f));
            g.fillPath (wedge);
        }
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        for (float rr = t * 0.2f; rr < t; rr += t * 0.12f)
            g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 0.6f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (c.x - t, c.y - t, t * 2, t * 2, 1.0f);
        // index
        juce::Path idx;
        idx.addRoundedRectangle (-1.6f, -r * 0.98f, 3.2f, r * 0.42f, 1.2f);
        g.setColour (juce::Colours::white);
        g.fillPath (idx, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }

    void darkKnob (juce::Graphics& g, juce::Point<float> c, float r, float angle)
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (c.x - r * 1.05f, c.y - r * 0.95f, r * 2.1f, r * 2.15f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b4048), c.x, c.y - r,
                                                 juce::Colour (0xff050608), c.x, c.y + r, false));
        g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawEllipse (c.x - r + 1, c.y - r + 1, r * 2 - 2, r * 2 - 2, 1.0f);
        juce::Path idx;
        idx.addRoundedRectangle (-1.4f, -r * 0.95f, 2.8f, r * 0.5f, 1.0f);
        g.setColour (col::neonHi);
        g.fillPath (idx, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }

    void chromeText (juce::Graphics& g, const juce::String& s, F area, float size, bool italic, juce::Justification j)
    {
        juce::Font f (juce::FontOptions (size, italic ? juce::Font::bold | juce::Font::italic : juce::Font::bold));
        g.setFont (f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawText (s, area.translated (1.5f, 2.0f), j);
        juce::ColourGradient cg (juce::Colours::white, 0, area.getY(), juce::Colour (0xff7d8590), 0, area.getBottom(), false);
        cg.addColour (0.55, juce::Colour (0xffc7ced6));
        g.setGradientFill (cg);
        g.drawText (s, area, j);
    }

    void label (juce::Graphics& g, const juce::String& s, F area, float size, juce::Colour c = col::text,
                juce::Justification j = juce::Justification::centred)
    {
        g.setFont (juce::FontOptions (size, juce::Font::bold));
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawText (s, area.translated (0.8f, 1.0f), j);
        g.setColour (c);
        g.drawText (s, area, j);
    }
}

//==============================================================================
BakrLookAndFeel::BakrLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0b0d10));
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::ComboBox::outlineColourId, col::neon.withAlpha (0.7f));
    setColour (juce::ComboBox::arrowColourId, col::neonHi);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff101318));
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::neon.withAlpha (0.45f));
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xee0b0d10));
    setColour (juce::BubbleComponent::outlineColourId, col::neon);
    setColour (juce::TooltipWindow::textColourId, col::text);
}

void BakrLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float a0, float a1, juce::Slider& s)
{
    const auto b = F ((float) x, (float) y, (float) w, (float) h);
    const auto c = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const juce::String size = s.getProperties()["size"].toString();
    const float ang = a0 + pos * (a1 - a0);

    if (size == "big")
    {
        // graduations blanches
        for (int i = 0; i <= 12; ++i)
        {
            const float a = a0 + (a1 - a0) * (float) i / 12.0f;
            const float r1 = R * (i % 3 == 0 ? 0.86f : 0.90f), r2 = R * 0.98f;
            g.setColour (col::text.withAlpha (0.85f));
            g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1,
                        c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, i % 3 == 0 ? 2.0f : 1.2f);
        }
        juce::Path track; track.addCentredArc (c.x, c.y, R * 0.76f, R * 0.76f, 0.0f, a0, a1, true);
        g.setColour (juce::Colour (0xff0a1a33));
        g.strokePath (track, juce::PathStrokeType (5.0f));
        glowArc (g, c, R * 0.76f, a0, ang, 4.0f, col::neon);
        chromeKnob (g, c, R * 0.64f, ang);
    }
    else if (size == "medium")
    {
        for (int i = 0; i <= 10; ++i)
        {
            const float a = a0 + (a1 - a0) * (float) i / 10.0f;
            g.setColour (col::text.withAlpha (0.7f));
            g.drawLine (c.x + std::sin (a) * R * 0.88f, c.y - std::cos (a) * R * 0.88f,
                        c.x + std::sin (a) * R * 0.98f, c.y - std::cos (a) * R * 0.98f, 1.2f);
        }
        glowArc (g, c, R * 0.76f, a0, ang, 3.0f, col::neon);
        chromeKnob (g, c, R * 0.62f, ang);
    }
    else // small / sélecteur
    {
        glowArc (g, c, R * 0.86f, a0, ang, 2.2f, col::neon);
        darkKnob (g, c, R * 0.68f, ang);
    }
}

void BakrLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    const auto style = b.getProperties()["style"].toString();
    auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();

    if (style == "push") // BYPASS : allumé quand le plugin est bypassé (power = off)
    {
        const bool lit = ! on;
        auto c = r.withSizeKeepingCentre (34, 34);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.fillEllipse (c.expanded (3.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (lit ? 0xff3a2a10 : 0xff2a2e33), c.getCentreX(), c.getY(),
                                                 juce::Colour (0xff050607), c.getCentreX(), c.getBottom(), false));
        g.fillEllipse (c);
        g.setColour (lit ? col::orange : juce::Colours::white.withAlpha (hl ? 0.35f : 0.18f));
        g.drawEllipse (c, lit ? 2.0f : 1.0f);
    }
    else if (style == "switch") // interrupteur à levier
    {
        auto c = r.withSizeKeepingCentre (18, 18);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd0d5db), c.getX(), c.getY(),
                                                 juce::Colour (0xff2b2f35), c.getRight(), c.getBottom(), false));
        g.fillEllipse (c);
        const float dy = on ? -11.0f : 11.0f;
        g.setColour (juce::Colour (0xffe8ecf0));
        g.drawLine (c.getCentreX(), c.getCentreY(), c.getCentreX(), c.getCentreY() + dy, 5.0f);
        g.fillEllipse (c.getCentreX() - 4, c.getCentreY() + dy - 4, 8, 8);
    }
    else if (style == "slide") // EQ ON
    {
        auto c = r.withSizeKeepingCentre (52, 22);
        g.setColour (juce::Colour (0xff050608));
        g.fillRoundedRectangle (c, 11.0f);
        g.setColour (juce::Colours::white.withAlpha (0.2f));
        g.drawRoundedRectangle (c, 11.0f, 1.0f);
        auto knob = F (18, 18).withCentre ({ on ? c.getRight() - 11.0f : c.getX() + 11.0f, c.getCentreY() });
        if (on)
        {
            g.setColour (col::neon.withAlpha (0.35f));
            g.fillEllipse (knob.expanded (5.0f));
            g.setColour (col::neonHi);
        }
        else
            g.setColour (juce::Colour (0xff6a7078));
        g.fillEllipse (knob);
    }
    else if (style == "radio" || style == "lamp")
    {
        auto lamp = F (16, 16).withCentre ({ r.getX() + 10.0f, r.getCentreY() });
        if (on)
        {
            g.setColour (col::neon.withAlpha (0.35f));
            g.fillEllipse (lamp.expanded (6.0f));
            g.setGradientFill (juce::ColourGradient (juce::Colours::white, lamp.getCentreX(), lamp.getCentreY(),
                                                     col::neon, lamp.getRight(), lamp.getBottom(), true));
        }
        else
            g.setColour (juce::Colour (0xff2a2e34));
        g.fillEllipse (lamp);
        g.setColour (juce::Colours::black);
        g.drawEllipse (lamp, 1.0f);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.setColour (col::text);
        g.drawText (b.getButtonText(), r.withTrimmedLeft (26.0f), juce::Justification::centredLeft);
    }
    else if (style == "headphone")
    {
        auto c = r.withSizeKeepingCentre (54, 54);
        if (on)
        {
            for (int i = 4; i >= 1; --i)
            {
                g.setColour (col::neon.withAlpha (0.10f));
                g.drawEllipse (c, 3.0f + (float) i * 3.0f);
            }
        }
        g.setColour (juce::Colour (0xff07090b));
        g.fillEllipse (c);
        g.setColour (on ? col::neon : col::neon.withAlpha (0.35f));
        g.drawEllipse (c, 3.0f);
        // casque
        const auto cc = c.getCentre();
        juce::Path hp;
        hp.addCentredArc (cc.x, cc.y + 2, 12, 12, 0.0f, -1.9f, 1.9f, true);
        g.setColour (on ? juce::Colours::white : col::dim);
        g.strokePath (hp, juce::PathStrokeType (3.0f));
        g.fillRoundedRectangle (cc.x - 14, cc.y + 1, 6, 11, 2.0f);
        g.fillRoundedRectangle (cc.x + 8,  cc.y + 1, 6, 11, 2.0f);
    }
}

//==============================================================================
void VuMeter::setValue (float v, int m)
{
    if (std::abs (v - value) > 0.02f || m != mode) { value = v; mode = m; repaint(); }
}

void VuMeter::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // halo ambre sous le vumètre
    g.setGradientFill (juce::ColourGradient (col::amber.withAlpha (0.35f), b.getCentreX(), b.getBottom() - 6,
                                             juce::Colours::transparentBlack, b.getCentreX(), b.getBottom() + 40, true));
    g.fillRect (b.withTrimmedTop (b.getHeight() * 0.7f));

    // cadre noir/chrome
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4b5058), b.getX(), b.getY(),
                                             juce::Colour (0xff050607), b.getRight(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 14.0f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawRoundedRectangle (b.reduced (1.0f), 14.0f, 1.2f);
    for (auto p : { b.getTopLeft() + juce::Point<float> (14, 14), b.getTopRight() + juce::Point<float> (-14, 14),
                    b.getBottomLeft() + juce::Point<float> (14, -14), b.getBottomRight() + juce::Point<float> (-14, -14) })
        screw (g, p, 5.0f);

    auto face = b.reduced (26.0f, 22.0f).withTrimmedBottom (34.0f);
    juce::ColourGradient fg (juce::Colour (0xfffff3d2), face.getCentreX(), face.getCentreY() + face.getHeight() * 0.3f,
                             juce::Colour (0xffd98a1e), face.getX(), face.getY(), true);
    g.setGradientFill (fg);
    g.fillRoundedRectangle (face, 6.0f);

    const juce::Point<float> pivot (face.getCentreX(), face.getBottom() + face.getHeight() * 0.35f);
    const float R = face.getHeight() * 1.08f;
    constexpr float aMin = -0.78f, aMax = 0.78f;
    auto angleFor = [&] (float vu)
    {
        const float lin = std::pow (10.0f, vu / 20.0f);
        const float t = (lin - 0.1f) / (1.4125f - 0.1f);
        return aMin + juce::jlimit (0.0f, 1.0f, t) * (aMax - aMin);
    };

    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (face.toNearestInt());

    const juce::Colour ink (0xff2a1a0c), red (0xffd0241c);
    juce::Path arc;  arc.addCentredArc (pivot.x, pivot.y, R * 0.80f, R * 0.80f, 0.0f, aMin, angleFor (0.0f), true);
    g.setColour (ink); g.strokePath (arc, juce::PathStrokeType (1.6f));
    juce::Path ra;   ra.addCentredArc (pivot.x, pivot.y, R * 0.80f, R * 0.80f, 0.0f, angleFor (0.0f), aMax, true);
    g.setColour (red); g.strokePath (ra, juce::PathStrokeType (4.5f));

    const float marks[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    for (float m : marks)
    {
        const float a = angleFor (m);
        const float sa = std::sin (a), ca = std::cos (a);
        g.setColour (m > 0 ? red : ink);
        g.drawLine (pivot.x + sa * R * 0.80f, pivot.y - ca * R * 0.80f,
                    pivot.x + sa * R * 0.89f, pivot.y - ca * R * 0.89f, 1.5f);
        if (m == -2.0f) continue;
        const juce::String s = m > 0 ? "+" + juce::String ((int) m) : juce::String ((int) m);
        g.drawText (s, F (30, 14).withCentre ({ pivot.x + sa * R * 0.97f, pivot.y - ca * R * 0.97f }),
                    juce::Justification::centred);
    }
    // petites graduations
    for (float m = -20.0f; m <= 3.0f; m += 0.5f)
    {
        const float a = angleFor (m);
        g.setColour (m > 0 ? red : ink);
        g.drawLine (pivot.x + std::sin (a) * R * 0.80f, pivot.y - std::cos (a) * R * 0.80f,
                    pivot.x + std::sin (a) * R * 0.84f, pivot.y - std::cos (a) * R * 0.84f, 0.8f);
    }

    g.setColour (juce::Colour (0xff5a1408));
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.drawText ("VU", face.withTrimmedTop (face.getHeight() * 0.48f).withHeight (32.0f), juce::Justification::centred);
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.setColour (ink.withAlpha (0.7f));
    g.drawText (mode == 0 ? "INPUT" : (mode == 1 ? "GAIN REDUCTION" : "OUTPUT"),
                face.withTrimmedTop (face.getHeight() * 0.76f).withHeight (12.0f), juce::Justification::centred);

    const float a = angleFor (value);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawLine (pivot.x + std::sin (a) * R * 0.4f + 2, pivot.y - std::cos (a) * R * 0.4f + 2,
                pivot.x + std::sin (a) * R * 0.92f + 2, pivot.y - std::cos (a) * R * 0.92f + 2, 2.0f);
    g.setColour (juce::Colour (0xff111111));
    g.drawLine (pivot.x + std::sin (a) * R * 0.4f, pivot.y - std::cos (a) * R * 0.4f,
                pivot.x + std::sin (a) * R * 0.92f, pivot.y - std::cos (a) * R * 0.92f, 2.0f);

    // reflet de verre
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.3f), 6.0f);

    g.restoreState();
    // vis de réglage du zéro
    const auto zc = juce::Point<float> (b.getCentreX(), b.getBottom() - 30.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3f46), zc.x, zc.y - 14, juce::Colour (0xff050607), zc.x, zc.y + 14, false));
    g.fillEllipse (zc.x - 14, zc.y - 14, 28, 28);
    g.setColour (juce::Colours::white.withAlpha (0.3f));
    g.drawEllipse (zc.x - 14, zc.y - 14, 28, 28, 1.0f);
    g.drawLine (zc.x - 7, zc.y, zc.x + 7, zc.y, 1.5f);
    g.drawLine (zc.x, zc.y - 7, zc.x, zc.y + 7, 1.5f);
}

//==============================================================================
BakrPanel::BakrPanel (BakrTubeCompAudioProcessor& p) : proc (p)
{
    setLookAndFeel (&lnf);

    setupKnob (input,    "input",    "INPUT",  "-12", "30", {}, "big");
    setupKnob (output,   "output",   "OUTPUT", "-24", "24", {}, "big");
    setupKnob (mode,     "mode",     "MODE",   "", "", { "SINGLE", "DOUBLE", "TRIPLE", "OPTO" }, "small");
    setupKnob (recovery, "recovery", "RECOVERY TIME", "", "", { "FAST", "SLOW" }, "small");
    setupKnob (meter,    "meter",    "METER",  "", "", { "INPUT", "GR", "OUTPUT" }, "small");
    setupKnob (mix,      "mix",      "MIX",    "0%", "100%", {}, "small");

    setupKnob (scHp,   "schp",   "DETECTION", "OFF", "1.2k", {}, "small");
    setupKnob (detect, "detect", "DETECTION MODE", "", "",
               { "DUAL", "NORMAL", "M/S", "MID ONLY", "SIDE ONLY", "REVERSED" }, "medium");
    setupKnob (lp,     "sclp",   "LP FREQ", "1kHz", "OFF", {}, "small");
    setupKnob (pkF,    "scpkf",  "FREQ",    "30Hz", "16kHz", {}, "small");
    setupKnob (pkG,    "scpkg",  "GAIN",    "-18dB", "+18dB", {}, "small");
    setupKnob (pkQ,    "scq",    "Q",       "0.1", "10.0", {}, "small");
    setupKnob (scGain, "scgain", "SC GAIN", "-12", "+24", {}, "small");

    auto toggle = [this] (juce::ToggleButton& b, const char* style, const juce::String& txt)
    {
        b.setButtonText (txt);
        b.getProperties().set ("style", style);
        addAndMakeVisible (b);
    };
    toggle (bypassBtn, "push", "");
    toggle (powerSwitch, "switch", "");
    toggle (eqSwitch, "slide", "");
    toggle (listenBtn, "headphone", "");
    toggle (srcInt, "radio", "INTERNAL");
    toggle (srcExt, "radio", "EXTERNAL");

    bypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "power", bypassBtn);
    powerAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "power", powerSwitch);
    eqAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "eqon", eqSwitch);
    listenAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "listen", listenBtn);

    auto setSource = [this] (float v)
    {
        if (auto* prm = proc.apvts.getParameter ("scsource"))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
            prm->endChangeGesture();
        }
    };
    srcInt.setClickingTogglesState (false);
    srcExt.setClickingTogglesState (false);
    srcInt.onClick = [setSource] { setSource (0.0f); };
    srcExt.onClick = [setSource] { setSource (1.0f); };

    bypassBtn.setTooltip ("Bypass");
    listenBtn.setTooltip ("Ecouter le signal du side-chain");

    presetBox.addItemList (BakrTubeCompAudioProcessor::presetNames(), 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0) proc.setCurrentProgram (idx);
    };
    addAndMakeVisible (presetBox);
    addAndMakeVisible (vu);

    setSize (baseW, baseH);
}

BakrPanel::~BakrPanel() { setLookAndFeel (nullptr); }

float BakrPanel::param (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }

void BakrPanel::setupKnob (Knob& k, const juce::String& id, const juce::String& lab,
                           const juce::String& mn, const juce::String& mx, juce::StringArray marks, const juce::String& size)
{
    k.label = lab; k.minTxt = mn; k.maxTxt = mx; k.marks = marks;
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * -0.75f, juce::MathConstants<float>::pi * 0.75f, true);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.getProperties().set ("size", size);
    if (marks.isEmpty())
        k.slider.setPopupDisplayEnabled (true, true, this);
    auto* prm = proc.apvts.getParameter (id);
    k.slider.setDoubleClickReturnValue (true, proc.apvts.getParameterRange (id).convertFrom0to1 (prm->getDefaultValue()));
    addAndMakeVisible (k.slider);
    k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
}

void BakrPanel::buildBackground()
{
    const float sc = 2.0f;
    background = juce::Image (juce::Image::ARGB, (int) (baseW * sc), (int) (baseH * sc), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (sc));

    g.fillAll (juce::Colour (0xff050608));

    auto plate = [&] (F r)
    {
        // biseau chromé
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffbfc5cc), r.getX(), r.getY(),
                                                 juce::Colour (0xff2b2f35), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 16.0f);
        auto in = r.reduced (7.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c3036), in.getX(), in.getY(),
                                                 juce::Colour (0xff0e1013), in.getX(), in.getBottom(), false));
        g.fillRoundedRectangle (in, 12.0f);
        // métal brossé
        juce::Random rnd ((juce::int64) r.getY());
        for (float y = in.getY(); y < in.getBottom(); y += 0.7f)
        {
            g.setColour (juce::Colours::white.withAlpha (rnd.nextFloat() * 0.035f));
            g.drawHorizontalLine ((int) y, in.getX() + 4.0f, in.getRight() - 4.0f);
        }
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), in.getCentreX(), in.getY(),
                                                 juce::Colours::transparentWhite, in.getCentreX(), in.getCentreY(), false));
        g.fillRoundedRectangle (in, 12.0f);
        // néons bleus des bords
        glowLine (g, { r.getX() + 4, r.getY() + 30, r.getX() + 4, r.getBottom() - 30 }, 2.5f, col::neon);
        glowLine (g, { r.getRight() - 4, r.getY() + 30, r.getRight() - 4, r.getBottom() - 30 }, 2.5f, col::neon);
        glowLine (g, { r.getX() + 60, r.getBottom() - 3, r.getRight() - 60, r.getBottom() - 3 }, 1.6f, col::neon);
        for (auto pt : { r.getTopLeft() + juce::Point<float> (22, 22), r.getTopRight() + juce::Point<float> (-22, 22),
                         r.getBottomLeft() + juce::Point<float> (22, -22), r.getBottomRight() + juce::Point<float> (-22, -22) })
            screw (g, pt, 8.0f);
    };

    plate ({ 6, 6, 1088, 290 });
    plate ({ 6, 302, 1088, 242 });

    // fentes d'aération
    for (auto r : { F (95, 18, 260, 10), F (745, 18, 260, 10) })
    {
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawRoundedRectangle (r, 5.0f, 1.0f);
    }

    // logo Bakr + battements de coeur
    auto heartbeat = [&] (float x0, float x1, bool leftSide)
    {
        juce::Path p;
        const float y = 36.0f;
        const float pk = leftSide ? x1 - 60.0f : x0 + 60.0f;
        p.startNewSubPath (x0, y);
        p.lineTo (pk - 14, y); p.lineTo (pk - 8, y - 14); p.lineTo (pk - 2, y + 16);
        p.lineTo (pk + 4, y - 24); p.lineTo (pk + 10, y + 10); p.lineTo (pk + 15, y);
        p.lineTo (x1, y);
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (col::orange.withAlpha (0.12f));
            g.strokePath (p, juce::PathStrokeType (1.5f + (float) i * 3.0f));
        }
        g.setColour (col::amber);
        g.strokePath (p, juce::PathStrokeType (1.6f));
    };
    heartbeat (360, 485, true);
    heartbeat (615, 740, false);
    chromeText (g, "Bakr", F (470, 8, 160, 48), 46.0f, true, juce::Justification::centred);
    label (g, "AUDIO PLUGINS", F (470, 52, 160, 14), 11.0f, col::text);

    // cadres de la partie basse
    auto box = [&] (F r)
    {
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawRoundedRectangle (r, 10.0f, 1.2f);
    };
    box ({ 28, 338, 180, 190 });
    box ({ 214, 338, 270, 190 });
    box ({ 494, 322, 440, 206 });
    box ({ 944, 322, 130, 206 });

    // onglets
    auto tab = [&] (F r, const juce::String& s, bool active)
    {
        juce::Path t;
        t.startNewSubPath (r.getX(), r.getBottom());
        t.lineTo (r.getX() + 14, r.getY()); t.lineTo (r.getRight() - 14, r.getY());
        t.lineTo (r.getRight(), r.getBottom()); t.closeSubPath();
        g.setGradientFill (juce::ColourGradient (active ? juce::Colour (0xff1a5fc4) : juce::Colour (0xff3a3f46), 0, r.getY(),
                                                 active ? juce::Colour (0xff0a2a66) : juce::Colour (0xff15181c), 0, r.getBottom(), false));
        g.fillPath (t);
        if (active)
        {
            g.setColour (col::neon.withAlpha (0.4f)); g.strokePath (t, juce::PathStrokeType (4.0f));
        }
        g.setColour (active ? col::neonHi : juce::Colours::white.withAlpha (0.25f));
        g.strokePath (t, juce::PathStrokeType (1.2f));
        label (g, s, r, 14.0f, active ? col::text : col::dim);
    };
    tab ({ 80, 308, 132, 26 }, "ADVANCED", true);
    tab ({ 212, 308, 132, 26 }, "SIDE-CHAIN", false);
    tab ({ 344, 308, 120, 26 }, "CONTROL", false);

    // nom du produit
    chromeText (g, "TUBECOMP", F (250, 232, 200, 34), 30.0f, true, juce::Justification::centred);
    g.setColour (col::gold);
    g.drawLine (240, 275, 285, 275, 1.0f);
    g.drawLine (415, 275, 470, 275, 1.0f);
    label (g, "BAKR EDITION", F (285, 268, 130, 14), 11.0f, col::gold);

    // titres statiques
    label (g, "ON",      F (40, 82, 60, 18), 14.0f);
    label (g, "BYPASS",  F (30, 178, 80, 18), 14.0f);
    label (g, "dB", F (163, 238, 70, 14), 11.0f);
    label (g, "dB", F (870, 238, 70, 14), 11.0f);
    label (g, "POWER", F (720, 60, 80, 18), 15.0f);
    label (g, "ON",  F (740, 125, 40, 14), 11.0f);
    label (g, "OFF", F (740, 176, 40, 14), 11.0f);
    label (g, "SOURCE", F (28, 344, 180, 18), 14.0f);
    label (g, "LISTEN", F (944, 334, 130, 18), 15.0f);
    label (g, "SC HPF", F (78, 516, 76, 12), 10.0f, col::dim);
    label (g, "BAND EQUALIZER", F (640, 370, 150, 16), 12.0f);
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.drawLine (535, 378, 636, 378, 1.0f);
    g.drawLine (794, 378, 895, 378, 1.0f);
    // badge EQ
    g.setColour (juce::Colour (0xff0a2a66));
    g.fillRoundedRectangle (626, 334, 38, 22, 4.0f);
    g.setColour (col::neon);
    g.drawRoundedRectangle (626, 334, 38, 22, 4.0f, 1.5f);
    label (g, "EQ", F (626, 334, 38, 22), 13.0f, col::neonHi);
    label (g, "ON", F (722, 336, 30, 18), 12.0f);

    // grille du haut-parleur
    for (int yy = 0; yy < 3; ++yy)
        for (int xx = 0; xx < 5; ++xx)
        {
            const float cx = 990.0f + (float) xx * 10.0f + (yy % 2 ? 5.0f : 0.0f), cy = 508.0f + (float) yy * 8.0f;
            g.setColour (juce::Colours::black);
            g.fillEllipse (cx - 3, cy - 3, 6, 6);
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.drawEllipse (cx - 3, cy - 3, 6, 6, 0.6f);
        }
}

void BakrPanel::drawKnobText (juce::Graphics& g, const Knob& k, float gap)
{
    auto b = k.slider.getBounds().toFloat();
    if (k.label.isNotEmpty())
        label (g, k.label, F (b.getCentreX() - 80, b.getY() - gap, 160, 16), 15.0f);

    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float a0 = juce::MathConstants<float>::pi * -0.75f, a1 = juce::MathConstants<float>::pi * 0.75f;

    if (k.marks.isEmpty())
    {
        auto at = [&] (float a) { return juce::Point<float> (c.x + std::sin (a) * R * 0.95f, c.y - std::cos (a) * R * 0.95f + 14.0f); };
        label (g, k.minTxt, F (54, 14).withCentre (at (a0)), 12.0f);
        label (g, k.maxTxt, F (54, 14).withCentre (at (a1)), 12.0f);
        return;
    }
    const int n = k.marks.size();
    const int sel = (int) std::round (k.slider.getValue());
    // 2 positions : gauche/droite ; plus : réparties sur l'arc
    for (int i = 0; i < n; ++i)
    {
        const float span = n <= 3 ? 0.55f : (n == 4 ? 0.8f : 1.0f);
        const float a = n == 1 ? 0.0f : (a0 * span) + ((a1 - a0) * span) * (float) i / (float) (n - 1);
        const float rr = R + (n > 4 ? 22.0f : 12.0f);
        juce::Point<float> p (c.x + std::sin (a) * rr, c.y - std::cos (a) * rr);
        const float w = 74.0f;
        auto area = F (w, 14).withCentre (p);
        if (std::sin (a) < -0.3f) area = area.withX (p.x - w + 14.0f);
        else if (std::sin (a) > 0.3f) area = area.withX (p.x - 14.0f);
        const auto just = std::sin (a) < -0.3f ? juce::Justification::centredRight
                        : (std::sin (a) > 0.3f ? juce::Justification::centredLeft : juce::Justification::centred);
        label (g, k.marks[i], area, 12.0f, i == sel ? col::neonHi : col::text, just);
    }
}

void BakrPanel::paint (juce::Graphics& g)
{
    if (background.isNull()) buildBackground();
    g.drawImage (background, getLocalBounds().toFloat());

    const bool on = param ("power") > 0.5f;

    // voyant ON (bleu)
    {
        const auto c = juce::Point<float> (70.0f, 122.0f);
        if (on)
        {
            g.setColour (col::neon.withAlpha (0.25f));
            g.fillEllipse (c.x - 22, c.y - 22, 44, 44);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white, c.x, c.y, col::neon, c.x + 13, c.y + 13, true));
        }
        else
            g.setColour (juce::Colour (0xff1a2433));
        g.fillEllipse (c.x - 13, c.y - 13, 26, 26);
        g.setColour (juce::Colours::black);
        g.drawEllipse (c.x - 13, c.y - 13, 26, 26, 2.0f);
    }
    // voyant POWER (orange)
    {
        const auto c = juce::Point<float> (760.0f, 103.0f);
        if (on)
        {
            g.setColour (col::orange.withAlpha (0.3f));
            g.fillEllipse (c.x - 28, c.y - 28, 56, 56);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffe08a), c.x, c.y, juce::Colour (0xffc63a08), c.x + 18, c.y + 18, true));
        }
        else
            g.setColour (juce::Colour (0xff3a1a0a));
        g.fillEllipse (c.x - 17, c.y - 17, 34, 34);
        g.setColour (juce::Colour (0xff6a6f76));
        g.drawEllipse (c.x - 19, c.y - 19, 38, 38, 3.0f);
    }

    drawKnobText (g, input, 22.0f);
    drawKnobText (g, output, 22.0f);
    drawKnobText (g, mode, 46.0f);
    drawKnobText (g, recovery, 30.0f);
    drawKnobText (g, meter, 40.0f);
    drawKnobText (g, mix, 26.0f);
    drawKnobText (g, scHp, 30.0f);
    drawKnobText (g, detect, 52.0f);
    drawKnobText (g, lp, 22.0f);
    drawKnobText (g, pkF, 22.0f);
    drawKnobText (g, pkG, 22.0f);
    drawKnobText (g, pkQ, 22.0f);
    drawKnobText (g, scGain, 16.0f);
}

void BakrPanel::resized()
{
    presetBox.setBounds (800, 266, 220, 22);

    input.slider.setBounds  (128, 92, 150, 150);
    output.slider.setBounds (830, 92, 150, 150);
    mode.slider.setBounds     (330, 112, 50, 50);
    recovery.slider.setBounds (330, 196, 50, 50);
    meter.slider.setBounds    (740, 242, 40, 40);
    mix.slider.setBounds      (998, 184, 62, 62);
    vu.setBounds (432, 70, 248, 200);

    bypassBtn.setBounds   (50, 196, 40, 40);
    powerSwitch.setBounds (745, 140, 30, 36);

    srcInt.setBounds (78, 368, 120, 22);
    srcExt.setBounds (78, 394, 120, 22);
    scHp.slider.setBounds (88, 450, 56, 56);

    detect.slider.setBounds (309, 404, 90, 90);

    eqSwitch.setBounds (666, 334, 56, 22);
    lp.slider.setBounds  (522, 420, 64, 64);
    pkF.slider.setBounds (628, 420, 64, 64);
    pkG.slider.setBounds (734, 420, 64, 64);
    pkQ.slider.setBounds (840, 420, 64, 64);

    listenBtn.setBounds (979, 356, 60, 60);
    scGain.slider.setBounds (989, 440, 40, 40);
}

void BakrPanel::tick()
{
    const int m = (int) param ("meter");
    float target;
    if (m == 1)      target = -proc.meterGrDb.load();
    else if (m == 0) target = proc.meterInDb.load() + 18.0f;
    else             target = proc.meterOutDb.load() + 18.0f;
    vuBallistic += 0.22f * (target - vuBallistic);
    vu.setValue (juce::jlimit (-22.0f, 4.0f, vuBallistic), m);

    if (presetBox.getSelectedItemIndex() != proc.getCurrentProgram())
        presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);

    const bool ext = param ("scsource") > 0.5f;
    srcInt.setToggleState (! ext, juce::dontSendNotification);
    srcExt.setToggleState (ext, juce::dontSendNotification);

    // ne repeindre le panneau que si un choix / l'état power a changé
    const juce::int64 h = (juce::int64) param ("power") + 2 * (juce::int64) param ("mode") + 8 * (juce::int64) param ("recovery")
                        + 16 * (juce::int64) param ("meter") + 64 * (juce::int64) param ("detect");
    if (h != lastStateHash) { lastStateHash = h; repaint(); }
}

//==============================================================================
BakrTubeCompAudioProcessorEditor::BakrTubeCompAudioProcessorEditor (BakrTubeCompAudioProcessor& p)
    : AudioProcessorEditor (&p), panel (p)
{
    addAndMakeVisible (panel);
    setResizable (true, true);
    setResizeLimits (BakrPanel::baseW / 2, BakrPanel::baseH / 2, BakrPanel::baseW * 2, BakrPanel::baseH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) BakrPanel::baseW / BakrPanel::baseH);
    setSize (BakrPanel::baseW, BakrPanel::baseH);
    startTimerHz (30);
}

void BakrTubeCompAudioProcessorEditor::resized()
{
    const float s = (float) getWidth() / (float) BakrPanel::baseW;
    panel.setBounds (0, 0, BakrPanel::baseW, BakrPanel::baseH);
    panel.setTransform (juce::AffineTransform::scale (s));
}
