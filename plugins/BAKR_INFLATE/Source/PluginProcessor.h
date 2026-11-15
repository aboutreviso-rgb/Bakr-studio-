#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

// BAKR INFLATE : rend le son plus fort et plus plein sans compresser
// (mise en forme d'onde douce, réglable par Effet et Courbe), avec mode 3 bandes.
class BakrInflateProcessor : public juce::AudioProcessor
{
public:
    BakrInflateProcessor();
    ~BakrInflateProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BAKR INFLATE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override { applyPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    static juce::StringArray getPresetNames();
    void applyPreset (int index);
    int currentPreset = 0;

    // Comparaison A / B
    juce::ValueTree slotA, slotB;
    bool slotBActive = false;
    void switchAB (bool toB);

    // Vumètres (crête, linéaire) lus par l'interface
    std::atomic<float> inL { 0 }, inR { 0 }, outL { 0 }, outR { 0 }, effectMeter { 0 };
    std::atomic<bool> clipped { false };

    // Courbe de transfert (pour l'affichage)
    static float shape (float x, float curve, bool clip);

private:
    void processChannelSamples (float* data, int n, float A, float B, float C, float D, bool clip, float wet);

    std::atomic<float> *pIn, *pEffect, *pCurve, *pOut, *pClip, *pBand, *pEffectIn;
    juce::dsp::Oversampling<float> oversampling { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };
    juce::SmoothedValue<float> inGain, outGain, wetSm, curveSm;

    // séparation en 3 bandes (Linkwitz-Riley 4e ordre, somme à phase plate)
    juce::dsp::LinkwitzRileyFilter<float> lp1, hp1, lp2, hp2, ap2;
    juce::AudioBuffer<float> bandLow, bandMid, bandHigh;
    float rmsIn = 0, rmsOut = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakrInflateProcessor)
};
