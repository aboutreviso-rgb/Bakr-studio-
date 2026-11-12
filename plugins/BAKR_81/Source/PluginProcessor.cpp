#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <complex>

using APVTS = juce::AudioProcessorValueTreeState;

static juce::String dbStr (float v, int) { return (v > 0.04f ? "+" : "") + juce::String (v, 1) + " dB"; }
static juce::String hzStr (float v, int)
{
    if (v >= 1000.0f) return juce::String (v / 1000.0f, v < 10000.0f ? 2 : 1) + " kHz";
    return juce::String (juce::roundToInt (v)) + " Hz";
}

APVTS::ParameterLayout Bakr81Processor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto b = [&] (const char* id, const char* name, bool def)
    { p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def)); };
    auto db = [&] (const char* id, const char* name, float lo, float hi, float def)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float> (lo, hi, 0.1f), def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbStr)));
    };
    auto hz = [&] (const char* id, const char* name, float lo, float hi, float def)
    {
        juce::NormalisableRange<float> r (lo, hi,
            [] (float s, float e, float v) { return s * std::pow (e / s, v); },
            [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); });
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction (hzStr)));
    };

    db ("input",  "Input",   -20, 20, 0);
    db ("lfG",    "LF Gain", -12, 12, 0);   hz ("lfF",  "LF Freq",    20,   200,    60);
    db ("lmfG",   "LMF Gain",-12, 12, 0);   hz ("lmfF", "LMF Freq",  200,  2000,   800);
    db ("hmfG",   "HMF Gain",-12, 12, 0);   hz ("hmfF", "HMF Freq", 1000, 10000,  3500);
    db ("hfG",    "HF Gain", -12, 12, 0);   hz ("hfF",  "HF Freq",  5000, 20000, 12000);
    b  ("lowIn",  "Low Section In", true);
    b  ("lfBell", "LF Bell", false);
    b  ("highIn", "High Section In", true);
    b  ("hfBell", "HF Bell", false);
    db ("output", "Output",  -20, 20, 0);
    db ("eqTrim", "EQ Level",-12, 12, 0);
    b  ("eqIn",   "EQ In", true);
    b  ("phase",  "Phase", false);
    b  ("power",  "Power", true);
    return { p.begin(), p.end() };
}

Bakr81Processor::Bakr81Processor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BAKR_81", createLayout())
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    pIn = g ("input");
    pLfG = g ("lfG"); pLfF = g ("lfF"); pLmfG = g ("lmfG"); pLmfF = g ("lmfF");
    pHmfG = g ("hmfG"); pHmfF = g ("hmfF"); pHfG = g ("hfG"); pHfF = g ("hfF");
    pLowIn = g ("lowIn"); pLfBell = g ("lfBell"); pHighIn = g ("highIn"); pHfBell = g ("hfBell");
    pOut = g ("output"); pEqTrim = g ("eqTrim"); pEqIn = g ("eqIn"); pPhase = g ("phase"); pPower = g ("power");
    otherSlot = apvts.copyState();
}

bool Bakr81Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet() == out;
}

//==============================================================================
// Courbes "inductance" façon console classe A :
// shelf avec petit creux de compensation, cloches dont le Q se resserre avec le gain.
Bakr81Processor::BandDesign Bakr81Processor::design (BandType t, double f, double g, bool bell, double sr)
{
    BandDesign d;
    const double ag = std::abs (g);
    switch (t)
    {
        case LF:
            if (bell) d.main = b81::peak (f, sr, 0.75 + 0.03 * ag, g);
            else
            {
                d.main = b81::lowShelf (f, sr, 0.8, g);
                d.dip  = b81::peak (f * 2.6, sr, 0.9, -g * 0.16);   // le fameux creux au-dessus du shelf
                d.hasDip = true;
            }
            break;
        case LMF: d.main = b81::peak (f, sr, 0.7 + 0.06 * ag, g); break;
        case HMF: d.main = b81::peak (f, sr, 0.75 + 0.07 * ag, g); break;
        case HF:
            if (bell) d.main = b81::peak (f, sr, 0.8, g);
            else
            {
                d.main = b81::highShelf (f, sr, 0.75, g);
                d.dip  = b81::peak (f * 0.42, sr, 0.9, -g * 0.12);
                d.hasDip = true;
            }
            break;
    }
    return d;
}

