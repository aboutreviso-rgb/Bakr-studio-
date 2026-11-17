#include "PluginEditor.h"

using namespace bgui;
using juce::Colour; using juce::Rectangle; using juce::Graphics; using juce::Path;

static constexpr int kW = 1000, kH = 640;

//==============================================================================
// LookAndFeel
LnF::LnF()
{
    setColour (juce::ComboBox::backgroundColourId, panel2);
    setColour (juce::ComboBox::textColourId, cText);
    setColour (juce::ComboBox::outlineColourId, edge);
    setColour (juce::ComboBox::arrowColourId, gold);
    setColour (juce::PopupMenu::backgroundColourId, Colour (0xff1e1c1a));
    setColour (juce::PopupMenu::textColourId, cText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, goldDim);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::Label::textColourId, cText);
    setColour (juce::Slider::textBoxTextColourId, textDim);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextButton::textColourOffId, cText);
    setColour (juce::TextButton::textColourOnId, Colour (0xff1a1408));
}

juce::Label* LnF::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::Font (juce::FontOptions (11.5f)));
    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::textColourId, cText.withAlpha (0.75f));
    return l;
}

void LnF::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&)
{
    auto r = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    float d = juce::jmin (r.getWidth(), r.getHeight());
    auto c = r.getCentre();
    auto k = Rectangle<float> (d, d).withCentre (c);

    // ombre
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (k.translated (0, 3).expanded (1));
    // anneau d'arc
    float ang = a0 + pos * (a1 - a0);
    Path track; track.addCentredArc (c.x, c.y, d * 0.5f, d * 0.5f, 0, a0, a1, true);
    g.setColour (Colour (0xff2c2925)); g.strokePath (track, juce::PathStrokeType (2.5f));
    Path val; val.addCentredArc (c.x, c.y, d * 0.5f, d * 0.5f, 0, a0, ang, true);
    g.setColour (gold); g.strokePath (val, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    // corps du bouton
    auto body = k.reduced (d * 0.11f);
    juce::ColourGradient grad (Colour (0xff4a4640), body.getX(), body.getY(), Colour (0xff0d0c0b), body.getRight(), body.getBottom(), false);
    g.setGradientFill (grad); g.fillEllipse (body);
    g.setColour (Colour (0xff5a554d)); g.drawEllipse (body, 1.0f);
    auto inner = body.reduced (body.getWidth() * 0.12f);
    juce::ColourGradient grad2 (Colour (0xff2e2b27), inner.getCentreX(), inner.getY(), Colour (0xff141311), inner.getCentreX(), inner.getBottom(), false);
    g.setGradientFill (grad2); g.fillEllipse (inner);
    // repère
    float rr = inner.getWidth() * 0.5f;
    juce::Point<float> p1 (c.x + std::sin (ang) * rr * 0.35f, c.y - std::cos (ang) * rr * 0.35f);
    juce::Point<float> p2 (c.x + std::sin (ang) * rr * 0.92f, c.y - std::cos (ang) * rr * 0.92f);
    g.setColour (cText); g.drawLine ({ p1, p2 }, 2.2f);
}

void LnF::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float,
                            juce::Slider::SliderStyle, juce::Slider&)
{
    float cy = (float) y + (float) h * 0.5f;
    g.setColour (Colour (0xff2c2925)); g.fillRoundedRectangle ((float) x, cy - 2, (float) w, 4, 2);
    g.setColour (gold); g.fillRoundedRectangle ((float) x, cy - 2, pos - (float) x, 4, 2);
    g.setColour (Colour (0xffe9d2a6)); g.fillEllipse (pos - 6, cy - 6, 12, 12);
    g.setColour (Colour (0xff6a5638)); g.drawEllipse (pos - 6, cy - 6, 12, 12, 1);
}

void LnF::drawButtonBackground (Graphics& g, juce::Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    bool on = b.getToggleState();
    if (on)
    {
        juce::ColourGradient gr (Colour (0xff6e5a3a), r.getX(), r.getY(), Colour (0xff4b3d27), r.getX(), r.getBottom(), false);
        g.setGradientFill (gr);
    }
    else g.setColour (down ? Colour (0xff2d2a26) : over ? Colour (0xff292622) : Colour (0xff201e1c));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (on ? gold.withAlpha (0.8f) : edge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void LnF::drawButtonText (Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.getToggleState() ? Colour (0xfff5e3c0) : cText.withAlpha (b.isEnabled() ? 0.85f : 0.4f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (3, 0), juce::Justification::centred, 1);
}

void LnF::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (Colour (0xff1f1d1b)); g.fillRoundedRectangle (r, 4);
    g.setColour (edge); g.drawRoundedRectangle (r, 4, 1);
    Path a; float ax = (float) w - 14, ay = (float) h * 0.5f;
    a.startNewSubPath (ax - 4, ay - 2); a.lineTo (ax, ay + 2); a.lineTo (ax + 4, ay - 2);
    g.setColour (gold); g.strokePath (a, juce::PathStrokeType (1.5f));
}

//==============================================================================
void Wheel::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto slot = r.withTrimmedBottom (16).reduced (4, 2);
    g.setColour (Colour (0xff0b0a09)); g.fillRoundedRectangle (slot, 5);
    g.setColour (edge); g.drawRoundedRectangle (slot, 5, 1);
    float v = value.load();
    float pos = centre ? (0.5f - v * 0.5f) : (1.0f - v);
    float y = slot.getY() + 6 + pos * (slot.getHeight() - 12);
    auto knob = Rectangle<float> (slot.getX() + 3, y - 6, slot.getWidth() - 6, 12);
    juce::ColourGradient gr (Colour (0xff6a655d), knob.getX(), knob.getY(), Colour (0xff2a2724), knob.getX(), knob.getBottom(), false);
    g.setGradientFill (gr); g.fillRoundedRectangle (knob, 3);
    g.setColour (textDim); g.setFont (juce::FontOptions (10.0f));
    g.drawText (label, r.removeFromBottom (14), juce::Justification::centred);
}

