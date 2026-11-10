#include "PluginEditor.h"

using namespace juce;

namespace vui
{
Font font (float h, bool bold) { return Font (FontOptions().withHeight (h).withStyle (bold ? "Bold" : "Regular")); }

static String noteName (int m)
{
    static const char* n[] = { "Do", "Do#", "Re", "Re#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" };
    String s = String::fromUTF8 (n[((m % 12) + 12) % 12]).replace ("Re", String::fromUTF8 ("Ré"));
    return s + String (m / 12 - 1);
}

LookAndFeel::LookAndFeel()
{
    setColour (PopupMenu::backgroundColourId, col::panel);
    setColour (PopupMenu::textColourId, col::text);
    setColour (PopupMenu::highlightedBackgroundColourId, col::accLo);
    setColour (ComboBox::textColourId, col::text);
    setColour (ComboBox::arrowColourId, col::dim);
    setColour (TextButton::textColourOffId, col::text);
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (BubbleComponent::backgroundColourId, col::panel);
    setColour (BubbleComponent::outlineColourId, col::accent);
    setColour (Label::textColourId, col::text);
}

void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const float r = std::min (w, h) * 0.5f - 4.0f, cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float ringW = std::max (3.0f, r * 0.12f), ar = r - ringW * 0.5f;
    const bool bip = (bool) s.getProperties()["bipolar"];
    const float ang = a0 + pos * (a1 - a0), from = bip ? (a0 + a1) * 0.5f : a0;
    Path t; t.addCentredArc (cx, cy, ar, ar, 0, a0, a1, true);
    g.setColour (col::track); g.strokePath (t, PathStrokeType (ringW, PathStrokeType::curved, PathStrokeType::rounded));
    if (std::abs (ang - from) > 0.01f)
    {
        Path v; v.addCentredArc (cx, cy, ar, ar, 0, std::min (from, ang), std::max (from, ang), true);
        g.setColour (col::accent.withAlpha (0.25f)); g.strokePath (v, PathStrokeType (ringW * 2.4f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (col::accent); g.strokePath (v, PathStrokeType (ringW, PathStrokeType::curved, PathStrokeType::rounded));
    }
    const float kr = r - ringW * 2.0f;
    g.setGradientFill (ColourGradient (Colour (0xff2b3345), cx, cy - kr, Colour (0xff0b0f17), cx, cy + kr, false));
    g.fillEllipse (cx - kr, cy - kr, kr * 2, kr * 2);
    g.setColour (Colours::white.withAlpha (0.08f)); g.drawEllipse (cx - kr, cy - kr, kr * 2, kr * 2, 1.0f);
    g.setColour (Colours::white);
    g.drawLine (cx + (kr * 0.35f) * std::sin (ang), cy - (kr * 0.35f) * std::cos (ang), cx + (kr - 3) * std::sin (ang), cy - (kr - 3) * std::cos (ang), 2.2f);
}

void LookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& b)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (col::module); g.fillRoundedRectangle (r, 8);
    g.setColour (b.hasKeyboardFocus (false) ? col::accent : col::border); g.drawRoundedRectangle (r, 8, 1.2f);
    Path a; a.addTriangle (w - 23.0f, h * 0.5f - 2.5f, w - 13.0f, h * 0.5f - 2.5f, w - 18.0f, h * 0.5f + 3.5f);
    g.setColour (col::dim); g.fillPath (a);
}

void LookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1);
    const bool on = b.getToggleState();
    const Colour base = b.getProperties()["rec"] ? col::rec : col::accent;
    g.setColour (on ? base.withAlpha (0.85f) : col::module.brighter (over ? 0.15f : 0.0f).darker (down ? 0.2f : 0.0f));
    g.fillRoundedRectangle (r, 8);
    g.setColour (on ? base : col::border); g.drawRoundedRectangle (r, 8, 1.2f);
}

void PillSwitch::paintButton (Graphics& g, bool over, bool)
{
    const bool on = getToggleState();
    auto r = getLocalBounds().toFloat();
    auto pill = Rectangle<float> (0, (r.getHeight() - 20) * 0.5f, 40, 20);
    g.setColour (on ? col::accent.withAlpha (0.25f) : Colour (0xff2b3243)); g.fillRoundedRectangle (pill, 10);
    g.setColour (on ? col::accent : col::border); g.drawRoundedRectangle (pill, 10, 1.2f);
    const float kx = on ? pill.getRight() - 18 : pill.getX() + 2;
    g.setColour (on ? col::accent : Colour (0xffc9cfdb)); g.fillEllipse (kx, pill.getY() + 2, 16, 16);
    g.setColour (over ? Colours::white : col::text); g.setFont (font (14.0f, true));
    g.drawText (getButtonText(), r.withTrimmedLeft (48), Justification::centredLeft);
}

