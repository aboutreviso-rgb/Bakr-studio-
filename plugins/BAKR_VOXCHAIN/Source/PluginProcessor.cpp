#include "PluginProcessor.h"
#include "PluginEditor.h"

using Coef = juce::dsp::IIR::Coefficients<float>;

namespace
{
struct PresetDef
{
    const char* name;
    int compOn; float cin, camt; int slow; float cout; int autoM;
    int enhOn; float deess, denoise, air; int enhPre;
    int eqOn; float hi, mid, low; int eqPre, adlib;
    int mulOn; float spread, offset, amount, mlow;
    int spcOn; float revTime, revMix; int revType; int dTime; float dMix; int dType;
};

const PresetDef presets[] =
{
//    nom                       C  in   amt  sl out au   E  dess dnoi air pre  Q  hi   mid  low pre adl  M  spr off  amt low   S  rT  rM ty dt dM  dTy
    { "Defaut",                 1, 0,   35, 0,  0, 1,    1, 30,  10, 20, 0,   1, 1,   0,  25, 0,  0,   0, 60, 40,  30, 50,   1, 45, 15, 1, 1, 10, 0 },
    { "Voix Afro claire",       1, 2,   50, 0,  0, 1,    1, 45,  15, 40, 0,   1, 3,   1,  35, 0,  0,   0, 60, 40,  25, 50,   1, 40, 18, 1, 2, 14, 0 },
    { "Refrain tres large",     1, 2,   55, 0,  0, 1,    1, 40,  10, 35, 0,   1, 2,   0,  35, 0,  0,   1, 90, 55,  55, 60,   1, 55, 22, 2, 2, 16, 2 },
    { "Ad-libs",                1, 3,   60, 0,  0, 1,    1, 35,  15, 25, 0,   1, 2,   3,  45, 0,  1,   1, 85, 60,  40, 70,   1, 50, 25, 4, 3, 25, 2 },
    { "Voix mandingue chaude",  1, 1,   40, 1,  0, 1,    1, 30,  10, 15, 0,   1, -1,  2,  30, 0,  0,   0, 60, 40,  25, 50,   1, 60, 20, 2, 1, 12, 1 },
    { "Rap lead sec",           1, 4,   70, 0,  0, 1,    1, 50,  20, 30, 0,   1, 2,   2,  40, 1,  0,   0, 50, 30,  20, 50,   0, 30, 8,  0, 3, 8,  3 },
    { "Ballade (hall)",         1, 0,   40, 1,  0, 1,    1, 30,  10, 30, 0,   1, 2,   0,  25, 0,  0,   1, 50, 35,  20, 50,   1, 75, 30, 2, 1, 18, 1 },
    { "Voix radio",             1, 4,   65, 0,  0, 1,    1, 40,  20, 10, 0,   1, -2,  6,  60, 0,  1,   0, 50, 30,  20, 50,   1, 20, 10, 0, 4, 10, 3 },
    { "Cathedrale",             1, 0,   35, 1,  0, 1,    1, 30,  10, 30, 0,   1, 2,   0,  30, 0,  0,   1, 70, 45,  30, 55,   1, 85, 40, 3, 0, 15, 1 },
    { "Chorus large (doubles)", 1, 1,   45, 0,  0, 1,    1, 35,  10, 30, 0,   1, 2,   0,  30, 0,  0,   1, 100,75,  65, 60,   1, 45, 18, 1, 2, 12, 2 },
};
constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));
}