static double magDb (const b81::Coefs& c, double f, double sr)
{
    const std::complex<double> z = std::polar (1.0, -2.0 * b81::pi * f / sr);
    const auto num = c.b0 + c.b1 * z + c.b2 * z * z;
    const auto den = 1.0 + c.a1 * z + c.a2 * z * z;
    return 20.0 * std::log10 (std::max (1e-9, std::abs (num / den)));
}

float Bakr81Processor::getResponseDb (double f) const
{
    const double dsr = 96000.0;
    const bool eqIn = pEqIn->load() > 0.5f, lowIn = pLowIn->load() > 0.5f, highIn = pHighIn->load() > 0.5f;
    double total = pEqTrim->load();
    if (! eqIn) return 0.0f;
    struct B { BandType t; float g, fr; bool bell, on; };
    const B bs[4] = { { LF,  pLfG->load(),  pLfF->load(),  pLfBell->load() > 0.5f, lowIn },
                      { LMF, pLmfG->load(), pLmfF->load(), false,                  lowIn },
                      { HMF, pHmfG->load(), pHmfF->load(), false,                  highIn },
                      { HF,  pHfG->load(),  pHfF->load(),  pHfBell->load() > 0.5f, highIn } };
    for (auto& b : bs)
    {
        if (! b.on || std::abs (b.g) < 0.01f) continue;
        auto d = design (b.t, b.fr, b.g, b.bell, dsr);
        total += magDb (d.main, f, dsr);
        if (d.hasDip) total += magDb (d.dip, f, dsr);
    }
    return (float) total;
}

//==============================================================================
void Bakr81Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    os = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    maxBlock = std::max (32, samplesPerBlock);
    os->initProcessing ((size_t) maxBlock);
    dry.setSize (2, maxBlock);
    latency = (int) std::round (os->getLatencyInSamples());
    setLatencySamples (latency);

    for (auto& p : pre) p.prepare (sr * 2.0);
    for (auto& d : dryDelay) d.assign ((size_t) latency + 1, 0.0f);
    dryPos = 0;

    std::atomic<float>* gp[4] = { pLfG, pLmfG, pHmfG, pHfG };
    std::atomic<float>* fp[4] = { pLfF, pLmfF, pHmfF, pHfF };
    for (int i = 0; i < 4; ++i)
    {
        bands[i].gainDb.reset (sr, 0.04); bands[i].gainDb.setCurrentAndTargetValue (gp[i]->load());
        bands[i].logF.reset (sr, 0.04);   bands[i].logF.setCurrentAndTargetValue (std::log (fp[i]->load()));
        for (int c = 0; c < 2; ++c) { bands[i].main[c].reset(); bands[i].dip[c].reset(); }
    }
    inGain.reset (sr, 0.03);  inGain.setCurrentAndTargetValue (b81::dbToGain (pIn->load()));
    outGain.reset (sr, 0.03); outGain.setCurrentAndTargetValue (b81::dbToGain (pOut->load()));
    eqTrim.reset (sr, 0.03);  eqTrim.setCurrentAndTargetValue (b81::dbToGain (pEqTrim->load()));
    powerMix.reset (sr, 0.02); powerMix.setCurrentAndTargetValue (pPower->load() > 0.5f ? 1.0f : 0.0f);
    phaseSm.reset (sr, 0.01);  phaseSm.setCurrentAndTargetValue (pPhase->load() > 0.5f ? -1.0f : 1.0f);
}

void Bakr81Processor::processBlock (juce::AudioBuffer<float>& full, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = std::min (2, full.getNumChannels());
    const int total = full.getNumSamples();
    for (int c = numCh; c < full.getNumChannels(); ++c) full.clear (c, 0, total);
    if (total == 0 || numCh == 0) return;
    for (int off = 0; off < total; off += maxBlock)
    {
        const int len = std::min (maxBlock, total - off);
        juce::AudioBuffer<float> seg (full.getArrayOfWritePointers(), numCh, off, len);
        processSegment (seg, numCh);
    }
    if (numCh == 1 && full.getNumChannels() > 1) full.copyFrom (1, 0, full, 0, 0, total);
}