void Wheel::mouseDrag (const juce::MouseEvent& e)
{
    auto slot = getLocalBounds().toFloat().withTrimmedBottom (16).reduced (4, 2);
    float pos = juce::jlimit (0.0f, 1.0f, ((float) e.y - slot.getY() - 6) / (slot.getHeight() - 12));
    value = centre ? (0.5f - pos) * 2.0f : 1.0f - pos;
    repaint();
}

//==============================================================================
Colour Keyboard::zoneColour (int note) const
{
    if (note >= BakrGuitarProcessor::kKeyswitchLow && note < BakrGuitarProcessor::kKeyswitchLow + 9)
        return note - BakrGuitarProcessor::kKeyswitchLow == selectedArtic ? Colour (0xffe06b5a) : Colour (0xffd9a39a);
    if (note >= BakrGuitarProcessor::kFxLow && note < BakrGuitarProcessor::kFxLow + 4)
        return Colour (0xffa8d69a);
    if (note >= 40 && note <= 88) return Colour (0xfff2efe8);
    return Colour (0xffb9b4ab);
}

void Keyboard::drawWhiteNote (int note, Graphics& g, Rectangle<float> a, bool isDown, bool isOver, Colour, Colour)
{
    auto c = zoneColour (note);
    if (isDown) c = gold;
    else if (isOver) c = c.brighter (0.1f);
    juce::ColourGradient gr (c, a.getX(), a.getY(), c.darker (0.15f), a.getX(), a.getBottom(), false);
    g.setGradientFill (gr);
    g.fillRect (a.reduced (0.5f, 0));
    g.setColour (Colour (0xff3a3632)); g.drawVerticalLine ((int) a.getRight(), a.getY(), a.getBottom());
    if (note % 12 == 0)
    {
        g.setColour (Colour (0xff6a645c)); g.setFont (juce::FontOptions (9.0f));
        g.drawText ("C" + juce::String (note / 12), a.withTrimmedTop (a.getHeight() - 14), juce::Justification::centred);
    }
}

void Keyboard::drawBlackNote (int note, Graphics& g, Rectangle<float> a, bool isDown, bool isOver, Colour)
{
    Colour c (0xff15130f);
    if (note >= BakrGuitarProcessor::kKeyswitchLow && note < BakrGuitarProcessor::kKeyswitchLow + 9)
        c = note - BakrGuitarProcessor::kKeyswitchLow == selectedArtic ? Colour (0xff9a3a2c) : Colour (0xff4a2420);
    else if (note >= BakrGuitarProcessor::kFxLow && note < BakrGuitarProcessor::kFxLow + 4) c = Colour (0xff2d4a26);
    if (isDown) c = goldDim; else if (isOver) c = c.brighter (0.2f);
    g.setColour (c); g.fillRoundedRectangle (a, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.08f)); g.fillRect (a.reduced (2).withHeight (a.getHeight() * 0.4f));
}

//==============================================================================
void FxButton::paintButton (Graphics& g, bool over, bool)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    bool on = getToggleState();
    g.setColour (on ? Colour (0xff3a3125) : over ? Colour (0xff262320) : Colour (0xff1f1d1b));
    g.fillRoundedRectangle (r, 5);
    g.setColour (on ? gold.withAlpha (0.85f) : edge); g.drawRoundedRectangle (r, 5, 1);

    auto ic = r.withTrimmedBottom (r.getHeight() * 0.42f).reduced (r.getWidth() * 0.26f, 6);
    g.setColour (on ? gold : textDim);
    juce::PathStrokeType st (1.6f);
    Path p;
    switch (icon)
    {
        case 0: // ampli
            g.drawRoundedRectangle (ic, 2, 1.6f);
            for (int i = 0; i < 3; ++i) g.fillEllipse (ic.getX() + 4 + (float) i * (ic.getWidth() - 8) / 2.0f - 1.5f, ic.getY() + 4, 3, 3);
            g.drawLine (ic.getX() + 3, ic.getY() + 10, ic.getRight() - 3, ic.getY() + 10, 1.0f);
            break;
        case 1: // EQ
            p.startNewSubPath (ic.getX(), ic.getCentreY());
            p.cubicTo (ic.getX() + ic.getWidth() * 0.3f, ic.getCentreY(), ic.getX() + ic.getWidth() * 0.35f, ic.getY(), ic.getCentreX(), ic.getY());
            p.cubicTo (ic.getX() + ic.getWidth() * 0.65f, ic.getY(), ic.getX() + ic.getWidth() * 0.7f, ic.getBottom(), ic.getRight(), ic.getCentreY());
            g.strokePath (p, st); break;
        case 2: // compresseur
            g.drawRoundedRectangle (ic, 2, 1.6f);
            g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
            g.drawText ("C", ic, juce::Justification::centred); break;
        case 3: // chorus
            for (int k = 0; k < 2; ++k)
            {
                Path w; w.startNewSubPath (ic.getX(), ic.getCentreY() + (float) k * 4 - 2);
                for (float t = 0; t <= 1.0f; t += 0.05f)
                    w.lineTo (ic.getX() + t * ic.getWidth(), ic.getCentreY() + (float) k * 4 - 2 + std::sin (t * 12.6f + (float) k) * ic.getHeight() * 0.25f);
                g.strokePath (w, st);
            }
            break;
        case 4: // delay
            for (int k = 0; k < 3; ++k)
            {
                Path a; float x0 = ic.getX() + (float) k * ic.getWidth() * 0.3f;
                a.startNewSubPath (x0, ic.getY()); a.lineTo (x0 + ic.getWidth() * 0.25f, ic.getCentreY()); a.lineTo (x0, ic.getBottom());
                g.setColour ((on ? gold : textDim).withAlpha (1.0f - (float) k * 0.3f));
                g.strokePath (a, st);
            }
            break;
        default: // reverb
        {
            Path cube;
            float s = juce::jmin (ic.getWidth(), ic.getHeight());
            auto b = Rectangle<float> (s, s).withCentre (ic.getCentre());
            cube.addRectangle (b.reduced (s * 0.15f).translated (-s * 0.1f, s * 0.1f));
            cube.addRectangle (b.reduced (s * 0.15f).translated (s * 0.1f, -s * 0.1f));
            g.strokePath (cube, juce::PathStrokeType (1.2f));
        }
    }
    g.setColour (on ? cText : textDim);
    g.setFont (juce::FontOptions (12.0f));
    g.drawText (getName(), r.withTrimmedTop (r.getHeight() * 0.55f).withTrimmedBottom (10), juce::Justification::centred);
    g.setColour (on ? gold : Colour (0xff3a3632));
    g.fillEllipse (r.getCentreX() - 2.5f, r.getBottom() - 9, 5, 5);
}

