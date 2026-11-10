#include "PluginEditor.h"

using namespace juce;

namespace ui
{
Font font (float h, bool bold)
{
    return Font (FontOptions().withHeight (h).withStyle (bold ? "Bold" : "Regular"));
}

//==============================================================================
void drawIcon (Graphics& g, const String& name, Rectangle<float> r, Colour c)
{
    g.setColour (c);
    const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight();
    const float cx = r.getCentreX(), cy = r.getCentreY();
    PathStrokeType st (std::max (1.4f, w * 0.07f), PathStrokeType::curved, PathStrokeType::rounded);

    if (name == "denoise")
    {
        Path p; const float pts[] = { 0.5f, 0.35f, 0.65f, 0.1f, 0.8f, 0.3f, 0.55f, 0.15f, 0.7f, 0.5f };
        p.startNewSubPath (x, y + h * 0.5f);
        for (int i = 0; i < 10; ++i) p.lineTo (x + w * (i + 1) / 11.0f, y + h * (1.0f - pts[i]) * 0.9f);
        p.lineTo (x + w, y + h * 0.5f);
        g.strokePath (p, st);
    }
    else if (name == "dereverb")
    {
        for (int i = 1; i <= 3; ++i)
        {
            const float rr = w * 0.5f * i / 3.0f;
            g.drawEllipse (cx - rr, cy - rr, rr * 2, rr * 2, st.getStrokeThickness() * 0.8f);
        }
        g.fillEllipse (cx - w * 0.06f, cy - w * 0.06f, w * 0.12f, w * 0.12f);
    }
    else if (name == "tone")
    {
        Path p; p.startNewSubPath (x, cy);
        p.cubicTo (x + w * 0.25f, cy, x + w * 0.3f, y, x + w * 0.45f, y);
        p.lineTo (x + w * 0.5f, y + h);
        p.cubicTo (x + w * 0.65f, y + h, x + w * 0.75f, cy, x + w, cy);
        g.strokePath (p, st);
    }
    else if (name == "shape")
    {
        Path p; p.startNewSubPath (x, cy);
        p.cubicTo (x + w * 0.2f, y, x + w * 0.35f, y, x + w * 0.5f, cy);
        p.cubicTo (x + w * 0.65f, y + h, x + w * 0.8f, y + h, x + w, cy);
        g.strokePath (p, st);
    }
    else if (name == "thresh")
    {
        g.setFont (font (h * 0.62f, true));
        g.drawText ("Ss", r.withTrimmedRight (w * 0.25f), Justification::centred);
        Path p; p.startNewSubPath (x + w * 0.8f, y + h * 0.15f); p.lineTo (x + w * 0.95f, y + h * 0.15f);
        p.lineTo (x + w * 0.95f, y + h * 0.85f); p.lineTo (x + w * 0.8f, y + h * 0.85f);
        g.strokePath (p, st);
    }
    else if (name == "gain")
    {
        const float hs[] = { 0.3f, 0.6f, 1.0f, 0.7f, 0.4f };
        for (int i = 0; i < 5; ++i)
        {
            const float bx = x + w * (0.1f + i * 0.2f), bh = h * hs[i];
            g.drawLine (bx, cy - bh / 2, bx, cy + bh / 2, st.getStrokeThickness());
        }
    }
    else if (name == "voice")
    {
        Path p;
        p.addEllipse (x + w * 0.22f, y + h * 0.12f, w * 0.42f, h * 0.48f);
        p.startNewSubPath (x + w * 0.3f, y + h * 0.58f);
        p.lineTo (x + w * 0.3f, y + h * 0.9f);
        p.startNewSubPath (x + w * 0.58f, y + h * 0.58f);
        p.lineTo (x + w * 0.62f, y + h * 0.9f);
        g.strokePath (p, st);
        Path wv; wv.addCentredArc (x + w * 0.66f, y + h * 0.36f, w * 0.22f, h * 0.22f, 0, 0.6f, 2.5f, true);
        g.strokePath (wv, st);
    }
    else if (name == "piano")
    {
        auto k = r.reduced (w * 0.08f, h * 0.15f);
        g.drawRoundedRectangle (k, 2.0f, st.getStrokeThickness());
        for (int i = 1; i < 4; ++i) g.drawLine (k.getX() + k.getWidth() * i / 4.0f, k.getY(), k.getX() + k.getWidth() * i / 4.0f, k.getBottom(), 1.2f);
        for (int i : { 1, 2, 3 }) g.fillRect (k.getX() + k.getWidth() * i / 4.0f - k.getWidth() * 0.07f, k.getY(), k.getWidth() * 0.14f, k.getHeight() * 0.55f);
    }
    else if (name == "reset")
    {
        Path p; p.addCentredArc (cx, cy, w * 0.38f, h * 0.38f, 0, 0.5f, MathConstants<float>::twoPi - 0.2f, true);
        g.strokePath (p, st);
        Path a; const float ax = cx + w * 0.38f * std::sin (0.5f), ay = cy - h * 0.38f * std::cos (0.5f);
        a.addTriangle (ax - w * 0.16f, ay - h * 0.08f, ax + w * 0.06f, ay - h * 0.16f, ax + w * 0.02f, ay + h * 0.1f);
        g.fillPath (a);
    }
    else if (name == "gear")
    {
        Path p;
        for (int i = 0; i < 8; ++i)
        {
            const float a = i * MathConstants<float>::twoPi / 8.0f;
            Path t; t.addRoundedRectangle (-w * 0.08f, -h * 0.48f, w * 0.16f, h * 0.2f, 1.5f);
            p.addPath (t, AffineTransform::rotation (a).translated (cx, cy));
        }
        g.fillPath (p);
        g.drawEllipse (cx - w * 0.3f, cy - h * 0.3f, w * 0.6f, h * 0.6f, st.getStrokeThickness());
        g.drawEllipse (cx - w * 0.11f, cy - h * 0.11f, w * 0.22f, h * 0.22f, st.getStrokeThickness());
    }
    else if (name == "help")
    {
        g.drawEllipse (r.reduced (w * 0.06f), st.getStrokeThickness());
        g.setFont (font (h * 0.62f, true));
        g.drawText ("?", r, Justification::centred);
    }
    else if (name == "ear")
    {
        Path p;
        p.startNewSubPath (x + w * 0.3f, y + h * 0.4f);
        p.cubicTo (x + w * 0.3f, y + h * 0.05f, x + w * 0.85f, y + h * 0.05f, x + w * 0.8f, y + h * 0.42f);
        p.cubicTo (x + w * 0.77f, y + h * 0.62f, x + w * 0.55f, y + h * 0.62f, x + w * 0.55f, y + h * 0.8f);
        p.cubicTo (x + w * 0.55f, y + h * 0.98f, x + w * 0.3f, y + h * 0.98f, x + w * 0.28f, y + h * 0.82f);
        p.startNewSubPath (x + w * 0.45f, y + h * 0.42f);
        p.cubicTo (x + w * 0.45f, y + h * 0.25f, x + w * 0.67f, y + h * 0.25f, x + w * 0.65f, y + h * 0.42f);
        g.strokePath (p, st);
    }
    else if (name == "logo")
    {
        for (int i = 1; i <= 4; ++i)
        {
            const float rr = w * 0.5f * i / 4.0f;
            Path a; a.addCentredArc (cx, cy, rr, rr, 0, -2.3f, -0.85f, true);
            Path b; b.addCentredArc (cx, cy, rr, rr, 0, 0.85f, 2.3f, true);
            g.strokePath (a, st); g.strokePath (b, st);
        }
        g.fillEllipse (cx - w * 0.07f, cy - w * 0.07f, w * 0.14f, w * 0.14f);
    }
}

//==============================================================================
LookAndFeel::LookAndFeel()
{
    setColour (PopupMenu::backgroundColourId, col::panel);
    setColour (PopupMenu::textColourId, col::text);
    setColour (PopupMenu::highlightedBackgroundColourId, col::accentLo);
    setColour (ComboBox::textColourId, col::text);
    setColour (ComboBox::backgroundColourId, col::module);
    setColour (ComboBox::outlineColourId, col::border);
    setColour (ComboBox::arrowColourId, col::dim);
    setColour (TextButton::buttonColourId, col::module);
    setColour (TextButton::buttonOnColourId, col::amber.withAlpha (0.85f));
    setColour (TextButton::textColourOffId, col::text);
    setColour (TextButton::textColourOnId, Colours::black);
    setColour (BubbleComponent::backgroundColourId, col::panel);
    setColour (BubbleComponent::outlineColourId, col::accent);
    setColour (TooltipWindow::textColourId, col::text);
    setColour (Label::textColourId, col::text);
}

void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = std::min (w, h) * 0.5f - 6.0f;
    const float cx = bounds.getCentreX(), cy = bounds.getCentreY();
    const float ringW = std::max (3.0f, r * 0.11f);
    const float ar = r - ringW * 0.5f;
    const bool bipolar = (bool) s.getProperties()["bipolar"];
    const float ang = a0 + pos * (a1 - a0);

