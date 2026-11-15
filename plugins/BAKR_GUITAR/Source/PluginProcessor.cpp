#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace bg;

//==============================================================================
namespace
{
struct PresetDef
{
    const char* name; int guitar; float attack, release, bright, width, dly, rvb, out;
    int fxAmp, fxEq, fxComp, fxChorus, fxDelay, fxReverb;
    float drive, lo, mid, hi, comp, chorus; int dtime; int strum, strumDir; float space; int noise;
};

// dtime : 0 = 1/4, 1 = 1/8 pointée, 2 = 1/8, 3 = 1/16, 4 = 1/4 pointée
const PresetDef presets[] =
{
    // nom                       gtr att  rel    bri  lar  dly  rvb  out   amp eq cmp cho dly rvb  drv   lo   mid  hi  cmp  cho  dt st dir esp nz
    { "Le Jazz Guitar Afro",       2, 0,   300,  62, 36, 13.4f,28, -7,    1, 1, 1,  0,  1,  1, 0.15f, 1,  2, -1, .35f, .2f, 1, 0, 0, 18, 1 },
    { "Highlife Ghana",            3, 0,   220,  75, 45, 10,  18, -6,     1, 1, 1,  1,  1,  1, 0.08f, 0,  1,  2, .40f, .30f,2, 0, 0, 18, 1 },
    { "Soukous Sebene",            3, 0,   180,  85, 40, 18,  15, -6,     1, 1, 1,  1,  1,  1, 0.06f,-1,  0,  3, .50f, .35f,1, 0, 0, 18, 1 },
    { "Afrobeats Lead Nigeria",    2, 0,   350,  60, 50, 22,  25, -7,     1, 1, 1,  1,  1,  1, 0.12f, 0,  2,  1, .40f, .20f,1, 0, 0, 18, 1 },
    { "Mandingue Acoustique",      0, 0,   400,  70, 55, 0,   22, -7,     0, 1, 1,  0,  0,  1, 0.0f,  1,  0,  2, .30f, .0f, 1, 0, 0, 18, 1 },
    { "Coupe-decale Rythmique",    3, 0,   90,   80, 60, 8,   10, -6,     1, 1, 1,  1,  1,  1, 0.05f,-1,  1,  3, .60f, .25f,3, 0, 0, 18, 0 },
    { "Reggae Skank",              3, 0,   70,   55, 45, 12,  20, -6,     1, 1, 1,  0,  1,  1, 0.10f,-2,  3,  0, .50f, .0f, 4, 1, 1, 10, 0 },
    { "Amapiano Guitare",          1, 0,   500,  55, 50, 20,  35, -1,     0, 1, 1,  0,  1,  1, 0.0f,  1,  0,  1, .30f, .0f, 0, 0, 0, 18, 1 },
    { "Rumba Congolaise",          3, 0,   250,  78, 45, 14,  20, -6,     1, 1, 1,  1,  1,  1, 0.06f, 0,  1,  2, .45f, .25f,1, 0, 0, 18, 1 },
    { "Folk Acoustique (accords)", 0, 0,   450,  65, 70, 0,   18, -9,     0, 1, 1,  0,  0,  1, 0.0f,  0,  0,  2, .35f, .0f, 1, 1, 0, 18, 1 },
    { "Nylon Classique",           1, 0,   600,  50, 55, 0,   25,  0,     0, 0, 0,  0,  0,  1, 0.0f,  0,  0,  0, .20f, .0f, 1, 0, 0, 18, 1 },
    { "Gratte Alternee",           0, 0,   300,  68, 75, 0,   15, -9,     0, 1, 1,  0,  0,  1, 0.0f,  0, -1,  2, .45f, .0f, 1, 1, 2, 22, 1 },
    { "Blues Crunch",              2, 0,   350,  60, 40, 10,  20, -6,     1, 1, 1,  0,  1,  1, 0.55f, 1,  2,  0, .30f, .0f, 2, 0, 0, 18, 1 },
    { "Rock Humbucker",            2, 0,   300,  65, 60, 0,   12, -6,     1, 1, 1,  0,  0,  1, 0.85f, 2,  1,  1, .30f, .0f, 1, 0, 0, 18, 0 },
    { "Funk Simple Bobinage",      3, 0,   60,   85, 50, 0,   10, -9,     1, 1, 1,  0,  0,  1, 0.05f,-2,  0,  3, .60f, .0f, 3, 0, 0, 18, 1 },
    { "Ambiance Pad Guitare",      3, 40,  1500, 50, 90, 40,  55,  2,     0, 1, 1,  1,  1,  1, 0.0f,  0, -2,  1, .30f, .50f,0, 0, 0, 18, 0 },
};
constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));
}

