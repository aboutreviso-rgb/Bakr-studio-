#include "PluginEditor.h"

using namespace binf;
using juce::Colour; using juce::Graphics; using juce::Rectangle; using juce::Path;

namespace
{
constexpr int kW = 780, kH = 500;
const Colour cBg     { 0xff141414 };
const Colour cPanel  { 0xff1d1c1a };
const Colour cEdge   { 0xff3b352c };
const Colour cGold   { 0xffd4aa63 };
const Colour cText   { 0xffece5d8 };
const Colour cDim    { 0xff968e82 };

float toDb (float g) { return juce::Decibels::gainToDecibels (g, -60.0f); }

// position verticale (0 en haut) d'une valeur en dB sur l'échelle des vumètres (+6 .. -42)
float meterPos (float db)
{
    db = juce::jlimit (-42.0f, 6.0f, db);
    // échelle un peu dilatée près de 0 dB
    const float t = (6.0f - db) / 48.0f;
    return std::pow (t, 0.8f);
}
}

//==============================================================================
LnF::LnF()
{
    setColour (juce::ComboBox::textColourId, cText);
    setColour (juce::ComboBox::arrowColourId, cGold);
    setColour (juce::PopupMenu::backgroundColourId, Colour (0xff1e1c1a));
    setColour (juce::PopupMenu::textColourId, cText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Colour (0xff8a7048));
    setColour (juce::Label::textColourId, cText);
}

juce::Label* LnF::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::FontOptions (13.0f, juce::Font::bold));
    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::textColourId, cGold);
    l->setColour (juce::Label::backgroundColourId, Colour (0xff0b0b0a));
    l->setColour (juce::Label::outlineColourId, cEdge);
    l->setColour (juce::TextEditor::textColourId, cGold);
    l->setColour (juce::TextEditor::backgroundColourId, Colour (0xff0b0b0a));
    return l;
}

void LnF::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float,
                            juce::Slider::SliderStyle, juce::Slider&)
{
    const float cx = (float) x + (float) w * 0.5f;
    // rail
    auto rail = Rectangle<float> (cx - 3.0f, (float) y, 6.0f, (float) h);
    g.setColour (Colour (0xff070707)); g.fillRoundedRectangle (rail, 3);
    g.setColour (cEdge); g.drawRoundedRectangle (rail, 3, 1);
    // graduations
    g.setColour (cDim.withAlpha (0.4f));
    for (int i = 0; i <= 10; ++i)
    {
        float yy = (float) y + (float) h * (float) i / 10.0f;
        g.drawHorizontalLine ((int) yy, cx - 16, cx - 8);
        g.drawHorizontalLine ((int) yy, cx + 8, cx + 16);
    }
    // capuchon
    auto cap = Rectangle<float> (cx - 17, pos - 22, 34, 44);
    g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (cap.translated (0, 4), 4);
    juce::ColourGradient gr (Colour (0xff5a5650), cap.getX(), cap.getY(), Colour (0xff1c1b19), cap.getX(), cap.getBottom(), false);
    gr.addColour (0.48, Colour (0xff3a3733)); gr.addColour (0.52, Colour (0xff24221f));
    g.setGradientFill (gr); g.fillRoundedRectangle (cap, 4);
    g.setColour (Colour (0xff6c665e)); g.drawRoundedRectangle (cap, 4, 1);
    for (int i = -2; i <= 2; ++i)
    {
        g.setColour (Colour (0xff121110)); g.drawHorizontalLine ((int) (pos + (float) i * 6), cap.getX() + 6, cap.getRight() - 6);
    }
    g.setColour (cGold); g.fillRect (cap.getX() + 4, pos - 1, cap.getWidth() - 8, 2.0f);
}

void LnF::drawButtonBackground (Graphics& g, juce::Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    Colour base = Colour (0xff22201e);
    Colour onCol = b.getProperties().contains ("onColour")
                     ? Colour ((juce::uint32) (int) b.getProperties()["onColour"]) : cGold;
    if (on)
    {
        juce::ColourGradient gr (onCol.brighter (0.15f), r.getX(), r.getY(), onCol.darker (0.35f), r.getX(), r.getBottom(), false);
        g.setGradientFill (gr);
    }
    else g.setColour (down ? base.brighter (0.15f) : over ? base.brighter (0.08f) : base);
    g.fillRoundedRectangle (r, 5);
    g.setColour (on ? onCol.brighter (0.4f) : cEdge); g.drawRoundedRectangle (r, 5, 1.0f);
}

