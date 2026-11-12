#include "PluginEditor.h"

using namespace juce;

namespace ui81
{
Font font (float h, bool bold)
{
    return Font (FontOptions().withHeight (h).withStyle (bold ? "Bold" : "Regular"));
}

static const Colour cText   { 0xffeef1f5 };
static const Colour cDim    { 0xffa3abb8 };
static const Colour cPanel  { 0xff1b1e23 };
static const Colour cBorder { 0xff353c48 };

//==============================================================================
LookAndFeel::LookAndFeel()
{
    setColour (ComboBox::textColourId, cText);
    setColour (ComboBox::arrowColourId, cText);
    setColour (PopupMenu::backgroundColourId, Colour (0xff1a212c));
    setColour (PopupMenu::textColourId, cText);
    setColour (PopupMenu::highlightedBackgroundColourId, Colour (0xff2f6fd1));
    setColour (PopupMenu::highlightedTextColourId, Colours::white);
    setColour (TextButton::textColourOffId, cText);
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (Label::textColourId, cText);
    setColour (BubbleComponent::backgroundColourId, Colour (0xf0141a24));
    setColour (BubbleComponent::outlineColourId, cBorder);
    setColour (TooltipWindow::textColourId, cText);
}

void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const String kind = s.getProperties()["kind"].toString();
    const float size = (float) jmin (w, h);
    const float cx = x + w * 0.5f, cy = y + h * 0.5f;
    const bool small = kind == "small";

    // Couronne de points
    const int nDots = small ? 11 : 21;
    const float dotR = size * 0.47f;
    for (int i = 0; i < nDots; ++i)
    {
        const float a = a0 + (a1 - a0) * i / (nDots - 1);
        const bool big = i == 0 || i == nDots - 1 || i == nDots / 2;
        const float r = size * (big ? 0.022f : 0.013f);
        g.setColour (Colours::white.withAlpha (big ? 0.95f : 0.75f));
        g.fillEllipse (cx + std::sin (a) * dotR - r, cy - std::cos (a) * dotR - r, r * 2, r * 2);
    }

    const float R = size * (small ? 0.36f : 0.37f);

    // Ombre
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (cx - R * 1.04f, cy - R * 0.96f, R * 2.12f, R * 2.16f);

    // Jupe (skirt)
    Colour sk1 { 0xfff2f3f5 }, sk2 { 0xff5a5e66 };
    if (kind == "red") { sk1 = Colour (0xff4a4c52); sk2 = Colour (0xff0e0f11); }
    g.setGradientFill (ColourGradient (sk1, cx - R * 0.6f, cy - R * 0.9f, sk2, cx + R * 0.5f, cy + R, false));
    g.fillEllipse (cx - R, cy - R, R * 2, R * 2);
    g.setColour (Colours::black.withAlpha (0.18f));
    for (int i = 0; i < 60; ++i)
    {
        const float a = MathConstants<float>::twoPi * i / 60.0f;
        g.drawLine (cx + std::sin (a) * R * 0.8f, cy - std::cos (a) * R * 0.8f,
                    cx + std::sin (a) * R * 0.99f, cy - std::cos (a) * R * 0.99f, 0.8f);
    }
    g.setColour (Colours::black.withAlpha (0.7f));
    g.drawEllipse (cx - R, cy - R, R * 2, R * 2, 1.0f);

    // Chapeau (cap)
    const float Rc = R * (kind == "red" || kind == "blue" ? 0.82f : 0.72f);
    Colour c1 { 0xffc9ccd1 }, c2 { 0xff6f747b };
    if (kind == "red")  { c1 = Colour (0xffef3a40); c2 = Colour (0xff7c0b10); }
    if (kind == "blue") { c1 = Colour (0xff3c8ff5); c2 = Colour (0xff0b3a8e); }
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (cx - Rc * 1.03f, cy - Rc * 0.97f, Rc * 2.08f, Rc * 2.1f);
    g.setGradientFill (ColourGradient (c1, cx - Rc * 0.5f, cy - Rc * 0.9f, c2, cx + Rc * 0.5f, cy + Rc, false));
    g.fillEllipse (cx - Rc, cy - Rc, Rc * 2, Rc * 2);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.28f), cx, cy - Rc, Colours::transparentWhite, cx, cy, false));
    g.fillEllipse (cx - Rc * 0.8f, cy - Rc * 0.95f, Rc * 1.6f, Rc * 1.1f);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawEllipse (cx - Rc, cy - Rc, Rc * 2, Rc * 2, 1.0f);

    // Index
    const float a = a0 + pos * (a1 - a0);
    const float sa = std::sin (a), ca = -std::cos (a);
    if (kind == "red" || kind == "blue")
    {
        g.setColour (Colours::white);
        g.drawLine (cx + sa * Rc * 0.3f, cy + ca * Rc * 0.3f, cx + sa * R * 0.97f, cy + ca * R * 0.97f, size * 0.04f);
    }
    else
    {
        g.setColour (Colour (0xff24262a));
        g.drawLine (cx + sa * Rc * 0.25f, cy + ca * Rc * 0.25f, cx + sa * Rc * 0.92f, cy + ca * Rc * 0.92f, size * 0.03f);
        g.setColour (Colours::white);
        g.drawLine (cx + sa * R * 0.8f, cy + ca * R * 0.8f, cx + sa * R * 0.98f, cy + ca * R * 0.98f, size * 0.03f);
    }
}

void LookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& box)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (Colour (0xff1e2633));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.hasKeyboardFocus (true) ? Colour (0xff2f6fd1) : cBorder);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    Path p;
    const float ax = w - 22.0f, ay = h * 0.5f;
    p.addTriangle (ax - 6, ay - 3, ax + 6, ay - 3, ax, ay + 4);
    g.setColour (cText);
    g.fillPath (p);
}

void LookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    Colour c = b.getToggleState() ? Colour (0xff2f6fd1) : Colour (0xff1e2633);
    if (over) c = c.brighter (0.12f);
    if (down) c = c.darker (0.2f);
    g.setColour (c);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (cBorder);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
}

//==============================================================================
Knob::Knob (const String& kind, double resetValue)
{
    setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    setRotaryParameters (MathConstants<float>::pi * 1.22f, MathConstants<float>::pi * 2.78f, true);
    setMouseDragSensitivity (260);
    setDoubleClickReturnValue (true, resetValue);
    getProperties().set ("kind", kind);
}

void RackSwitch::paintButton (Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    if (down) r.translate (0, 1);
    const bool on = getToggleState();
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (0, 2), 3.0f);

    Colour a, b;
    if (isRed) { a = on ? Colour (0xffff4a50) : Colour (0xff5c1418); b = on ? Colour (0xffb0141c) : Colour (0xff35090c); }
    else       { a = on ? Colour (0xfffbf6e8) : Colour (0xffa29c8e); b = on ? Colour (0xffdcd3bb) : Colour (0xff77726a); }
    if (on)
    {
        g.setColour ((isRed ? Colour (0xffff3030) : Colour (0xfffff2c0)).withAlpha (0.25f));
        g.fillRoundedRectangle (r.expanded (3.0f), 5.0f);
    }
    g.setGradientFill (ColourGradient (a, r.getX(), r.getY(), b, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (Colours::white.withAlpha (over ? 0.35f : 0.2f));
    g.drawLine (r.getX() + 3, r.getY() + 1.5f, r.getRight() - 3, r.getY() + 1.5f, 1.0f);
    g.setColour (Colours::black.withAlpha (0.7f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
}

//==============================================================================
static const char* bandNames[4] = { "LF", "LMF", "HMF", "HF" };
static const Colour nodeColours[4] = { Colour (0xffe0262e), Colour (0xffa9aeb6), Colour (0xffa9aeb6), Colour (0xff2f86f0) };
constexpr double fMin = 20.0, fMax = 22000.0;
constexpr float dbRange = 15.0f;

EqGraph::EqGraph (Bakr81Processor& p) : proc (p)
{
    const char* g[4] = { "lfG", "lmfG", "hmfG", "hfG" };
    const char* f[4] = { "lfF", "lmfF", "hmfF", "hfF" };
    for (int i = 0; i < 4; ++i) { gainP[i] = proc.apvts.getParameter (g[i]); freqP[i] = proc.apvts.getParameter (f[i]); }
}

Rectangle<float> EqGraph::plot() const { return getLocalBounds().toFloat().withTrimmedLeft (46).withTrimmedRight (12).withTrimmedTop (14).withTrimmedBottom (28); }
float EqGraph::xForF (double f) const { auto p = plot(); return p.getX() + p.getWidth() * (float) (std::log (f / fMin) / std::log (fMax / fMin)); }
double EqGraph::fForX (float x) const { auto p = plot(); return fMin * std::pow (fMax / fMin, (double) ((x - p.getX()) / p.getWidth())); }
float EqGraph::yForDb (float db) const { auto p = plot(); return p.getCentreY() - db / dbRange * p.getHeight() * 0.5f; }
float EqGraph::dbForY (float y) const { auto p = plot(); return (p.getCentreY() - y) / (p.getHeight() * 0.5f) * dbRange; }

Point<float> EqGraph::nodePos (int i) const
{
    const float f = freqP[i]->convertFrom0to1 (freqP[i]->getValue());
    const float g = gainP[i]->convertFrom0to1 (gainP[i]->getValue());
    return { xForF (f), yForDb (g) };
}

int EqGraph::hitNode (Point<float> p) const
{
    int best = -1; float bestD = 18.0f;
    for (int i = 0; i < 4; ++i)
    {
        const float d = nodePos (i).getDistanceFrom (p);
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

void EqGraph::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff121b28), 0, b.getY(), Colour (0xff0a1018), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (cBorder);
    g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

    const auto p = plot();
    g.setColour (Colour (0xff0c131d));
    g.fillRect (p);

    // Grille fréquences
    const double majors[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    const char* majorTxt[] = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
    for (double dec = 10; dec <= 10000; dec *= 10)
        for (int m = 1; m <= 9; ++m)
        {
            const double f = dec * m;
            if (f < fMin || f > fMax) continue;
            g.setColour (Colours::white.withAlpha (0.05f));
            g.drawVerticalLine (roundToInt (xForF (f)), p.getY(), p.getBottom());
        }
    g.setFont (font (15.0f, true));
    for (int i = 0; i < 10; ++i)
    {
        const float x = xForF (majors[i]);
        g.setColour (Colours::white.withAlpha (0.13f));
        g.drawVerticalLine (roundToInt (x), p.getY(), p.getBottom());
        g.setColour (cText);
        g.drawText (majorTxt[i], Rectangle<float> (x - 22, p.getBottom() + 5, 44, 18), Justification::centred);
    }

    // Grille dB
    for (int db : { 12, 6, 0, -6, -12 })
    {
        const float y = yForDb ((float) db);
        g.setColour (Colours::white.withAlpha (db == 0 ? 0.16f : 0.08f));
        g.drawHorizontalLine (roundToInt (y), p.getX(), p.getRight());
        g.setColour (cText);
        g.drawText ((db > 0 ? "+" : "") + String (db), Rectangle<float> (b.getX() + 2, y - 9, 38, 18), Justification::centredRight);
    }
    g.drawText ("dB", Rectangle<float> (b.getX() + 2, b.getY() + 2, 38, 16), Justification::centredRight);

    // Courbe
    Path curve, fill;
    const int steps = (int) p.getWidth() / 2;
    const float y0 = yForDb (0.0f);
    for (int i = 0; i <= steps; ++i)
    {
        const float x = p.getX() + p.getWidth() * i / steps;
        const float y = jlimit (p.getY(), p.getBottom(), yForDb (proc.getResponseDb (fForX (x))));
        if (i == 0) { curve.startNewSubPath (x, y); fill.startNewSubPath (x, p.getBottom()); }
        else curve.lineTo (x, y);
        fill.lineTo (x, y);
    }
    fill.lineTo (p.getRight(), p.getBottom());
    fill.closeSubPath();
    g.setGradientFill (ColourGradient (Colour (0xff1d63c4).withAlpha (0.55f), 0, p.getY(), Colour (0xff0d2a55).withAlpha (0.35f), 0, p.getBottom(), false));
    g.fillPath (fill);
    g.setColour (Colours::white.withAlpha (0.06f));
    g.drawHorizontalLine (roundToInt (y0), p.getX(), p.getRight());
    g.setColour (Colours::white);
    g.strokePath (curve, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));

    // Points
    for (int i = 0; i < 4; ++i)
    {
        const auto np = nodePos (i);
        const float r = (i == dragging || i == hover) ? 11.0f : 9.0f;
        if (i == dragging || i == hover)
        {
            g.setColour (nodeColours[i].withAlpha (0.3f));
            g.fillEllipse (np.x - r - 5, np.y - r - 5, (r + 5) * 2, (r + 5) * 2);
        }
        g.setColour (Colours::black.withAlpha (0.5f));
        g.fillEllipse (np.x - r, np.y - r + 2, r * 2, r * 2);
        g.setGradientFill (ColourGradient (nodeColours[i].brighter (0.5f), np.x - r * 0.4f, np.y - r * 0.6f, nodeColours[i].darker (0.3f), np.x + r * 0.4f, np.y + r, false));
        g.fillEllipse (np.x - r, np.y - r, r * 2, r * 2);
        g.setColour (Colours::black.withAlpha (0.5f));
        g.drawEllipse (np.x - r, np.y - r, r * 2, r * 2, 1.0f);
    }

    // Info du point actif
    const int info = dragging >= 0 ? dragging : hover;
    if (info >= 0)
    {
        const auto np = nodePos (info);
        const String txt = String (bandNames[info]) + "   " + freqP[info]->getCurrentValueAsText() + "   " + gainP[info]->getCurrentValueAsText();
        Rectangle<float> box (np.x - 90, np.y - 46, 180, 24);
        box = box.constrainedWithin (p.reduced (4));
        g.setColour (Colour (0xe0141a24));
        g.fillRoundedRectangle (box, 4.0f);
        g.setColour (cBorder);
        g.drawRoundedRectangle (box, 4.0f, 1.0f);
        g.setColour (cText);
        g.setFont (font (14.0f, true));
        g.drawText (txt, box, Justification::centred);
    }
}

void EqGraph::mouseMove (const MouseEvent& e)
{
    const int h = hitNode (e.position);
    if (h != hover) { hover = h; setMouseCursor (h >= 0 ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor); repaint(); }
}

void EqGraph::mouseDown (const MouseEvent& e)
{
    dragging = hitNode (e.position);
    if (dragging >= 0) { gainP[dragging]->beginChangeGesture(); freqP[dragging]->beginChangeGesture(); }
}

void EqGraph::mouseDrag (const MouseEvent& e)
{
    if (dragging < 0) return;
    const auto& fr = freqP[dragging]->getNormalisableRange();
    const float f = jlimit (fr.start, fr.end, (float) fForX (e.position.x));
    const float db = jlimit (-12.0f, 12.0f, dbForY (e.position.y));
    freqP[dragging]->setValueNotifyingHost (freqP[dragging]->convertTo0to1 (f));
    gainP[dragging]->setValueNotifyingHost (gainP[dragging]->convertTo0to1 (db));
    repaint();
}

void EqGraph::mouseUp (const MouseEvent&)
{
    if (dragging >= 0) { gainP[dragging]->endChangeGesture(); freqP[dragging]->endChangeGesture(); }
    dragging = -1;
    repaint();
}

void EqGraph::mouseDoubleClick (const MouseEvent& e)
{
    const int h = hitNode (e.position);
    if (h < 0) return;
    gainP[h]->beginChangeGesture();
    gainP[h]->setValueNotifyingHost (gainP[h]->convertTo0to1 (0.0f));
    gainP[h]->endChangeGesture();
}

void EqGraph::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    const int h = hitNode (e.position);
    if (h < 0) return;
    auto* p = freqP[h];
    const float f = p->convertFrom0to1 (p->getValue()) * std::pow (2.0f, w.deltaY * 0.6f);
    p->beginChangeGesture();
    p->setValueNotifyingHost (p->convertTo0to1 (jlimit (p->getNormalisableRange().start, p->getNormalisableRange().end, f)));
    p->endChangeGesture();
}

//==============================================================================
void Meters::tick()
{
    std::atomic<float>* src[4] = { &proc.inPeak[0], &proc.inPeak[1], &proc.outPeak[0], &proc.outPeak[1] };
    for (int i = 0; i < 4; ++i)
    {
        const float pk = src[i]->exchange (0.0f);
        const float db = pk > 1e-6f ? 20.0f * std::log10 (pk) : -100.0f;
        lvl[i] = std::max (db, lvl[i] - 1.2f);
        if (db >= hold[i]) { hold[i] = db; holdT[i] = 40; }
        else if (--holdT[i] <= 0) hold[i] = std::max (-100.0f, hold[i] - 1.0f);
    }
    repaint();
}

void Meters::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0d131c));
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (cBorder);
    g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

    g.setColour (cText);
    g.setFont (font (16.0f, true));
    g.drawText ("IN",  Rectangle<float> (4, 6, 30, 20), Justification::centred);
    g.drawText ("OUT", Rectangle<float> (36, 6, 34, 20), Justification::centred);

    const float top = 34, bottom = b.getHeight() - 12;
    constexpr int nSeg = 22;
    constexpr float lo = -30.0f, hi = 3.0f;
    auto yFor = [&] (float db) { return bottom - (db - lo) / (hi - lo) * (bottom - top); };
    const float segH = (bottom - top) / nSeg;
    const float xs[4] = { 8, 21, 40, 53 };

    for (int m = 0; m < 4; ++m)
        for (int s = 0; s < nSeg; ++s)
        {
            const float dbLo = lo + (hi - lo) * s / nSeg;
            Colour c = dbLo < -6 ? Colour (0xff35d04a) : dbLo < -1.5f ? Colour (0xffe6d33a) : dbLo < 0 ? Colour (0xfff28c28) : Colour (0xffff3b30);
            const bool lit = lvl[m] > dbLo || (hold[m] > dbLo && hold[m] < dbLo + (hi - lo) / nSeg);
            g.setColour (lit ? c : c.withAlpha (0.12f));
            g.fillRect (xs[m], bottom - (s + 1) * segH + 1.0f, 10.0f, segH - 2.0f);
        }

    g.setFont (font (14.0f, true));
    for (int db : { 0, -6, -12, -18, -24, -30 })
    {
        g.setColour (cText);
        g.drawText (String (db), Rectangle<float> (66, yFor ((float) db) - 8, b.getWidth() - 68, 16), Justification::centredLeft);
    }
}