//==============================================================================
BakrGuitarProcessor::BakrGuitarProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BAKR_GUITAR", createLayout())
{
    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    pGuitar = P ("guitar"); pAttack = P ("attack"); pRelease = P ("release"); pBright = P ("bright");
    pWidth = P ("width"); pDelay = P ("delay"); pReverb = P ("reverb"); pOut = P ("output"); pArtic = P ("artic");
    pFxAmp = P ("fxAmp"); pFxEq = P ("fxEq"); pFxComp = P ("fxComp"); pFxChorus = P ("fxChorus");
    pFxDelay = P ("fxDelay"); pFxReverb = P ("fxReverb");
    pStrum = P ("strum"); pStrumDir = P ("strumDir"); pStrumSpace = P ("strumSpace"); pNoise = P ("handNoise");
    pDrive = P ("ampDrive"); pEqLow = P ("eqLow"); pEqMid = P ("eqMid"); pEqHigh = P ("eqHigh");
    pComp = P ("compAmount"); pChorus = P ("chorusMix"); pDelayTime = P ("delayTime");
    for (auto& k : keyToString) k = -1;
    applyPreset (0);
}

juce::AudioProcessorValueTreeState::ParameterLayout BakrGuitarProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto pct = [] (float v, int) { return String (v, 1) + " %"; };
    auto ms  = [] (float v, int) { return String (v, v < 10 ? 1 : 0) + " ms"; };

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "guitar", 1 }, "Guitare",
                   StringArray { "Acier acoustique", "Classique nylon", "Double bobinage", "Simple bobinage" }, 2));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 }, "Attaque",
                   NormalisableRange<float> (0.0f, 200.0f, 0.1f, 0.4f), 0.0f, AudioParameterFloatAttributes().withStringFromValueFunction (ms)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Relachement",
                   NormalisableRange<float> (10.0f, 3000.0f, 1.0f, 0.4f), 300.0f, AudioParameterFloatAttributes().withStringFromValueFunction (ms)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "bright", 1 }, "Brillance",
                   NormalisableRange<float> (0.0f, 100.0f, 0.1f), 62.0f, AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "width", 1 }, "Largeur",
                   NormalisableRange<float> (0.0f, 100.0f, 0.1f), 36.0f, AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "delay", 1 }, "Delay",
                   NormalisableRange<float> (0.0f, 100.0f, 0.1f), 13.4f, AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "reverb", 1 }, "Reverb",
                   NormalisableRange<float> (0.0f, 100.0f, 0.1f), 28.0f, AudioParameterFloatAttributes().withStringFromValueFunction (pct)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Sortie",
                   NormalisableRange<float> (-24.0f, 12.0f, 0.1f), -7.0f,
                   AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "artic", 1 }, "Articulation",
                   StringArray { "Sustain", "Palm mute", "Harmoniques", "Hammer-on", "Pull-off", "Slide", "Vibrato", "Slide up", "Slide down" }, 0));

    for (auto id : { "fxAmp", "fxEq", "fxComp", "fxChorus", "fxDelay", "fxReverb" })
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, String ("FX ") + String (id).substring (2), true));

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "strum", 1 }, "Strum (jeu en accords)", false));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "strumDir", 1 }, "Sens du coup",
                   StringArray { "Bas", "Haut", "Alterne" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "strumSpace", 1 }, "Espacement",
                   NormalisableRange<float> (2.0f, 80.0f, 0.1f), 18.0f, AudioParameterFloatAttributes().withStringFromValueFunction (ms)));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "handNoise", 1 }, "Bruits de main", true));

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "ampDrive", 1 }, "Gain ampli",
                   NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.15f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "eqLow", 1 }, "EQ Grave", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "eqMid", 1 }, "EQ Medium", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "eqHigh", 1 }, "EQ Aigu", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "compAmount", 1 }, "Compression", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "chorusMix", 1 }, "Chorus", NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "delayTime", 1 }, "Temps delay",
                   StringArray { "1/4", "1/8 pointee", "1/8", "1/16", "1/4 pointee" }, 1));
    return { p.begin(), p.end() };
}

juce::StringArray BakrGuitarProcessor::getPresetNames()
{
    juce::StringArray s;
    for (auto& p : presets) s.add (p.name);
    return s;
}

