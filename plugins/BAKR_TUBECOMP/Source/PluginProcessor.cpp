#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr float kThresholdDb = -24.0f;  // seuil fixe : c'est INPUT qui pousse dans le compresseur
    constexpr float kKneeDb      = 12.0f;

    juce::NormalisableRange<float> freqRange (float lo, float hi)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (std::sqrt (lo * hi));
        return r;
    }

    struct Preset
    {
        const char* name;
        float input, output, mix, heat;
        int mode, recovery, detect;
        bool eqOn; float hp, pkF, pkG, lp;
    };

    // hp <= 15 Hz = OFF, lp >= 20 kHz = OFF
    const Preset kPresets[] =
    {
        { "Init",                          6.0f,  0.0f, 100.0f, 50.0f, 0, 0, 1, false,  10.0f, 1000.0f,  0.0f, 21000.0f },
        { "Voix Afrobeats (Nigeria)",     10.0f, -3.0f, 100.0f, 55.0f, 1, 0, 1, true,  120.0f, 3500.0f, -2.0f, 21000.0f },
        { "Voix Mandingue chaude",         8.0f, -2.0f, 100.0f, 70.0f, 0, 1, 1, true,   90.0f, 1000.0f,  0.0f, 21000.0f },
        { "Bus Batterie Afro (parallele)",16.0f, -4.0f,  45.0f, 62.0f, 0, 0, 1, true,   60.0f, 1000.0f,  0.0f, 21000.0f },
        { "Basse Reggae ronde",            9.0f, -2.0f, 100.0f, 65.0f, 1, 1, 1, true,   10.0f,  120.0f,  2.0f,  4000.0f },
        { "Master Glue",                   4.0f, -1.0f, 100.0f, 40.0f, 2, 1, 1, true,   90.0f, 1000.0f,  0.0f, 21000.0f },
        { "Balafon / Kora douce",          6.0f, -1.0f, 100.0f, 50.0f, 1, 0, 0, true,  150.0f, 3000.0f, -3.0f, 21000.0f },
        { "Guitare Highlife",              7.0f, -1.5f, 100.0f, 55.0f, 0, 0, 1, true,  100.0f, 1000.0f,  0.0f, 21000.0f },
        { "Choeurs Amapiano",             12.0f, -3.0f,  70.0f, 60.0f, 2, 0, 1, true,  150.0f, 1000.0f,  0.0f, 21000.0f },
        { "Largeur stereo (Side only)",    8.0f,  0.0f, 100.0f, 45.0f, 1, 1, 4, true,  200.0f, 1000.0f,  0.0f, 21000.0f },
        { "Centre solide (Mid only)",      8.0f, -1.0f, 100.0f, 50.0f, 1, 0, 3, false,  10.0f, 1000.0f,  0.0f, 21000.0f },
        { "Voix Opto douce",              10.0f, -2.0f, 100.0f, 55.0f, 3, 0, 1, true,  100.0f, 1000.0f,  0.0f, 21000.0f },
        { "Basse Opto (reggae / afro)",   11.0f, -2.5f, 100.0f, 60.0f, 3, 1, 1, false,  10.0f, 1000.0f,  0.0f, 21000.0f },
        { "Bus voix Opto (choeurs)",       8.0f, -1.5f, 100.0f, 50.0f, 3, 1, 1, true,  150.0f, 1000.0f,  0.0f, 21000.0f },
    };
    constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));
}

//==============================================================================
void BakrTubeCompAudioProcessor::Biquad::setHighPass (double sr, double f, double q)
{
    const double w = juce::MathConstants<double>::twoPi * f / sr;
    const double c = std::cos (w), a = std::sin (w) / (2.0 * q);
    const double a0 = 1.0 + a;
    b0 = (float) (((1.0 + c) * 0.5) / a0); b1 = (float) (-(1.0 + c) / a0); b2 = b0;
    a1 = (float) (-2.0 * c / a0); a2 = (float) ((1.0 - a) / a0);
}

void BakrTubeCompAudioProcessor::Biquad::setLowPass (double sr, double f, double q)
{
    const double w = juce::MathConstants<double>::twoPi * f / sr;
    const double c = std::cos (w), a = std::sin (w) / (2.0 * q);
    const double a0 = 1.0 + a;
    b0 = (float) (((1.0 - c) * 0.5) / a0); b1 = (float) ((1.0 - c) / a0); b2 = b0;
    a1 = (float) (-2.0 * c / a0); a2 = (float) ((1.0 - a) / a0);
}