//==============================================================================
Content::Content (Bakr81Processor& p) : proc (p), graph (p), meters (p)
{
    setLookAndFeel (&lnf);
    setSize (W, H);

    auto addKnob = [&] (Knob& k, const char* id, float cx, float cy, float size)
    {
        addAndMakeVisible (k);
        placeKnob (k, cx, cy, size);
        k.setPopupDisplayEnabled (true, true, this);
        sliderAtt.push_back (std::make_unique<SA> (proc.apvts, id, k));
    };
    auto addSwitch = [&] (RackSwitch& s, const char* id, float cx, float cy, float w = 36, float h = 28)
    {
        addAndMakeVisible (s);
        placeSwitch (s, cx, cy, w, h);
        buttonAtt.push_back (std::make_unique<BA> (proc.apvts, id, s));
    };

    // Barre du haut
    for (auto* b : { &abButton, &prevButton, &nextButton, &saveButton, &loadButton }) addAndMakeVisible (b);
    addAndMakeVisible (presetBox);
    abButton.setBounds (12, 8, 74, 30);
    presetBox.setBounds (96, 8, 300, 30);
    prevButton.setBounds (406, 8, 34, 30);
    nextButton.setBounds (444, 8, 34, 30);
    saveButton.setBounds (490, 8, 70, 30);
    loadButton.setBounds (566, 8, 70, 30);
    presetBox.addItemList (Bakr81Processor::getPresetNames(), 1);
    presetBox.setTextWhenNothingSelected ("Default");
    presetBox.onChange = [this] { const int i = presetBox.getSelectedItemIndex(); if (i >= 0 && i != proc.currentPreset) proc.applyPreset (i); };
    prevButton.onClick = [this] { const int n = presetBox.getNumItems(); proc.applyPreset ((proc.currentPreset + n - 1) % n); };
    nextButton.onClick = [this] { const int n = presetBox.getNumItems(); proc.applyPreset ((proc.currentPreset + 1) % n); };
    abButton.onClick   = [this] { proc.toggleAB(); };
    saveButton.onClick = [this] { savePreset(); };
    loadButton.onClick = [this] { loadPreset(); };
    abButton.setTooltip ("Compare deux reglages A et B");

    // Rack (centre des boutons y = 146)
    constexpr float ry = 146;
    addKnob (inputK,  "input",  192, ry, 104);
    addKnob (lfK,     "lfG",    314, ry, 96);
    addKnob (lmfK,    "lmfG",   474, ry, 96);
    addKnob (hmfK,    "hmfG",   592, ry, 96);
    addKnob (hfK,     "hfG",    743, ry, 96);
    addKnob (outputK, "output", 863, ry, 104);
    addKnob (eqTrimK, "eqTrim", 950, ry - 6, 62);
    addSwitch (lowInS,  "lowIn",  395, 124);
    addSwitch (lfBellS, "lfBell", 395, 166);
    addSwitch (highInS, "highIn", 667, 124);
    addSwitch (hfBellS, "hfBell", 667, 166);
    addSwitch (eqInS,   "eqIn",   1012, 124);
    addSwitch (phaseS,  "phase",  1012, 166);
    addSwitch (powerS,  "power",  1060, 152, 38, 30);
    lowInS.setTooltip ("Section basse (LF + LMF) active");
    lfBellS.setTooltip ("LF en cloche (sinon shelf)");
    highInS.setTooltip ("Section haute (HMF + HF) active");
    hfBellS.setTooltip ("HF en cloche (sinon shelf)");

    // Graphe + vumètres
    addAndMakeVisible (graph);
    addAndMakeVisible (meters);
    graph.setBounds (10, 244, 1058, 208);
    meters.setBounds (1074, 244, 96, 208);

    // Bas
    constexpr float by = 516;
    addKnob (lfK2,  "lfG",  82,  by, 64);  addKnob (lfF,  "lfF",  200, by, 52);
    addKnob (lmfK2, "lmfG", 336, by, 64);  addKnob (lmfF, "lmfF", 436, by, 52);
    addKnob (hmfK2, "hmfG", 556, by, 64);  addKnob (hmfF, "hmfF", 656, by, 52);
    addKnob (hfK2,  "hfG",  780, by, 64);  addKnob (hfF,  "hfF",  880, by, 52);
    addKnob (eqTrimK2, "eqTrim", 996, by, 64);
    addSwitch (phaseS2, "phase", 1070, 526, 40, 34);
    addSwitch (powerS2, "power", 1130, 526, 40, 34);

    const float vx[9] = { 82, 200, 336, 436, 556, 656, 780, 880, 996 };
    for (int i = 0; i < 9; ++i)
    {
        values[i].setJustificationType (Justification::centred);
        values[i].setFont (font (15.0f, true));
        values[i].setBounds (roundToInt (vx[i] - 42), 564, 84, 23);
        addAndMakeVisible (values[i]);
    }

    refreshPresetBox();
    timerCallback();
    startTimerHz (30);
}

