#pragma once
#include <JuceHeader.h>
#include "BakrDSP.h"
#include <array>
#include <atomic>

namespace bakr
{
    struct PresetDef
    {
        juce::String name;
        std::vector<std::pair<const char*, float>> values; // valeurs réelles (pas normalisées)
    };

    // Processeur de base : suréchantillonnage ×2, Mix (si "mix" existe), mesures, presets, état
    class BaseProcessor : public juce::AudioProcessor
    {
    public:
        BaseProcessor (const juce::String& pluginName,
                       juce::AudioProcessorValueTreeState::ParameterLayout layout,
                       std::vector<PresetDef> presets, bool oversample = true);

        void prepareToPlay (double sampleRate, int samplesPerBlock) override;
        void releaseResources() override {}
        bool isBusesLayoutSupported (const BusesLayout&) const override;
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

        bool hasEditor() const override { return true; }
        const juce::String getName() const override { return name; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        bool isMidiEffect() const override { return false; }
        double getTailLengthSeconds() const override { return 0.0; }

        int getNumPrograms() override { return juce::jmax (1, (int) presets.size()); }
        int getCurrentProgram() override { return currentPreset; }
        void setCurrentProgram (int index) override;
        const juce::String getProgramName (int index) override;
        void changeProgramName (int, const juce::String&) override {}
        juce::StringArray getPresetNames() const;

        void getStateInformation (juce::MemoryBlock&) override;
        void setStateInformation (const void*, int) override;

        float p (const char* id) const noexcept { return apvts.getRawParameterValue (id)->load(); }
        bool  has (const char* id) const noexcept { return apvts.getRawParameterValue (id) != nullptr; }

        juce::AudioProcessorValueTreeState apvts;

        // mesures (dB) lues par l'interface : réduction de gain, entrée, sortie (L/R)
        std::array<std::atomic<float>, 2> grDb { 0.0f, 0.0f }, inDb { -80.0f, -80.0f }, outDb { -80.0f, -80.0f };
        std::array<std::atomic<float>, 4> extra { 0.0f, 0.0f, 0.0f, 0.0f };   // mesures spécifiques (LUFS, clip...)

    protected:
        virtual void prepareCore (double fsOS, int maxBlockOS, int numChannels) = 0;
        // traitement au double de la fréquence d'échantillonnage
        virtual void processCore (juce::dsp::AudioBlock<float>& block, double fsOS) = 0;

        void reportGr (int ch, float db) noexcept { grAcc[(size_t) juce::jlimit (0, 1, ch)] = juce::jmax (grAcc[(size_t) juce::jlimit (0, 1, ch)], db); }

    private:
        juce::String name;
        std::vector<PresetDef> presets;
        bool useOS = true;   // false = traitement direct, latence 0
        int currentPreset = 0;

        juce::dsp::Oversampling<float> oversampler { 2, 1,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
        juce::dsp::DryWetMixer<float> dryWet { 64 };
        double fs = 44100.0;
        std::array<float, 2> grAcc { 0, 0 };
        std::array<float, 2> grHold { 0, 0 };
    };
}

// Aides pour déclarer les paramètres en une ligne
namespace bakr
{
    using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;

    inline void addFloat (Layout& l, const char* id, const char* nm, float lo, float hi, float def,
                          const char* unit = "", float centre = 0.0f, int decimals = 1)
    {
        juce::NormalisableRange<float> r (lo, hi);
        if (centre > lo && centre < hi) r.setSkewForCentre (centre);
        const juce::String u (unit);
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, nm, r, def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction ([u, decimals] (float v, int)
            {
                if (u == "Hz" && v >= 1000.0f) return juce::String (v / 1000.0f, 2) + " kHz";
                if (u == "ms" && v >= 1000.0f) return juce::String (v / 1000.0f, 2) + " s";
                const juce::String num = decimals <= 0 ? juce::String (juce::roundToInt (v)) : juce::String (v, decimals);
                return num + (u.isEmpty() ? "" : " " + u);
            })));
    }
    inline void addChoice (Layout& l, const char* id, const char* nm, juce::StringArray c, int def)
    {
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, nm, c, def));
    }
    inline void addBool (Layout& l, const char* id, const char* nm, bool def)
    {
        l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, nm, def));
    }
}