void BakrTubeCompAudioProcessor::Biquad::setPeak (double sr, double f, double q, double g)
{
    const double A = std::pow (10.0, g / 40.0);
    const double w = juce::MathConstants<double>::twoPi * f / sr;
    const double c = std::cos (w), a = std::sin (w) / (2.0 * q);
    const double a0 = 1.0 + a / A;
    b0 = (float) ((1.0 + a * A) / a0); b1 = (float) (-2.0 * c / a0); b2 = (float) ((1.0 - a * A) / a0);
    a1 = b1; a2 = (float) ((1.0 - a / A) / a0);
}

void BakrTubeCompAudioProcessor::Biquad::setLowShelf (double sr, double f, double g)
{
    const double A = std::pow (10.0, g / 40.0);
    const double w = juce::MathConstants<double>::twoPi * f / sr;
    const double c = std::cos (w), s = std::sin (w);
    const double alpha = s / 2.0 * std::sqrt (2.0);
    const double sq = 2.0 * std::sqrt (A) * alpha;
    const double a0 = (A + 1) + (A - 1) * c + sq;
    b0 = (float) (A * ((A + 1) - (A - 1) * c + sq) / a0);
    b1 = (float) (2 * A * ((A - 1) - (A + 1) * c) / a0);
    b2 = (float) (A * ((A + 1) - (A - 1) * c - sq) / a0);
    a1 = (float) (-2 * ((A - 1) + (A + 1) * c) / a0);
    a2 = (float) (((A + 1) + (A - 1) * c - sq) / a0);
}

//==============================================================================
BakrTubeCompAudioProcessor::BakrTubeCompAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "BAKR_TUBECOMP", createLayout())
{
}

juce::StringArray BakrTubeCompAudioProcessor::presetNames()
{
    juce::StringArray s;
    for (auto& p : kPresets) s.add (p.name);
    return s;
}

juce::AudioProcessorValueTreeState::ParameterLayout BakrTubeCompAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto db = [] (float v, int) { return String (v, 1) + " dB"; };
    auto hz = [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String ((int) v) + " Hz"; };
    auto pc = [] (float v, int) { return String ((int) v) + " %"; };

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "input", 1 }, "Input",
        NormalisableRange<float> (-12.0f, 30.0f, 0.1f), 6.0f, AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
        NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode",
        StringArray { "Single", "Double", "Triple", "Opto" }, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "recovery", 1 }, "Recovery Time",
        StringArray { "Fast", "Slow" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, AudioParameterFloatAttributes().withStringFromValueFunction (pc)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "heat", 1 }, "Chaleur",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, AudioParameterFloatAttributes().withStringFromValueFunction (pc)));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "meter", 1 }, "Meter",
        StringArray { "Input", "GR", "Output" }, 1));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "power", 1 }, "Power", true));

    // Side-chain avancé
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "scsource", 1 }, "SC Source",
        StringArray { "Internal", "External" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "scgain", 1 }, "SC Gain",
        NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "detect", 1 }, "Detection Mode",
        StringArray { "Dual", "Linked", "M/S", "Mid Only", "Side Only", "Reversed" }, 1));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "eqon", 1 }, "SC EQ On", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "schp", 1 }, "SC High Pass",
        freqRange (10.0f, 1200.0f), 10.0f, AudioParameterFloatAttributes().withStringFromValueFunction (
            [hz] (float v, int n) { return v <= 15.0f ? String ("OFF") : hz (v, n); })));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "scpkf", 1 }, "SC Band Freq",
        freqRange (30.0f, 16000.0f), 1000.0f, AudioParameterFloatAttributes().withStringFromValueFunction (hz)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "scpkg", 1 }, "SC Band Gain",
        NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    {
        NormalisableRange<float> qr (0.1f, 10.0f); qr.setSkewForCentre (1.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "scq", 1 }, "SC Band Q", qr, 1.0f,
            AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 2); })));
    }
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "sclp", 1 }, "SC Low Pass",
        freqRange (1000.0f, 21000.0f), 21000.0f, AudioParameterFloatAttributes().withStringFromValueFunction (
            [hz] (float v, int n) { return v >= 20000.0f ? String ("OFF") : hz (v, n); })));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "listen", 1 }, "SC Listen", false));

    return { p.begin(), p.end() };
}