BakrVoxChainProcessor::BakrVoxChainProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BAKR_VOXCHAIN", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout BakrVoxChainProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (juce::roundToInt (v)) + " %"; });
    auto db  = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; });
    auto onOff = [&] (const char* id, const char* name, bool def) { p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };
    auto f = [&] (const char* id, const char* name, float lo, float hi, float def, AudioParameterFloatAttributes a)
    { p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.01f), def, a)); };

    onOff ("comp_on", "Compression ON", true);
    f ("comp_in", "Entree", -12, 12, 0, db);
    f ("comp_amt", "Compression", 0, 100, 35, pct);
    onOff ("comp_slow", "Lent", false);
    f ("comp_out", "Sortie", -12, 12, 0, db);
    onOff ("comp_auto", "Auto gain", true);

    onOff ("enh_on", "Amelioration ON", true);
    f ("deess", "De-Ess", 0, 100, 30, pct);
    f ("denoise", "De-Noise", 0, 100, 10, pct);
    f ("air", "Air", 0, 100, 20, pct);
    onOff ("enh_pre", "Avant compression", false);

    onOff ("eq_on", "EQ ON", true);
    f ("eq_high", "Aigus", -12, 12, 1, db);
    f ("eq_mid", "Mediums", -12, 12, 0, db);
    f ("eq_low", "Coupe-bas", 0, 100, 25, pct);
    onOff ("eq_pre", "EQ avant compression", false);
    onOff ("eq_adlib", "Mode Ad-lib", false);

    onOff ("mul_on", "Doubleur ON", false);
    f ("mul_spread", "Largeur", 0, 100, 60, pct);
    f ("mul_offset", "Decalage", 0, 100, 40, pct);
    f ("mul_amount", "Quantite", 0, 100, 30, pct);
    f ("mul_low", "Graves au centre", 0, 100, 50, pct);

    onOff ("spc_on", "Espace ON", true);
    f ("rev_time", "Taille reverb", 0, 100, 45, pct);
    f ("rev_mix", "Reverb", 0, 100, 15, pct);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "rev_type", 1 }, "Type reverb", reverbTypes(), 1));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "dly_time", 1 }, "Temps delay", delayTimes(), 1));
    f ("dly_mix", "Delay", 0, 100, 10, pct);
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "dly_type", 1 }, "Type delay", delayTypes(), 0));
    return { p.begin(), p.end() };
}

juce::StringArray BakrVoxChainProcessor::getPresetNames() { juce::StringArray s; for (auto& p : presets) s.add (p.name); return s; }
int BakrVoxChainProcessor::getNumPrograms() { return numPresets; }
const juce::String BakrVoxChainProcessor::getProgramName (int i) { return juce::isPositiveAndBelow (i, numPresets) ? presets[i].name : ""; }

void BakrVoxChainProcessor::applyPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, numPresets)) return;
    currentPreset = i;
    const auto& p = presets[i];
    auto s = [this] (const char* id, float v) { if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v)); };
    s ("comp_on", (float) p.compOn); s ("comp_in", p.cin); s ("comp_amt", p.camt); s ("comp_slow", (float) p.slow); s ("comp_out", p.cout); s ("comp_auto", (float) p.autoM);
    s ("enh_on", (float) p.enhOn); s ("deess", p.deess); s ("denoise", p.denoise); s ("air", p.air); s ("enh_pre", (float) p.enhPre);
    s ("eq_on", (float) p.eqOn); s ("eq_high", p.hi); s ("eq_mid", p.mid); s ("eq_low", p.low); s ("eq_pre", (float) p.eqPre); s ("eq_adlib", (float) p.adlib);
    s ("mul_on", (float) p.mulOn); s ("mul_spread", p.spread); s ("mul_offset", p.offset); s ("mul_amount", p.amount); s ("mul_low", p.mlow);
    s ("spc_on", (float) p.spcOn); s ("rev_time", p.revTime); s ("rev_mix", p.revMix); s ("rev_type", (float) p.revType);
    s ("dly_time", (float) p.dTime); s ("dly_mix", p.dMix); s ("dly_type", (float) p.dType);
}