//==============================================================================
void LibraryItem::paintButton (Graphics& g, bool over, bool)
{
    auto r = getLocalBounds().toFloat().reduced (2, 1);
    bool on = getToggleState();
    if (on)
    {
        juce::ColourGradient gr (Colour (0xff6a5634), r.getX(), r.getY(), Colour (0xff4a3b22), r.getRight(), r.getY(), false);
        g.setGradientFill (gr); g.fillRoundedRectangle (r, 4);
    }
    else if (over) { g.setColour (Colour (0x20ffffff)); g.fillRoundedRectangle (r, 4); }
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.setColour (on ? Colour (0xfff5e3c0) : textDim);
    g.drawText (codeText, r.withWidth (34), juce::Justification::centred);
    g.setFont (juce::FontOptions (12.5f));
    g.setColour (on ? Colour (0xfffff4dc) : cText.withAlpha (0.85f));
    g.drawText (getName(), r.withTrimmedLeft (36), juce::Justification::centredLeft);
    if (on) { g.setColour (gold); g.drawText (">", r.withTrimmedRight (8), juce::Justification::centredRight); }
}

//==============================================================================
// Illustration de guitare (dessinée en code, aucune image)
void GuitarArt::paint (Graphics& g)
{
    auto R = getLocalBounds().toFloat();
    // fond chaud façon studio
    juce::ColourGradient bgGr (Colour (0xff2b1d12), R.getCentreX(), R.getCentreY(), Colour (0xff0b0807), R.getX(), R.getY(), true);
    g.setGradientFill (bgGr); g.fillRect (R);
    juce::ColourGradient glow (Colour (0x55d08a3a), R.getWidth() * 0.62f, R.getHeight() * 0.35f, Colour (0x00000000), R.getWidth() * 0.95f, R.getHeight(), true);
    g.setGradientFill (glow); g.fillRect (R);

    const bool acoustic = guitar <= 1;
    const float H = R.getHeight();
    // repère : corps à gauche, manche qui monte vers la droite
    auto tr = juce::AffineTransform::rotation (-0.17f, 0, 0).scaled (0.78f).translated (R.getWidth() * 0.45f, H * 0.58f);

    {
    juce::Graphics::ScopedSaveState ss (g);
    g.addTransform (tr);

    // ---- couleurs du bois ----
    Colour c1, c2, c3;
    switch (guitar)
    {
        case 0:  c1 = Colour (0xfff0d39a); c2 = Colour (0xffd8a85e); c3 = Colour (0xff7a4a1e); break;  // épicéa naturel
        case 1:  c1 = Colour (0xffd99a55); c2 = Colour (0xffb06a2e); c3 = Colour (0xff4a2410); break;  // cèdre
        case 2:  c1 = Colour (0xfff2b04a); c2 = Colour (0xffa8461a); c3 = Colour (0xff2a0a04); break;  // cerise sunburst
        default: c1 = Colour (0xffe8b84a); c2 = Colour (0xff8a2c12); c3 = Colour (0xff100604); break;  // 3 tons
    }

    // ---- manche ----
    const float neckX0 = 120, neckLen = 760, nw = 54;
    // ombre du manche
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRect (Rectangle<float> (neckX0, -nw * 0.5f + 8, neckLen, nw));
    auto neck = Rectangle<float> (neckX0, -nw * 0.5f, neckLen, nw);
    juce::ColourGradient ng (Colour (0xff3a2416), 0, neck.getY(), Colour (0xff1a0f08), 0, neck.getBottom(), false);
    g.setGradientFill (ng); g.fillRect (neck);
    // filet de manche
    if (guitar == 2) { g.setColour (Colour (0xffe8dcc0)); g.drawRect (neck, 1.5f); }
    // frettes
    const float scale = 1300.0f;
    float lastX = neckX0 + neckLen;
    for (int f = 1; f <= 22; ++f)
    {
        float x = neckX0 + neckLen - (scale - scale / std::pow (2.0f, (float) f / 12.0f)) * 0.95f;
        if (x < neckX0) break;
        g.setColour (Colour (0xffcfc8bc)); g.fillRect (x - 1.2f, neck.getY(), 2.4f, nw);
        g.setColour (juce::Colours::white.withAlpha (0.5f)); g.fillRect (x - 0.4f, neck.getY(), 0.8f, nw);
        // repères
        float mid = (x + lastX) * 0.5f;
        bool dot = (f == 3 || f == 5 || f == 7 || f == 9 || f == 15 || f == 17 || f == 19 || f == 21);
        bool dbl = (f == 12);
        Colour inlay = acoustic ? Colour (0xffe9e4da) : Colour (0xfff3ead6);
        g.setColour (inlay);
        if (guitar == 2 && (dot || dbl))
        {
            float tw = (lastX - x) * 0.5f;
            Path tp; tp.addQuadrilateral (mid - tw * 0.45f, -nw * 0.3f, mid + tw * 0.5f, -nw * 0.33f, mid + tw * 0.5f, nw * 0.33f, mid - tw * 0.45f, nw * 0.3f);
            g.fillPath (tp);
        }
        else if (dot) g.fillEllipse (mid - 4, -4, 8, 8);
        else if (dbl) { g.fillEllipse (mid - 4, -nw * 0.3f - 4, 8, 8); g.fillEllipse (mid - 4, nw * 0.3f - 4, 8, 8); }
        lastX = x;
    }

    // ---- corps ----
    Path bodyP;
    {
        // contour de caisse d'un seul tenant (taille fine, bas de caisse large)
        const float k = acoustic ? 1.12f : 1.0f;
        bodyP.startNewSubPath (70 * k, -55 * k);
        bodyP.cubicTo (50 * k, -118 * k, -20 * k, -128 * k, -78 * k, -112 * k);
        bodyP.cubicTo (-125 * k, -98 * k, -150 * k, -86 * k, -182 * k, -100 * k);
        bodyP.cubicTo (-245 * k, -150 * k, -335 * k, -138 * k, -338 * k, -5 * k);
        bodyP.cubicTo (-342 * k, 115 * k, -255 * k, 152 * k, -182 * k, 132 * k);
        bodyP.cubicTo (-150 * k, 124 * k, -128 * k, 104 * k, -80 * k, 116 * k);
        bodyP.cubicTo (-15 * k, 134 * k, 62 * k, 118 * k, 76 * k, 55 * k);
        if (guitar == 2)        // pan coupé (single cut)
            bodyP.cubicTo (40 * k, 40 * k, 30 * k, -20 * k, 70 * k, -55 * k);
        else if (guitar == 3)   // double pan coupé
        {
            bodyP.cubicTo (110 * k, 70 * k, 120 * k, 40 * k, 60 * k, 20 * k);
            bodyP.cubicTo (40 * k, 0, 60 * k, -20 * k, 130 * k, -60 * k);
            bodyP.cubicTo (140 * k, -75 * k, 100 * k, -80 * k, 70 * k, -55 * k);
        }
        bodyP.closeSubPath();
    }
    // ombre portée
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillPath (bodyP, juce::AffineTransform::translation (6, 12));
    juce::ColourGradient bg2 (c1, -170, -10, c3, -170 + 270, -10 + 120, true);
    bg2.addColour (0.55, c2);
    g.setGradientFill (bg2); g.fillPath (bodyP);
    // reflet vernis
    juce::ColourGradient shine (juce::Colours::white.withAlpha (0.20f), -250, -120, juce::Colours::transparentWhite, -120, 40, false);
    g.setGradientFill (shine); g.fillPath (bodyP);
    // filet de caisse
    g.setColour (guitar == 3 ? Colour (0xff1a1a1a) : Colour (0xffeee4cc));
    g.strokePath (bodyP, juce::PathStrokeType (3.0f));

    auto metal = [&] (Rectangle<float> r, bool gold_)
    {
        juce::ColourGradient mg (gold_ ? Colour (0xfff3d79a) : Colour (0xffeeeeee), r.getX(), r.getY(),
                                 gold_ ? Colour (0xff8a6a30) : Colour (0xff7a7a7a), r.getX(), r.getBottom(), false);
        g.setGradientFill (mg); g.fillRoundedRectangle (r, 3);
    };

    if (acoustic)
    {
        // rosace + rosette
        float hx = 10;
        g.setColour (Colour (0xff120a05)); g.fillEllipse (hx - 48, -48, 96, 96);
        for (int i = 0; i < (guitar == 1 ? 5 : 3); ++i)
        {
            g.setColour (i % 2 == 0 ? Colour (0xff2a1a0c) : Colour (0xffd9b47a));
            g.drawEllipse (hx - 56 - (float) i * 3, -56 - (float) i * 3, 112 + (float) i * 6, 112 + (float) i * 6, guitar == 1 ? 2.2f : 1.4f);
        }
        if (guitar == 0)
        {
            Path pg; pg.addEllipse (hx - 10, 30, 110, 70);
            g.setColour (Colour (0xcc2a140a)); g.fillPath (pg);
        }
        // chevalet
        auto bridge = Rectangle<float> (-250, -60, 30, 120);
        g.setColour (Colour (0xff2a170b)); g.fillRoundedRectangle (bridge, 8);
        g.setColour (Colour (0xffeee8d8)); g.fillRect (bridge.getX() + 6, bridge.getY() + 10, 3.0f, bridge.getHeight() - 20);
        for (int s = 0; s < 6; ++s)
        {
            g.setColour (guitar == 1 ? Colour (0xffd9c79a) : Colour (0xfff2ead6));
            g.fillEllipse (bridge.getX() + 16, -20 + (float) s * 8 - 2.5f, 5, 5);
        }
    }
    else
    {
        // plaque / micros
        if (guitar == 2)
        {
            Path pg; pg.addRoundedRectangle (-140, 25, 120, 80, 25);
            g.setColour (Colour (0xdd17110c)); g.fillPath (pg);
            for (float px : { -175.0f, -60.0f })
            {
                auto pu = Rectangle<float> (px - 22, -42, 44, 84);
                g.setColour (Colour (0xffe9dfc6)); g.fillRoundedRectangle (pu.expanded (5), 5);
                metal (pu, false);
                g.setColour (Colour (0xff222222)); g.fillRect (pu.withWidth (2).withX (pu.getCentreX() - 1));
                for (int s = 0; s < 6; ++s)
                    for (int k = 0; k < 2; ++k)
                    {
                        g.setColour (Colour (0xff5a5a5a));
                        g.fillEllipse (pu.getX() + 8 + (float) k * 22 - 3, -34 + (float) s * 13.5f - 3, 6, 6);
                    }
            }
            for (int i = 0; i < 4; ++i) // potards dorés
            {
                float kx = -290 + (float) (i % 2) * 45, ky = 40 + (float) (i / 2) * 45;
                g.setColour (Colour (0xff1a0c04)); g.fillEllipse (kx - 15, ky - 15, 30, 30);
                metal (Rectangle<float> (kx - 12, ky - 12, 24, 24), true);
            }
            auto br = Rectangle<float> (-255, -50, 18, 100);
            metal (br, false);
            metal (Rectangle<float> (-300, -45, 14, 90), false);
        }
        else
        {
            Path pg; pg.addRoundedRectangle (-235, -100, 215, 180, 55);
            g.setColour (Colour (0xfff2efe6)); g.fillPath (pg);
            g.setColour (Colour (0xffb8b2a6)); g.strokePath (pg, juce::PathStrokeType (1.2f));
            for (float px : { -190.0f, -125.0f, -60.0f })
            {
                auto pu = Rectangle<float> (px - 9, -44, 18, 88);
                g.setColour (Colour (0xffe8e6df)); g.fillRoundedRectangle (pu, 8);
                g.setColour (Colour (0xff9a968c)); g.drawRoundedRectangle (pu, 8, 1);
                for (int s = 0; s < 6; ++s) { g.setColour (Colour (0xff6a6a6a)); g.fillEllipse (px - 2.5f, -33 + (float) s * 13.2f - 2.5f, 5, 5); }
            }
            metal (Rectangle<float> (-265, -48, 30, 96), false);
            for (int i = 0; i < 3; ++i)
            {
                float kx = -150 + (float) i * 38, ky = 60 + (float) i * 8;
                g.setColour (Colour (0xfff4f1ea)); g.fillEllipse (kx - 12, ky - 12, 24, 24);
                g.setColour (Colour (0xffa8a296)); g.drawEllipse (kx - 12, ky - 12, 24, 24, 1);
            }
        }
    }

    // ---- cordes ----
    const float bridgeX = acoustic ? -237.0f : -246.0f;
    for (int s = 0; s < 6; ++s)
    {
        float y0 = -20 + (float) s * 8, y1 = -nw * 0.42f + (float) s * (nw * 0.84f / 5.0f);
        float th = 0.8f + (float) (5 - s) * 0.25f;
        Colour sc = guitar == 1 ? (s < 3 ? Colour (0xfff0ead8) : Colour (0xffd8b878)) : Colour (0xffdcdcdc);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine (bridgeX, y0 + 2, neckX0 + neckLen + 40, y1 + 2, th);
        g.setColour (sc);
        g.drawLine (bridgeX, y0, neckX0 + 10, y0 + (y1 - y0) * 0.1f, th);
        g.drawLine (neckX0 + 10, y0 + (y1 - y0) * 0.1f, neckX0 + neckLen + 40, y1, th);
    }

    } // fin du dessin de la guitare

    // vignette
    juce::ColourGradient vig (Colour (0x00000000), R.getCentreX(), R.getCentreY(), Colour (0xcc000000), 0, R.getCentreY(), true);
    g.setGradientFill (vig); g.fillRect (R);
    juce::ColourGradient bot (Colour (0x00121212), 0, H * 0.5f, Colour (0xff121212), 0, H, false);
    g.setGradientFill (bot); g.fillRect (R);

    // panneau "Bibliothèque" (les boutons sont posés par-dessus)
    auto lib = Rectangle<float> (18, 16, 220, 158);
    g.setColour (Colour (0xd8141210)); g.fillRoundedRectangle (lib, 7);
    g.setColour (edge); g.drawRoundedRectangle (lib, 7, 1);
    g.setColour (gold);
    Path folder; folder.addRoundedRectangle (lib.getX() + 12, lib.getY() + 11, 13, 10, 1.5f);
    g.strokePath (folder, juce::PathStrokeType (1.3f));
    g.setColour (cText); g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (juce::String::fromUTF8 ("BIBLIOTH\xc3\x88QUE"), lib.withTrimmedLeft (32).withHeight (30), juce::Justification::centredLeft);
}