void LnF::drawButtonText (Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.setColour (b.getToggleState() ? Colour (0xff120e08) : cText.withAlpha (0.85f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (3, 0), juce::Justification::centred, 2);
}

void LnF::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (Colour (0xff0e0e0d)); g.fillRoundedRectangle (r, 4);
    g.setColour (cEdge); g.drawRoundedRectangle (r, 4, 1);
    Path a; float ax = (float) w - 14, ay = (float) h * 0.5f;
    a.startNewSubPath (ax - 4, ay - 2); a.lineTo (ax, ay + 2); a.lineTo (ax + 4, ay - 2);
    g.setColour (cGold); g.strokePath (a, juce::PathStrokeType (1.5f));
}

//==============================================================================
void Meter::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto scaleArea = right ? r.removeFromRight (26) : r.removeFromLeft (26);
    auto bars = r.reduced (2, 0);
    const float top = bars.getY() + 4, H = bars.getHeight() - 8;
    g.setColour (cDim); g.setFont (juce::FontOptions (10.0f));
    for (int db : { 6, 3, 0, -3, -6, -10, -14, -18, -24, -30, -42 })
    {
        float y = top + meterPos ((float) db) * H;
        g.drawText (juce::String (db > 0 ? "+" : "") + juce::String (db), scaleArea.withY (y - 6).withHeight (12), juce::Justification::centred);
    }
    float bw = (bars.getWidth() - 4) * 0.5f;
    for (int c = 0; c < 2; ++c)
    {
        auto b = Rectangle<float> (bars.getX() + (float) c * (bw + 4), top, bw, H);
        g.setColour (Colour (0xff070707)); g.fillRect (b);
        float db = toDb (c == 0 ? L.load() : R.load());
        float yv = top + meterPos (db) * H;
        // segments
        for (float y = top + H - 3; y > yv; y -= 4)
        {
            float segDb = 6.0f - 48.0f * std::pow ((y - top) / H, 1.0f / 0.8f);
            Colour col = segDb > 0 ? Colour (0xffe0533f) : segDb > -6 ? Colour (0xffe8c25a) : Colour (0xff6fc06a);
            g.setColour (col); g.fillRect (b.getX() + 1, y, b.getWidth() - 2, 2.5f);
        }
        g.setColour (cEdge); g.drawRect (b, 1.0f);
    }
}

void EffectMeter::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (cDim); g.setFont (juce::FontOptions (10.0f));
    g.drawText ("dB", r.removeFromTop (14), juce::Justification::centred);
    auto b = r.reduced (6, 2);
    g.setColour (Colour (0xff070707)); g.fillRect (b);
    float v = juce::jlimit (0.0f, 6.0f, val.load());
    for (float y = b.getBottom() - 3; y > b.getBottom() - (v / 6.0f) * b.getHeight(); y -= 4)
    {
        g.setColour (cGold.interpolatedWith (Colour (0xffffe9b0), 1.0f - (y - b.getY()) / b.getHeight()));
        g.fillRect (b.getX() + 1, y, b.getWidth() - 2, 2.5f);
    }
    g.setColour (cEdge); g.drawRect (b, 1.0f);
}

void CurveView::tick()
{
    auto& ap = proc.apvts;
    float c = ap.getRawParameterValue ("curve")->load(), e = ap.getRawParameterValue ("effect")->load();
    bool cl = ap.getRawParameterValue ("clip")->load() > 0.5f, on = ap.getRawParameterValue ("effectIn")->load() > 0.5f;
    if (std::abs (c - lastC) > 1.0e-4f || std::abs (e - lastE) > 1.0e-4f || cl != lastClip || on != lastOn) { lastC = c; lastE = e; lastClip = cl; lastOn = on; repaint(); }
}

void CurveView::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0a0a09)); g.fillRoundedRectangle (r, 5);
    g.setColour (cEdge); g.drawRoundedRectangle (r.reduced (0.5f), 5, 1);
    auto p = r.reduced (10);
    g.setColour (cDim.withAlpha (0.25f));
    for (int i = 1; i < 4; ++i)
    {
        g.drawHorizontalLine ((int) (p.getY() + p.getHeight() * (float) i / 4.0f), p.getX(), p.getRight());
        g.drawVerticalLine ((int) (p.getX() + p.getWidth() * (float) i / 4.0f), p.getY(), p.getBottom());
    }
    // diagonale (son d'origine)
    g.setColour (cDim.withAlpha (0.6f));
    g.drawLine (p.getX(), p.getBottom(), p.getRight(), p.getY(), 1.0f);
    // courbe d'inflation (entrée 0..1 -> sortie 0..1)
    Path path;
    const float wet = lastOn ? lastE * 0.01f : 0.0f;
    for (int i = 0; i <= 100; ++i)
    {
        float x = (float) i / 100.0f;
        float y = x * (1.0f - wet) + BakrInflateProcessor::shape (x, lastC, true) * wet;
        float px = p.getX() + x * p.getWidth(), py = p.getBottom() - y * p.getHeight();
        if (i == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
    }
    g.setColour (cGold); g.strokePath (path, juce::PathStrokeType (2.0f));
    g.setColour (cDim); g.setFont (juce::FontOptions (9.5f));
    g.drawText ("entree", p.withTrimmedTop (p.getHeight() - 12), juce::Justification::bottomRight);
    g.drawText ("sortie", p.withHeight (12), juce::Justification::topLeft);
}