Content::~Content() { setLookAndFeel (nullptr); }

void Content::placeKnob (Slider& k, float cx, float cy, float size)
{
    k.setBounds (roundToInt (cx - size / 2), roundToInt (cy - size / 2), roundToInt (size), roundToInt (size));
}

void Content::placeSwitch (Button& b, float cx, float cy, float w, float h)
{
    b.setBounds (roundToInt (cx - w / 2), roundToInt (cy - h / 2), roundToInt (w), roundToInt (h));
}

void Content::refreshPresetBox()
{
    if (proc.currentPreset != lastPreset)
    {
        presetBox.setSelectedItemIndex (proc.currentPreset, dontSendNotification);
        lastPreset = proc.currentPreset;
    }
}

void Content::timerCallback()
{
    meters.tick();
    graph.repaint();
    refreshPresetBox();
    abButton.setButtonText (proc.isB() ? "B" : "A");
    repaint (1050, 118, 20, 20);   // LED POWER
    for (int i = 0; i < 9; ++i)
        if (auto* prm = proc.apvts.getParameter (valueIds[i]))
        {
            const String t = prm->getCurrentValueAsText();
            if (values[i].getText() != t) values[i].setText (t, dontSendNotification);
        }
}

static File presetDir()
{
    auto d = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("BAKR").getChildFile ("BAKR 81 Presets");
    d.createDirectory();
    return d;
}