//==============================================================================
void BakrGuitarEditor::AdvPanel::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (r.translated (0, 4), 9);
    g.setColour (Colour (0xf61a1816)); g.fillRoundedRectangle (r, 9);
    g.setColour (gold.withAlpha (0.7f)); g.drawRoundedRectangle (r.reduced (0.5f), 9, 1.2f);
    g.setColour (cText); g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (juce::String::fromUTF8 ("R\xc3\xa9glages avanc\xc3\xa9s de la cha\xc3\xaene d'effets"), 18, 10, 400, 22, juce::Justification::centredLeft);
    g.setColour (textDim); g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Temps du delay (sur le tempo)", 18, 170, 160, 22, juce::Justification::centredLeft);
}

void BakrGuitarEditor::Root::paint (Graphics& g)
{
    g.fillAll (cBg);
    // barre du haut
    auto top = Rectangle<int> (0, 0, kW, 58);
    juce::ColourGradient tg (Colour (0xff1c1a18), 0, 0, Colour (0xff121110), 0, 58, false);
    g.setGradientFill (tg); g.fillRect (top);
    g.setColour (edge); g.drawHorizontalLine (57, 0, (float) kW);

    // logo
    g.setColour (gold);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.drawText ("B", 18, 8, 30, 32, juce::Justification::centred);
    g.drawRoundedRectangle (16, 9, 34, 31, 6, 1.5f);
    g.setColour (cText); g.setFont (juce::FontOptions (21.0f, juce::Font::bold));
    g.drawText ("BAKR", 58, 6, 80, 26, juce::Justification::centredLeft);
    g.setColour (gold); g.setFont (juce::FontOptions (16.0f));
    g.drawText ("Guitar", 128, 9, 80, 22, juce::Justification::centredLeft);
    g.setColour (textDim); g.setFont (juce::FontOptions (9.5f));
    g.drawText ("GUITARES ACOUSTIQUES & ELECTRIQUES", 58, 33, 240, 14, juce::Justification::centredLeft);

    auto drawPanel = [&] (Rectangle<int> r, const juce::String& title, int icon)
    {
        auto rf = r.toFloat();
        g.setColour (panel); g.fillRoundedRectangle (rf, 7);
        g.setColour (edge); g.drawRoundedRectangle (rf.reduced (0.5f), 7, 1);
        g.setColour (gold);
        auto ic = Rectangle<float> ((float) r.getX() + 12, (float) r.getY() + 10, 12, 12);
        if (icon == 0) { g.drawLine (ic.getX(), ic.getBottom(), ic.getRight(), ic.getY(), 1.6f); g.fillEllipse (ic.getRight() - 4, ic.getY() - 1, 5, 5); }
        if (icon == 1) for (int i = 0; i < 3; ++i) g.drawLine (ic.getX(), ic.getY() + (float) i * 5, ic.getRight(), ic.getY() + (float) i * 5, 1.4f);
        if (icon == 2) for (int i = 0; i < 3; ++i) { g.drawLine (ic.getX(), ic.getY() + (float) i * 5, ic.getRight(), ic.getY() + (float) i * 5, 1.2f); g.fillEllipse (ic.getX() + (float) ((i * 5) % 9), ic.getY() + (float) i * 5 - 2, 4, 4); }
        g.setColour (cText); g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (title, r.getX() + 32, r.getY() + 6, 200, 20, juce::Justification::centredLeft);
    };
    drawPanel (articRect, "Articulations", 0);
    drawPanel (fxRect, juce::String::fromUTF8 ("Cha\xc3\xae" "ne d'effets"), 1);
    drawPanel (perfRect, "Performance", 2);

    // cadre du clavier
    auto kr = keysRect.toFloat().expanded (6, 6);
    g.setColour (Colour (0xff0d0c0b)); g.fillRoundedRectangle (kr, 7);
    g.setColour (edge); g.drawRoundedRectangle (kr, 7, 1);
}

