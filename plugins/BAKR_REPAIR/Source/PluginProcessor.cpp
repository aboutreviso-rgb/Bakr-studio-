#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static juce::String tenth (float v, int) { return juce::String (v * 10.0f, 1); }

APVTS::ParameterLayout BakrRepairProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto b = [&] (const char* id, const char* name, bool def) { p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def)); };
    auto f = [&] (const char* id, const char* name, float lo, float hi, float def, const char* unit, std::function<juce::String (float, int)> toStr = nullptr)
    {
        auto attr = juce::AudioParameterFloatAttributes().withLabel (unit);
        if (toStr) attr = attr.withStringFromValueFunction (toStr);
        else attr = attr.withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); });
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (lo, hi, 0.001f), def, attr));
    };

    b ("instrument", "Mode Instrument", false);
    b ("cuOn", "Clean Up On", true);
    f ("denoise", "De-noise", 0, 1, 0.3f, "", tenth);
    f ("dereverb", "De-reverb", 0, 1, 0.3f, "", tenth);
    b ("dehum", "De-hum", true);
    b ("declick", "De-click", true);
    b ("hum60", "Hum 60 Hz", false);
    b ("toneOn", "Tone On", true);
    f ("tone", "Tone", -1, 1, 0.0f, "", [] (float v, int) { return (v > 0 ? "+" : "") + juce::String (v * 10.0f, 1); });
    b ("toneScoop", "Tone Scoop", false);
    b ("locut", "Lo-cut", false);
    b ("hicut", "Hi-cut", false);
    b ("dsOn", "De-ess On", true);
    f ("dsShape", "Shaping", 0, 1, 0.5f, "", [] (float v, int) { return juce::String ((3500.0f + v * 5500.0f) / 1000.0f, 1) + " kHz"; });
    f ("dsThresh", "De-ess Threshold", -60, 0, -24.0f, "dB");
    b ("dsSlow", "De-ess Slow", false);
    b ("dcOn", "De-clip On", true);
    f ("dcThresh", "De-clip Threshold", -64, 0, 0.0f, "dB");
    f ("outGain", "Output Gain", -12, 12, 0.0f, "dB");
    b ("limiter", "Limiter", true);
    b ("listenCU", "Ecoute Clean Up", false);
    b ("listenDS", "Ecoute De-ess", false);
    b ("listenDC", "Ecoute De-clip", false);
    b ("bypass", "Bypass", false);
    return { p.begin(), p.end() };
}

BakrRepairProcessor::BakrRepairProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BAKR_REPAIR", createLayout())
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    pInstrument = g ("instrument"); pCuOn = g ("cuOn"); pDenoise = g ("denoise"); pDereverb = g ("dereverb");
    pDehum = g ("dehum"); pDeclick = g ("declick"); pHum60 = g ("hum60");
    pToneOn = g ("toneOn"); pTone = g ("tone"); pToneScoop = g ("toneScoop"); pLocut = g ("locut"); pHicut = g ("hicut");
    pDsOn = g ("dsOn"); pDsShape = g ("dsShape"); pDsThresh = g ("dsThresh"); pDsSlow = g ("dsSlow");
    pDcOn = g ("dcOn"); pDcThresh = g ("dcThresh"); pOutGain = g ("outGain"); pLimiter = g ("limiter");
    pListenCU = g ("listenCU"); pListenDS = g ("listenDS"); pListenDC = g ("listenDC"); pBypass = g ("bypass");
}

bool BakrRepairProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet() == out;
}

void BakrRepairProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    const int total = bakr::LookaheadRepair::latency + bakr::SpectralCleaner::latency;
    setLatencySamples (total);

    for (int c = 0; c < 2; ++c)
    {
        look[c].reset();
        spec[c].prepare (sr);
        for (auto& h : hum[c]) h.reset();
        dlyCU[c].prepare (bakr::SpectralCleaner::latency);
        dlyDC[c].prepare (bakr::SpectralCleaner::latency);
        dryDly[c].prepare (total);
        for (auto* bq : { &tiltLo[c], &tiltHi[c], &scoop[c], &locut1[c], &locut2[c], &hicut[c], &dsSide[c], &dsSplitA[c], &dsSplitB[c] }) bq->reset();
        locut1[c].highPass (80.0, sr, 0.54);
        locut2[c].highPass (80.0, sr, 1.31);
        hicut[c].lowPass (15000.0, sr);
    }
    learner.prepare (sr);
    dsEnv = 0; limGain = 1; dcPeakHold = 0;
    outGainSm.reset (sr, 0.03); outGainSm.setCurrentAndTargetValue (bakr::dbToGain (pOutGain->load()));
    bypassMix.reset (sr, 0.02); bypassMix.setCurrentAndTargetValue (pBypass->load() > 0.5f ? 1.0f : 0.0f);
    lastTone = 99; lastHum60 = -1; lastShape = -1;
    updateFilters();
}

