#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP.h"

class BakrRepairProcessor : public juce::AudioProcessor
{
public:
    BakrRepairProcessor();
    ~BakrRepairProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BAKR REPAIR"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.05; }

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

    // Learn
    void startLearn();
    void cancelLearn() { learner.cancel(); }
    bool isLearning() const { return learner.active.load() || learner.requested.load(); }
    float getLearnProgress() const { return learner.progress.load(); }
    bool applyLearnResultIfReady();   // à appeler depuis le thread GUI

    // Affichage
    static constexpr int specSize = 4096;
    std::array<float, specSize> specPre {}, specPost {};
    std::atomic<int> specWritePos { 0 };
    std::atomic<float> dsGainReductionDb { 0.0f }, dcInputPeakDb { -100.0f }, dcActivity { 0.0f };
    double getCurrentSampleRate() const { return sr; }

private:
    void updateFilters();

    double sr = 48000.0;
    bakr::Learner learner;

    bakr::LookaheadRepair look[2];
    bakr::SpectralCleaner spec[2];
    bakr::Biquad hum[2][5];
    bakr::DelayLine dlyCU[2], dlyDC[2], dryDly[2];
    bakr::Biquad tiltLo[2], tiltHi[2], scoop[2], locut1[2], locut2[2], hicut[2];
    bakr::Biquad dsSide[2], dsSplitA[2], dsSplitB[2];
    float dsEnv = 0.0f, limGain = 1.0f, dcPeakHold = 0.0f;
    juce::SmoothedValue<float> outGainSm, bypassMix;
    int clickCounter = 0;

    // cache des paramètres
    std::atomic<float> *pInstrument, *pCuOn, *pDenoise, *pDereverb, *pDehum, *pDeclick, *pHum60,
                       *pToneOn, *pTone, *pToneScoop, *pLocut, *pHicut,
                       *pDsOn, *pDsShape, *pDsThresh, *pDsSlow,
                       *pDcOn, *pDcThresh, *pOutGain, *pLimiter,
                       *pListenCU, *pListenDS, *pListenDC, *pBypass;

    // valeurs des filtres déjà calculées
    float lastTone = 99, lastHum60 = -1, lastShape = -1;
    bool lastScoop = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakrRepairProcessor)
};