bool BakrTubeCompAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != out)
        return false;
    const auto sc = layouts.getChannelSet (true, 1);
    return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
}

//==============================================================================
void BakrTubeCompAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    const int nCh = juce::jmax (1, getMainBusNumOutputChannels());

    oversampler.numChannels = (size_t) nCh;
    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler.reset();

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) nCh };
    dryWet.prepare (spec);
    dryWet.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    dryWet.setWetLatency (oversampler.getLatencyInSamples());
    setLatencySamples ((int) std::round (oversampler.getLatencyInSamples()));

    scBuffer.setSize (2, samplesPerBlock);
    grBuffer.setSize (2, samplesPerBlock);
    listenBuffer.setSize (2, samplesPerBlock);

    const double osFs = sampleRate * 2.0;
    for (auto& c : ch)
    {
        c = ChannelState();
        c.weight.setLowShelf (osFs, 90.0, 0.8); // petite bosse de grave du transfo
    }

    inGainSm.reset (sampleRate, 0.03);
    outGainSm.reset (sampleRate, 0.03);
    scGainSm.reset (sampleRate, 0.03);
    inGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("input")->load()));
    outGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load()));
    scGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("scgain")->load()));

    lastHp = lastPkF = lastLp = lastQ = -1.0f; lastPkG = -99.0f;
    updateFilters();
}

void BakrTubeCompAudioProcessor::updateFilters()
{
    const float hp  = apvts.getRawParameterValue ("schp")->load();
    const float pkF = apvts.getRawParameterValue ("scpkf")->load();
    const float pkG = apvts.getRawParameterValue ("scpkg")->load();
    const float lp  = apvts.getRawParameterValue ("sclp")->load();
    const float q   = apvts.getRawParameterValue ("scq")->load();

    if (hp != lastHp || pkF != lastPkF || pkG != lastPkG || lp != lastLp || q != lastQ)
    {
        const double nyq = fs * 0.45;
        for (auto& c : ch)
        {
            if (hp <= 15.0f) c.scHp.setIdentity(); else c.scHp.setHighPass (fs, juce::jmin ((double) hp, nyq), 0.707);
            if (std::abs (pkG) < 0.05f) c.scPeak.setIdentity(); else c.scPeak.setPeak (fs, juce::jmin ((double) pkF, nyq), (double) q, pkG);
            if (lp >= 20000.0f || lp >= nyq) c.scLp.setIdentity(); else c.scLp.setLowPass (fs, lp, 0.707);
        }
        lastHp = hp; lastPkF = pkF; lastPkG = pkG; lastLp = lp; lastQ = q;
    }
}

// Courbe variable-mu : genou très doux, puis ratio qui monte avec le niveau (≈2:1 → ≈12:1)
float BakrTubeCompAudioProcessor::computeStaticGr (float levelDb) noexcept
{
    constexpr float r0 = 2.0f, rMax = 12.0f, tau = 18.0f;
    const float x = levelDb - kThresholdDb + kKneeDb * 0.5f;
    if (x <= 0.0f) return 0.0f;
    const float s0 = 1.0f - 1.0f / r0;
    if (x < kKneeDb) return s0 * x * x / (2.0f * kKneeDb);
    const float u = x - kKneeDb;
    return s0 * (x - kKneeDb * 0.5f) + (1.0f / r0 - 1.0f / rMax) * (u - tau * (1.0f - std::exp (-u / tau)));
}

// Opto : cellule lumineuse, ratio ≈ 3:1, genou très large (18 dB)
static float computeOptoGr (float levelDb) noexcept
{
    constexpr float knee = 18.0f, slope = 1.0f - 1.0f / 3.0f;
    const float x = levelDb - kThresholdDb + knee * 0.5f;
    if (x <= 0.0f) return 0.0f;
    if (x < knee) return slope * x * x / (2.0f * knee);
    return slope * (x - knee * 0.5f);
}

// Triode : courbe asymétrique (harmoniques paires), gain petit-signal normalisé à 1
inline float BakrTubeCompAudioProcessor::triode (float x, float drive) const noexcept
{
    const float a = 1.0f + 3.0f * drive;
    constexpr float bias = 0.22f;
    const float tb = std::tanh (a * bias);
    const float norm = a * (1.0f - tb * tb);
    // côté positif plus doux (grille), côté négatif qui s'écrase plus tard (cut-off)
    const float y = (std::tanh (a * (x + bias)) - tb) / norm;
    return y;
}