void BakrRepairProcessor::updateFilters()
{
    const float tone = pTone->load();
    const bool sc = pToneScoop->load() > 0.5f;
    if (tone != lastTone || sc != lastScoop)
    {
        for (int c = 0; c < 2; ++c)
        {
            tiltLo[c].lowShelf (900.0, sr, -tone * 6.0);
            tiltHi[c].highShelf (900.0, sr, tone * 6.0);
            // Scoop : + = creuse les médiums boueux, - = les remplit
            scoop[c].peak (450.0, sr, 0.8, -tone * 7.0);
        }
        lastTone = tone; lastScoop = sc;
    }
    const float h60 = pHum60->load();
    if (h60 != lastHum60)
    {
        const double f0 = h60 > 0.5f ? 60.0 : 50.0;
        for (int c = 0; c < 2; ++c)
            for (int h = 0; h < 5; ++h) hum[c][h].notch (f0 * (h + 1), sr, 18.0 + 6.0 * h);
        lastHum60 = h60;
    }
    const float shape = pDsShape->load();
    if (shape != lastShape)
    {
        const double fc = 3500.0 + shape * 5500.0;
        for (int c = 0; c < 2; ++c) { dsSide[c].bandPass (fc, sr, 1.2); dsSplitA[c].lowPass (fc * 0.62, sr, 0.54); dsSplitB[c].lowPass (fc * 0.62, sr, 1.31); }
        lastShape = shape;
    }
}

void BakrRepairProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = std::min (2, getTotalNumInputChannels());
    const int n = buffer.getNumSamples();
    for (int c = numCh; c < getTotalNumOutputChannels(); ++c) buffer.clear (c, 0, n);
    if (numCh == 0) return;

    updateFilters();

    float* ch[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };
    learner.push (ch[0], ch[1], n);

    const bool instrument = pInstrument->load() > 0.5f;
    const bool cuOn = pCuOn->load() > 0.5f, toneOn = pToneOn->load() > 0.5f;
    const bool dsOn = pDsOn->load() > 0.5f, dcOn = pDcOn->load() > 0.5f;
    const bool dehum = cuOn && pDehum->load() > 0.5f, declick = cuOn && pDeclick->load() > 0.5f;
    const bool scoopMode = pToneScoop->load() > 0.5f;
    const bool locutOn = toneOn && pLocut->load() > 0.5f, hicutOn = toneOn && pHicut->load() > 0.5f;
    const bool limiterOn = dcOn && pLimiter->load() > 0.5f;
    const bool lCU = pListenCU->load() > 0.5f, lDS = pListenDS->load() > 0.5f, lDC = pListenDC->load() > 0.5f;
    const bool listening = lCU || lDS || lDC;
    const float dcThr = bakr::dbToGain (pDcThresh->load());
    const float dsThr = pDsThresh->load();
    const bool slow = pDsSlow->load() > 0.5f;
    const float atk = std::exp (-1.0f / (float) ((slow ? 0.004 : 0.0007) * sr));
    const float rel = std::exp (-1.0f / (float) ((slow ? 0.14 : 0.045) * sr));
    const float limRel = std::exp (-1.0f / (float) (0.08 * sr));
    const float ceiling = bakr::dbToGain (-0.3f);
    const float tone = pTone->load();
    const bool toneActive = toneOn && std::abs (tone) > 0.001f;

    for (int c = 0; c < numCh; ++c)
        spec[c].setParams (pDenoise->load(), pDereverb->load(), cuOn, instrument, lCU);

    outGainSm.setTargetValue (dcOn ? bakr::dbToGain (pOutGain->load()) : 1.0f);
    bypassMix.setTargetValue (pBypass->load() > 0.5f ? 1.0f : 0.0f);

    float maxGR = 0.0f, inPeak = 0.0f;
    int pre = specWritePos.load();

    for (int i = 0; i < n; ++i)
    {
        float y[2] = { 0, 0 }, dry[2] = { 0, 0 }, cuRem[2] = { 0, 0 }, dcAdd[2] = { 0, 0 }, dsRem[2] = { 0, 0 };
        float monoIn = 0;

        for (int c = 0; c < numCh; ++c)
        {
            const float x = ch[c][i];
            monoIn += x;
            inPeak = std::max (inPeak, std::abs (x));
            dry[c] = dryDly[c].process (x);

            float clipAdded = 0, clickRem = 0;
            float s = look[c].process (x, dcOn, dcThr, declick, clipAdded, clickRem, clickCounter);

            float humRem = 0;
            if (dehum)
            {
                const float before = s;
                for (auto& h : hum[c]) s = h.process (s);
                humRem = before - s;
            }

            float specRem = 0;
            s = spec[c].process (s, specRem);
            cuRem[c] = dlyCU[c].process (clickRem + humRem) + specRem;
            dcAdd[c] = dlyDC[c].process (clipAdded);

            if (toneActive)
            {
                if (scoopMode) s = scoop[c].process (s);
                else s = tiltHi[c].process (tiltLo[c].process (s));
            }
            if (locutOn) s = locut2[c].process (locut1[c].process (s));
            if (hicutOn) s = hicut[c].process (s);
            y[c] = s;
        }

        // De-ess (détection liée stéréo)
        if (dsOn)
        {
            float side[2] = { 0, 0 }, hi[2] = { 0, 0 }, lvl = 0;
            for (int c = 0; c < numCh; ++c)
            {
                side[c] = dsSide[c].process (y[c]);
                hi[c] = y[c] - dsSplitB[c].process (dsSplitA[c].process (y[c]));   // split complémentaire exact
                lvl = std::max (lvl, std::abs (side[c]));
            }
            dsEnv = lvl > dsEnv ? atk * dsEnv + (1 - atk) * lvl : rel * dsEnv + (1 - rel) * lvl;
            const float over = bakr::gainToDb (dsEnv) - dsThr;
            const float grDb = over > 0 ? std::min (18.0f, over * 0.75f) : 0.0f;
            maxGR = std::max (maxGR, grDb);
            const float g = bakr::dbToGain (-grDb);
            for (int c = 0; c < numCh; ++c)
            {
                dsRem[c] = hi[c] * (1.0f - g);
                y[c] -= dsRem[c];
            }
        }

        const float og = outGainSm.getNextValue();
        for (int c = 0; c < numCh; ++c) y[c] *= og;

        if (limiterOn)
        {
            float pk = 0; for (int c = 0; c < numCh; ++c) pk = std::max (pk, std::abs (y[c]));
            const float target = pk > ceiling ? ceiling / pk : 1.0f;
            limGain = target < limGain ? target : limRel * limGain + (1 - limRel) * target;
            for (int c = 0; c < numCh; ++c) y[c] = std::clamp (y[c] * limGain, -ceiling, ceiling);
        }

        if (listening)
            for (int c = 0; c < numCh; ++c)
                y[c] = (lCU ? cuRem[c] : 0.0f) + (lDS ? dsRem[c] : 0.0f) + (lDC ? dcAdd[c] * 2.0f : 0.0f);

        const float bm = bypassMix.getNextValue();
        float monoOut = 0;
        for (int c = 0; c < numCh; ++c)
        {
            const float o = y[c] * (1.0f - bm) + dry[c] * bm;
            ch[c][i] = o;
            monoOut += o;
        }

        specPre[(size_t) pre] = monoIn / (float) numCh;
        specPost[(size_t) pre] = monoOut / (float) numCh;
        pre = (pre + 1) & (specSize - 1);
    }
    specWritePos.store (pre);

    if (numCh == 1 && getTotalNumOutputChannels() > 1) buffer.copyFrom (1, 0, buffer, 0, 0, n);

    // Mètres
    dsGainReductionDb.store (dsOn ? std::max (maxGR, dsGainReductionDb.load() * 0.85f) : 0.0f);
    dcPeakHold = std::max (inPeak, dcPeakHold * 0.92f);
    dcInputPeakDb.store (bakr::gainToDb (dcPeakHold));
}

//==============================================================================
void BakrRepairProcessor::setParam (const juce::String& id, float plainValue)
{
    if (auto* prm = apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
        prm->endChangeGesture();
    }
}