//==============================================================================
BakrGuitarEditor::BakrGuitarEditor (BakrGuitarProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, p),
      pitch (p.pitchWheel, true, &p.uiPitchActive, "Pitch"),
      mod (p.modWheel, false, nullptr, "Mod")
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (root);
    root.addAndMakeVisible (art);

    // ---- barre du haut ----
    for (auto* b : { &prevBtn, &nextBtn }) root.addAndMakeVisible (*b);
    prevBtn.onClick = [this] { int n = proc.getNumPrograms(); proc.applyPreset ((proc.currentPreset.load() + n - 1) % n); };
    nextBtn.onClick = [this] { int n = proc.getNumPrograms(); proc.applyPreset ((proc.currentPreset.load() + 1) % n); };
    presetBox.addItemList (BakrGuitarProcessor::getPresetNames(), 1);
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.onChange = [this] { int id = presetBox.getSelectedItemIndex(); if (id >= 0 && id != proc.currentPreset.load()) proc.applyPreset (id); };
    root.addAndMakeVisible (presetBox);
    bankLabel.setText (juce::String::fromUTF8 ("\xe2\x97\x8f  Banque pr\xc3\xaa" "te"), juce::dontSendNotification);
    bankLabel.setColour (juce::Label::textColourId, Colour (0xff8fc98a));
    bankLabel.setFont (juce::FontOptions (12.0f));
    root.addAndMakeVisible (bankLabel);
    fileBtn.setTooltip ("Sauvegarder / charger un preset");
    fileBtn.onClick = [this] { showFileMenu(); };
    root.addAndMakeVisible (fileBtn);
    gearBtn.setButtonText (juce::String::fromUTF8 ("\xe2\x9a\x99"));
    gearBtn.onClick = [this] { advPanel.setVisible (! advPanel.isVisible()); if (advPanel.isVisible()) advPanel.toFront (true); };
    root.addAndMakeVisible (gearBtn);

    // ---- bibliothèque ----
    const char* codes[] = { "AC", "CN", "DB", "SB" };
    const char* names[] = { "Acier acoustique", "Classique nylon", "Double bobinage", "Simple bobinage" };
    for (int i = 0; i < 4; ++i)
    {
        auto* it = library.add (new LibraryItem (codes[i], names[i]));
        it->onClick = [this, i] { if (guitarAtt) guitarAtt->setValueAsCompleteGesture ((float) i); };
        root.addAndMakeVisible (it);
    }
    guitarAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter ("guitar"), [this] (float v)
    {
        int g = juce::roundToInt (v);
        for (int i = 0; i < library.size(); ++i) library[i]->setToggleState (i == g, juce::dontSendNotification);
        art.setGuitar (g);
    });
    guitarAtt->sendInitialUpdate();

    // ---- potards ----
    const char* kIds[] = { "attack", "release", "bright", "width", "delay", "reverb", "output" };
    const char* kNames[] = { "Attaque", "Rel\xc3\xa2" "chement", "Brillance", "Largeur", "Delay", "R\xc3\xa9verb", "Sortie" };
    for (int i = 0; i < 7; ++i)
    {
        auto* s = knobs.add (new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow));
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 80, 16);
        s->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        root.addAndMakeVisible (s);
        knobAtts.add (new SA (proc.apvts, kIds[i], *s));
        auto* l = knobLabels.add (new juce::Label ({}, juce::String::fromUTF8 (kNames[i])));
        l->setJustificationType (juce::Justification::centred);
        l->setFont (juce::FontOptions (12.5f));
        root.addAndMakeVisible (l);
    }

    // ---- articulations ----
    const char* aNames[] = { "Sustain", "Palm mute", "Harmoniques", "Hammer-on", "Pull-off", "Slide", "Vibrato", "Slide up", "Slide down" };
    for (int i = 0; i < 9; ++i)
    {
        auto* b = articBtns.add (new juce::TextButton (aNames[i]));
        b->setTooltip ("Keyswitch : " + juce::MidiMessage::getMidiNoteName (BakrGuitarProcessor::kKeyswitchLow + i, true, true, 5));
        b->onClick = [this, i] { if (articAtt) articAtt->setValueAsCompleteGesture ((float) i); };
        root.addAndMakeVisible (b);
    }
    articAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter ("artic"), [this] (float v)
    {
        int a = juce::roundToInt (v);
        for (int i = 0; i < articBtns.size(); ++i) articBtns[i]->setToggleState (i == a, juce::dontSendNotification);
        keyboard.selectedArtic = a; keyboard.repaint();
    });
    articAtt->sendInitialUpdate();

    // ---- chaîne d'effets ----
    const char* fxIds[] = { "fxAmp", "fxEq", "fxComp", "fxChorus", "fxDelay", "fxReverb" };
    const char* fxNames[] = { "Amp", "EQ", "Comp", "Chorus", "Delay", "R\xc3\xa9verb" };
    for (int i = 0; i < 6; ++i)
    {
        auto* b = fxBtns.add (new FxButton (juce::String::fromUTF8 (fxNames[i]), i));
        root.addAndMakeVisible (b);
        fxAtts.add (new BA (proc.apvts, fxIds[i], *b));
    }

    // ---- performance ----
    noiseBtn.setClickingTogglesState (true);
    strumBtn.setClickingTogglesState (true);
    root.addAndMakeVisible (noiseBtn); root.addAndMakeVisible (strumBtn);
    noiseAtt = std::make_unique<BA> (proc.apvts, "handNoise", noiseBtn);
    strumAtt = std::make_unique<BA> (proc.apvts, "strum", strumBtn);
    auto setupLabel = [this] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification); l.setFont (juce::FontOptions (12.0f));
        l.setColour (juce::Label::textColourId, textDim); root.addAndMakeVisible (l);
    };
    setupLabel (chordLabel, "Jeu en accords");
    setupLabel (dirLabel, "Sens du coup");
    setupLabel (spaceLabel, "Espacement");
    dirBox.addItemList ({ "Bas", "Haut", juce::String::fromUTF8 ("Altern\xc3\xa9") }, 1);
    root.addAndMakeVisible (dirBox);
    dirAtt = std::make_unique<CA> (proc.apvts, "strumDir", dirBox);
    spaceSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    spaceSlider.setTextBoxStyle (juce::Slider::TextBoxRight, true, 56, 16);
    root.addAndMakeVisible (spaceSlider);
    spaceAtt = std::make_unique<SA> (proc.apvts, "strumSpace", spaceSlider);

    // ---- clavier ----
    keyboard.setAvailableRange (BakrGuitarProcessor::kKeyswitchLow, 100);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setOctaveForMiddleC (5);   // comme FL Studio (Do central = C5)
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    root.addAndMakeVisible (keyboard);
    root.addAndMakeVisible (pitch);
    root.addAndMakeVisible (mod);

    // ---- réglages avancés ----
    const char* advIds[] = { "ampDrive", "eqLow", "eqMid", "eqHigh", "compAmount", "chorusMix" };
    const char* advNames[] = { "Gain ampli", "EQ Grave", "EQ M\xc3\xa9" "dium", "EQ Aigu", "Compression", "Chorus" };
    for (int i = 0; i < 6; ++i)
    {
        auto* s = advKnobs.add (new juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow));
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, true, 70, 16);
        s->setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        advPanel.addAndMakeVisible (s);
        advAtts.add (new SA (proc.apvts, advIds[i], *s));
        auto* l = advLabels.add (new juce::Label ({}, juce::String::fromUTF8 (advNames[i])));
        l->setJustificationType (juce::Justification::centred); l->setFont (juce::FontOptions (12.0f));
        advPanel.addAndMakeVisible (l);
    }
    delayTimeBox.addItemList ({ "1/4", juce::String::fromUTF8 ("1/8 point\xc3\xa9" "e"), "1/8", "1/16",
                                juce::String::fromUTF8 ("1/4 point\xc3\xa9" "e") }, 1);
    advPanel.addAndMakeVisible (delayTimeBox);
    delayTimeAtt = std::make_unique<CA> (proc.apvts, "delayTime", delayTimeBox);
    advClose.onClick = [this] { advPanel.setVisible (false); };
    advPanel.addAndMakeVisible (advClose);
    root.addChildComponent (advPanel);

    sendLookAndFeelChange();   // applique le style BAKR à tous les éléments créés ci-dessus
    setResizable (true, true);
    setResizeLimits (kW * 6 / 10, kH * 6 / 10, kW * 2, kH * 2);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize (kW, kH);
    refreshPresetBox();
    startTimerHz (30);
}