int BakrGuitarProcessor::getNumPrograms() { return numPresets; }
const juce::String BakrGuitarProcessor::getProgramName (int i) { return juce::isPositiveAndBelow (i, numPresets) ? presets[i].name : ""; }

void BakrGuitarProcessor::setParam (const juce::String& id, float v)
{
    if (auto* prm = apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

void BakrGuitarProcessor::applyPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, numPresets)) return;
    const auto& p = presets[i];
    currentPreset = i;
    setParam ("guitar", (float) p.guitar);   setParam ("attack", p.attack);  setParam ("release", p.release);
    setParam ("bright", p.bright);           setParam ("width", p.width);    setParam ("delay", p.dly);
    setParam ("reverb", p.rvb);              setParam ("output", p.out);
    setParam ("fxAmp", (float) p.fxAmp);     setParam ("fxEq", (float) p.fxEq);       setParam ("fxComp", (float) p.fxComp);
    setParam ("fxChorus", (float) p.fxChorus); setParam ("fxDelay", (float) p.fxDelay); setParam ("fxReverb", (float) p.fxReverb);
    setParam ("ampDrive", p.drive);  setParam ("eqLow", p.lo); setParam ("eqMid", p.mid); setParam ("eqHigh", p.hi);
    setParam ("compAmount", p.comp); setParam ("chorusMix", p.chorus); setParam ("delayTime", (float) p.dtime);
    setParam ("strum", (float) p.strum); setParam ("strumDir", (float) p.strumDir); setParam ("strumSpace", p.space);
    setParam ("handNoise", (float) p.noise);
    setParam ("artic", 0.0f);
}

//==============================================================================
void BakrGuitarProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    for (auto& s : strings) s.prepare (sr);
    noise.prepare (sr); body.prepare (sr); amp.prepare (sr); comp.prepare (sr); delay.prepare (sr);
    for (auto& ch : eq) for (auto& b : ch) b.reset();
    lastEq[0] = lastEq[1] = lastEq[2] = 99;

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) juce::jmax (1, samplesPerBlock), 2 };
    chorus.prepare (spec);
    chorus.setRate (0.7f); chorus.setDepth (0.22f); chorus.setCentreDelay (9.0f); chorus.setFeedback (0.0f);
    reverb.setSampleRate (sr);
    reverb.reset();
    outGain.reset (sr, 0.03);
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    scheduled.clear(); pendingChord.clear(); heldKeys.clear();
    scheduled.reserve (256); pendingChord.reserve (32); heldKeys.reserve (32);
    for (auto& k : keyToString) k = -1;
    keyboardState.reset();
}

bool BakrGuitarProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

float BakrGuitarProcessor::delayTimeSeconds() const
{
    double bpm = 100.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = juce::jlimit (40.0, 300.0, *b);
    const double beat = 60.0 / bpm;
    static const double mult[] = { 1.0, 0.75, 0.5, 0.25, 1.5 };
    return (float) (beat * mult[juce::jlimit (0, 4, (int) pDelayTime->load())]);
}

//==============================================================================
// Choix de la corde : comme un vrai guitariste, on joue la note sur la corde libre
// la plus aiguë qui peut l'atteindre (moins de 19 cases) -> les accords sonnent naturels.
int BakrGuitarProcessor::chooseString (int note)
{
    int best = -1;
    for (int s = 5; s >= 0; --s)
    {
        int fret = note - openNotes[s];
        if (fret < 0 || fret > 19) continue;
        bool busy = false;
        for (int k = 0; k < 128; ++k) if (keyToString[k] == s && k != note) { busy = true; break; }
        if (! busy) { best = s; break; }
    }
    if (best >= 0) return best;
    // toutes prises : on reprend la corde jouée depuis le plus longtemps parmi celles possibles
    uint64_t oldest = UINT64_MAX;
    for (int s = 0; s < 6; ++s)
    {
        int fret = note - openNotes[s];
        if (fret < 0 || fret > 24) continue;
        if (strings[(size_t) s].age < oldest) { oldest = strings[(size_t) s].age; best = s; }
    }
    if (best < 0) best = note < openNotes[0] ? 0 : 5;
    return best;
}