    Path track; track.addCentredArc (cx, cy, ar, ar, 0, a0, a1, true);
    g.setColour (col::track);
    g.strokePath (track, PathStrokeType (ringW, PathStrokeType::curved, PathStrokeType::rounded));

    const float from = bipolar ? (a0 + a1) * 0.5f : a0;
    if (std::abs (ang - from) > 0.01f)
    {
        Path v; v.addCentredArc (cx, cy, ar, ar, 0, std::min (from, ang), std::max (from, ang), true);
        g.setColour (col::accent.withAlpha (0.22f));
        g.strokePath (v, PathStrokeType (ringW * 2.6f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (col::accent);
        g.strokePath (v, PathStrokeType (ringW, PathStrokeType::curved, PathStrokeType::rounded));
    }

    // corps du bouton
    const float kr = r - ringW * 2.0f;
    g.setColour (Colours::black.withAlpha (0.55f));
    g.fillEllipse (cx - kr - 2, cy - kr + 2, kr * 2 + 4, kr * 2 + 4);
    ColourGradient grad (Colour (0xff2b3345), cx, cy - kr, Colour (0xff0b0f17), cx, cy + kr, false);
    g.setGradientFill (grad);
    g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawEllipse (cx - kr, cy - kr, kr * 2, kr * 2, 1.2f);

    // repère
    const float ix = cx + (ar) * std::sin (ang), iy = cy - (ar) * std::cos (ang);
    g.setColour (Colours::white);
    g.fillEllipse (ix - ringW * 0.45f, iy - ringW * 0.45f, ringW * 0.9f, ringW * 0.9f);
    const float tx0 = cx + (kr - 4) * std::sin (ang), ty0 = cy - (kr - 4) * std::cos (ang);
    const float tx1 = cx + (kr - kr * 0.28f) * std::sin (ang), ty1 = cy - (kr - kr * 0.28f) * std::cos (ang);
    g.setColour (Colours::white.withAlpha (0.85f));
    g.drawLine (tx0, ty0, tx1, ty1, 2.0f);

    const String icon = s.getProperties()["icon"].toString();
    if (icon.isNotEmpty())
        drawIcon (g, icon, Rectangle<float> (kr * 0.75f, kr * 0.55f).withCentre ({ cx, cy }), Colours::white.withAlpha (0.85f));
}

void LookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                    Slider::SliderStyle, Slider&)
{
    const float cx = x + w * 0.5f;
    g.setColour (col::track);
    g.fillRoundedRectangle (cx - 2.5f, (float) y, 5.0f, (float) h, 2.5f);
    g.setColour (col::accent);
    g.fillRoundedRectangle (cx - 2.5f, pos, 5.0f, (float) (y + h) - pos, 2.5f);
    g.setColour (Colours::white);
    g.fillEllipse (cx - 9, pos - 9, 18, 18);
    g.setColour (Colours::black.withAlpha (0.4f));
    g.drawEllipse (cx - 9, pos - 9, 18, 18, 1.0f);
}

void LookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (col::module);
    g.fillRoundedRectangle (r, h * 0.5f);
    g.setColour (box.hasKeyboardFocus (false) ? col::accent : col::border);
    g.drawRoundedRectangle (r, h * 0.5f, 1.2f);
    Path a; const float ax = w - 20.0f, ay = h * 0.5f;
    a.addTriangle (ax - 5, ay - 2.5f, ax + 5, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (col::dim); g.fillPath (a);
}

//==============================================================================
void PillSwitch::paintButton (Graphics& g, bool over, bool)
{
    const bool on = getToggleState();
    auto r = getLocalBounds().toFloat();
    auto pill = Rectangle<float> (0, (r.getHeight() - 20) * 0.5f, 40, 20);
    g.setColour (on ? col::accent.withAlpha (0.25f) : Colour (0xff2b3243));
    g.fillRoundedRectangle (pill, 10);
    g.setColour (on ? col::accent : col::border);
    g.drawRoundedRectangle (pill, 10, 1.2f);
    const float kx = on ? pill.getRight() - 18 : pill.getX() + 2;
    if (on) { g.setColour (col::accent.withAlpha (0.35f)); g.fillEllipse (kx - 3, pill.getY() - 1, 22, 22); }
    g.setColour (on ? col::accent : Colour (0xffc9cfdb));
    g.fillEllipse (kx, pill.getY() + 2, 16, 16);
    g.setColour (over ? Colours::white : col::text);
    g.setFont (font (16.0f));
    g.drawText (getButtonText(), r.withTrimmedLeft (48), Justification::centredLeft);
}

void PowerButton::paintButton (Graphics& g, bool over, bool)
{
    auto r = getLocalBounds().toFloat().reduced (2);
    const bool on = getToggleState();
    const Colour c = on ? col::accent : col::dim;
    if (on) { g.setColour (col::accent.withAlpha (0.18f)); g.fillEllipse (r); }
    g.setColour (over ? c.brighter (0.3f) : c);
    g.drawEllipse (r.reduced (1), 1.6f);
    g.drawLine (r.getCentreX(), r.getY() + r.getHeight() * 0.28f, r.getCentreX(), r.getY() + r.getHeight() * 0.72f, 2.2f);
}

void EarButton::paintButton (Graphics& g, bool over, bool)
{
    const bool on = getToggleState();
    auto r = getLocalBounds().toFloat().reduced (3);
    if (on) { g.setColour (col::accent.withAlpha (0.22f)); g.fillRoundedRectangle (r.expanded (2), 6); }
    drawIcon (g, "ear", r, on ? col::accent : (over ? col::text : col::dim));
}

void IconButton::paintButton (Graphics& g, bool over, bool down)
{
    drawIcon (g, icon, getLocalBounds().toFloat().reduced (3), down ? col::accent : (over ? Colours::white : col::text));
}

//==============================================================================
SegPair::SegPair (RangedAudioParameter& p, String a, String b, bool vertical, String iconA, String iconB)
    : att (p, [this] (float v) { value = v > 0.5f; repaint(); }),
      la (std::move (a)), lb (std::move (b)), ia (std::move (iconA)), ib (std::move (iconB)), vert (vertical)
{
    att.sendInitialUpdate();
}

void SegPair::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    const float rad = vert ? 8.0f : r.getHeight() * 0.5f;
    g.setColour (Colour (0xff0c1220));
    g.fillRoundedRectangle (r, rad);
    g.setColour (col::border);
    g.drawRoundedRectangle (r, rad, 1.2f);