//==============================================================================
void NoteDisplay::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float m = proc.dispMidi.load();
    const int note = proc.dispNote.load();
    const float rms = proc.dispRms.load();

    g.setColour (col::dim); g.setFont (font (13.0f));
    g.drawText ("NOTE", r.removeFromTop (18), Justification::centredLeft);
    const String txt = note >= 0 ? noteName (note) : (m > 0 ? noteName ((int) std::round (m)) : "--");
    g.setColour (note >= 0 ? Colours::white : col::dim); g.setFont (font (52.0f, true));
    g.drawText (txt, r.removeFromTop (60), Justification::centred);

    // cents
    const float cents = m > 0 ? (m - (note >= 0 ? (float) note : std::round (m))) * 100.0f : 0.0f;
    shownCents += (cents - shownCents) * 0.4f;
    auto cr = r.removeFromTop (34).reduced (8, 6);
    g.setColour (col::track); g.fillRoundedRectangle (cr, 4);
    g.setColour (col::dim.withAlpha (0.6f)); g.drawVerticalLine ((int) cr.getCentreX(), cr.getY(), cr.getBottom());
    if (m > 0)
    {
        const float x = cr.getCentreX() + jlimit (-1.0f, 1.0f, shownCents / 50.0f) * cr.getWidth() * 0.5f;
        g.setColour (std::abs (shownCents) < 12 ? col::green : col::amber);
        g.fillRoundedRectangle (x - 3, cr.getY() - 3, 6, cr.getHeight() + 6, 3);
    }
    g.setColour (col::dim); g.setFont (font (11.0f));
    g.drawText ((m > 0 ? (shownCents >= 0 ? "+" : "") + String ((int) std::round (shownCents)) : String ("0")) + " cents", r.removeFromTop (16), Justification::centred);

    // niveau d'entrée
    level = std::max (rms, level - 1.5f);
    auto lr = r.removeFromTop (22).reduced (8, 6);
    g.setColour (col::track); g.fillRoundedRectangle (lr, 3);
    const float p = jlimit (0.0f, 1.0f, (level + 70.0f) / 70.0f);
    g.setGradientFill (ColourGradient (col::green, lr.getX(), 0, col::rec, lr.getRight(), 0, false));
    g.fillRoundedRectangle (lr.withWidth (lr.getWidth() * p), 3);
    const float gate = proc.apvts.getRawParameterValue ("gate")->load();
    g.setColour (Colours::white); g.drawVerticalLine ((int) (lr.getX() + lr.getWidth() * (gate + 70.0f) / 70.0f), lr.getY() - 3, lr.getBottom() + 3);
}