void BakrGuitarProcessor::triggerString (int note, float vel, int artic)
{
    int s = chooseString (note);
    for (auto& k : keyToString) if (k == s) k = -1;
    keyToString[note] = s;
    auto& v = strings[(size_t) s];
    float offset = 0.0f, glide = 0.0f;
    if (artic == SlideUp) { offset = -3.0f; glide = 110.0f; }
    v.pluck (note, vel, (int) pGuitar->load(), artic, pBright->load() * 0.01f, pAttack->load(),
             offset, glide, false, 1.0f);
    v.age = ++ageCounter;
    v.pan = ((float) s - 2.5f) / 2.5f;

    if (pNoise->load() > 0.5f)
    {
        noise.trigger (NoiseFx::PickClick, 0.04f * vel, v.pan * 0.3f);
        if (artic == SlideUp) noise.trigger (NoiseFx::FretSqueak, 0.05f, v.pan * 0.3f);
    }
}

void BakrGuitarProcessor::handleNoteOn (int note, float vel)
{
    // keyswitches d'articulation (touches rouges)
    if (note >= kKeyswitchLow && note < kKeyswitchLow + 9)
    {
        keyswitchArtic = note - kKeyswitchLow;
        return;
    }
    // bruits de guitare (touches vertes)
    if (note >= kFxLow && note < kFxLow + 4)
    {
        int fx = note - kFxLow;
        if (fx == 0) noise.trigger (NoiseFx::FretSqueak, 0.12f * vel, 0.0f);
        if (fx == 1) noise.trigger (NoiseFx::BodyKnock, 0.5f * vel, 0.0f);
        if (fx == 2) // coup étouffé sur les 6 cordes
            for (int s = 0; s < 6; ++s)
                scheduled.push_back ({ sampleCounter + (uint64_t) (s * 0.006 * sr), true, 1000 + openNotes[s], vel * 0.8f });
        if (fx == 3) noise.trigger (NoiseFx::PickScrape, 0.08f * vel, 0.0f);
        return;
    }

    int artic = keyswitchArtic.load() >= 0 ? keyswitchArtic.load() : (int) pArtic->load();
    bool legatoArtic = (artic == HammerOn || artic == PullOff || artic == Slide);

    if (legatoArtic && ! heldKeys.empty())
    {
        int prev = heldKeys.back();
        int s = keyToString[prev];
        if (s >= 0)
        {
            auto& v = strings[(size_t) s];
            float glide = artic == Slide ? 60.0f + 4.0f * (float) std::abs (note - prev) : 0.0f;
            float ex = artic == HammerOn ? 0.3f * vel : artic == PullOff ? 0.45f * vel : 0.0f;
            v.legato (note, glide, ex);
            v.age = ++ageCounter;
            keyToString[prev] = -1;
            keyToString[note] = s;
            heldKeys.push_back (note);
            if (pNoise->load() > 0.5f && artic == Slide) noise.trigger (NoiseFx::FretSqueak, 0.06f, v.pan * 0.3f);
            return;
        }
    }

    heldKeys.push_back (note);
    triggerString (note, vel, artic);
}

void BakrGuitarProcessor::handleNoteOff (int note)
{
    if (note >= kKeyswitchLow && note < kKeyswitchLow + 9) return;
    if (note >= kFxLow && note < kFxLow + 4) return;

    bool wasTop = ! heldKeys.empty() && heldKeys.back() == note;
    heldKeys.erase (std::remove (heldKeys.begin(), heldKeys.end(), note), heldKeys.end());

    int s = keyToString[note];
    if (s < 0) return;
    keyToString[note] = -1;
    auto& v = strings[(size_t) s];

    int artic = keyswitchArtic.load() >= 0 ? keyswitchArtic.load() : (int) pArtic->load();
    bool legatoArtic = (artic == HammerOn || artic == PullOff || artic == Slide);
    // legato : on retombe sur la note encore tenue (trilles hammer-on / pull-off)
    if (legatoArtic && wasTop && ! heldKeys.empty() && keyToString[heldKeys.back()] < 0)
    {
        int back = heldKeys.back();
        v.legato (back, artic == Slide ? 70.0f : 0.0f, artic == PullOff || artic == HammerOn ? 0.35f : 0.0f);
        keyToString[back] = s;
        return;
    }

    v.release (pRelease->load());
    if (pNoise->load() > 0.5f && (rng.next() > 0.2f))
        noise.trigger (NoiseFx::ReleaseNoise, 0.02f, v.pan * 0.3f);
}