    auto first = vert ? r.withHeight (r.getHeight() * 0.5f) : r.withWidth (r.getWidth() * 0.5f);
    auto second = vert ? r.withTrimmedTop (r.getHeight() * 0.5f) : r.withTrimmedLeft (r.getWidth() * 0.5f);
    auto sel = value ? second : first;
    g.setColour (vert ? Colour (0xff3a4a6b) : col::accent);
    g.fillRoundedRectangle (sel.reduced (3), rad - 2);

    auto drawSeg = [&] (Rectangle<float> s, const String& label, const String& icon, bool active)
    {
        const Colour c = active ? Colours::white : col::dim;
        if (icon.isNotEmpty())
        {
            drawIcon (g, icon, Rectangle<float> (s.getHeight() * 0.62f, s.getHeight() * 0.62f).withCentre (s.getCentre()), c);
        }
        else
        {
            g.setColour (c); g.setFont (font (17.0f));
            g.drawText (label, s, Justification::centred);
        }
    };
    drawSeg (first, la, ia, ! value);
    drawSeg (second, lb, ib, value);
}

void SegPair::mouseDown (const MouseEvent& e)
{
    const bool second = vert ? e.position.y > getHeight() * 0.5f : e.position.x > getWidth() * 0.5f;
    if (second != value) att.setValueAsCompleteGesture (second ? 1.0f : 0.0f);
}

