#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static const int SCALES[][12] = {
    { 1,1,1,1,1,1,1,1,1,1,1,1 },   // Chromatique
    { 1,0,1,0,1,1,0,1,0,1,0,1 },   // Majeur
    { 1,0,1,1,0,1,0,1,1,0,1,0 },   // Mineur
    { 1,0,1,0,1,0,0,1,0,1,0,0 },   // Penta majeure
    { 1,0,0,1,0,1,0,1,0,0,1,0 },   // Penta mineure
    { 1,0,0,1,0,1,1,1,0,0,1,0 },   // Blues
    { 1,0,1,0,1,1,0,1,0,1,1,0 },   // Mixolydien
    { 1,0,1,1,0,1,0,1,0,1,1,0 },   // Dorien
};

juce::StringArray VoxMidiProcessor::instrumentNames() { return { "Guitare", "Basse", "Cuivres", "Lead", "Kora", "Balafon", "808", juce::String::fromUTF8 ("Flûte") }; }
juce::StringArray VoxMidiProcessor::scaleNames() { return { "Chromatique", "Majeur", "Mineur", "Penta majeure", "Penta mineure", "Blues", "Mixolydien (Highlife)", "Dorien" }; }
juce::StringArray VoxMidiProcessor::keyNames()
{
    juce::StringArray a;
    for (auto* n : { "Do", "Do#", "Ré", "Ré#", "Mi", "Fa", "Fa#", "Sol", "Sol#", "La", "La#", "Si" }) a.add (juce::String::fromUTF8 (n));
    return a;
}

APVTS::ParameterLayout VoxMidiProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto fl = [&] (const char* id, const char* name, float lo, float hi, float def, std::function<juce::String (float, int)> f)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (lo, hi, 0.01f), def,
                                                                  juce::AudioParameterFloatAttributes().withStringFromValueFunction (f)));
    };
    auto db = [] (float v, int) { return v <= -60.0f ? juce::String ("-inf dB") : juce::String (v, 1) + " dB"; };
    auto ms = [] (float v, int) { return juce::String ((int) v) + " ms"; };
    auto pc = [] (float v, int) { return juce::String ((int) std::round (v * 100)) + " %"; };

    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "midiOut", 1 }, "MIDI Out", true));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "synthOn", 1 }, "Synth", true));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "inst", 1 }, "Instrument", instrumentNames(), 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "key", 1 }, "Tonalite", keyNames(), 9));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "scale", 1 }, "Gamme", scaleNames(), 2));
    p.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "octave", 1 }, "Octave", -3, 3, 0));
    fl ("gate", "Sensibilite", -70, -20, -45, db);
    fl ("stab", "Stabilite", 10, 150, 45, ms);
    fl ("glide", "Glide", 0, 300, 25, ms);
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "follow", 1 }, "Suivre la voix", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bend", 1 }, "Pitch Bend MIDI", false));
    fl ("velSens", "Velocite", 0, 1, 0.7f, pc);
    fl ("voice", "Voix", -60, 0, -60, db);
    fl ("instLvl", "Instrument", -30, 6, 0, db);
    fl ("tone", "Brillance", 0, 1, 0.5f, pc);
    fl ("out", "Sortie", -24, 6, 0, db);
    return { p.begin(), p.end() };
}

VoxMidiProcessor::VoxMidiProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BAKR_VOXMIDI", createLayout())
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    pMidiOut = g ("midiOut"); pSynthOn = g ("synthOn"); pInst = g ("inst"); pKey = g ("key"); pScale = g ("scale"); pOctave = g ("octave");
    pGate = g ("gate"); pStab = g ("stab"); pGlide = g ("glide"); pFollow = g ("follow"); pBend = g ("bend"); pVelSens = g ("velSens");
    pVoice = g ("voice"); pInstLvl = g ("instLvl"); pTone = g ("tone"); pOut = g ("out");
}

bool VoxMidiProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    if (o != juce::AudioChannelSet::mono() && o != juce::AudioChannelSet::stereo()) return false;
    const auto i = l.getMainInputChannelSet();
    return i == juce::AudioChannelSet::mono() || i == juce::AudioChannelSet::stereo();
}

void VoxMidiProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    tracker.prepare (sr);
    synth.prepare (sr);
    curNote = candNote = -1; candMs = unvoicedMs = 0; medBuf = { 0, 0, 0 }; lastInst = -1; lastBend = 8192; activeCap = -1;
    for (auto* s : { &voiceG, &instG, &outG }) s->reset (sr, 0.03);
    voiceG.setCurrentAndTargetValue (vox::dbToGain (pVoice->load()));
    instG.setCurrentAndTargetValue (vox::dbToGain (pInstLvl->load()));
    outG.setCurrentAndTargetValue (vox::dbToGain (pOut->load()));
}

int VoxMidiProcessor::snapToScale (float m) const
{
    const int sc = std::clamp ((int) pScale->load(), 0, 7), key = (int) pKey->load();
    const int r = (int) std::round (m);
    int best = r; float bestD = 99;
    for (int c = r - 3; c <= r + 3; ++c)
    {
        const int pcl = ((c - key) % 12 + 12) % 12;
        if (! SCALES[sc][pcl]) continue;
        const float d = std::abs (m - (float) c);
        if (d < bestD) { bestD = d; best = c; }
    }
    return std::clamp (best, 0, 127);
}