void Content::savePreset()
{
    chooser = std::make_unique<FileChooser> ("Enregistrer un preset BAKR 81", presetDir().getChildFile ("Mon preset.b81preset"), "*.b81preset");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
        [this] (const FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == File()) return;
            if (auto xml = proc.apvts.copyState().createXml())
                xml->writeTo (f.withFileExtension ("b81preset"));
        });
}

void Content::loadPreset()
{
    chooser = std::make_unique<FileChooser> ("Charger un preset BAKR 81", presetDir(), "*.b81preset");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
        [this] (const FileChooser& fc)
        {
            auto f = fc.getResult();
            if (! f.existsAsFile()) return;
            if (auto xml = XmlDocument::parse (f))
                if (xml->hasTagName (proc.apvts.state.getType()))
                {
                    proc.apvts.replaceState (ValueTree::fromXml (*xml));
                    presetBox.setText (f.getFileNameWithoutExtension(), dontSendNotification);
                }
        });
}

//==============================================================================
static void screw (Graphics& g, float cx, float cy, float r, bool phillips)
{
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillEllipse (cx - r - 1, cy - r + 1, (r + 1) * 2, (r + 1) * 2);
    g.setGradientFill (ColourGradient (Colour (0xff9a9fa6), cx - r, cy - r, Colour (0xff2c2f34), cx + r, cy + r, false));
    g.fillEllipse (cx - r, cy - r, r * 2, r * 2);
    g.setColour (Colour (0xff141518));
    if (phillips)
    {
        g.drawLine (cx - r * 0.55f, cy, cx + r * 0.55f, cy, 1.6f);
        g.drawLine (cx, cy - r * 0.55f, cx, cy + r * 0.55f, 1.6f);
    }
    else g.drawLine (cx - r * 0.7f, cy + r * 0.2f, cx + r * 0.7f, cy - r * 0.2f, 2.0f);
}