//==============================================================================
Spectrum::Spectrum (BakrRepairProcessor& p) : proc (p)
{
    win.resize (N); buf.assign (2 * N, 0); preDb.assign (N / 2, -100); postDb.assign (N / 2, -100);
    for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (MathConstants<float>::twoPi * i / (float) N);
}

void Spectrum::update()
{
    const int wp = proc.specWritePos.load();
    auto run = [&] (const std::array<float, BakrRepairProcessor::specSize>& src, std::vector<float>& dst)
    {
        for (int i = 0; i < N; ++i)
            buf[(size_t) i] = src[(size_t) ((wp - N + i) & (BakrRepairProcessor::specSize - 1))] * win[(size_t) i];
        std::fill (buf.begin() + N, buf.end(), 0.0f);
        fft.performFrequencyOnlyForwardTransform (buf.data());
        for (int k = 0; k < N / 2; ++k)
        {
            const float db = Decibels::gainToDecibels (buf[(size_t) k] * 4.0f / N, -120.0f);
            dst[(size_t) k] = db > dst[(size_t) k] ? 0.5f * dst[(size_t) k] + 0.5f * db : 0.85f * dst[(size_t) k] + 0.15f * db;
        }
    };
    run (proc.specPre, preDb);
    run (proc.specPost, postDb);
    repaint();
}