juce::StringArray BakrRepairProcessor::getPresetNames()
{
    juce::StringArray a;
    for (auto* n : { "Par défaut", "Voix - chambre qui résonne", "Voix - micro téléphone", "Afrobeats - lead brillant",
                     "Mandingue - voix chaude", "Rap / Drill - sec et propre", "Reggae - roots doux",
                     "Voix - bruit de fond fort", "Instrument - guitare acoustique" })
        a.add (juce::String::fromUTF8 (n));
    return a;
}

void BakrRepairProcessor::applyPreset (int index)
{
    struct P { float inst, dn, dr, dehum, declick, tone, scoop, locut, hicut, dsOn, shape, thr, dsSlow, dcThr, gain; };
    static const P presets[] = {
        //  inst  dn    dr    hum  clk  tone   scp  lo   hi   ds   shape thr   slow dcT   gain
        { 0, 0.30f, 0.30f, 1, 1,  0.00f, 0, 0, 0, 1, 0.50f, -24, 0,  0.0f, 0 },
        { 0, 0.35f, 0.75f, 1, 1,  0.15f, 0, 1, 0, 1, 0.50f, -26, 0,  0.0f, 0 },
        { 0, 0.55f, 0.30f, 1, 1,  0.30f, 0, 1, 0, 1, 0.45f, -28, 0, -1.0f, 0 },
        { 0, 0.25f, 0.35f, 1, 1,  0.35f, 0, 1, 0, 1, 0.60f, -22, 0,  0.0f, 0 },
        { 0, 0.25f, 0.30f, 1, 1, -0.15f, 0, 1, 0, 1, 0.40f, -26, 1,  0.0f, 0 },
        { 0, 0.30f, 0.50f, 1, 1,  0.30f, 1, 1, 0, 1, 0.55f, -24, 0,  0.0f, 0 },
        { 0, 0.20f, 0.25f, 1, 1, -0.10f, 0, 1, 1, 1, 0.45f, -26, 1,  0.0f, 0 },
        { 0, 0.75f, 0.35f, 1, 1,  0.10f, 0, 1, 1, 1, 0.50f, -26, 0,  0.0f, 0 },
        { 1, 0.25f, 0.20f, 1, 1,  0.10f, 0, 0, 0, 0, 0.50f, -24, 1,  0.0f, 0 },
    };
    if (index < 0 || index >= (int) std::size (presets)) return;
    const auto& p = presets[index];
    setParam ("instrument", p.inst); setParam ("cuOn", 1); setParam ("denoise", p.dn); setParam ("dereverb", p.dr);
    setParam ("dehum", p.dehum); setParam ("declick", p.declick);
    setParam ("toneOn", 1); setParam ("tone", p.tone); setParam ("toneScoop", p.scoop); setParam ("locut", p.locut); setParam ("hicut", p.hicut);
    setParam ("dsOn", p.dsOn); setParam ("dsShape", p.shape); setParam ("dsThresh", p.thr); setParam ("dsSlow", p.dsSlow);
    setParam ("dcOn", 1); setParam ("dcThresh", p.dcThr); setParam ("outGain", p.gain); setParam ("limiter", 1);
    setParam ("listenCU", 0); setParam ("listenDS", 0); setParam ("listenDC", 0); setParam ("bypass", 0);
}

void BakrRepairProcessor::startLearn()
{
    learner.request (pInstrument->load() > 0.5f);
}

bool BakrRepairProcessor::applyLearnResultIfReady()
{
    if (! learner.done.exchange (false)) return false;
    const auto r = learner.result;
    setParam ("cuOn", 1);
    setParam ("denoise", r.denoise); setParam ("dereverb", r.dereverb);
    setParam ("dehum", r.dehum ? 1.0f : 0.0f); setParam ("hum60", r.hum60 ? 1.0f : 0.0f);
    setParam ("declick", r.declick ? 1.0f : 0.0f);
    setParam ("toneOn", 1); setParam ("toneScoop", 0); setParam ("tone", r.tone);
    setParam ("locut", r.locut ? 1.0f : 0.0f); setParam ("hicut", r.hicut ? 1.0f : 0.0f);
    setParam ("dsOn", r.dsOn ? 1.0f : 0.0f); setParam ("dsThresh", r.dsThresh); setParam ("dsShape", r.dsShape);
    setParam ("dcOn", r.dcOn ? 1.0f : 0.0f); setParam ("dcThresh", r.dcThresh);
    return true;
}

//==============================================================================
void BakrRepairProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void BakrRepairProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* BakrRepairProcessor::createEditor() { return new BakrRepairEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BakrRepairProcessor(); }