//==============================================================================
void BakrVoxChainProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    for (int c = 0; c < 2; ++c)
    {
        deessLp[c].setCoefs (*Coef::makeLowPass (sr, 5500.0, 0.7071));
        deessHp[c].setCoefs (*Coef::makeHighPass (sr, 5500.0, 0.7071));
        deessDet[c].setCoefs (*Coef::makeBandPass (sr, 7000.0, 1.2));
        airShelf[c].setCoefs (*Coef::makeHighShelf (sr, 10000.0, 0.7071, 1.0f));
        excHp[c].setCoefs (*Coef::makeHighPass (sr, 4000.0, 0.7071));
        adlibLp[c].setCoefs (*Coef::makeLowPass (sr, 7000.0, 0.7071));
        adlibPk[c].setCoefs (*Coef::makePeakFilter (sr, 2500.0, 0.9, juce::Decibels::decibelsToGain (4.0f)));
        revHp[c].setCoefs (*Coef::makeHighPass (sr, 220.0, 0.7071));
        for (auto* b : { &deessLp[c], &deessHp[c], &deessDet[c], &airShelf[c], &excHp[c], &adlibLp[c], &adlibPk[c], &revHp[c],
                         &eqHigh[c], &eqMid[c], &eqLow1[c], &eqLow2[c] })
            b->reset();
        deessEnv[c] = fullEnv[c] = 0;
    }
    for (auto& v : lastEq) v = -999;
    lastDblLow = -1;
    compEnv = 0; noiseEnv = 0; noiseGain = 1;

    int n = 1; while (n < (int) (sr * 0.08)) n <<= 1;
    dblBuf.assign ((size_t) n, 0.0f); dblMask = n - 1; dblPos = 0;
    for (auto& h : dblHp) h.reset();

    n = 1; while (n < (int) (sr * 2.6)) n <<= 1;
    dlL.assign ((size_t) n, 0.0f); dlR.assign ((size_t) n, 0.0f); dlMask = n - 1; dlPos = 0;
    dlFbL = dlFbR = tapeLpL = tapeLpR = 0; smDelay = delaySeconds();

    reverb.setSampleRate (sr); reverb.reset();
    revBuf.setSize (2, samplesPerBlock);
    inG.reset (sr, 0.03); outG.reset (sr, 0.03);
    inG.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (P ("comp_in")->load()));
    outG.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (P ("comp_out")->load()));
    updateFilters();
}

bool BakrVoxChainProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& o = l.getMainOutputChannelSet();
    if (o != juce::AudioChannelSet::stereo()) return false;
    const auto& i = l.getMainInputChannelSet();
    return i == juce::AudioChannelSet::stereo() || i == juce::AudioChannelSet::mono();
}

void BakrVoxChainProcessor::updateFilters()
{
    const float hi = P ("eq_high")->load(), mid = P ("eq_mid")->load(), low = P ("eq_low")->load();
    const float adl = P ("eq_adlib")->load(), air = P ("air")->load(), mlow = P ("mul_low")->load();
    if (hi != lastEq[0] || mid != lastEq[1] || low != lastEq[2] || adl != lastEq[3] || air != lastEq[4])
    {
        // coupe-bas : 20 Hz -> 300 Hz (au moins 250 Hz en mode ad-lib)
        float hpf = 20.0f * std::pow (15.0f, low * 0.01f);
        if (adl > 0.5f) hpf = juce::jmax (hpf, 250.0f);
        for (int c = 0; c < 2; ++c)
        {
            eqHigh[c].setCoefs (*Coef::makeHighShelf (sr, 8000.0, 0.7071, juce::Decibels::decibelsToGain (hi)));
            eqMid[c].setCoefs (*Coef::makePeakFilter (sr, 1500.0, 0.9, juce::Decibels::decibelsToGain (mid)));
            eqLow1[c].setCoefs (*Coef::makeHighPass (sr, hpf, 0.5412));   // Butterworth 4e ordre (24 dB/oct)
            eqLow2[c].setCoefs (*Coef::makeHighPass (sr, hpf, 1.3065));
            airShelf[c].setCoefs (*Coef::makeHighShelf (sr, 10000.0, 0.7071, juce::Decibels::decibelsToGain (air * 0.08f)));
        }
        lastEq[0] = hi; lastEq[1] = mid; lastEq[2] = low; lastEq[3] = adl; lastEq[4] = air;
    }
    if (mlow != lastDblLow)
    {
        float f = 40.0f * std::pow (12.0f, mlow * 0.01f);   // 40 Hz -> 480 Hz
        for (auto& h : dblHp) h.setCoefs (*Coef::makeHighPass (sr, f, 0.7071));
        lastDblLow = mlow;
    }
}

float BakrVoxChainProcessor::delaySeconds() const
{
    double bpm = 100.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = juce::jlimit (40.0, 300.0, *b);
    static const double mult[] = { 2.0, 1.0, 0.75, 0.5, 0.25 };
    int t = juce::jlimit (0, 4, (int) apvts.getRawParameterValue ("dly_time")->load());
    return (float) (60.0 / bpm * mult[t]);
}