void Spectrum::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b1424));
    g.fillRect (r);
    const double fs = proc.getCurrentSampleRate();
    const float W = r.getWidth(), H = r.getHeight();
    auto fx = [&] (double f) { return (float) (std::log (f / 20.0) / std::log (20000.0 / 20.0)) * W; };

    g.setColour (Colours::white.withAlpha (0.05f));
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
        g.drawVerticalLine ((int) fx (f), 0, H);
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (H * i / 4.0f), 0, W);

    auto makePath = [&] (const std::vector<float>& db, bool closed)
    {
        Path p; bool started = false;
        for (int px = 0; px <= (int) W; px += 2)
        {
            const double f = 20.0 * std::pow (1000.0, px / (double) W);
            const double bin = f * N / fs;
            const int b0 = std::clamp ((int) bin, 1, N / 2 - 2);
            const float frac = (float) (bin - b0);
            float d = db[(size_t) b0] * (1 - frac) + db[(size_t) b0 + 1] * frac;
            d += 4.5f * (float) std::log2 (f / 1000.0);   // pente d'affichage
            const float yy = jlimit (0.0f, H, H * (1.0f - (d + 90.0f) / 84.0f));
            if (! started) { p.startNewSubPath ((float) px, yy); started = true; }
            else p.lineTo ((float) px, yy);
        }
        if (closed) { p.lineTo (W, H); p.lineTo (0, H); p.closeSubPath(); }
        return p;
    };

    auto fill = makePath (postDb, true);
    g.setGradientFill (ColourGradient (col::accent.withAlpha (0.45f), 0, 0, col::accent.withAlpha (0.05f), 0, H, false));
    g.fillPath (fill);
    g.setColour (Colour (0xff9aa3b5).withAlpha (0.9f));
    g.strokePath (makePath (preDb, false), PathStrokeType (1.3f));
    g.setColour (col::accent.brighter (0.2f));
    g.strokePath (makePath (postDb, false), PathStrokeType (2.0f));

    g.setColour (col::border);
    g.drawRect (r, 1.0f);
}

//==============================================================================
void DeclipMeter::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto box = r.withTrimmedLeft (34);
    g.setColour (Colour (0xff0a0f1a)); g.fillRect (box);
    g.setColour (col::border); g.drawRect (box, 1.0f);

    auto yOf = [&] (float db) { return box.getY() + box.getHeight() * (-db / 64.0f); };
    g.setFont (font (14.0f)); g.setColour (col::text);
    for (int d : { 0, -32, -64 })
        g.drawText (String (d), Rectangle<float> (0, jlimit (box.getY(), box.getBottom() - 14, yOf ((float) d) - 7), 30, 14), Justification::centredRight);

    const float pk = jlimit (-64.0f, 0.0f, proc.dcInputPeakDb.load());
    const float thr = proc.apvts.getRawParameterValue ("dcThresh")->load();
    const float yp = yOf (pk);
    auto bar = Rectangle<float> (box.getX() + 4, yp, box.getWidth() - 8, box.getBottom() - yp - 1);
    g.setGradientFill (ColourGradient (pk > thr - 0.5f ? col::amber : col::accent, 0, box.getY(), col::accentLo.withAlpha (0.4f), 0, box.getBottom(), false));
    g.fillRect (bar);
    g.setColour (col::accent.brighter (0.4f));
    g.drawHorizontalLine ((int) yOf (thr) + 1, box.getX() + 1, box.getRight() - 1);
}

void GRMeter::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff3a4254)); g.fillRect (r);
    const float gr = jlimit (0.0f, 18.0f, proc.dsGainReductionDb.load());
    g.setColour (col::accent);
    g.fillRect (r.withHeight (r.getHeight() * gr / 18.0f));
}

void LearnButton::paintButton (Graphics& g, bool over, bool)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    const bool learning = proc.isLearning();
    g.setColour (Colour (0xff0c1220)); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    if (learning)
    {
        g.setColour (col::accent.withAlpha (0.35f));
        g.fillRoundedRectangle (r.withWidth (r.getWidth() * proc.getLearnProgress()), r.getHeight() * 0.5f);
    }
    g.setColour (over || learning ? col::accent : col::border);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.4f);

    auto ic = Rectangle<float> (r.getHeight() * 0.55f, r.getHeight() * 0.55f).withCentre ({ r.getX() + r.getHeight() * 0.75f, r.getCentreY() });
    ColourGradient grad (Colour (0xffff5a5a), ic.getX(), ic.getY(), col::accent, ic.getRight(), ic.getBottom(), false);
    grad.addColour (0.5, Colour (0xffffb340));
    g.setGradientFill (grad); g.fillEllipse (ic);
    g.setColour (Colour (0xff0c1220)); g.fillEllipse (ic.reduced (ic.getWidth() * 0.28f));

    g.setColour (col::text); g.setFont (font (18.0f));
    const String t = learning ? "Ecoute " + String (roundToInt (proc.getLearnProgress() * 100)) + "%" : "Learn";
    g.drawText (t, r.withTrimmedLeft (r.getHeight() * 1.2f), Justification::centredLeft);
}

