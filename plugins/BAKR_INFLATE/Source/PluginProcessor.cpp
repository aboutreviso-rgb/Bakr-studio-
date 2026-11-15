#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
struct PresetDef { const char* name; float in, effect, curve, out; int clip, band; };
const PresetDef presets[] =
{
    { "Defaut",                    0.0f, 100.0f,   0.0f,  0.0f, 1, 0 },
    { "Master doux",               2.0f,  50.0f,   0.0f, -1.5f, 1, 0 },
    { "Master fort",               6.0f,  60.0f,  50.0f, -6.0f, 1, 0 },
    { "Master 3 bandes",           4.0f,  70.0f,  20.0f, -4.0f, 1, 1 },
    { "Voix presente",             3.0f,  45.0f,  30.0f, -3.0f, 1, 0 },
    { "Batterie punchy",           5.0f,  80.0f, -20.0f, -4.5f, 1, 1 },
    { "Basse 808 pleine",          4.0f,  65.0f,  40.0f, -4.0f, 1, 0 },
    { "Beat Afro chaud",           3.0f,  70.0f,  10.0f, -3.0f, 1, 1 },
    { "Bus de mix (colle)",        1.5f,  40.0f, -10.0f, -1.5f, 1, 0 },
    { "Saturation douce (sans clip)", 0.0f, 100.0f, -50.0f, -1.0f, 0, 0 },
};
constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));
}

BakrInflateProcessor::BakrInflateProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BAKR_INFLATE", createLayout())
{
    pIn = apvts.getRawParameterValue ("input");
    pEffect = apvts.getRawParameterValue ("effect");
    pCurve = apvts.getRawParameterValue ("curve");
    pOut = apvts.getRawParameterValue ("output");
    pClip = apvts.getRawParameterValue ("clip");
    pBand = apvts.getRawParameterValue ("bandSplit");
    pEffectIn = apvts.getRawParameterValue ("effectIn");
    slotA = apvts.copyState(); slotB = apvts.copyState();
}

juce::AudioProcessorValueTreeState::ParameterLayout BakrInflateProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto db = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; });
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "input", 1 }, "Entree",
                   NormalisableRange<float> (-6.0f, 12.0f, 0.01f), 0.0f, db));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "effect", 1 }, "Effet",
                   NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                   AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " %"; })));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "curve", 1 }, "Courbe",
                   NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f,
                   AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1); })));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Sortie",
                   NormalisableRange<float> (-12.0f, 0.0f, 0.01f), 0.0f, db));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "clip", 1 }, "Clip 0 dB", true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "bandSplit", 1 }, "3 bandes", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "effectIn", 1 }, "Effet actif", true));
    return { p.begin(), p.end() };
}

juce::StringArray BakrInflateProcessor::getPresetNames()
{
    juce::StringArray s;
    for (auto& p : presets) s.add (p.name);
    return s;
}
int BakrInflateProcessor::getNumPrograms() { return numPresets; }
const juce::String BakrInflateProcessor::getProgramName (int i) { return juce::isPositiveAndBelow (i, numPresets) ? presets[i].name : ""; }

void BakrInflateProcessor::applyPreset (int i)
{
    if (! juce::isPositiveAndBelow (i, numPresets)) return;
    currentPreset = i;
    auto set = [this] (const char* id, float v) { if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v)); };
    const auto& p = presets[i];
    set ("input", p.in); set ("effect", p.effect); set ("curve", p.curve); set ("output", p.out);
    set ("clip", (float) p.clip); set ("bandSplit", (float) p.band); set ("effectIn", 1.0f);
}

void BakrInflateProcessor::switchAB (bool toB)
{
    if (toB == slotBActive) return;
    // on mémorise l'état courant dans le slot actif, puis on charge l'autre
    (slotBActive ? slotB : slotA) = apvts.copyState();
    slotBActive = toB;
    auto& target = toB ? slotB : slotA;
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
        {
            auto child = target.getChildWithProperty ("id", rp->paramID);
            if (child.isValid())
                rp->setValueNotifyingHost (rp->convertTo0to1 ((float) child.getProperty ("value")));
        }
}

//==============================================================================
// Courbe d'inflation : polynôme qui gonfle les niveaux moyens et reste à 1 en 0 dBFS
// (pente nulle en 1 -> transition douce vers la limite).
float BakrInflateProcessor::shape (float x, float curve, bool clip)
{
    const float c = curve * 0.01f;                 // -0.5 .. +0.5
    const float A = 1.5f + c, B = -2.0f * c, C = c - 0.5f, D = 0.0625f - 0.25f * c + 0.25f * c * c;
    float a = std::abs (x);
    if (clip) a = juce::jmin (a, 1.0f);
    float y;
    if (a < 1.0f)
    {
        const float a2 = a * a, a3 = a2 * a;
        y = A * a + B * a2 + C * a3 - D * (a2 - 2.0f * a3 + a2 * a2);
    }
    else y = clip ? 1.0f : a;                      // sans clip : on laisse passer au-dessus de 0 dB
    return std::copysign (y, x);
}

void BakrInflateProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    oversampling.reset();
    oversampling.initProcessing ((size_t) samplesPerBlock);
    setLatencySamples ((int) std::round (oversampling.getLatencyInSamples()));

    const double osr = sampleRate * 2.0;
    juce::dsp::ProcessSpec spec { osr, (juce::uint32) (samplesPerBlock * 2), 2 };
    for (auto* f : { &lp1, &hp1, &lp2, &hp2, &ap2 }) { f->prepare (spec); f->reset(); }
    lp1.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);  lp1.setCutoffFrequency (240.0f);
    hp1.setType (juce::dsp::LinkwitzRileyFilterType::highpass); hp1.setCutoffFrequency (240.0f);
    lp2.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);  lp2.setCutoffFrequency (2400.0f);
    hp2.setType (juce::dsp::LinkwitzRileyFilterType::highpass); hp2.setCutoffFrequency (2400.0f);
    ap2.setType (juce::dsp::LinkwitzRileyFilterType::allpass);  ap2.setCutoffFrequency (2400.0f);
    bandLow.setSize (2, samplesPerBlock * 2); bandMid.setSize (2, samplesPerBlock * 2); bandHigh.setSize (2, samplesPerBlock * 2);

    for (auto* s : { &inGain, &outGain, &wetSm, &curveSm }) s->reset (osr, 0.03);
    inGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pIn->load()));
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    wetSm.setCurrentAndTargetValue (pEffect->load() * 0.01f);
    curveSm.setCurrentAndTargetValue (pCurve->load());
    rmsIn = rmsOut = 0;
}

bool BakrInflateProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& o = layouts.getMainOutputChannelSet();
    if (o != juce::AudioChannelSet::stereo() && o != juce::AudioChannelSet::mono()) return false;
    return layouts.getMainInputChannelSet() == o;
}

void BakrInflateProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);

    inGain.setTargetValue (juce::Decibels::decibelsToGain (pIn->load()));
    outGain.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    wetSm.setTargetValue (pEffect->load() * 0.01f);
    curveSm.setTargetValue (pCurve->load());
    const bool clip = pClip->load() > 0.5f, band = pBand->load() > 0.5f, active = pEffectIn->load() > 0.5f;

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);
    auto up = oversampling.processSamplesUp (block);
    const int un = (int) up.getNumSamples();

    float pkIn[2] = { 0, 0 }, pkOut[2] = { 0, 0 };
    double sIn = 0, sOut = 0;
    bool anyClip = false;

    for (int i = 0; i < un; ++i)
    {
        const float gi = inGain.getNextValue(), go = outGain.getNextValue();
        const float wet = wetSm.getNextValue(), cv = curveSm.getNextValue();
        for (int c = 0; c < numCh; ++c)
        {
            float* d = up.getChannelPointer ((size_t) c);
            float x = d[i] * gi;
            pkIn[c] = juce::jmax (pkIn[c], std::abs (x));
            if (std::abs (x) > 1.0f) anyClip = true;
            sIn += (double) x * x;

            float y = x;
            if (active)
            {
                float xs = clip ? juce::jlimit (-1.0f, 1.0f, x) : x;
                if (band)
                {
                    float lo = lp1.processSample (c, xs);
                    float rest = hp1.processSample (c, xs);
                    float mid = lp2.processSample (c, rest);
                    float hi = hp2.processSample (c, rest);
                    lo = ap2.processSample (c, lo);           // alignement de phase de la bande grave
                    float wl = shape (lo, cv, clip), wm = shape (mid, cv, clip), wh = shape (hi, cv, clip);
                    float wetSig = wl + wm + wh;
                    // le "dry" est la somme des bandes (même phase que le signal traité)
                    y = (lo + mid + hi) * (1.0f - wet) + wetSig * wet;
                }
                else
                    y = xs * (1.0f - wet) + shape (xs, cv, clip) * wet;
            }
            y *= go;
            sOut += (double) y * y;
            pkOut[c] = juce::jmax (pkOut[c], std::abs (y));
            d[i] = y;
        }
    }
    oversampling.processSamplesDown (block);
    if (numCh == 1 && buffer.getNumChannels() > 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);

    // vumètres
    auto fall = [] (std::atomic<float>& m, float v) { m = juce::jmax (v, m.load() * 0.85f); };
    fall (inL, pkIn[0]); fall (inR, numCh > 1 ? pkIn[1] : pkIn[0]);
    fall (outL, pkOut[0]); fall (outR, numCh > 1 ? pkOut[1] : pkOut[0]);
    const double cnt = (double) juce::jmax (1, un * numCh);
    rmsIn = 0.8f * rmsIn + 0.2f * (float) std::sqrt (sIn / cnt);
    rmsOut = 0.8f * rmsOut + 0.2f * (float) std::sqrt (sOut / cnt / juce::jmax (1.0e-6, (double) outGain.getCurrentValue() * outGain.getCurrentValue()));
    // gain de "gonflement" ajouté (dB), indépendant du réglage de sortie
    effectMeter = (rmsIn > 1.0e-5f && active) ? juce::jlimit (0.0f, 12.0f, juce::Decibels::gainToDecibels (rmsOut / rmsIn)) : 0.0f;
    if (anyClip) clipped = true;
}

void BakrInflateProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, destData);
}

void BakrInflateProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPreset = (int) vt.getProperty ("preset", 0);
            apvts.replaceState (vt);
        }
}

juce::AudioProcessorEditor* BakrInflateProcessor::createEditor() { return new BakrInflateEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BakrInflateProcessor(); }