void VoxMidiProcessor::sendNoteOff (int sampleIndex, juce::MidiBuffer& midi, double ppqNow)
{
    if (curNote < 0) return;
    if (pMidiOut->load() > 0.5f) midi.addEvent (juce::MidiMessage::noteOff (1, curNote), sampleIndex);
    synth.noteOff();
    if (activeCap >= 0 && activeCap < maxNotes) capNotes[(size_t) activeCap].end = ppqNow;
    activeCap = -1;
    curNote = -1;
}

void VoxMidiProcessor::handleAnalysis (int si, juce::MidiBuffer& midi, double blockPpq, bool hostPlaying, double bpm)
{
    const float hopMs = (float) (tracker.getHopSeconds() * 1000.0);
    const double ppqNow = hostPlaying ? blockPpq + si / sr * bpm / 60.0 : (internalSamples + si) / sr * bpm / 60.0;
    const float gate = pGate->load();
    const bool voiced = tracker.f0 > 0 && tracker.conf > 0.8f && tracker.rmsDb > gate;
    const bool midiOn = pMidiOut->load() > 0.5f;
    float mf = 0;

    if (voiced)
    {
        const float m = 69.0f + 12.0f * std::log2 (tracker.f0 / 440.0f);
        medBuf[(size_t) medPos] = m; medPos = (medPos + 1) % 3;
        auto s = medBuf; std::sort (s.begin(), s.end());
        const float med = (s[0] == 0 || unvoicedMs > 0) ? m : s[1];
        mf = med + 12.0f * pOctave->load();
        lastMf = mf;

        int q = snapToScale (mf);
        if (curNote >= 0 && std::abs (mf - (float) curNote) < 0.75f) q = curNote;   // hystérésis

        if (q == curNote) { candNote = -1; candMs = 0; }
        else
        {
            if (q == candNote) candMs += hopMs; else { candNote = q; candMs = hopMs; }
            if (candMs >= pStab->load())
            {
                const float vs = pVelSens->load();
                const float lvl = std::clamp ((tracker.rmsDb - gate) / 35.0f, 0.0f, 1.0f);
                const int vel = std::clamp ((int) std::round (100.0f * (1.0f - vs) + (35.0f + 92.0f * lvl) * vs), 1, 127);
                sendNoteOff (si, midi, ppqNow);
                curNote = q; candNote = -1; candMs = 0;
                if (midiOn) midi.addEvent (juce::MidiMessage::noteOn (1, curNote, (juce::uint8) vel), si);
                synth.noteOn (pFollow->load() > 0.5f ? mf : (float) curNote, vel / 127.0f);
                if (capturing.load())
                {
                    if (clearRequest.exchange (false)) capCount.store (0);
                    const int n = capCount.load();
                    if (n < maxNotes)
                    {
                        if (n == 0) { capSynced.store (hostPlaying); capBpm.store (bpm); }
                        capNotes[(size_t) n] = { ppqNow, -1.0, curNote, vel };
                        activeCap = n;
                        capCount.store (n + 1);
                    }
                }
            }
        }
        unvoicedMs = 0;
        if (curNote >= 0)
        {
            synth.setTarget (pFollow->load() > 0.5f ? mf : (float) curNote);
            if (midiOn && pBend->load() > 0.5f)
            {
                const float b = std::clamp ((mf - (float) curNote) / 2.0f, -1.0f, 1.0f);
                const int v = std::clamp ((int) std::round (8192 + b * 8191), 0, 16383);
                if (std::abs (v - lastBend) > 40) { midi.addEvent (juce::MidiMessage::pitchWheel (1, v), si); lastBend = v; }
            }
        }
    }
    else
    {
        unvoicedMs += hopMs;
        medBuf = { 0, 0, 0 };
        candNote = -1; candMs = 0;
        if (unvoicedMs > 60.0f && curNote >= 0)
        {
            sendNoteOff (si, midi, ppqNow);
            if (midiOn && lastBend != 8192) { midi.addEvent (juce::MidiMessage::pitchWheel (1, 8192), si); lastBend = 8192; }
        }
    }

    synth.setExpression ((tracker.rmsDb - gate) / 30.0f);

    // historique pour l'affichage
    const int hp = histPos.load();
    histPitch[(size_t) hp] = voiced ? mf : 0.0f;
    histNote[(size_t) hp] = (float) curNote;
    histPos.store ((hp + 1) % histSize);
    dispMidi.store (voiced ? mf : 0.0f);
    dispConf.store (tracker.conf);
    dispRms.store (tracker.rmsDb);
    dispNote.store (curNote);
}

void VoxMidiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    midi.clear();
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels(), outCh = getTotalNumOutputChannels();
    if (inCh == 0) { buffer.clear(); return; }

    double bpm = 120.0, ppq = 0.0; bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            hostPlaying = pos->getIsPlaying();
        }

    const int inst = (int) pInst->load();
    if (inst != lastInst) { synth.setInstrument (inst); lastInst = inst; }
    synth.setParams (pGlide->load(), pTone->load());
    voiceG.setTargetValue (vox::dbToGain (pVoice->load()));
    instG.setTargetValue (pSynthOn->load() > 0.5f ? vox::dbToGain (pInstLvl->load()) : 0.0f);
    outG.setTargetValue (vox::dbToGain (pOut->load()));
    if (! capturing.load() && clearRequest.exchange (false)) capCount.store (0);

    const float* in0 = buffer.getReadPointer (0);
    const float* in1 = inCh > 1 ? buffer.getReadPointer (1) : nullptr;
    float* o0 = buffer.getWritePointer (0);
    float* o1 = outCh > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        const float x = in1 ? 0.5f * (in0[i] + in1[i]) : in0[i];
        if (tracker.push (x)) handleAnalysis (i, midi, ppq, hostPlaying, bpm);
        const float s = synth.render();
        const float vg = voiceG.getNextValue(), ig = instG.getNextValue(), og = outG.getNextValue();
        const float l = (in0[i] * vg + s * ig) * og;
        const float r = ((in1 ? in1[i] : in0[i]) * vg + s * ig) * og;
        o0[i] = std::tanh (l);
        if (o1) o1[i] = std::tanh (r);
    }
    internalSamples += n;
}

//==============================================================================
bool VoxMidiProcessor::writeMidiFile (const juce::File& f)
{
    const int n = capCount.load();
    if (n == 0) return false;
    const double bpm = capBpm.load();
    const bool synced = capSynced.load();
    const double offset = synced ? 0.0 : std::floor (capNotes[0].start);
    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)), 0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, "BAKR VOXMIDI"), 0);
    const int tpq = 960;
    for (int i = 0; i < n; ++i)
    {
        const auto c = capNotes[(size_t) i];
        const double s = std::max (0.0, c.start - offset);
        const double e = c.end > c.start ? c.end - offset : s + 0.25;
        seq.addEvent (juce::MidiMessage::noteOn (1, c.note, (juce::uint8) c.vel), s * tpq);
        seq.addEvent (juce::MidiMessage::noteOff (1, c.note), std::max (s + 0.02, e) * tpq);
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);
    mf.addTrack (seq);
    f.deleteFile();
    juce::FileOutputStream os (f);
    if (! os.openedOk()) return false;
    return mf.writeTo (os);
}

juce::File VoxMidiProcessor::makeMidiFileInDocuments()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("BAKR VOXMIDI");
    dir.createDirectory();
    auto f = dir.getChildFile ("VoxMidi_" + juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S") + ".mid");
    return writeMidiFile (f) ? f : juce::File();
}

//==============================================================================
void VoxMidiProcessor::setParam (const juce::String& id, float plain)
{
    if (auto* prm = apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (plain));
        prm->endChangeGesture();
    }
}

juce::StringArray VoxMidiProcessor::presetNames()
{
    juce::StringArray a;
    for (auto* n : { "Par défaut", "Guitare Afrobeats", "Basse Mandingue", "Cuivres Highlife", "Lead Amapiano", "Kora Mandingue",
                     "Balafon", "808 Trap / Drill", "Flûte Peule", "MIDI seulement (vers FL Studio)" })
        a.add (juce::String::fromUTF8 (n));
    return a;
}

void VoxMidiProcessor::applyPreset (int i)
{
    struct P { int inst, key, scale, oct; float stab, glide, tone, follow, synth; };
    static const P ps[] = {
        // inst key scale oct stab glide tone follow synth
        { 0, 9, 2,  0, 45, 25, 0.5f, 0, 1 },
        { 0, 9, 4,  0, 40, 10, 0.6f, 0, 1 },
        { 1, 7, 3, -2, 55, 30, 0.4f, 0, 1 },
        { 2, 7, 6,  0, 45, 40, 0.6f, 1, 1 },
        { 3, 5, 2,  0, 35, 60, 0.7f, 1, 1 },
        { 4, 7, 3,  0, 35, 10, 0.7f, 0, 1 },
        { 5, 9, 3,  0, 35,  0, 0.5f, 0, 1 },
        { 6, 1, 2, -2, 50, 80, 0.5f, 0, 1 },
        { 7, 2, 4,  1, 40, 50, 0.4f, 1, 1 },
        { 0, 9, 0,  0, 45, 25, 0.5f, 0, 0 },
    };
    if (i < 0 || i >= (int) std::size (ps)) return;
    const auto& p = ps[i];
    setParam ("inst", (float) p.inst); setParam ("key", (float) p.key); setParam ("scale", (float) p.scale);
    setParam ("octave", (float) p.oct); setParam ("stab", p.stab); setParam ("glide", p.glide); setParam ("tone", p.tone);
    setParam ("follow", p.follow); setParam ("synthOn", p.synth); setParam ("midiOut", 1);
}

void VoxMidiProcessor::getStateInformation (juce::MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}
void VoxMidiProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* VoxMidiProcessor::createEditor() { return new VoxMidiEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoxMidiProcessor(); }