inline float BakrVoxChainProcessor::readDelay (const std::vector<float>& b, int wp, float d) const
{
    float rp = (float) wp - d;
    const int size = (int) b.size();
    while (rp < 0) rp += (float) size;
    int i = (int) rp; float fr = rp - (float) i;
    return b[(size_t) (i & (size - 1))] * (1.0f - fr) + b[(size_t) ((i + 1) & (size - 1))] * fr;
}

void BakrVoxChainProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    if (getTotalNumInputChannels() == 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);
    updateFilters();

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    // ---- réglages ----
    const bool compOn = P ("comp_on")->load() > 0.5f, slow = P ("comp_slow")->load() > 0.5f, autoM = P ("comp_auto")->load() > 0.5f;
    const float amt = P ("comp_amt")->load() * 0.01f;
    const float thr = -2.0f - amt * 28.0f, ratio = 1.5f + amt * 6.5f, knee = 6.0f;
    const float atk = std::exp (-1.0f / ((slow ? 0.025f : 0.004f) * (float) sr));
    const float rel = std::exp (-1.0f / ((slow ? 0.30f : 0.08f) * (float) sr));
    const float makeup = autoM ? juce::Decibels::decibelsToGain (-thr * (1.0f - 1.0f / ratio) * 0.3f) : 1.0f;
    inG.setTargetValue (juce::Decibels::decibelsToGain (P ("comp_in")->load()));
    outG.setTargetValue (juce::Decibels::decibelsToGain (P ("comp_out")->load()));

    const bool enhOn = P ("enh_on")->load() > 0.5f, enhPre = P ("enh_pre")->load() > 0.5f;
    const float deess = P ("deess")->load() * 0.01f, denoise = P ("denoise")->load() * 0.01f, air = P ("air")->load() * 0.01f;
    const float deessThr = juce::Decibels::decibelsToGain (-6.0f - deess * 24.0f);
    const float noiseThr = juce::Decibels::decibelsToGain (-72.0f + denoise * 34.0f);
    const float envA = std::exp (-1.0f / (0.002f * (float) sr)), envR = std::exp (-1.0f / (0.06f * (float) sr));

    const bool eqOn = P ("eq_on")->load() > 0.5f, eqPre = P ("eq_pre")->load() > 0.5f, adlib = P ("eq_adlib")->load() > 0.5f;

    float peakIn = 0, maxGR = 0, maxDe = 0;

    auto enhance = [&] (float& l, float& r)
    {
        // de-noise : expandeur vers le bas commun aux deux canaux
        if (denoise > 0.001f)
        {
            float lvl = juce::jmax (std::abs (l), std::abs (r));
            noiseEnv = lvl > noiseEnv ? lvl + envA * (noiseEnv - lvl) : lvl + envR * (noiseEnv - lvl);
            float target = 1.0f;
            if (noiseEnv < noiseThr)
                target = std::pow (juce::jmax (1.0e-6f, noiseEnv / noiseThr), 1.5f + denoise * 2.0f);
            target = juce::jmax (target, 0.03f);
            noiseGain += (target - noiseGain) * (target < noiseGain ? 0.004f : 0.02f);
            l *= noiseGain; r *= noiseGain;
        }
        float* ch[2] = { &l, &r };
        for (int c = 0; c < 2; ++c)
        {
            float x = *ch[c];
            // de-ess : on n'atténue que la bande aiguë quand les « s » dépassent le seuil
            if (deess > 0.001f)
            {
                float det = std::abs (deessDet[c].p (x));
                deessEnv[c] = det > deessEnv[c] ? det + envA * (deessEnv[c] - det) : det + envR * (deessEnv[c] - det);
                float lo = deessLp[c].p (x), hi = deessHp[c].p (x);
                float g = 1.0f;
                if (deessEnv[c] > deessThr) g = juce::jmax (0.25f, std::pow (deessThr / deessEnv[c], 0.75f));
                maxDe = juce::jmax (maxDe, 1.0f - g);
                x = lo + hi * g;
            }
            // air : étagère haute + léger excitateur
            if (air > 0.001f)
            {
                x = airShelf[c].p (x);
                float h = excHp[c].p (x);
                x += std::tanh (h * 3.0f) * air * 0.06f;
            }
            *ch[c] = x;
        }
    };

    auto eq = [&] (float& l, float& r)
    {
        float* ch[2] = { &l, &r };
        for (int c = 0; c < 2; ++c)
        {
            float x = eqLow2[c].p (eqLow1[c].p (*ch[c]));
            x = eqHigh[c].p (eqMid[c].p (x));
            if (adlib) x = adlibPk[c].p (adlibLp[c].p (x));
            *ch[c] = x;
        }
    };

    auto compress = [&] (float& l, float& r)
    {
        float lvl = juce::jmax (std::abs (l), std::abs (r));
        compEnv = lvl > compEnv ? lvl + atk * (compEnv - lvl) : lvl + rel * (compEnv - lvl);
        float dbIn = juce::Decibels::gainToDecibels (compEnv, -100.0f);
        float over = dbIn - thr, gr = 0.0f;
        if (over > knee * 0.5f) gr = over * (1.0f - 1.0f / ratio);
        else if (over > -knee * 0.5f) { float t = over + knee * 0.5f; gr = (1.0f - 1.0f / ratio) * t * t / (2.0f * knee); }
        maxGR = juce::jmax (maxGR, gr);
        float g = juce::Decibels::decibelsToGain (-gr) * makeup;
        l *= g; r *= g;
    };

    for (int i = 0; i < n; ++i)
    {
        float gi = inG.getNextValue(), go = outG.getNextValue();
        float l = L[i] * gi, r = R[i] * gi;
        peakIn = juce::jmax (peakIn, std::abs (l), std::abs (r));
        if (enhOn && enhPre) enhance (l, r);
        if (eqOn && eqPre) eq (l, r);
        if (compOn) compress (l, r);
        if (enhOn && ! enhPre) enhance (l, r);
        if (eqOn && ! eqPre) eq (l, r);
        L[i] = l * go; R[i] = r * go;
    }

    // ---- doubleur ----
    if (P ("mul_on")->load() > 0.5f)
    {
        const float spread = P ("mul_spread")->load() * 0.01f, off = P ("mul_offset")->load() * 0.01f, amount = P ("mul_amount")->load() * 0.01f;
        static const float base[4] = { 0.011f, 0.017f, 0.023f, 0.029f };
        static const float rate[4] = { 0.31f, 0.43f, 0.57f, 0.69f };
        static const float pan[4]  = { -1.0f, 0.45f, -0.45f, 1.0f };
        const float depth = (0.0004f + off * 0.0026f) * (float) sr;     // variation de hauteur (désaccord)
        for (int i = 0; i < n; ++i)
        {
            float mono = 0.5f * (L[i] + R[i]);
            dblBuf[(size_t) dblPos] = mono;
            float dl = 0, dr = 0;
            for (int v = 0; v < 4; ++v)
            {
                lfoPh[(size_t) v] += rate[v] / (float) sr;
                if (lfoPh[(size_t) v] >= 1.0f) lfoPh[(size_t) v] -= 1.0f;
                float d = base[v] * (0.6f + off * 0.9f) * (float) sr + depth * (0.5f + 0.5f * std::sin (6.2831853f * lfoPh[(size_t) v]));
                float s = dblHp[(size_t) v].p (readDelay (dblBuf, dblPos, d));
                float p = pan[v] * spread;
                dl += s * (p > 0 ? 1.0f - p : 1.0f);
                dr += s * (p < 0 ? 1.0f + p : 1.0f);
            }
            dblPos = (dblPos + 1) & dblMask;
            L[i] += dl * amount * 0.45f;
            R[i] += dr * amount * 0.45f;
        }
    }

    // ---- espace : delay puis reverb (en parallèle sur la voix) ----
    if (P ("spc_on")->load() > 0.5f)
    {
        const float dMix = P ("dly_mix")->load() * 0.01f;
        const int dType = (int) P ("dly_type")->load();
        float target = dType == 3 ? 0.09f : delaySeconds();
        const float fb = dType == 3 ? 0.12f : dType == 1 ? 0.45f : 0.38f;
        const float rMix = P ("rev_mix")->load() * 0.01f;
        revBuf.setSize (2, n, false, false, true);
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            float wl = 0, wr = 0;
            if (dMix > 0.001f)
            {
                smDelay += (target - smDelay) * 0.0005f;
                float d = juce::jlimit (1.0f, (float) dlMask - 4.0f, smDelay * (float) sr);
                if (dType == 1) { wowPh += 0.6f / (float) sr; if (wowPh > 1) wowPh -= 1; d += std::sin (6.2831853f * wowPh) * 0.0015f * (float) sr; }
                float yl = readDelay (dlL, dlPos, d), yr = readDelay (dlR, dlPos, d);
                if (dType == 1) { tapeLpL += 0.25f * (yl - tapeLpL); tapeLpR += 0.25f * (yr - tapeLpR); yl = std::tanh (tapeLpL * 1.2f); yr = std::tanh (tapeLpR * 1.2f); }
                float inM = 0.5f * (l + r);
                if (dType == 2) { dlL[(size_t) dlPos] = inM + yr * fb; dlR[(size_t) dlPos] = yl * fb; }   // ping-pong
                else            { dlL[(size_t) dlPos] = l + yl * fb;   dlR[(size_t) dlPos] = r + yr * fb; }
                dlPos = (dlPos + 1) & dlMask;
                wl = yl * dMix * 0.8f; wr = yr * dMix * 0.8f;
            }
            // envoi vers la reverb (graves coupés pour garder la voix claire)
            revBuf.setSample (0, i, revHp[0].p (l + wl * 0.5f));
            revBuf.setSample (1, i, revHp[1].p (r + wr * 0.5f));
            L[i] = l + wl; R[i] = r + wr;
        }
        if (rMix > 0.001f)
        {
            static const float sizeLo[5] = { 0.25f, 0.45f, 0.65f, 0.82f, 0.10f };
            static const float sizeHi[5] = { 0.55f, 0.75f, 0.88f, 0.99f, 0.35f };
            static const float damp[5]   = { 0.55f, 0.20f, 0.40f, 0.30f, 0.65f };
            int t = juce::jlimit (0, 4, (int) P ("rev_type")->load());
            float tm = P ("rev_time")->load() * 0.01f;
            juce::Reverb::Parameters rp;
            rp.roomSize = sizeLo[t] + (sizeHi[t] - sizeLo[t]) * tm;
            rp.damping = damp[t]; rp.width = 1.0f; rp.wetLevel = 1.0f; rp.dryLevel = 0.0f;
            reverb.setParameters (rp);
            reverb.processStereo (revBuf.getWritePointer (0), revBuf.getWritePointer (1), n);
            const float wet = rMix * 0.35f;
            for (int i = 0; i < n; ++i) { L[i] += revBuf.getSample (0, i) * wet; R[i] += revBuf.getSample (1, i) * wet; }
        }
    }

    // écrêteur doux de sécurité (n'agit qu'au-dessus de -1 dBFS)
    auto safety = [] (float x) { float a = std::abs (x); return a < 0.89f ? x : std::copysign (0.89f + 0.1f * std::tanh ((a - 0.89f) / 0.1f), x); };
    float peakOut = 0;
    for (int i = 0; i < n; ++i) { L[i] = safety (L[i]); R[i] = safety (R[i]); peakOut = juce::jmax (peakOut, std::abs (L[i]), std::abs (R[i])); }
    auto fall = [] (std::atomic<float>& m, float v, float k) { m = juce::jmax (v, m.load() * k); };
    fall (inLevel, peakIn, 0.85f);
    fall (outLevel, peakOut, 0.85f);
    fall (gainReduction, maxGR, 0.9f);
    fall (deEssGR, maxDe, 0.85f);
    deNoiseGR = enhOn && denoise > 0.001f ? 1.0f - noiseGain : 0.0f;
}

void BakrVoxChainProcessor::getStateInformation (juce::MemoryBlock& d)
{
    auto st = apvts.copyState(); st.setProperty ("preset", currentPreset.load(), nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, d);
}

void BakrVoxChainProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPreset = (int) vt.getProperty ("preset", 0);
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* BakrVoxChainProcessor::createEditor() { return new BakrVoxChainEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BakrVoxChainProcessor(); }