void Bakr81Processor::processSegment (juce::AudioBuffer<float>& buffer, int numCh)
{
    const int n = buffer.getNumSamples();

    // Vumètre entrée
    for (int c = 0; c < 2; ++c)
    {
        const float pk = buffer.getMagnitude (std::min (c, numCh - 1), 0, n);
        inPeak[c].store (std::max (inPeak[c].load(), pk));
    }

    // Copie "sèche" retardée pour le bouton POWER (bypass sans clic)
    for (int c = 0; c < numCh; ++c)
    {
        const float* src = buffer.getReadPointer (c);
        float* d = dry.getWritePointer (c);
        auto& line = dryDelay[c];
        int pos = dryPos;
        for (int i = 0; i < n; ++i)
        {
            line[(size_t) pos] = src[i];
            int rd = pos - latency; if (rd < 0) rd += (int) line.size();
            d[i] = line[(size_t) rd];
            if (++pos >= (int) line.size()) pos = 0;
        }
    }
    dryPos = (dryPos + n) % (int) dryDelay[0].size();

    // Gain d'entrée + phase
    inGain.setTargetValue (b81::dbToGain (pIn->load()));
    phaseSm.setTargetValue (pPhase->load() > 0.5f ? -1.0f : 1.0f);
    for (int i = 0; i < n; ++i)
    {
        const float g = inGain.getNextValue() * phaseSm.getNextValue();
        for (int c = 0; c < numCh; ++c) buffer.getWritePointer (c)[i] *= g;
    }

    // Préampli classe A + transformateurs, suréchantillonné x2
    const float colour = juce::jlimit (0.0f, 1.0f, (pIn->load() + 20.0f) / 40.0f);
    {
        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);
        auto up = os->processSamplesUp (block);
        for (size_t c = 0; c < up.getNumChannels(); ++c)
        {
            float* d = up.getChannelPointer (c);
            for (size_t i = 0; i < up.getNumSamples(); ++i) d[i] = pre[c].process (d[i], colour);
        }
        os->processSamplesDown (block);
    }

    // EQ
    const bool eqIn = pEqIn->load() > 0.5f, lowIn = pLowIn->load() > 0.5f, highIn = pHighIn->load() > 0.5f;
    std::atomic<float>* gp[4] = { pLfG, pLmfG, pHmfG, pHfG };
    std::atomic<float>* fp[4] = { pLfF, pLmfF, pHmfF, pHfF };
    const bool secOn[4] = { lowIn, lowIn, highIn, highIn };
    for (int b = 0; b < 4; ++b)
    {
        bands[b].gainDb.setTargetValue (eqIn && secOn[b] ? gp[b]->load() : 0.0f);
        bands[b].logF.setTargetValue (std::log (fp[b]->load()));
    }
    const bool lfBell = pLfBell->load() > 0.5f, hfBell = pHfBell->load() > 0.5f;
    eqTrim.setTargetValue (b81::dbToGain (eqIn ? pEqTrim->load() : 0.0f));
    outGain.setTargetValue (b81::dbToGain (pOut->load()));
    powerMix.setTargetValue (pPower->load() > 0.5f ? 1.0f : 0.0f);

    constexpr int chunk = 32;
    for (int start = 0; start < n; start += chunk)
    {
        const int len = std::min (chunk, n - start);
        for (int b = 0; b < 4; ++b)
        {
            auto& band = bands[b];
            const float g = band.gainDb.getCurrentValue();
            const float f = std::exp (band.logF.getCurrentValue());
            band.gainDb.skip (len); band.logF.skip (len);
            if (std::abs (g) < 0.005f && ! band.gainDb.isSmoothing()) continue;
            const bool bell = (b == LF && lfBell) || (b == HF && hfBell);
            const auto d = design ((BandType) b, f, g, bell, sr);
            for (int c = 0; c < numCh; ++c)
            {
                band.main[c].c = d.main;
                band.dip[c].c = d.hasDip ? d.dip : b81::Coefs();
                float* x = buffer.getWritePointer (c, start);
                for (int i = 0; i < len; ++i) x[i] = band.dip[c].process (band.main[c].process (x[i]));
            }
        }
        for (int i = 0; i < len; ++i)
        {
            const float g = eqTrim.getNextValue() * outGain.getNextValue();
            const float wet = powerMix.getNextValue();
            for (int c = 0; c < numCh; ++c)
            {
                float* x = buffer.getWritePointer (c, start);
                const float dv = dry.getReadPointer (c, start)[i];
                x[i] = dv + wet * (x[i] * g - dv);
            }
        }
    }

    for (int c = 0; c < 2; ++c)
    {
        const float pk = buffer.getMagnitude (std::min (c, numCh - 1), 0, n);
        outPeak[c].store (std::max (outPeak[c].load(), pk));
    }
}