static void knobLabels (Graphics& g, float cx, float cy, float size, const String& name, const String& lo, const String& hi, const String& unit, float nameSize = 16.0f)
{
    g.setColour (cText);
    g.setFont (font (nameSize, true));
    g.drawText (name, Rectangle<float> (cx - 60, cy - size * 0.5f - 20, 120, 18), Justification::centred);
    g.setFont (font (14.0f, true));
    g.drawText (lo, Rectangle<float> (cx - size * 0.36f - 40, cy + size * 0.33f, 40, 16), Justification::centredRight);
    g.drawText (hi, Rectangle<float> (cx + size * 0.36f, cy + size * 0.33f, 40, 16), Justification::centredLeft);
    g.drawText (unit, Rectangle<float> (cx - 40, cy + size * 0.5f + 2, 80, 16), Justification::centred);
}

void Content::paint (Graphics& g)
{
    // Fond
    g.setGradientFill (ColourGradient (Colour (0xff161b23), 0, 0, Colour (0xff0b0e13), 0, (float) H, false));
    g.fillAll();

    // Barre du haut
    g.setColour (Colour (0xff10141a));
    g.fillRect (0, 0, W, 46);
    g.setColour (cBorder);
    g.drawHorizontalLine (46, 0, (float) W);
    g.setColour (cText);
    g.setFont (font (19.0f, true));
    g.drawText ("BAKR 81", Rectangle<float> (W - 230.0f, 3, 216, 22), Justification::centredRight);
    g.setColour (cDim);
    g.setFont (font (11.0f, true));
    g.drawText ("CLASS A PREAMP EQ", Rectangle<float> (W - 230.0f, 24, 216, 16), Justification::centredRight);

    // Rack
    auto rack = Rectangle<float> (6, 54, W - 12.0f, 182);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (rack.translated (0, 3), 10.0f);
    g.setGradientFill (ColourGradient (Colour (0xff2a2e35), 0, rack.getY(), Colour (0xff15171b), 0, rack.getBottom(), false));
    g.fillRoundedRectangle (rack, 10.0f);
    g.setColour (Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (roundToInt (rack.getY() + 1), rack.getX() + 10, rack.getRight() - 10);
    g.setColour (Colours::black);
    g.drawRoundedRectangle (rack, 10.0f, 1.5f);
    for (float sx : { 34.0f, W - 34.0f })
        for (float sy : { 74.0f, 216.0f }) screw (g, sx, sy, 11, false);
    for (float sx : { 132.0f })
        for (float sy : { 82.0f, 208.0f }) screw (g, sx, sy, 8, true);

    // Logo BAKR
    {
        const float lx = 66, ly = 146;
        g.setColour (cText);
        g.drawEllipse (lx - 46, ly - 26, 52, 52, 2.5f);
        g.setFont (font (34.0f, true));
        g.drawText ("B", Rectangle<float> (lx - 46, ly - 26, 52, 52), Justification::centred);
        g.setFont (font (22.0f, true));
        g.drawText ("AKR", Rectangle<float> (lx + 6, ly - 28, 70, 26), Justification::centredLeft);
        g.setFont (font (17.0f));
        g.drawText ("Audio", Rectangle<float> (lx + 6, ly - 2, 70, 24), Justification::centredLeft);
    }

    // Étiquettes du rack
    constexpr float ry = 146;
    knobLabels (g, 192, ry, 104, "0", "-20", "+20", "INPUT  dB");
    knobLabels (g, 314, ry, 96, "LF", "-12", "+12", "dB");
    knobLabels (g, 474, ry, 96, "LMF", "-12", "+12", "dB");
    knobLabels (g, 592, ry, 96, "HMF", "-12", "+12", "dB");
    knobLabels (g, 743, ry, 96, "HF", "-12", "+12", "dB");
    knobLabels (g, 863, ry, 104, "", "-20", "+20", "OUTPUT  dB");
    knobLabels (g, 950, ry - 6, 62, "EQ LEVEL", "-12", "+12", "dB", 13.0f);
    g.setFont (font (14.0f, true));
    g.setColour (cText);
    for (float sx : { 395.0f, 667.0f })
    {
        g.drawText ("IN", Rectangle<float> (sx - 30, 92, 60, 16), Justification::centred);
        g.drawText ("BELL", Rectangle<float> (sx - 30, 184, 60, 16), Justification::centred);
    }
    g.drawText ("IN", Rectangle<float> (982, 92, 60, 16), Justification::centred);
    g.drawText ("PHASE", Rectangle<float> (977, 184, 70, 16), Justification::centred);
    g.drawText ("POWER", Rectangle<float> (1025, 172, 70, 16), Justification::centred);
    // LED d'alimentation
    {
        const bool on = proc.apvts.getRawParameterValue ("power")->load() > 0.5f;
        g.setColour (on ? Colour (0xffff4040) : Colour (0xff3a1012));
        g.fillEllipse (1056, 124, 8, 8);
    }
    g.setColour (cText);
    g.setFont (font (14.0f, true));
    g.drawText ("BAKR 81 EQ", Rectangle<float> (1084, 108, 84, 20), Justification::centred);
    g.setFont (font (11.0f, true));
    g.setColour (cDim);
    g.drawText ("ALL DISCRETE", Rectangle<float> (1084, 130, 84, 14), Justification::centred);
    g.drawText ("CLASS A", Rectangle<float> (1084, 144, 84, 14), Justification::centred);

    // Panneaux du bas
    const Rectangle<float> boxes[5] = { { 14, 458, 252, 134 }, { 274, 458, 222, 134 }, { 504, 458, 210, 134 },
                                        { 722, 458, 218, 134 }, { 948, 458, 222, 134 } };
    for (auto& r : boxes)
    {
        g.setColour (Colour (0xff121821));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (cBorder);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }
    constexpr float by = 516;
    knobLabels (g, 82,  by, 64, "LF",  "-12", "+12", "dB");
    knobLabels (g, 200, by, 52, "FREQ", "20", "200", "Hz");
    knobLabels (g, 336, by, 64, "LMF", "-12", "+12", "dB");
    knobLabels (g, 436, by, 52, "FREQ", "200", "2k", "Hz");
    knobLabels (g, 556, by, 64, "HMF", "-12", "+12", "dB");
    knobLabels (g, 656, by, 52, "FREQ", "1k", "10k", "Hz");
    knobLabels (g, 780, by, 64, "HF",  "-12", "+12", "dB");
    knobLabels (g, 880, by, 52, "FREQ", "5k", "20k", "Hz");
    knobLabels (g, 996, by, 64, "EQ",  "-12", "+12", "dB");
    g.setColour (cText);
    g.setFont (font (16.0f, true));
    g.drawText ("PHASE", Rectangle<float> (1035, 472, 70, 20), Justification::centred);
    g.drawText ("POWER", Rectangle<float> (1095, 472, 70, 20), Justification::centred);

    // Cadres des valeurs
    for (auto& l : values)
    {
        auto r = l.getBounds().toFloat();
        g.setColour (Colour (0xff080b10));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (Colour (0xff3d4757));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);
    }
}
} // namespace ui81

//==============================================================================
Bakr81Editor::Bakr81Editor (Bakr81Processor& p) : AudioProcessorEditor (p), content (p)
{
    addAndMakeVisible (content);
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) ui81::W / ui81::H);
    setResizeLimits (roundToInt (ui81::W * 0.55), roundToInt (ui81::H * 0.55), roundToInt (ui81::W * 1.6), roundToInt (ui81::H * 1.6));
    setSize (ui81::W, ui81::H);
}

void Bakr81Editor::resized()
{
    const float s = getWidth() / (float) ui81::W;
    content.setTransform (AffineTransform::scale (s));
}