void PitchRoll::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colour (0xff0b1322)); g.fillRoundedRectangle (r, 8);
    const int key = (int) proc.apvts.getRawParameterValue ("key")->load();
    const int sc = (int) proc.apvts.getRawParameterValue ("scale")->load();
    static const int S[][12] = { {1,1,1,1,1,1,1,1,1,1,1,1},{1,0,1,0,1,1,0,1,0,1,0,1},{1,0,1,1,0,1,0,1,1,0,1,0},{1,0,1,0,1,0,0,1,0,1,0,0},
                                 {1,0,0,1,0,1,0,1,0,0,1,0},{1,0,0,1,0,1,1,1,0,0,1,0},{1,0,1,0,1,1,0,1,0,1,1,0},{1,0,1,1,0,1,0,1,0,1,1,0} };
    const int hp = proc.histPos.load();
    const int show = 600;
    // centre suit la voix
    float sum = 0; int cnt = 0;
    for (int i = 0; i < 200; ++i) { const float v = proc.histPitch[(size_t) ((hp - 1 - i + VoxMidiProcessor::histSize) % VoxMidiProcessor::histSize)]; if (v > 0) { sum += v; ++cnt; } }
    if (cnt > 10) centre += (sum / cnt - centre) * 0.08f;
    const float span = 24.0f, lo = centre - span / 2, keyW = 46.0f;
    const float rowH = r.getHeight() / span;
    auto yOf = [&] (float m) { return r.getBottom() - (m - lo) * rowH; };
    const float x0 = r.getX() + keyW, W = r.getWidth() - keyW;

    for (int m = (int) std::floor (lo); m <= (int) std::ceil (lo + span); ++m)
    {
        const float y = yOf ((float) m + 0.5f);
        const bool in = S[jlimit (0, 7, sc)][((m - key) % 12 + 12) % 12];
        g.setColour (in ? Colour (0xff15203a) : Colour (0xff0d1526));
        g.fillRect (x0, y, W, rowH - 1);
        const bool blk = String ("010100101010").substring (((m % 12) + 12) % 12, ((m % 12) + 12) % 12 + 1) == "1";
        g.setColour (blk ? Colour (0xff1b2333) : Colour (0xffd9dee8));
        g.fillRect (r.getX(), y, keyW - 4, rowH - 1);
        if (((m - key) % 12 + 12) % 12 == 0 || m % 12 == 0)
        {
            g.setColour (blk ? col::text : Colour (0xff1b2333)); g.setFont (font (std::min (11.0f, rowH), true));
            g.drawText (noteName (m), Rectangle<float> (r.getX() + 2, y, keyW - 8, rowH), Justification::centredLeft);
        }
    }
    // notes
    const float dx = W / show;
    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (Rectangle<int> ((int) x0, (int) r.getY(), (int) W, (int) r.getHeight()));
    int i = 0;
    while (i < show)
    {
        const int idx = (hp - show + i + VoxMidiProcessor::histSize * 2) % VoxMidiProcessor::histSize;
        const float n = proc.histNote[(size_t) idx];
        if (n < 0) { ++i; continue; }
        int j = i;
        while (j < show && proc.histNote[(size_t) ((hp - show + j + VoxMidiProcessor::histSize * 2) % VoxMidiProcessor::histSize)] == n) ++j;
        const float y = yOf (n + 0.5f);
        g.setColour (col::accent.withAlpha (0.9f));
        g.fillRoundedRectangle (x0 + i * dx, y + 1, std::max (3.0f, (j - i) * dx - 1), rowH - 2, 3);
        i = j;
    }
    // courbe de hauteur
    Path p; bool started = false;
    for (int k = 0; k < show; ++k)
    {
        const float v = proc.histPitch[(size_t) ((hp - show + k + VoxMidiProcessor::histSize * 2) % VoxMidiProcessor::histSize)];
        if (v <= 0) { started = false; continue; }
        const float x = x0 + k * dx, y = yOf (v);
        if (! started) { p.startNewSubPath (x, y); started = true; } else p.lineTo (x, y);
    }
    g.setColour (Colours::white.withAlpha (0.85f)); g.strokePath (p, PathStrokeType (1.6f));
    g.setColour (col::amber); g.drawVerticalLine ((int) (x0 + W - 2), r.getY(), r.getBottom());
}

void DragMidi::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1);
    const bool has = proc.capCount.load() > 0;
    g.setColour (has ? col::accent.withAlpha (0.18f) : col::module); g.fillRoundedRectangle (r, 8);
    float dl[] = { 5, 4 };
    Path b; b.addRoundedRectangle (r, 8);
    Path d; PathStrokeType (1.4f).createDashedStroke (d, b, dl, 2);
    g.setColour (has ? col::accent : col::border); g.fillPath (d);
    g.setColour (has ? Colours::white : col::dim); g.setFont (font (13.0f, true));
    g.drawText (has ? String::fromUTF8 ("⇲  Glisse le MIDI vers FL Studio") : String ("Capture des notes pour glisser le MIDI"), r, Justification::centred);
}

void DragMidi::mouseDrag (const MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6 || proc.capCount.load() == 0) return;
    dragging = true;
    auto f = File::getSpecialLocation (File::tempDirectory).getChildFile ("BAKR_VOXMIDI.mid");
    if (proc.writeMidiFile (f))
        DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; });
    else dragging = false;
}
} // namespace vui

