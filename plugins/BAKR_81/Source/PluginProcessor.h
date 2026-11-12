#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP.h"

class Bakr81Processor : public juce::AudioProcessor
{
public:
    Bakr81Processor();
    ~Bakr81Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BAKR 81"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Presets
    static juce::StringArray getPresetNames();
    void applyPreset (int index);
    void setParam (const juce::String& id, float plainValue);
    int currentPreset = 0;

    // A/B
    void toggleAB();
    bool isB() const { return slotB; }

    // Courbe pour l'affichage (réponse de l'EQ à la fréquence f, en dB)
    float getResponseDb (double f) const;

    // Vumètres (crête linéaire, lus par l'interface)
    std::atomic<float> inPeak[2] { { 0.0f }, { 0.0f } }, outPeak[2] { { 0.0f }, { 0.0f } };

private:
    struct Band
    {
        juce::SmoothedValue<float> gainDb, logF;
        b81::Biquad main[2], dip[2];
    };
    struct BandDesign { b81::Coefs main, dip; bool hasDip = false; };

    enum BandType { LF, LMF, HMF, HF };
    void processSegment (juce::AudioBuffer<float>& buffer, int numCh);
    static BandDesign design (BandType t, double f, double g, bool bell, double sr);

    double sr = 48000.0;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    b81::Preamp pre[2];
    Band bands[4];
    juce::SmoothedValue<float> inGain, outGain, eqTrim, powerMix, phaseSm;
    std::vector<float> dryDelay[2];
    juce::AudioBuffer<float> dry;
    int dryPos = 0, latency = 0, maxBlock = 512;
    bool slotB = false;
    juce::ValueTree otherSlot;

    std::atomic<float> *pIn, *pLfG, *pLfF, *pLmfG, *pLmfF, *pHmfG, *pHmfF, *pHfG, *pHfF,
                       *pLowIn, *pLfBell, *pHighIn, *pHfBell, *pOut, *pEqTrim, *pEqIn, *pPhase, *pPower;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Bakr81Processor)
};
