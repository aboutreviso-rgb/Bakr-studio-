#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

// BAKR VOXCHAIN : chaîne vocale complète en 5 modules
// Compression -> Amélioration (de-ess, de-noise, air) -> EQ -> Doubleur -> Espace (reverb + delay)
class BakrVoxChainProcessor : public juce::AudioProcessor
{
public:
    BakrVoxChainProcessor();
    ~BakrVoxChainProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "BAKR VOXCHAIN"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int i) override { applyPreset (i); }
    const juce::String getProgramName (int i) override;
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;
    static juce::StringArray getPresetNames();
    void applyPreset (int i);
    std::atomic<int> currentPreset { 0 };

    static juce::StringArray reverbTypes() { return { "Room", "Plate", "Hall", "Cathedrale", "Ambiance" }; }
    static juce::StringArray delayTypes()  { return { "Moderne", "Tape", "Ping-pong", "Slap" }; }
    static juce::StringArray delayTimes()  { return { "1/2", "1/4", "1/8 pointee", "1/8", "1/16" }; }

    // vumètres
    std::atomic<float> inLevel { 0 }, gainReduction { 0 }, outLevel { 0 }, deEssGR { 0 }, deNoiseGR { 0 };

private:
    struct Bq
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        inline float p (float x) { float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
        void reset() { z1 = z2 = 0; }
        void setCoefs (const juce::dsp::IIR::Coefficients<float>& c)
        {
            auto* r = c.coefficients.begin();   // b0 b1 b2 a0 a1 a2 (normalisés, a0 omis selon taille)
            const auto n = c.coefficients.size();
            if (n == 5) { b0 = r[0]; b1 = r[1]; b2 = r[2]; a1 = r[3]; a2 = r[4]; }
            else if (n == 3) { b0 = r[0]; b1 = r[1]; b2 = 0; a1 = r[2]; a2 = 0; }
        }
    };

    void updateFilters();
    float delaySeconds() const;
    inline float readDelay (const std::vector<float>& b, int wp, float d) const;

    double sr = 48000.0;
    std::atomic<float>* P (const char* id) { return apvts.getRawParameterValue (id); }

    // compression
    float compEnv = 0;
    juce::SmoothedValue<float> inG, outG;
    // amélioration
    std::array<Bq, 2> deessLp, deessHp, deessDet, airShelf, excHp;
    float deessEnv[2] = { 0, 0 }, fullEnv[2] = { 0, 0 }, noiseEnv = 0, noiseGain = 1;
    // EQ
    std::array<Bq, 2> eqHigh, eqMid, eqLow1, eqLow2, adlibLp, adlibPk;
    float lastEq[6] = { -999, -999, -999, -999, -999, -999 };
    // doubleur
    std::vector<float> dblBuf; int dblMask = 0, dblPos = 0;
    std::array<float, 4> lfoPh { 0.0f, 0.37f, 0.71f, 0.15f };
    std::array<Bq, 4> dblHp;
    float lastDblLow = -1;
    // espace
    std::vector<float> dlL, dlR; int dlMask = 0, dlPos = 0;
    float dlFbL = 0, dlFbR = 0, tapeLpL = 0, tapeLpR = 0, wowPh = 0, smDelay = 0.3f;
    juce::Reverb reverb;
    std::array<Bq, 2> revHp;
    juce::AudioBuffer<float> revBuf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BakrVoxChainProcessor)
};
