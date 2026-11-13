#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

//==============================================================================
// BAKR TUBECOMP — compresseur à lampes variable-mu
//  - Étage d'entrée triode (saturation asymétrique, harmoniques paires)
//  - Cellule variable-mu : le ratio augmente avec le niveau (genou très doux)
//  - Modes Single / Double / Triple (1, 2 ou 3 constantes de relâchement)
//  - Recovery Fast / Slow
//  - Transfo de sortie (saturation des graves + douceur des aigus)
//  - Side-chain avancé : interne/externe, gain SC, 6 modes de détection,
//    HPF / EQ en cloche / LPF, Listen
//==============================================================================
class BakrTubeCompAudioProcessor : public juce::AudioProcessor
{
public:
    BakrTubeCompAudioProcessor();
    ~BakrTubeCompAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BAKR TUBECOMP"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    // Mesures pour le VU-mètre (lues par l'interface)
    std::atomic<float> meterInDb  { -60.0f };
    std::atomic<float> meterOutDb { -60.0f };
    std::atomic<float> meterGrDb  { 0.0f };

    static juce::StringArray presetNames();

private:
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float z1 = 0, z2 = 0;
        inline float process (float x) noexcept
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
        void reset() { z1 = z2 = 0; }
        void setHighPass (double fs, double f, double q);
        void setLowPass  (double fs, double f, double q);
        void setPeak     (double fs, double f, double q, double gainDb);
        void setLowShelf (double fs, double f, double gainDb);
        void setIdentity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }
    };

    struct ChannelState
    {
        // détecteur
        float env1 = 0, env2 = 0, env3 = 0; // en dB de réduction (positif)
        float rms = 0;
        float optoCharge = 0;  // mémoire de la cellule opto
        Biquad scHp, scPeak, scLp;
        // couleur
        float dcIn = 0, dcOut = 0;
        float xfmrLf = 0;      // passe-bas pour la partie graves du transfo
        float hfState = 0;     // douceur des aigus
        Biquad weight;         // légère bosse de grave du transfo
    };

    void updateFilters();
    void applyPreset (int index);
    static float computeStaticGr (float levelDb) noexcept;
    inline float triode (float x, float drive) const noexcept;
    inline float transformer (ChannelState& st, float x, float amount) noexcept;

    std::array<ChannelState, 2> ch;
    juce::dsp::Oversampling<float> oversampler { 2, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    juce::dsp::DryWetMixer<float> dryWet { 64 };

    juce::AudioBuffer<float> scBuffer;
    juce::AudioBuffer<float> grBuffer;   // réduction de gain par canal (linéaire) à fs
    juce::AudioBuffer<float> listenBuffer;

    double fs = 44100.0;
    int currentPreset = 0;

    juce::SmoothedValue<float> inGainSm, outGainSm, scGainSm;

    // cache des paramètres de filtre
    float lastHp = -1, lastPkF = -1, lastPkG = -99, lastLp = -1, lastQ = -1;

    float meterInAcc = 0, meterOutAcc = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakrTubeCompAudioProcessor)
};