BakrGuitarEditor::~BakrGuitarEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BakrGuitarEditor::paint (Graphics& g) { g.fillAll (cBg); }

void BakrGuitarEditor::resized()
{
    root.setBounds (0, 0, kW, kH);
    root.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) kW));
    layoutRoot();
}

void BakrGuitarEditor::layoutRoot()
{
    // barre du haut
    prevBtn.setBounds (330, 14, 30, 30);
    presetBox.setBounds (364, 14, 272, 30);
    nextBtn.setBounds (640, 14, 30, 30);
    bankLabel.setBounds (790, 18, 110, 22);
    fileBtn.setBounds (906, 14, 36, 30);
    gearBtn.setBounds (948, 14, 36, 30);

    // illustration + bibliothèque
    art.setBounds (0, 58, kW, 280);
    {
        int x = 18, y = 74;
        for (int i = 0; i < library.size(); ++i) library[i]->setBounds (x + 6, y + 30 + i * 30, 208, 28);
    }

    // potards
    root.knobRow = { 0, 262, kW, 92 };
    const int kx0 = 190, kStep = 104;
    for (int i = 0; i < knobs.size(); ++i)
    {
        int cx = kx0 + i * kStep;
        knobs[i]->setBounds (cx - 40, 258, 80, 64);
        knobLabels[i]->setBounds (cx - 50, 322, 100, 18);
    }

    // panneaux
    root.articRect = { 16, 352, 318, 138 };
    root.fxRect = { 344, 352, 330, 138 };
    root.perfRect = { 684, 352, 300, 138 };

    for (int i = 0; i < articBtns.size(); ++i)
    {
        int col = i % 3, row = i / 3;
        articBtns[i]->setBounds (root.articRect.getX() + 12 + col * 100, root.articRect.getY() + 34 + row * 33, 94, 28);
    }
    for (int i = 0; i < fxBtns.size(); ++i)
        fxBtns[i]->setBounds (root.fxRect.getX() + 12 + i * 52, root.fxRect.getY() + 38, 48, 88);

    auto pr = root.perfRect;
    noiseBtn.setBounds (pr.getRight() - 122, pr.getY() + 6, 112, 24);
    chordLabel.setBounds (pr.getX() + 10, pr.getY() + 38, 120, 22);
    strumBtn.setBounds (pr.getRight() - 122, pr.getY() + 38, 112, 24);
    dirLabel.setBounds (pr.getX() + 10, pr.getY() + 70, 120, 22);
    dirBox.setBounds (pr.getRight() - 122, pr.getY() + 70, 112, 24);
    spaceLabel.setBounds (pr.getX() + 10, pr.getY() + 102, 90, 22);
    spaceSlider.setBounds (pr.getX() + 100, pr.getY() + 102, 190, 24);

    // clavier
    root.keysRect = { 16, 506, kW - 32, 118 };
    pitch.setBounds (22, 510, 30, 110);
    mod.setBounds (56, 510, 30, 110);
    keyboard.setBounds (94, 510, kW - 32 - 84, 110);
    {
        int whiteKeys = 0;
        for (int n = BakrGuitarProcessor::kKeyswitchLow; n <= 100; ++n)
            if (! juce::MidiMessage::isMidiNoteBlack (n)) ++whiteKeys;
        keyboard.setKeyWidth ((float) keyboard.getWidth() / (float) whiteKeys);
    }

    // panneau avancé
    advPanel.setBounds (220, 70, 560, 250);
    for (int i = 0; i < advKnobs.size(); ++i)
    {
        int cx = 60 + i * 88;
        advKnobs[i]->setBounds (cx - 36, 44, 72, 72);
        advLabels[i]->setBounds (cx - 44, 118, 88, 18);
    }
    delayTimeBox.setBounds (180, 168, 140, 26);
    advClose.setBounds (440, 206, 100, 28);
}