void HelpOverlay::paint (Graphics& g)
{
    g.fillAll (Colours::black.withAlpha (0.82f));
    auto r = getLocalBounds().reduced (50, 40);
    g.setColour (col::panel); g.fillRoundedRectangle (r.toFloat(), 12);
    g.setColour (col::accent); g.drawRoundedRectangle (r.toFloat(), 12, 1.5f);
    r.reduce (28, 22);
    g.setColour (col::text); g.setFont (font (22.0f, true));
    g.drawText ("BAKR REPAIR - mode d'emploi", r.removeFromTop (34), Justification::centredLeft);
    g.setFont (font (15.0f));
    const String txt =
        "LEARN : lance la lecture de ta voix dans FL Studio puis clique Learn. Le plugin ecoute 4 secondes\n"
        "et regle tout seul les modules. Choisis d'abord Voix ou Instrument.\n\n"
        "CLEAN UP : De-noise enleve le souffle et le bruit de fond. De-reverb enleve la resonance de la piece.\n"
        "De-hum coupe le ronflement electrique (50 Hz, ou 60 Hz dans la roue dentee). De-click enleve les clics.\n\n"
        "TONE : Tilt = plus brillant (+) ou plus chaud (-). Scoop = creuse les mediums boueux (+).\n"
        "Lo-cut coupe les graves sous 80 Hz, Hi-cut adoucit les aigus au-dessus de 15 kHz.\n\n"
        "DE-ESS : Threshold plus bas = plus de reduction des 'S'. Shaping choisit la frequence (3,5 a 9 kHz).\n\n"
        "DE-CLIP : place Threshold juste sous le niveau ou la voix sature : les cretes ecretees sont reconstruites.\n"
        "Output Gain + Limiter evitent de depasser 0 dB.\n\n"
        "OREILLE : ecoute uniquement ce que le module enleve (pour verifier qu'il ne mange pas la voix).\n\n"
        "Latence d'environ 27 ms (compensee par FL Studio) : a utiliser au mixage, pas pendant l'enregistrement.\n\n"
        "Clique n'importe ou pour fermer.";
    g.drawFittedText (txt, r, Justification::topLeft, 40, 1.0f);
}
} // namespace ui