// Transfo de sortie : saturation surtout dans les graves + aigus légèrement arrondis
inline float BakrTubeCompAudioProcessor::transformer (ChannelState& st, float x, float amount) noexcept
{
    // séparation graves (≈150 Hz à 2×fs)
    const float aLf = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 150.0f / (float) (fs * 2.0));
    st.xfmrLf += aLf * (x - st.xfmrLf);
    const float lf = st.xfmrLf;
    const float g = 1.0f + 2.5f * amount;
    const float lfSat = std::tanh (g * lf) / g;
    float y = (x - lf) + lfSat;

    // très légère compression douce globale (fer)
    const float k = 0.08f * amount;
    y = y - k * y * y * y / (1.0f + std::abs (y));

    // aigus arrondis : coupure ≈ 26 kHz → 17 kHz selon la chaleur
    const float fc = 26000.0f - 9000.0f * amount;
    const float aHf = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) (fs * 2.0));
    st.hfState += aHf * (y - st.hfState);
    return st.weight.process (st.hfState);
}

//==============================================================================
void BakrTubeCompAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto mainIn = getBusBuffer (buffer, true, 0);
    const int numCh = juce::jmin (2, mainIn.getNumChannels());
    const int n = buffer.getNumSamples();
    if (numCh == 0 || n == 0) return;

    if (scBuffer.getNumSamples() < n)
    {
        scBuffer.setSize (2, n, false, false, true);
        grBuffer.setSize (2, n, false, false, true);
        listenBuffer.setSize (2, n, false, false, true);
    }

    // --- paramètres
    const float inDb    = apvts.getRawParameterValue ("input")->load();
    const float outDb   = apvts.getRawParameterValue ("output")->load();
    const int   mode    = (int) apvts.getRawParameterValue ("mode")->load();
    const bool  slow    = apvts.getRawParameterValue ("recovery")->load() > 0.5f;
    const float mix     = apvts.getRawParameterValue ("mix")->load() * 0.01f;
    const float heat    = apvts.getRawParameterValue ("heat")->load() * 0.01f;
    const bool  power   = apvts.getRawParameterValue ("power")->load() > 0.5f;
    const bool  extSc   = apvts.getRawParameterValue ("scsource")->load() > 0.5f;
    const float scDb    = apvts.getRawParameterValue ("scgain")->load();
    int         detect  = (int) apvts.getRawParameterValue ("detect")->load();
    const bool  eqOn    = apvts.getRawParameterValue ("eqon")->load() > 0.5f;
    const bool  listen  = apvts.getRawParameterValue ("listen")->load() > 0.5f;

    if (numCh < 2) detect = 0;
    const bool msMode = (detect == 2 || detect == 3 || detect == 4);

    updateFilters();
    inGainSm.setTargetValue (juce::Decibels::decibelsToGain (inDb));
    outGainSm.setTargetValue (juce::Decibels::decibelsToGain (outDb));
    scGainSm.setTargetValue (juce::Decibels::decibelsToGain (scDb));

    // --- dry pour le Mix (et le Power = bypass doux)
    juce::dsp::AudioBlock<float> mainBlock (mainIn.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);
    dryWet.setWetMixProportion (power ? mix : 0.0f);
    dryWet.pushDrySamples (mainBlock);

    // --- constantes de temps (Single / Double / Triple + Recovery)
    auto coef = [this] (float seconds) { return 1.0f - std::exp (-1.0f / (seconds * (float) fs)); };
    const float rs = slow ? 3.0f : 1.0f;
    const float atk1 = coef (0.004f),  rel1 = coef (0.25f * rs);
    const float atk2 = coef (0.060f),  rel2 = coef (1.2f * rs);
    const float atk3 = coef (0.400f),  rel3 = coef (4.0f * rs);
    const float rmsC = coef (0.006f);
    const float atkO = coef (0.010f), relO = coef (0.06f * rs);
    const float chUp = coef (3.0f), chDown = coef (10.0f);

    auto scBus = getBusBuffer (buffer, true, 1);
    const bool hasExt = extSc && scBus.getNumChannels() > 0;

    float inAcc = 0.0f, maxGr = 0.0f;

    // --- 1) gain d'entrée + détection à fs
    for (int i = 0; i < n; ++i)
    {
        const float gIn = inGainSm.getNextValue();
        const float gSc = scGainSm.getNextValue();

        float m[2], s[2];
        for (int c = 0; c < numCh; ++c)
        {
            float* d = mainIn.getWritePointer (c);
            d[i] *= gIn;
            m[c] = d[i];
            inAcc = juce::jmax (inAcc, std::abs (d[i]));

            float sc;
            if (hasExt)
                sc = scBus.getReadPointer (juce::jmin (c, scBus.getNumChannels() - 1))[i] * gIn;
            else
                sc = d[i];
            s[c] = sc * gSc;
        }

        if (msMode)
        {
            const float mm = (m[0] + m[1]) * 0.5f, ms = (m[0] - m[1]) * 0.5f;
            m[0] = mm; m[1] = ms;
            mainIn.getWritePointer (0)[i] = mm;
            mainIn.getWritePointer (1)[i] = ms;
            const float sm = (s[0] + s[1]) * 0.5f, ss = (s[0] - s[1]) * 0.5f;
            s[0] = sm; s[1] = ss;
        }

        float lvlDb[2] = { -120.0f, -120.0f };
        for (int c = 0; c < numCh; ++c)
        {
            float x = s[c];
            if (eqOn)
                x = ch[(size_t) c].scLp.process (ch[(size_t) c].scPeak.process (ch[(size_t) c].scHp.process (x)));
            listenBuffer.getWritePointer (c)[i] = x;

            auto& st = ch[(size_t) c];
            st.rms += rmsC * (x * x - st.rms);
            // mélange crête/RMS : réagit comme une cellule à lampe
            const float pk = std::abs (x);
            const float lvl = juce::jmax (std::sqrt (st.rms) * 1.41f, pk * 0.7f);
            lvlDb[c] = juce::Decibels::gainToDecibels (lvl, -120.0f);
        }

        // routage de détection
        float useDb[2] = { lvlDb[0], lvlDb[1] };
        if (numCh == 2)
        {
            switch (detect)
            {
                case 1: useDb[0] = useDb[1] = juce::jmax (lvlDb[0], lvlDb[1]); break; // Linked
                case 5: useDb[0] = lvlDb[1]; useDb[1] = lvlDb[0]; break;               // Reversed
                default: break;                                                         // Dual, M/S, Mid, Side
            }
        }

        for (int c = 0; c < numCh; ++c)
        {
            auto& st = ch[(size_t) c];
            float gr;
            if (mode == 3)
            {
                // OPTO : attaque ≈ 10 ms, relâchement en 2 temps (rapide puis lent)
                // + effet mémoire : plus la cellule a compressé longtemps, plus elle relâche lentement
                const float target = computeOptoGr (useDb[c]);
                const float charging = target > 1.0f ? 1.0f : 0.0f;
                st.optoCharge += (charging > st.optoCharge ? chUp : chDown) * (charging - st.optoCharge);
                const float relSlow = coef ((0.5f + 4.5f * st.optoCharge) * rs);
                st.env1 += (target > st.env1 ? atkO : relO) * (target - st.env1);
                st.env3 += (target > st.env3 ? atkO : relSlow) * (target - st.env3);
                gr = 0.5f * st.env1 + 0.5f * st.env3;
            }
            else
            {
            const float target = computeStaticGr (useDb[c]);

            st.env1 += (target > st.env1 ? atk1 : rel1) * (target - st.env1);
            gr = st.env1;
            if (mode == 1 || mode == 2)
            {
                st.env2 += (target > st.env2 ? atk2 : rel2) * (target - st.env2);
                gr = juce::jmax (gr, st.env2);
            }
            if (mode == 2)
            {
                st.env3 += (target > st.env3 ? atk3 : rel3) * (target - st.env3);
                gr = juce::jmax (gr, st.env3);
            }
            }

            bool active = true;
            if (detect == 3 && c == 1) active = false; // Mid only
            if (detect == 4 && c == 0) active = false; // Side only
            if (! active) gr = 0.0f;

            maxGr = juce::jmax (maxGr, gr);
            grBuffer.getWritePointer (c)[i] = juce::Decibels::decibelsToGain (-gr);
        }
    }

    // --- 2) cellule variable-mu + lampe + transfo en suréchantillonnage ×2
    {
        auto osBlock = oversampler.processSamplesUp (mainBlock);
        const int osN = (int) osBlock.getNumSamples();
        const float drive = 0.25f + 0.6f * heat;

        for (int c = 0; c < numCh; ++c)
        {
            float* d = osBlock.getChannelPointer ((size_t) c);
            const float* g = grBuffer.getReadPointer (c);
            auto& st = ch[(size_t) c];
            const float aDc = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 6.0f / (float) (fs * 2.0));

            for (int i = 0; i < osN; ++i)
            {
                const int bi = juce::jmin (i >> 1, n - 1);
                const float gPrev = bi > 0 ? g[bi - 1] : g[bi];
                const float gain = (i & 1) ? g[bi] : 0.5f * (gPrev + g[bi]);

                // plus la lampe est "tirée" (forte réduction), plus elle colore
                const float grDrive = drive + 0.25f * (1.0f - gain);
                float x = triode (d[i] * gain, grDrive);
                // retrait du DC créé par l'asymétrie
                st.dcIn += aDc * (x - st.dcIn);
                x -= st.dcIn;
                d[i] = transformer (st, x, heat);
            }
        }
        oversampler.processSamplesDown (mainBlock);
    }

    // --- 3) retour L/R, sortie, Listen
    float outAcc = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float gOut = outGainSm.getNextValue();
        if (msMode)
        {
            float* l = mainIn.getWritePointer (0);
            float* r = mainIn.getWritePointer (1);
            const float mm = l[i], ss = r[i];
            l[i] = mm + ss; r[i] = mm - ss;
        }
        for (int c = 0; c < numCh; ++c)
        {
            float* d = mainIn.getWritePointer (c);
            d[i] *= gOut;
        }
    }

    if (listen)
    {
        for (int c = 0; c < numCh; ++c)
        {
            if (msMode && numCh == 2)
            {
                const float* m = listenBuffer.getReadPointer (0);
                const float* s = listenBuffer.getReadPointer (1);
                float* d = mainIn.getWritePointer (c);
                for (int i = 0; i < n; ++i) d[i] = c == 0 ? m[i] + s[i] : m[i] - s[i];
            }
            else
                mainIn.copyFrom (c, 0, listenBuffer, c, 0, n);
        }
        dryWet.setWetMixProportion (1.0f);
    }

    dryWet.mixWetSamples (mainBlock);

    for (int c = 0; c < numCh; ++c)
        outAcc = juce::jmax (outAcc, mainIn.getMagnitude (c, 0, n));

    // copie mono → canaux supplémentaires éventuels
    for (int c = numCh; c < mainIn.getNumChannels(); ++c)
        mainIn.copyFrom (c, 0, mainIn, 0, 0, n);

    meterInDb.store  (juce::Decibels::gainToDecibels (inAcc, -60.0f));
    meterOutDb.store (juce::Decibels::gainToDecibels (outAcc, -60.0f));
    meterGrDb.store  (power ? maxGr : 0.0f);
}

