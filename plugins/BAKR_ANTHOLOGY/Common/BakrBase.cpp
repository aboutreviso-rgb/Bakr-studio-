#include "BakrBase.h"

namespace bakr
{
BaseProcessor::BaseProcessor (const juce::String& n, juce::AudioProcessorValueTreeState::ParameterLayout layout,
                              std::vector<PresetDef> ps, bool oversample)
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BAKR_STATE", std::move (layout)),
      name (n), presets (std::move (ps)), useOS (oversample)
{
}

bool BaseProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        && l.getMainInputChannelSet() == out;
}

void BaseProcessor::prepareToPlay (double sampleRate, int block)
{
    fs = sampleRate;
    const int nCh = juce::jmax (1, juce::jmin (2, getMainBusNumOutputChannels()));
    oversampler.numChannels = (size_t) nCh;
    oversampler.initProcessing ((size_t) block);
    oversampler.reset();

    dryWet.prepare ({ sampleRate, (juce::uint32) block, (juce::uint32) nCh });
    dryWet.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    const float lat = useOS ? oversampler.getLatencyInSamples() : 0.0f;
    dryWet.setWetLatency (lat);
    setLatencySamples ((int) std::round (lat));

    if (useOS) prepareCore (sampleRate * 2.0, block * 2, nCh);
    else       prepareCore (sampleRate, block, nCh);
}

void BaseProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int nCh = juce::jmin (2, getMainBusNumOutputChannels(), buffer.getNumChannels());
    const int n = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0) return;

    for (int c = 0; c < nCh; ++c)
        inDb[(size_t) c].store (dB (buffer.getMagnitude (c, 0, n)));

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) nCh, (size_t) n);
    float mix = has ("mix") ? p ("mix") * 0.01f : 1.0f;
    if (has ("power") && p ("power") < 0.5f) mix = 0.0f;   // marche/arrêt = bypass sans clic
    dryWet.setWetMixProportion (mix);
    dryWet.pushDrySamples (block);

    grAcc = { 0.0f, 0.0f };
    if (useOS)
    {
        auto os = oversampler.processSamplesUp (block);
        processCore (os, fs * 2.0);
        oversampler.processSamplesDown (block);
    }
    else processCore (block, fs);

    dryWet.mixWetSamples (block);

    for (int c = 0; c < nCh; ++c)
    {
        outDb[(size_t) c].store (dB (buffer.getMagnitude (c, 0, n)));
        grDb[(size_t) c].store (grAcc[(size_t) c]);
    }
    if (nCh == 1)
    {
        grDb[1].store (grAcc[0]); inDb[1].store (inDb[0].load()); outDb[1].store (outDb[0].load());
    }
    for (int c = nCh; c < buffer.getNumChannels(); ++c)
        buffer.copyFrom (c, 0, buffer, 0, 0, n);
}

juce::StringArray BaseProcessor::getPresetNames() const
{
    juce::StringArray s;
    for (auto& pr : presets) s.add (pr.name);
    return s;
}

const juce::String BaseProcessor::getProgramName (int i)
{
    return juce::isPositiveAndBelow (i, (int) presets.size()) ? presets[(size_t) i].name : juce::String ("Init");
}

void BaseProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentPreset = index;
    // d'abord tout aux valeurs par défaut, puis les valeurs du preset
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            r->setValueNotifyingHost (r->getDefaultValue());
    for (auto& [id, v] : presets[(size_t) index].values)
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

void BaseProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto st = apvts.copyState();
    st.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, dest);
}

void BaseProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPreset = (int) vt.getProperty ("preset", 0);
            apvts.replaceState (vt);
        }
}
}