//==============================================================================
BakrRepairEditor::BakrRepairEditor (BakrRepairProcessor& p)
    : AudioProcessorEditor (&p), proc (p), spectrum (p), learn (p), grMeter (p), dcMeter (p)
{
    setLookAndFeel (&laf);
    auto& s = p.apvts;

    addAndMakeVisible (spectrum);
    addAndMakeVisible (learn);
    learn.onClick = [this] { if (proc.isLearning()) proc.cancelLearn(); else proc.startLearn(); };
    learn.setTooltip ("Joue ta voix puis clique : reglage automatique");

    for (auto* b : { &resetBtn, &settingsBtn, &helpBtn }) addAndMakeVisible (*b);
    resetBtn.onClick = [this] { proc.applyPreset (0); presets.setSelectedId (1, dontSendNotification); };
    settingsBtn.onClick = [this]
    {
        PopupMenu m;
        const bool h60 = proc.apvts.getRawParameterValue ("hum60")->load() > 0.5f;
        m.addSectionHeader ("Frequence du secteur (De-hum)");
        m.addItem (1, "50 Hz (Europe, Afrique)", true, ! h60);
        m.addItem (2, "60 Hz (USA)", true, h60);
        m.addSeparator();
        m.addItem (3, "Reinitialiser tous les reglages");
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (settingsBtn), [this] (int r)
        {
            if (r == 1) proc.setParam ("hum60", 0);
            else if (r == 2) proc.setParam ("hum60", 1);
            else if (r == 3) { proc.applyPreset (0); presets.setSelectedId (1, dontSendNotification); }
        });
    };
    helpBtn.onClick = [this] { help.setVisible (true); help.toFront (false); };

    modeSeg  = std::make_unique<ui::SegPair> (*s.getParameter ("instrument"), "Voix", "Instrument", false, "voice", "piano");
    toneSeg  = std::make_unique<ui::SegPair> (*s.getParameter ("toneScoop"), "Tilt", "Scoop", true);
    speedSeg = std::make_unique<ui::SegPair> (*s.getParameter ("dsSlow"), "Fast", "Slow", true);
    addAndMakeVisible (*modeSeg); addAndMakeVisible (*toneSeg); addAndMakeVisible (*speedSeg);

    setupToggle (cuPower, "cuOn"); setupToggle (tonePower, "toneOn"); setupToggle (dsPower, "dsOn"); setupToggle (dcPower, "dcOn");
    setupToggle (cuEar, "listenCU"); setupToggle (dsEar, "listenDS"); setupToggle (dcEar, "listenDC");
    for (auto* e : { &cuEar, &dsEar, &dcEar }) e->setTooltip ("Ecouter ce qui est enleve");

    setupKnob (denoise, "denoise", "denoise");
    setupKnob (dereverb, "dereverb", "dereverb");
    setupKnob (tone, "tone", "tone", true);
    setupKnob (shaping, "dsShape", "shape");
    setupKnob (dsThresh, "dsThresh", "thresh");
    setupKnob (outGain, "outGain", "gain", true);

    dcThresh.setSliderStyle (Slider::LinearVertical);
    dcThresh.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    dcThresh.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (dcThresh);
    sliderAtts.push_back (std::make_unique<SA> (s, "dcThresh", dcThresh));
    dcThresh.onValueChange = [this] { dcValue.setText (String (dcThresh.getValue(), 1), dontSendNotification); };
    dcValue.setFont (ui::font (15.0f)); dcValue.setJustificationType (Justification::centred);
    dcValue.setText (String (dcThresh.getValue(), 1), dontSendNotification);
    addAndMakeVisible (dcValue);

    setupToggle (dehum, "dehum"); setupToggle (declick, "declick");
    setupToggle (locut, "locut"); setupToggle (hicut, "hicut"); setupToggle (limiter, "limiter");
    addAndMakeVisible (grMeter); addAndMakeVisible (dcMeter);

    bypass.setClickingTogglesState (true);
    addAndMakeVisible (bypass);
    buttonAtts.push_back (std::make_unique<BA> (s, "bypass", bypass));

    presets.addItemList (BakrRepairProcessor::getPresetNames(), 1);
    presets.setTextWhenNothingSelected ("Presets BAKR...");
    presets.onChange = [this] { const int id = presets.getSelectedId(); if (id > 0) proc.applyPreset (id - 1); };
    addAndMakeVisible (presets);

    addChildComponent (help);

    setSize (820, 780);
    startTimerHz (30);
}

BakrRepairEditor::~BakrRepairEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BakrRepairEditor::setupKnob (Slider& sl, const String& id, const String& icon, bool bipolar)
{
    sl.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    sl.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    sl.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    sl.setPopupDisplayEnabled (true, true, this);
    sl.getProperties().set ("icon", icon);
    sl.getProperties().set ("bipolar", bipolar);
    sl.setDoubleClickReturnValue (true, (double) proc.apvts.getParameter (id)->convertFrom0to1 (proc.apvts.getParameter (id)->getDefaultValue()));
    addAndMakeVisible (sl);
    sliderAtts.push_back (std::make_unique<SA> (proc.apvts, id, sl));
}

void BakrRepairEditor::setupToggle (Button& b, const String& id)
{
    addAndMakeVisible (b);
    buttonAtts.push_back (std::make_unique<BA> (proc.apvts, id, b));
}

void BakrRepairEditor::timerCallback()
{
    spectrum.update();
    grMeter.repaint();
    dcMeter.repaint();
    learn.repaint();
    proc.applyLearnResultIfReady();
}

void BakrRepairEditor::drawModule (Graphics& g, Rectangle<int> r, const String& title)
{
    auto rf = r.toFloat();
    g.setColour (ui::col::module); g.fillRoundedRectangle (rf, 10);
    g.setColour (ui::col::border); g.drawRoundedRectangle (rf.reduced (0.5f), 10, 1.3f);
    g.setColour (ui::col::text); g.setFont (ui::font (17.0f));
    g.drawText (title, r.withHeight (46), Justification::centred);
}

