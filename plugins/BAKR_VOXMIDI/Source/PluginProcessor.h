#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "VoxDSP.h"

class VoxMidiProcessor : public juce::AudioProcessor
{
public:
    VoxMidiProcessor();
    ~VoxMidiProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BAKR VOXMIDI"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    static juce::StringArray instrumentNames();
    static juce::StringArray scaleNames();
    static juce::StringArray keyNames();
    static juce::StringArray presetNames();
    void applyPreset (int i);
    void setParam (const juce::String& id, float plain);

    // ---- Capture MIDI ----
    struct CapNote { double start = 0, end = -1; int note = 60; int vel = 100; };
    static constexpr int maxNotes = 8192;
    std::array<CapNote, maxNotes> capNotes;
    std::atomic<int> capCount { 0 };
    std::atomic<bool> capturing { false }, clearRequest { false };
    std::atomic<double> capBpm { 120.0 };
    std::atomic<bool> capSynced { false };
    bool writeMidiFile (const juce::File& f);   // thread GUI
    juce::File makeMidiFileInDocuments();

    // ---- Affichage ----
    static constexpr int histSize = 1024;
    std::array<float, histSize> histPitch {}, histNote {};
    std::atomic<int> histPos { 0 };
    std::atomic<float> dispMidi { 0 }, dispConf { 0 }, dispRms { -100 };
    std::atomic<int> dispNote { -1 };

private:
    void handleAnalysis (int sampleIndex, juce::MidiBuffer& midi, double blockPpq, bool hostPlaying, double bpm);
    int snapToScale (float m) const;
    void sendNoteOff (int sampleIndex, juce::MidiBuffer& midi, double ppqNow);

    double sr = 48000;
    vox::PitchTracker tracker;
    vox::VoxSynth synth;
    std::array<float, 3> medBuf { 0, 0, 0 };
    int medPos = 0, curNote = -1, candNote = -1, lastInst = -1, lastBend = 8192, activeCap = -1;
    float candMs = 0, unvoicedMs = 0, lastMf = 0;
    double internalSamples = 0, capStartPpq = -1;
    juce::SmoothedValue<float> voiceG, instG, outG;

    std::atomic<float> *pMidiOut, *pSynthOn, *pInst, *pKey, *pScale, *pOctave, *pGate, *pStab, *pGlide, *pFollow,
                       *pBend, *pVelSens, *pVoice, *pInstLvl, *pTone, *pOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxMidiProcessor)
};
