#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "GuitarDSP.h"

class BakrGuitarProcessor : public juce::AudioProcessor
{
public:
    BakrGuitarProcessor();
    ~BakrGuitarProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BAKR GUITAR"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override { applyPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Presets
    static juce::StringArray getPresetNames();
    void applyPreset (int index);
    void setParam (const juce::String& id, float plainValue);
    std::atomic<int> currentPreset { 0 };

    // Clavier (notes cliquées dans l'interface + notes reçues)
    juce::MidiKeyboardState keyboardState;

    // Molettes (affichage / réglage depuis l'interface)
    std::atomic<float> pitchWheel { 0.0f }, modWheel { 0.0f };
    std::atomic<bool> uiPitchActive { false };
    // Articulation choisie par keyswitch (lue par l'interface pour mettre à jour le paramètre)
    std::atomic<int> keyswitchArtic { -1 };
    // Niveau de sortie (pour l'interface)
    std::atomic<float> outLevel { 0.0f };

    static constexpr int kKeyswitchLow = 24;   // C0 ... G#0 : articulations
    static constexpr int kFxLow = 96;          // C6 ... D#6 : bruits de guitare

private:
    struct Event { uint64_t time; bool on; int note; float vel; };

    void handleNoteOn (int note, float vel);
    void handleNoteOff (int note);
    void triggerString (int note, float vel, int artic);
    int  chooseString (int note);
    void renderVoices (float* L, float* R, int n);
    float delayTimeSeconds() const;

    double sr = 48000.0;
    std::array<bg::StringVoice, 6> strings;
    static constexpr int openNotes[6] = { 40, 45, 50, 55, 59, 64 };
    int keyToString[128];
    std::vector<int> heldKeys;
    uint64_t sampleCounter = 0, ageCounter = 0;
    std::vector<Event> scheduled, pendingChord;
    uint64_t chordWindowEnd = 0;
    bool strumAlternateUp = false;
    int lastParamArtic = 0;
    bg::Rng rng;

    bg::NoiseFx noise;
    bg::BodyFilter body;
    bg::Amp amp;
    bg::Comp comp;
    bg::StereoDelay delay;
    std::array<std::array<bg::Biquad, 3>, 2> eq;
    float lastEq[3] = { 99, 99, 99 };
    juce::dsp::Chorus<float> chorus;
    juce::Reverb reverb;
    juce::SmoothedValue<float> outGain;
    float vibPhase = 0.0f;
    float smoothedBend = 0.0f;

    std::atomic<float> *pGuitar, *pAttack, *pRelease, *pBright, *pWidth, *pDelay, *pReverb, *pOut, *pArtic,
                       *pFxAmp, *pFxEq, *pFxComp, *pFxChorus, *pFxDelay, *pFxReverb,
                       *pStrum, *pStrumDir, *pStrumSpace, *pNoise,
                       *pDrive, *pEqLow, *pEqMid, *pEqHigh, *pComp, *pChorus, *pDelayTime;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakrGuitarProcessor)
};
