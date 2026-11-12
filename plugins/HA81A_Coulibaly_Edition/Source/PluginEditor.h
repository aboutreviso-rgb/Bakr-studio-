#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class HA81ACoulibalyAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit HA81ACoulibalyAudioProcessorEditor(HA81ACoulibalyAudioProcessor&);
    ~HA81ACoulibalyAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    HA81ACoulibalyAudioProcessor& processor;
    juce::Label title;
    std::array<juce::Slider, 10> knobs;
    std::array<juce::Label, 10> labels;
    std::array<std::unique_ptr<Attachment>, 10> sliderAttachments;
    std::array<juce::TextButton, 3> switches;
    std::array<std::unique_ptr<ButtonAttachment>, 3> buttonAttachments;
    const std::array<const char*, 10> ids { "input", "lfGain", "lfFreq", "lmfGain", "lmfFreq", "hmfGain", "hmfFreq", "hfGain", "hfFreq", "output" };
    const std::array<const char*, 10> names { "INPUT dB", "LF GAIN dB", "LF FREQ Hz", "LMF GAIN dB", "LMF FREQ Hz", "HMF GAIN dB", "HMF FREQ Hz", "HF GAIN dB", "HF FREQ Hz", "OUTPUT dB" };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HA81ACoulibalyAudioProcessorEditor)
};