void BakrRepairEditor::paint (Graphics& g)
{
    g.fillAll (Colour (0xff05080f));
    auto outer = getLocalBounds().toFloat().reduced (8);
    g.setColour (ui::col::panel); g.fillRoundedRectangle (outer, 16);
    g.setColour (ui::col::border.withAlpha (0.7f)); g.drawRoundedRectangle (outer, 16, 1.2f);

    // logo
    ui::drawIcon (g, "logo", { 26, 26, 40, 40 }, ui::col::text);
    g.setColour (Colours::white); g.setFont (ui::font (34.0f, true));
    g.drawText ("BAKR", 74, 24, 110, 44, Justification::centredLeft);
    g.setFont (ui::font (14.0f)); g.setColour (ui::col::text.withAlpha (0.85f));
    g.drawText ("Repair", 172, 28, 80, 18, Justification::centredLeft);
    g.drawText ("Assistant", 172, 45, 80, 18, Justification::centredLeft);

    drawModule (g, rCU, "CLEAN UP");
    drawModule (g, rTone, "TONE");
    drawModule (g, rDS, "DE-ESS");
    drawModule (g, rDC, "DE-CLIP");

    auto label = [&] (Slider& s, const String& t, float size = 17.0f)
    {
        g.setColour (ui::col::text); g.setFont (ui::font (size));
        g.drawText (t, s.getX() - 20, s.getBottom() - 2, s.getWidth() + 40, 22, Justification::centred);
    };
    label (denoise, "De-noise"); label (dereverb, "De-reverb"); label (tone, "Tone");
    label (shaping, "Shaping"); label (dsThresh, "Threshold"); label (outGain, "Output Gain");
    g.setColour (ui::col::text); g.setFont (ui::font (16.0f));
    g.drawText ("Threshold", dcMeter.getX() + 34, dcMeter.getY() - 26, dcMeter.getWidth() - 34, 20, Justification::centred);

    g.setColour (ui::col::dim); g.setFont (ui::font (12.0f));
    g.drawText ("BAKR REPAIR v1.0", getWidth() - 200, getHeight() - 52, 170, 30, Justification::centredRight);
}

void BakrRepairEditor::resized()
{
    const int W = getWidth();
    modeSeg->setBounds (268, 24, 200, 42);
    learn.setBounds (486, 24, 150, 42);
    resetBtn.setBounds (652, 30, 30, 30);
    settingsBtn.setBounds (692, 30, 30, 30);
    helpBtn.setBounds (732, 30, 30, 30);
    spectrum.setBounds (30, 86, W - 60, 92);

    const int colW = (W - 60 - 18) / 2;
    rCU   = { 30, 196, colW, 226 };
    rTone = { 30 + colW + 18, 196, colW, 226 };
    rDS   = { 30, 440, colW, 226 };
    rDC   = { 30 + colW + 18, 440, colW, 226 };

    auto power = [] (Button& b, Rectangle<int> r) { b.setBounds (r.getX() + 12, r.getY() + 12, 26, 26); };
    auto ear   = [] (Button& b, Rectangle<int> r) { b.setBounds (r.getRight() - 40, r.getY() + 10, 28, 28); };
    power (cuPower, rCU); power (tonePower, rTone); power (dsPower, rDS); power (dcPower, rDC);
    ear (cuEar, rCU); ear (dsEar, rDS); ear (dcEar, rDC);

    // CLEAN UP
    denoise.setBounds (rCU.getX() + 40, rCU.getY() + 48, 108, 108);
    dereverb.setBounds (rCU.getRight() - 148, rCU.getY() + 48, 108, 108);
    dehum.setBounds (rCU.getX() + 24, rCU.getBottom() - 42, 140, 28);
    declick.setBounds (rCU.getX() + rCU.getWidth() / 2 + 10, rCU.getBottom() - 42, 140, 28);

    // TONE
    tone.setBounds (rTone.getX() + 50, rTone.getY() + 48, 108, 108);
    toneSeg->setBounds (rTone.getRight() - 136, rTone.getY() + 62, 96, 76);
    locut.setBounds (rTone.getX() + 50, rTone.getBottom() - 42, 140, 28);
    hicut.setBounds (rTone.getX() + rTone.getWidth() / 2 + 20, rTone.getBottom() - 42, 140, 28);

    // DE-ESS
    shaping.setBounds (rDS.getX() + 18, rDS.getY() + 82, 78, 78);
    dsThresh.setBounds (rDS.getX() + 112, rDS.getY() + 56, 112, 112);
    grMeter.setBounds (rDS.getX() + 238, rDS.getY() + 82, 10, 72);
    speedSeg->setBounds (rDS.getRight() - 96, rDS.getY() + 76, 76, 76);

    // DE-CLIP
    dcMeter.setBounds (rDC.getX() + 14, rDC.getY() + 70, 116, 140);
    dcThresh.setBounds (rDC.getX() + 134, rDC.getY() + 62, 30, 128);
    dcValue.setBounds (rDC.getX() + 120, rDC.getY() + 190, 60, 22);
    outGain.setBounds (rDC.getRight() - 150, rDC.getY() + 52, 86, 86);
    limiter.setBounds (rDC.getRight() - 160, rDC.getBottom() - 54, 140, 28);

    bypass.setBounds (30, getHeight() - 58, 110, 34);
    presets.setBounds (156, getHeight() - 58, 300, 34);

    help.setBounds (getLocalBounds());
}