void BakrGuitarProcessor::renderVoices (float* L, float* R, int n)
{
    const float width = pWidth->load() * 0.01f;
    const float mw = modWheel.load();
    const float vibInc = 2.0f * kPi * 5.4f / (float) sr;
    const float bendTarget = pitchWheel.load() * 2.0f;

    for (int i = 0; i < n; ++i)
    {
        smoothedBend += (bendTarget - smoothedBend) * 0.002f;
        vibPhase += vibInc; if (vibPhase > 2.0f * kPi) vibPhase -= 2.0f * kPi;
        const float lfo = std::sin (vibPhase);
        float l = 0, r = 0;
        for (auto& v : strings)
        {
            if (! v.active) continue;
            float depth = mw * 0.5f + (v.articulation == Vibrato ? 0.35f : 0.0f);
            float x = v.render (smoothedBend + depth * lfo, rng);
            // étalement des cordes dans l'espace stéréo (grave à gauche, aigu à droite)
            float p = v.pan * width * 0.6f;
            l += x * (p > 0.0f ? 1.0f - p : 1.0f);
            r += x * (p < 0.0f ? 1.0f + p : 1.0f);
        }
        L[i] += l * 0.5f;
        R[i] += r * 0.5f;
    }
}

static inline float safety (float x)
{
    const float a = std::abs (x);
    if (a < 0.89f) return x;
    return std::copysign (0.89f + 0.1f * std::tanh ((a - 0.89f) / 0.1f), x);
}

void BakrGuitarProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    // notes cliquées sur le clavier de l'interface
    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    body.setGuitar ((int) pGuitar->load());
    // si l'articulation est changée à la souris / par automation, elle remplace le keyswitch
    {
        const int pa = (int) pArtic->load();
        if (pa != lastParamArtic) { lastParamArtic = pa; keyswitchArtic = -1; }
    }
    const bool strumOn = pStrum->load() > 0.5f;
    const uint64_t captureWindow = (uint64_t) (0.022 * sr);

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    auto midiIt = midi.cbegin();
    constexpr int sub = 32;
    for (int start = 0; start < numSamples; start += sub)
    {
        const int n = juce::jmin (sub, numSamples - start);
        const uint64_t now = sampleCounter;

        // 1) événements MIDI de ce petit bloc
        for (; midiIt != midi.cend() && (*midiIt).samplePosition < start + n; ++midiIt)
        {
            const auto m = (*midiIt).getMessage();
            if (m.isNoteOn())
            {
                const int nn = m.getNoteNumber();
                bool special = (nn >= kKeyswitchLow && nn < kKeyswitchLow + 9) || (nn >= kFxLow && nn < kFxLow + 4);
                if (strumOn && ! special)
                {
                    if (pendingChord.empty()) chordWindowEnd = now + captureWindow;
                    pendingChord.push_back ({ now, true, nn, m.getFloatVelocity() });
                }
                else handleNoteOn (nn, m.getFloatVelocity());
            }
            else if (m.isNoteOff())
            {
                const int nn = m.getNoteNumber();
                bool pending = false;
                for (auto& e : pendingChord) if (e.note == nn) pending = true;
                bool sched = false;
                for (auto& e : scheduled) if (e.on && e.note == nn) sched = true;
                if (pending || sched)
                    scheduled.push_back ({ juce::jmax (now, chordWindowEnd) + (uint64_t) (pStrumSpace->load() * 0.001 * sr * 6.0 + 0.01 * sr), false, nn, 0.0f });
                else handleNoteOff (nn);
            }
            else if (m.isPitchWheel())
            {
                if (! uiPitchActive.load()) pitchWheel = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
            }
            else if (m.isController() && m.getControllerNumber() == 1)
                modWheel = (float) m.getControllerValue() / 127.0f;
            else if (m.isAllNotesOff() || m.isAllSoundOff())
            {
                for (auto& v : strings) v.kill();
                for (auto& k : keyToString) k = -1;
                heldKeys.clear(); scheduled.clear(); pendingChord.clear();
            }
        }

        // 2) accord capturé -> on le gratte corde par corde
        if (! pendingChord.empty() && now >= chordWindowEnd)
        {
            int dir = (int) pStrumDir->load();
            bool up = dir == 1 || (dir == 2 && strumAlternateUp);
            if (dir == 2) strumAlternateUp = ! strumAlternateUp;
            std::sort (pendingChord.begin(), pendingChord.end(),
                       [up] (const Event& a, const Event& b) { return up ? a.note > b.note : a.note < b.note; });
            const double space = pStrumSpace->load() * 0.001 * sr;
            for (size_t i = 0; i < pendingChord.size(); ++i)
            {
                float vel = pendingChord[i].vel * (up ? 0.85f : 1.0f) * (1.0f - 0.03f * (float) i);
                scheduled.push_back ({ now + (uint64_t) (space * (double) i), true, pendingChord[i].note, vel });
            }
            pendingChord.clear();
        }

        // 3) événements programmés (grattage, notes étouffées)
        for (size_t i = 0; i < scheduled.size();)
        {
            auto e = scheduled[i];
            if (e.time < now + (uint64_t) n)
            {
                scheduled.erase (scheduled.begin() + (long) i);
                if (e.on && e.note >= 1000)   // coup étouffé
                {
                    int s = juce::jlimit (0, 5, (int) (std::find (std::begin (openNotes), std::end (openNotes), e.note - 1000) - std::begin (openNotes)));
                    auto& v = strings[(size_t) s];
                    v.pluck (e.note - 1000, e.vel, (int) pGuitar->load(), PalmMute, 0.3f, 0.0f, 0.0f, 0.0f, false, 0.7f);
                    v.pan = ((float) s - 2.5f) / 2.5f; v.age = ++ageCounter;
                    v.release (40.0f);
                    for (auto& k : keyToString) if (k == s) k = -1;
                }
                else if (e.on) handleNoteOn (e.note, e.vel);
                else handleNoteOff (e.note);
            }
            else ++i;
        }

        // 4) rendu des cordes et des bruits
        renderVoices (L + start, R + start, n);
        noise.render (L + start, R + start, n);
        sampleCounter += (uint64_t) n;
    }

    // ===== Chaîne d'effets =====
    const bool fxAmp = pFxAmp->load() > 0.5f, fxEq = pFxEq->load() > 0.5f, fxComp = pFxComp->load() > 0.5f;
    const float drive = pDrive->load(), compAmt = pComp->load();
    if (fxEq)
    {
        float lo = pEqLow->load(), mi = pEqMid->load(), hi = pEqHigh->load();
        if (lo != lastEq[0] || mi != lastEq[1] || hi != lastEq[2])
        {
            for (auto& ch : eq) { ch[0].lowShelf (sr, 140, lo); ch[1].peak (sr, 900, 0.8, mi); ch[2].highShelf (sr, 4500, hi); }
            lastEq[0] = lo; lastEq[1] = mi; lastEq[2] = hi;
        }
    }
    const bool electric = (int) pGuitar->load() >= DoubleBobinage;
    for (int i = 0; i < numSamples; ++i)
    {
        float l = body.process (0, L[i]), r = body.process (1, R[i]);
        if (fxAmp)
        {
            // l'ampli colore surtout les électriques ; sur les acoustiques il reste discret
            float d = electric ? drive : drive * 0.3f;
            float al = amp.process (0, l, d), ar = amp.process (1, r, d);
            float mix = electric ? 1.0f : 0.5f;
            l = l + (al - l) * mix; r = r + (ar - r) * mix;
        }
        if (fxEq) { for (auto& b : eq[0]) l = b.process (l); for (auto& b : eq[1]) r = b.process (r); }
        if (fxComp) comp.process (l, r, compAmt);
        L[i] = l; R[i] = r;
    }

    if (pFxChorus->load() > 0.5f && pChorus->load() > 0.001f)
    {
        chorus.setMix (pChorus->load() * 0.6f);
        juce::dsp::AudioBlock<float> blk (buffer);
        chorus.process (juce::dsp::ProcessContextReplacing<float> (blk));
    }

    if (pFxDelay->load() > 0.5f)
    {
        const float t = delayTimeSeconds(), mix = pDelay->load() * 0.01f * 0.7f;
        for (int i = 0; i < numSamples; ++i) delay.process (L[i], R[i], t, mix, 0.38f);
    }

    if (pFxReverb->load() > 0.5f)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.62f; rp.damping = 0.45f; rp.width = 1.0f;
        rp.wetLevel = pReverb->load() * 0.01f * 0.45f; rp.dryLevel = 1.0f;
        reverb.setParameters (rp);
        reverb.processStereo (L, R, numSamples);
    }

    outGain.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outGain.getNextValue();
        L[i] = safety (L[i] * g);   // écrêteur doux de sécurité (n'agit qu'au-dessus de -1 dB)
        R[i] = safety (R[i] * g);
        peak = juce::jmax (peak, std::abs (L[i]), std::abs (R[i]));
    }
    outLevel = juce::jmax (peak, outLevel.load() * 0.9f);
    midi.clear();
}

//==============================================================================
void BakrGuitarProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void BakrGuitarProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPreset = (int) vt.getProperty ("preset", 0);
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* BakrGuitarProcessor::createEditor() { return new BakrGuitarEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BakrGuitarProcessor(); }