//==============================================================================
void BakrInflateEditor::Root::paint (Graphics& g)
{
    g.fillAll (cBg);
    // en-tête
    juce::ColourGradient hg (Colour (0xff201e1b), 0, 0, Colour (0xff141312), 0, 56, false);
    g.setGradientFill (hg); g.fillRect (0, 0, kW, 56);
    g.setColour (cEdge); g.drawHorizontalLine (55, 0, (float) kW);
    g.setColour (cGold); g.drawRoundedRectangle (16, 12, 32, 32, 6, 1.5f);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold)); g.drawText ("B", 16, 12, 32, 32, juce::Justification::centred);
    g.setColour (cText); g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawText ("BAKR", 58, 8, 70, 24, juce::Justification::centredLeft);
    g.setColour (cGold); g.setFont (juce::FontOptions (20.0f));
    g.drawText ("INFLATE", 116, 8, 110, 24, juce::Justification::centredLeft);
    g.setColour (cDim); g.setFont (juce::FontOptions (9.5f));
    g.drawText ("VOLUME ET DENSITE SANS COMPRESSION", 58, 32, 260, 14, juce::Justification::centredLeft);

    auto panel = [&] (Rectangle<int> rr, const juce::String& title)
    {
        auto rf = rr.toFloat();
        juce::ColourGradient pg (Colour (0xff22201d), rf.getX(), rf.getY(), Colour (0xff181715), rf.getX(), rf.getBottom(), false);
        g.setGradientFill (pg); g.fillRoundedRectangle (rf, 8);
        g.setColour (cEdge); g.drawRoundedRectangle (rf.reduced (0.5f), 8, 1);
        g.setColour (cText); g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (title, rr.withHeight (28).translated (0, 4), juce::Justification::centred);
        g.setColour (cGold.withAlpha (0.6f));
        g.fillRect ((float) rr.getCentreX() - 14, (float) rr.getY() + 30, 28.0f, 1.5f);
    };
    panel (inRect, juce::String::fromUTF8 ("ENTR\xc3\x89" "E"));
    panel (fxRect, "EFFET  /  COURBE");
    panel (outRect, "SORTIE");
}