void BakrGuitarEditor::refreshPresetBox()
{
    int p = proc.currentPreset.load();
    if (p != shownPreset)
    {
        shownPreset = p;
        presetBox.setSelectedItemIndex (p, juce::dontSendNotification);
    }
}

void BakrGuitarEditor::timerCallback()
{
    // keyswitch reçu -> on met à jour le bouton d'articulation
    int ks = proc.keyswitchArtic.load();
    if (ks >= 0 && articAtt)
    {
        auto* prm = proc.apvts.getParameter ("artic");
        if (juce::roundToInt (prm->convertFrom0to1 (prm->getValue())) != ks)
            articAtt->setValueAsCompleteGesture ((float) ks);
    }
    refreshPresetBox();
    pitch.repaint(); mod.repaint();
}

void BakrGuitarEditor::showFileMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Sauvegarder le preset...");
    m.addItem (2, "Charger un preset...");
    m.addSeparator();
    m.addItem (3, "Ouvrir le dossier des presets");
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("BAKR").getChildFile ("BAKR GUITAR Presets");
    dir.createDirectory();

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (fileBtn), [this, dir] (int r)
    {
        if (r == 1)
        {
            chooser = std::make_unique<juce::FileChooser> ("Sauvegarder", dir.getChildFile ("Mon preset.bakrgtr"), "*.bakrgtr");
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                                    | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f == juce::File()) return;
                if (auto xml = proc.apvts.copyState().createXml()) xml->writeTo (f.withFileExtension ("bakrgtr"));
            });
        }
        else if (r == 2)
        {
            chooser = std::make_unique<juce::FileChooser> ("Charger", dir, "*.bakrgtr");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (! f.existsAsFile()) return;
                if (auto xml = juce::XmlDocument::parse (f))
                    if (xml->hasTagName (proc.apvts.state.getType()))
                    {
                        proc.apvts.replaceState (juce::ValueTree::fromXml (*xml));
                        presetBox.setText (f.getFileNameWithoutExtension(), juce::dontSendNotification);
                    }
            });
        }
        else if (r == 3) dir.startAsProcess();
    });
}