//==============================================================================
int BakrTubeCompAudioProcessor::getNumPrograms() { return kNumPresets; }

const juce::String BakrTubeCompAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, kNumPresets) ? juce::String (kPresets[index].name) : juce::String();
}

void BakrTubeCompAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, kNumPresets)) return;
    currentPreset = index;
    applyPreset (index);
}

void BakrTubeCompAudioProcessor::applyPreset (int index)
{
    const auto& p = kPresets[index];
    auto set = [this] (const char* id, float v)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };
    set ("input", p.input);   set ("output", p.output);
    set ("mix", p.mix);       set ("heat", p.heat);
    set ("mode", (float) p.mode); set ("recovery", (float) p.recovery);
    set ("detect", (float) p.detect);
    set ("eqon", p.eqOn ? 1.0f : 0.0f);
    set ("schp", p.hp); set ("scpkf", p.pkF); set ("scpkg", p.pkG); set ("sclp", p.lp);
    set ("scsource", 0.0f); set ("scgain", 0.0f); set ("listen", 0.0f); set ("power", 1.0f);
}

void BakrTubeCompAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BakrTubeCompAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPreset = (int) vt.getProperty ("preset", 0);
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* BakrTubeCompAudioProcessor::createEditor()
{
    return new BakrTubeCompAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BakrTubeCompAudioProcessor();
}