//==============================================================================
juce::StringArray Bakr81Processor::getPresetNames()
{
    return { "Default", "Voix Afrobeats - air & presence", "Voix Mandingue - chaleur", "Voix Rap / Drill - punch",
             "Voix Reggae - ronde", "Kick Afro", "Log drum Amapiano", "Balafon / Kora", "Guitare highlife",
             "Basse reggae", "Bus batterie - couleur", "Master - chaleur legere" };
}

void Bakr81Processor::setParam (const juce::String& id, float v)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

void Bakr81Processor::applyPreset (int index)
{
    struct P { float in, lfG, lfF, lmfG, lmfF, hmfG, hmfF, hfG, hfF, out; bool lfBell, hfBell; };
    static const P presets[] = {
        {  0,  0,  60,    0,  800,  0,   3500, 0,   12000,  0, false, false },
        {  4, -3, 100, -2.5,  400,  2.5, 3500, 4,   12000, -4, false, false },
        {  6, 1.5,120,   -2,  500,  2,   2500, 2.5, 10000, -6, true,  false },
        {  8, -4,  80,   -3,  300,  4,   4000, 3,   15000, -8, false, false },
        {  5,  1, 150,  1.5,  700,  1.5, 2000, -1,  16000, -5, true,  false },
        {  6,  5,  60,   -4,  400,  3,   3500, 0,   12000, -6, true,  false },
        {  8,  3,  50,   -2,  250,  2,   1500, 0,   12000, -8, false, false },
        {  3, -3, 120,   -2,  500,  2,   3000, 3,   10000, -3, false, false },
        {  4, -3, 150,  1.5,  900,  2.5, 2500, 1.5,  8000, -4, false, true  },
        {  6,  3,  80,   -3,  300,  2,   1200, -4,   6000, -6, true,  false },
        {  8,  2,  60,    0,  800,  1.5, 4000, 2,   12000, -8, false, false },
        {  3,  1,  40, -0.5,  300,  0,   3500, 1.5, 15000, -3, false, false },
    };
    if (index < 0 || index >= (int) std::size (presets)) return;
    const auto& p = presets[index];
    setParam ("input", p.in);
    setParam ("lfG", p.lfG);   setParam ("lfF", p.lfF);
    setParam ("lmfG", p.lmfG); setParam ("lmfF", p.lmfF);
    setParam ("hmfG", p.hmfG); setParam ("hmfF", p.hmfF);
    setParam ("hfG", p.hfG);   setParam ("hfF", p.hfF);
    setParam ("output", p.out);
    setParam ("lfBell", p.lfBell ? 1.0f : 0.0f); setParam ("hfBell", p.hfBell ? 1.0f : 0.0f);
    setParam ("lowIn", 1); setParam ("highIn", 1); setParam ("eqIn", 1);
    setParam ("eqTrim", 0); setParam ("phase", 0); setParam ("power", 1);
    currentPreset = index;
}

void Bakr81Processor::toggleAB()
{
    auto current = apvts.copyState();
    apvts.replaceState (otherSlot.createCopy());
    otherSlot = current;
    slotB = ! slotB;
}

void Bakr81Processor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void Bakr81Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* Bakr81Processor::createEditor() { return new Bakr81Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Bakr81Processor(); }