BakrInflateEditor::BakrInflateEditor (BakrInflateProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      inMeter (p.inL, p.inR, true), outMeter (p.outL, p.outR, true),
      fxMeter (p.effectMeter), curve (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (root);

    auto setupFader = [this] (juce::Slider& s, std::unique_ptr<SA>& att, const char* id)
    {
        s.setSliderStyle (juce::Slider::LinearVertical);
        root.addAndMakeVisible (s);
        s.setTextBoxStyle (juce::Slider::TextBoxAbove, false, 84, 24);
        att = std::make_unique<SA> (proc.apvts, id, s);
    };
    setupFader (inFader, inAtt, "input");
    setupFader (effectFader, effectAtt, "effect");
    setupFader (curveFader, curveAtt, "curve");
    setupFader (outFader, outAtt, "output");
    inFader.setDoubleClickReturnValue (true, 0.0);
    outFader.setDoubleClickReturnValue (true, 0.0);
    curveFader.setDoubleClickReturnValue (true, 0.0);
    effectFader.setDoubleClickReturnValue (true, 100.0);

    for (auto* b : { &clipBtn, &bandBtn, &effectBtn }) { b->setClickingTogglesState (true); root.addAndMakeVisible (b); }
    clipBtn.getProperties().set ("onColour", (int) 0xffd8503c);
    bandBtn.getProperties().set ("onColour", (int) 0xffd4aa63);
    effectBtn.getProperties().set ("onColour", (int) 0xff62b85a);
    clipBtn.setTooltip ("Limite le signal a 0 dBFS avant le traitement");
    bandBtn.setTooltip ("Traite separement graves (<240 Hz), mediums et aigus (>2,4 kHz)");
    clipAtt = std::make_unique<BA> (proc.apvts, "clip", clipBtn);
    bandAtt = std::make_unique<BA> (proc.apvts, "bandSplit", bandBtn);
    effectInAtt = std::make_unique<BA> (proc.apvts, "effectIn", effectBtn);

    root.addAndMakeVisible (inMeter); root.addAndMakeVisible (outMeter);
    root.addAndMakeVisible (fxMeter); root.addAndMakeVisible (curve);

    clipLed.setText ("OVER", juce::dontSendNotification);
    clipLed.setJustificationType (juce::Justification::centred);
    clipLed.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    clipLed.setColour (juce::Label::textColourId, cDim.withAlpha (0.35f));
    root.addAndMakeVisible (clipLed);

    presetBox.addItemList (BakrInflateProcessor::getPresetNames(), 1);
    presetBox.setSelectedItemIndex (proc.currentPreset, juce::dontSendNotification);
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.onChange = [this] { int i = presetBox.getSelectedItemIndex(); if (i >= 0) proc.applyPreset (i); };
    root.addAndMakeVisible (presetBox);
    prevBtn.onClick = [this] { int n = proc.getNumPrograms(); presetBox.setSelectedItemIndex ((proc.currentPreset + n - 1) % n); };
    nextBtn.onClick = [this] { int n = proc.getNumPrograms(); presetBox.setSelectedItemIndex ((proc.currentPreset + 1) % n); };
    root.addAndMakeVisible (prevBtn); root.addAndMakeVisible (nextBtn);

    aBtn.setRadioGroupId (77); bBtn.setRadioGroupId (77);
    aBtn.setClickingTogglesState (true); bBtn.setClickingTogglesState (true);
    aBtn.setToggleState (! proc.slotBActive, juce::dontSendNotification);
    bBtn.setToggleState (proc.slotBActive, juce::dontSendNotification);
    aBtn.onClick = [this] { if (aBtn.getToggleState()) proc.switchAB (false); };
    bBtn.onClick = [this] { if (bBtn.getToggleState()) proc.switchAB (true); };
    aBtn.setTooltip ("Comparaison A/B : deux reglages a comparer");
    root.addAndMakeVisible (aBtn); root.addAndMakeVisible (bBtn);

    sendLookAndFeelChange();
    setResizable (true, true);
    setResizeLimits (kW * 6 / 10, kH * 6 / 10, kW * 2, kH * 2);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize (kW, kH);
    startTimerHz (30);
}

BakrInflateEditor::~BakrInflateEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void BakrInflateEditor::paint (Graphics& g) { g.fillAll (cBg); }

void BakrInflateEditor::resized()
{
    root.setBounds (0, 0, kW, kH);
    root.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) kW));

    prevBtn.setBounds (330, 14, 28, 28);
    presetBox.setBounds (362, 14, 230, 28);
    nextBtn.setBounds (596, 14, 28, 28);
    aBtn.setBounds (680, 14, 36, 28);
    bBtn.setBounds (722, 14, 36, 28);

    root.inRect = { 16, 70, 190, 414 };
    root.fxRect = { 216, 70, 348, 414 };
    root.outRect = { 574, 70, 190, 414 };

    auto fader = [] (juce::Slider& s, int x, int y) { s.setBounds (x, y, 84, 360); };
    fader (inFader, root.inRect.getX() + 14, root.inRect.getY() + 42);
    inMeter.setBounds (root.inRect.getX() + 104, root.inRect.getY() + 66, 72, 334);

    fader (effectFader, root.fxRect.getX() + 8, root.fxRect.getY() + 42);
    fxMeter.setBounds (root.fxRect.getX() + 92, root.fxRect.getY() + 52, 34, 348);
    fader (curveFader, root.fxRect.getX() + 124, root.fxRect.getY() + 42);
    curve.setBounds (root.fxRect.getX() + 214, root.fxRect.getY() + 66, 122, 122);
    clipBtn.setBounds (root.fxRect.getX() + 222, root.fxRect.getY() + 214, 106, 40);
    clipLed.setBounds (root.fxRect.getX() + 222, root.fxRect.getY() + 258, 106, 18);
    bandBtn.setBounds (root.fxRect.getX() + 222, root.fxRect.getY() + 290, 106, 40);
    effectBtn.setBounds (root.fxRect.getX() + 222, root.fxRect.getY() + 350, 106, 40);

    fader (outFader, root.outRect.getX() + 14, root.outRect.getY() + 42);
    outMeter.setBounds (root.outRect.getX() + 104, root.outRect.getY() + 66, 72, 334);
}

void BakrInflateEditor::timerCallback()
{
    inMeter.tick(); outMeter.tick(); fxMeter.tick(); curve.tick();
    if (proc.clipped.exchange (false)) clipHold = 30;
    if (clipHold > 0) --clipHold;
    clipLed.setColour (juce::Label::textColourId, clipHold > 0 ? Colour (0xffff5a45) : cDim.withAlpha (0.35f));
    if (presetBox.getSelectedItemIndex() != proc.currentPreset)
        presetBox.setSelectedItemIndex (proc.currentPreset, juce::dontSendNotification);
}