//==============================================================================
VoxMidiEditor::VoxMidiEditor (VoxMidiProcessor& p)
    : AudioProcessorEditor (&p), proc (p), noteDisp (p), roll (p), dragMidi (p)
{
    setLookAndFeel (&laf);
    auto& s = p.apvts;
    addAndMakeVisible (noteDisp); addAndMakeVisible (roll); addAndMakeVisible (dragMidi);

    presets.addItemList (VoxMidiProcessor::presetNames(), 1);
    presets.setTextWhenNothingSelected ("Presets BAKR...");
    presets.onChange = [this] { if (presets.getSelectedId() > 0) proc.applyPreset (presets.getSelectedId() - 1); };
    addAndMakeVisible (presets);

    keyBox.addItemList (VoxMidiProcessor::keyNames(), 1); addAndMakeVisible (keyBox);
    scaleBox.addItemList (VoxMidiProcessor::scaleNames(), 1); addAndMakeVisible (scaleBox);
    ca.push_back (std::make_unique<CA> (s, "key", keyBox));
    ca.push_back (std::make_unique<CA> (s, "scale", scaleBox));

    for (auto* b : { &midiOut, &synthOn, &follow, &bend }) addAndMakeVisible (*b);
    ba.push_back (std::make_unique<BA> (s, "midiOut", midiOut));
    ba.push_back (std::make_unique<BA> (s, "synthOn", synthOn));
    ba.push_back (std::make_unique<BA> (s, "follow", follow));
    ba.push_back (std::make_unique<BA> (s, "bend", bend));

    const auto names = VoxMidiProcessor::instrumentNames();
    for (int i = 0; i < names.size(); ++i)
    {
        auto* b = instButtons.add (new TextButton (names[i]));
        b->onClick = [this, i] { if (instAtt) instAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    instAtt = std::make_unique<ParameterAttachment> (*s.getParameter ("inst"), [this] (float v) { instIndex = (int) std::round (v); updateInstButtons(); });
    instAtt->sendInitialUpdate();

    knob (octave, "octave", "Octave", true);
    knob (gate, "gate", String::fromUTF8 ("Sensibilité"));
    knob (stab, "stab", String::fromUTF8 ("Stabilité"));
    knob (glide, "glide", "Glide");
    knob (tone, "tone", "Brillance");
    knob (velSens, "velSens", String::fromUTF8 ("Vélocité"));
    knob (voice, "voice", "Voix");
    knob (instLvl, "instLvl", "Instrument");
    knob (out, "out", "Sortie", true);

    capBtn.setClickingTogglesState (true);
    capBtn.getProperties().set ("rec", true);
    capBtn.onClick = [this] { proc.capturing.store (capBtn.getToggleState()); };
    clearBtn.onClick = [this] { proc.clearRequest.store (true); if (! proc.capturing.load()) proc.capCount.store (0); };
    saveBtn.onClick = [this]
    {
        auto f = proc.makeMidiFileInDocuments();
        if (f.existsAsFile()) { capInfo.setText (String::fromUTF8 ("Enregistré : Documents/BAKR VOXMIDI/") + f.getFileName(), dontSendNotification); f.revealToUser(); }
        else capInfo.setText (String::fromUTF8 ("Aucune note capturée."), dontSendNotification);
    };
    for (auto* b : { &capBtn, &clearBtn, &saveBtn }) addAndMakeVisible (*b);
    capInfo.setFont (vui::font (12.0f)); capInfo.setColour (Label::textColourId, vui::col::dim);
    addAndMakeVisible (capInfo);

    setSize (940, 600);
    startTimerHz (30);
}

VoxMidiEditor::~VoxMidiEditor() { stopTimer(); setLookAndFeel (nullptr); }

void VoxMidiEditor::knob (Slider& sl, const String& id, const String& label, bool bipolar)
{
    sl.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    sl.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    sl.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    sl.setPopupDisplayEnabled (true, true, this);
    sl.getProperties().set ("bipolar", bipolar);
    addAndMakeVisible (sl);
    sa.push_back (std::make_unique<SA> (proc.apvts, id, sl));
    if (auto* prm = proc.apvts.getParameter (id)) sl.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    knobLabels.push_back ({ &sl, label });
}

void VoxMidiEditor::updateInstButtons()
{
    for (int i = 0; i < instButtons.size(); ++i) instButtons[i]->setToggleState (i == instIndex, dontSendNotification);
}

void VoxMidiEditor::timerCallback()
{
    noteDisp.repaint(); roll.repaint(); dragMidi.repaint();
    const int n = proc.capCount.load();
    if (proc.capturing.load()) capInfo.setText (String (n) + String::fromUTF8 (" notes capturées…"), dontSendNotification);
    else if (! capInfo.getText().startsWith (String::fromUTF8 ("Enregistré"))) capInfo.setText (n > 0 ? String (n) + String::fromUTF8 (" notes prêtes") : String(), dontSendNotification);
}

void VoxMidiEditor::paint (Graphics& g)
{
    using namespace vui;
    g.fillAll (Colour (0xff05080f));
    auto outer = getLocalBounds().toFloat().reduced (8);
    g.setColour (col::panel); g.fillRoundedRectangle (outer, 16);
    g.setColour (col::border.withAlpha (0.7f)); g.drawRoundedRectangle (outer, 16, 1.2f);

    // logo
    const float cx = 46, cy = 44;
    g.setColour (col::text);
    for (int i = 1; i <= 4; ++i)
    {
        const float rr = 20.0f * i / 4.0f;
        Path a; a.addCentredArc (cx, cy, rr, rr, 0, -2.3f, -0.85f, true);
        Path b; b.addCentredArc (cx, cy, rr, rr, 0, 0.85f, 2.3f, true);
        g.strokePath (a, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));
        g.strokePath (b, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    g.setColour (col::accent); g.fillEllipse (cx - 3, cy - 3, 6, 6);
    g.setColour (Colours::white); g.setFont (font (32.0f, true)); g.drawText ("BAKR", 76, 22, 100, 44, Justification::centredLeft);
    g.setColour (col::accent); g.setFont (font (22.0f, true)); g.drawText ("VOXMIDI", 166, 26, 120, 36, Justification::centredLeft);
    g.setColour (col::dim); g.setFont (font (12.0f)); g.drawText (String::fromUTF8 ("Voix → MIDI + instruments"), 168, 54, 200, 16, Justification::centredLeft);

    for (auto r : { rLeft, rRoll, rInst, rKnobs })
    {
        g.setColour (col::module); g.fillRoundedRectangle (r.toFloat(), 10);
        g.setColour (col::border); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10, 1.2f);
    }
    g.setColour (col::text); g.setFont (font (14.0f, true));
    g.drawText ("INSTRUMENT", rInst.getX() + 14, rInst.getY() + 8, 200, 20, Justification::centredLeft);
    g.drawText ("CAPTURE MIDI", rLeft.getX() + 14, rLeft.getY() + 186, 200, 20, Justification::centredLeft);
    g.setColour (col::dim); g.setFont (font (12.0f));
    g.drawText (String::fromUTF8 ("Tonalité"), keyBox.getX(), keyBox.getY() - 17, 100, 16, Justification::centredLeft);
    g.drawText ("Gamme", scaleBox.getX(), scaleBox.getY() - 17, 100, 16, Justification::centredLeft);

    g.setColour (col::text); g.setFont (font (13.0f));
    for (auto& [s, l] : knobLabels) g.drawText (l, s->getX() - 15, s->getBottom() - 2, s->getWidth() + 30, 18, Justification::centred);
}

void VoxMidiEditor::resized()
{
    presets.setBounds (380, 26, 250, 34);
    midiOut.setBounds (650, 28, 130, 30);
    synthOn.setBounds (790, 28, 120, 30);

    rLeft = { 24, 84, 250, 300 };
    rRoll = { 290, 84, 626, 300 };
    rInst = { 24, 398, 420, 178 };
    rKnobs = { 458, 398, 458, 178 };

    noteDisp.setBounds (rLeft.getX() + 12, rLeft.getY() + 8, rLeft.getWidth() - 24, 172);
    capBtn.setBounds (rLeft.getX() + 14, rLeft.getY() + 212, 108, 30);
    clearBtn.setBounds (rLeft.getX() + 128, rLeft.getY() + 212, 108, 30);
    dragMidi.setBounds (rLeft.getX() + 14, rLeft.getY() + 248, 222, 30);
    saveBtn.setBounds (rLeft.getRight() + 0, 0, 0, 0);
    capInfo.setBounds (rLeft.getX() + 10, rLeft.getBottom() - 20, rLeft.getWidth() - 20, 18);
    roll.setBounds (rRoll.reduced (8));

    // le bouton Enregistrer .mid se place à côté du titre CAPTURE
    saveBtn.setBounds (rLeft.getX() + 126, rLeft.getY() + 182, 110, 26);

    for (int i = 0; i < instButtons.size(); ++i)
        instButtons[i]->setBounds (rInst.getX() + 14 + (i % 4) * 100, rInst.getY() + 34 + (i / 4) * 40, 94, 34);
    keyBox.setBounds (rInst.getX() + 14, rInst.getY() + 136, 90, 30);
    scaleBox.setBounds (rInst.getX() + 112, rInst.getY() + 136, 170, 30);
    octave.setBounds (rInst.getX() + 300, rInst.getY() + 116, 50, 44);
    follow.setBounds (rInst.getX() + 14 + 300, rInst.getY() + 8, 0, 0);

    const int kx = rKnobs.getX() + 10, ky = rKnobs.getY() + 12, kw = 68;
    Slider* row1[] = { &gate, &stab, &glide, &tone };
    Slider* row2[] = { &velSens, &voice, &instLvl, &out };
    for (int i = 0; i < 4; ++i) { row1[i]->setBounds (kx + i * kw, ky, 56, 56); row2[i]->setBounds (kx + i * kw, ky + 84, 56, 56); }
    follow.setBounds (kx + 4 * kw + 6, ky + 14, 170, 28);
    bend.setBounds (kx + 4 * kw + 6, ky + 98, 170, 28);
}
