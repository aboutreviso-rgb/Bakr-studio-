#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::AudioParameterFloat* floatParam(const juce::String& id, const juce::String& name,
                                          float min, float max, float def)
    {
        return new juce::AudioParameterFloat(juce::ParameterID { id, 1 }, name,
            juce::NormalisableRange<float>(min, max, 0.01f), def);
    }
}

HA81ACoulibalyAudioProcessor::APVTS::ParameterLayout HA81ACoulibalyAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    layout.add(std::unique_ptr<juce::RangedAudioParameter>(floatParam("input", "Input", -24.0f, 24.0f, 0.0f)));
    layout.add(std::unique_ptr<juce::RangedAudioParameter>(floatParam("lfGain", "LF Gain", -12.0f, 12.0f, 0.0f)));
    layout.add(std::unique_ptr<juce::RangedAudioParameter>(floatParam("lfFreq", "LF Frequency", 20.0f, 200.0f, 60.0f)));
    layout.add(std::unique_ptr<juce::RangedAudioParameter>(floatParam("lmfGain", "LMF Gain", -12.0f, 12.0f, 0.0f)));
    layout.add(std::unique_ptr<juce::RangedAudioParameter>(floatParam("lmfFreq", "LMF Frequency", 200.0f, 2000.0f, 800.0f)));
    layout.add(std::unique_ptr<juce::RangedAudioParameter>(floatParam("hmfGain", "HMF Gain", -12.0f, 12.0f, 0.0f)));
    layout.add(std::unique_ptr<juce::RangedAudioParameter>(floatParam("hmfFreq", "HMF Frequency", 1000.0f, 10000.0f, 3500.0f)));
    layout.add(std::unique_ptr<juce::RangedAudioParameter>(floatParam("hfGain", "HF Gain", -12.0f, 12.0f, 0.0f)));
    layout.add(std::unique_ptr<juce::RangedAudioParameter>(floatParam("hfFreq", "HF Frequency", 5000.0f, 20000.0f, 12000.0f)));
    layout.add(std::unique_ptr<juce::RangedAudioParameter>(floatParam("output", "Output", -24.0f, 24.0f, 0.0f)));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", 1 }, "Bypass", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "phase", 1 }, "Phase", false));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "power", 1 }, "Power", true));
    return layout;
}

HA81ACoulibalyAudioProcessor::HA81ACoulibalyAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "HA81A_STATE", createParameterLayout())
{}

void HA81ACoulibalyAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock),
                                  static_cast<juce::uint32>(getTotalNumOutputChannels()) };
    currentSampleRate = sampleRate;
    for (int ch = 0; ch < 2; ++ch)
    {
        lowShelf[static_cast<size_t>(ch)].prepare(spec); lowMid[static_cast<size_t>(ch)].prepare(spec);
        highMid[static_cast<size_t>(ch)].prepare(spec); highShelf[static_cast<size_t>(ch)].prepare(spec);
        lowShelf[static_cast<size_t>(ch)].reset(); lowMid[static_cast<size_t>(ch)].reset();
        highMid[static_cast<size_t>(ch)].reset(); highShelf[static_cast<size_t>(ch)].reset();
    }
    updateFilters();
}

void HA81ACoulibalyAudioProcessor::releaseResources()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        lowShelf[static_cast<size_t>(ch)].reset(); lowMid[static_cast<size_t>(ch)].reset();
        highMid[static_cast<size_t>(ch)].reset(); highShelf[static_cast<size_t>(ch)].reset();
    }
}

bool HA81ACoulibalyAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        && out == layouts.getMainInputChannelSet();
}

void HA81ACoulibalyAudioProcessor::updateFilters()
{
    const auto value = [this](const char* id) { return parameters.getRawParameterValue(id)->load(); };
    const float nyquistSafe = static_cast<float>(currentSampleRate * 0.475);
    const auto clampFreq = [nyquistSafe](float f) { return juce::jlimit(10.0f, nyquistSafe, f); };
    const float lfF = clampFreq(value("lfFreq"));
    const float lmfF = clampFreq(value("lmfFreq"));
    const float hmfF = clampFreq(value("hmfFreq"));
    const float hfF = clampFreq(value("hfFreq"));
    auto lf = juce::dsp::IIR::Coefficients<float>::makeLowShelf(currentSampleRate, lfF, 0.7071f, juce::Decibels::decibelsToGain(value("lfGain")));
    auto lmf = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, lmfF, 0.8f, juce::Decibels::decibelsToGain(value("lmfGain")));
    auto hmf = juce::dsp::IIR::Coefficients<float>::makePeakFilter(currentSampleRate, hmfF, 0.8f, juce::Decibels::decibelsToGain(value("hmfGain")));
    auto hf = juce::dsp::IIR::Coefficients<float>::makeHighShelf(currentSampleRate, hfF, 0.7071f, juce::Decibels::decibelsToGain(value("hfGain")));
    for (int ch = 0; ch < 2; ++ch)
    {
        lowShelf[static_cast<size_t>(ch)].coefficients = lf;
        lowMid[static_cast<size_t>(ch)].coefficients = lmf;
        highMid[static_cast<size_t>(ch)].coefficients = hmf;
        highShelf[static_cast<size_t>(ch)].coefficients = hf;
    }
}

void HA81ACoulibalyAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = getTotalNumInputChannels();
    for (int ch = channels; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    const bool powered = parameters.getRawParameterValue("power")->load() > 0.5f;
    const bool bypassed = parameters.getRawParameterValue("bypass")->load() > 0.5f;
    if (!powered || bypassed) return;

    updateFilters();
    const float inGain = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("input")->load());
    const float outGain = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("output")->load());
    const bool invert = parameters.getRawParameterValue("phase")->load() > 0.5f;

    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        juce::FloatVectorOperations::multiply(data, inGain, buffer.getNumSamples());
        juce::dsp::AudioBlock<float> block(buffer);
        auto channelBlock = block.getSingleChannelBlock(static_cast<size_t>(ch));
        juce::dsp::ProcessContextReplacing<float> context(channelBlock);
        lowShelf[static_cast<size_t>(ch)].process(context);
        lowMid[static_cast<size_t>(ch)].process(context);
        highMid[static_cast<size_t>(ch)].process(context);
        highShelf[static_cast<size_t>(ch)].process(context);
        if (invert) juce::FloatVectorOperations::multiply(data, -1.0f, buffer.getNumSamples());
        juce::FloatVectorOperations::multiply(data, outGain, buffer.getNumSamples());
    }
}

juce::AudioProcessorEditor* HA81ACoulibalyAudioProcessor::createEditor()
{
    return new HA81ACoulibalyAudioProcessorEditor(*this);
}

void HA81ACoulibalyAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HA81ACoulibalyAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HA81ACoulibalyAudioProcessor();
}
