// BAKR INSTINCT — assistant vocal : il ÉCOUTE ta voix puis règle tout seul la chaîne
// (coupe-bas, EQ, de-esser, compresseur, niveau de sortie). Zéro latence.
// Analyse : hauteur (YIN), spectre moyen comparé à une cible de style, sibilance, dynamique, débit des syllabes.
#include "../../Common/BakrUI.h"
#include "../Calibre/CalibreDSP.h"

#define T(x) juce::String::fromUTF8 (x)

using namespace calib;

namespace
{
    const juce::StringArray kStyles { "Naturel", "Pop / R&B", T("Afro (Afrobeats, Coupé-décalé)"), "Mandingue / Tradi", "Rap", T("Douce / Chœurs") };

    // cibles de spectre vocal (dB relatifs aux médiums 300 Hz - 3 kHz)
    inline float styleTarget (int style, double f)
    {
        static const std::vector<std::pair<float, float>> base { { 100, -4 }, { 200, -1 }, { 350, 0 }, { 700, 0.5f }, { 1200, -0.5f }, { 2000, -3.5f }, { 3500, -6 }, { 5000, -8.5f }, { 8000, -13 }, { 12000, -18 }, { 16000, -24 } };
        float t = interpPoints (base, f);
        auto bump = [f] (double fc, double oct, double g) { const double x = std::log2 (f / fc) / oct; return (float) (g * std::exp (-x * x)); };
        auto shelf = [f] (double fc, double g) { return (float) (g / (1.0 + std::exp (-std::log2 (f / fc) * 3.0))); };
        switch (style)
        {
            case 1: t += bump (4000, 0.8, 1.5) + shelf (10000, 3.0) - bump (350, 0.7, 1.0); break;           // Pop / R&B
            case 2: t += bump (3500, 0.7, 2.5) + shelf (9000, 3.5) - bump (400, 0.6, 1.5); break;            // Afro
            case 3: t += bump (2500, 0.8, 2.0) + bump (220, 0.7, 1.0) + shelf (9000, 1.5); break;            // Mandingue
            case 4: t += bump (180, 0.8, 1.5) + bump (3000, 0.7, 2.5) + shelf (10000, 1.5); break;           // Rap
            case 5: t += -bump (2800, 0.8, 1.5) + shelf (11000, 3.5) - bump (250, 0.8, 1.5); break;          // Douce / chœurs
            default: break;
        }
        return t;
    }

    constexpr int kEqBands = 6;
}

//==============================================================================
class Instinct : public juce::AudioProcessor, private juce::Thread, private juce::AsyncUpdater
{
public:
    enum State { Idle = 0, Listening, Analysing, Done };

    struct Result
    {
        bool valid = false;
        float f0 = 0, f0low = 0, lowCut = 80;
        std::vector<Band> eq;
        float sibDb = -60, dessThr = -30;
        float compThr = -20, ratio = 3, attackMs = 10, releaseMs = 120, makeup = 0;
        juce::String pitchTxt, pitchSub, toneTxt, toneSub, sibTxt, sibSub, compTxt, compSub, cutTxt, cutSub;
        Curve ltas, target;
    };

    Instinct()
        : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          Thread ("BAKR INSTINCT analyse"),
          apvts (*this, nullptr, "BAKR_INSTINCT", layout())
    {
        startThread();
    }
    ~Instinct() override { stopThread (3000); cancelPendingUpdate(); }

    static juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        using namespace bakr;
        Layout l;
        addChoice (l, "style", "Style", kStyles, 2);
        addFloat (l, "amount", "Intensite", 0, 150, 100, "%", 0, 0);
        addFloat (l, "lowcut", "Coupe-bas", 20, 250, 80, "Hz", 90, 0);
        addFloat (l, "dess", "De-esser seuil", -50, 0, -30, "dB");
        addFloat (l, "compThr", "Compresseur seuil", -50, 0, -20, "dB");
        addFloat (l, "ratio", "Ratio", 1, 10, 3, ":1", 3, 1);
        addFloat (l, "attack", "Attaque", 0.5f, 50, 10, "ms", 8, 1);
        addFloat (l, "release", "Relachement", 20, 600, 120, "ms", 150, 0);
        addFloat (l, "warmth", "Chaleur", 0, 100, 15, "%", 0, 0);
        addFloat (l, "out", "Sortie", -24, 24, 0, "dB");
        addFloat (l, "mix", "Mix", 0, 100, 100, "%", 0, 0);
        addBool (l, "bypass", "Comparer (avant)", false);
        return l;
    }

    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    const juce::String getName() const override { return "BAKR INSTINCT"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;

    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        const auto out = l.getMainOutputChannelSet();
        return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == out;
    }

    void prepareToPlay (double sampleRate, int) override
    {
        fs = sampleRate;
        if (state.load() == Listening) state.store (Idle);
        listenBuf.assign ((size_t) (sampleRate * 20.0), 0.0f);
        listenPos.store (0);
        for (auto& c : ch) c.reset();
        bypassMix.reset (sampleRate, 0.05);
        dryMix.reset (sampleRate, 0.05);
        coefDirty.store (true);
    }
    void releaseResources() override {}

    //--------------------------------------------------------------------------
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals nd;
        const int n = buffer.getNumSamples(), nCh = juce::jmin (2, buffer.getNumChannels());
        if (n <= 0 || nCh <= 0) return;

        // visuel + écoute
        float pk = 0;
        for (int i = 0; i < n; ++i)
        {
            float m = buffer.getSample (0, i);
            if (nCh > 1) m = 0.5f * (m + buffer.getSample (1, i));
            scope[(size_t) (scopeW & (kScope - 1))] = m;
            ++scopeW;
            pk = juce::jmax (pk, std::abs (m));
        }
        pushWave (pk);

        if (state.load() == Listening && pk > juce::Decibels::decibelsToGain (-50.0f))
        {
            int pos = listenPos.load();
            const int cap = (int) listenBuf.size();
            for (int i = 0; i < n && pos < cap; ++i)
            {
                float m = buffer.getSample (0, i);
                if (nCh > 1) m = 0.5f * (m + buffer.getSample (1, i));
                listenBuf[(size_t) pos++] = m;
            }
            listenPos.store (pos);
            if (pos >= (int) (fs * kListenSec)) state.store (Analysing);
        }

        if (coefDirty.exchange (false)) updateCoefs();

        const bool byp = p ("bypass") > 0.5f;
        bypassMix.setTargetValue (byp ? 0.0f : 1.0f);
        dryMix.setTargetValue (p ("mix") * 0.01f);
        const float dThr = p ("dess"), cThr = p ("compThr"), ratio = p ("ratio");
        const float cA = coefMs (p ("attack")), cR = coefMs (p ("release")), cRms = coefMs (6.0f);
        const float dA = coefMs (0.5f), dR = coefMs (60.0f), dRms = coefMs (1.5f);
        const float warm = p ("warmth") * 0.01f;
        const float outG = juce::Decibels::decibelsToGain (p ("out") + makeupDb.load());

        float grMax = 0, dsMax = 0;
        for (int i = 0; i < n; ++i)
        {
            const float wet = bypassMix.getNextValue() * dryMix.getNextValue();
            for (int c = 0; c < nCh; ++c)
            {
                auto& s = ch[(size_t) c];
                const float x = buffer.getSample (c, i);
                double y = s.hp2.process (s.hp1.process (x));
                for (int b = 0; b < kEqBands; ++b) y = s.eq[(size_t) b].process (y);

                // de-esser
                const double band = s.ds2.process (s.ds1.process (y));
                s.dsRms = dRms * s.dsRms + (1 - dRms) * (float) (band * band);
                const float dLv = juce::Decibels::gainToDecibels (std::sqrt (s.dsRms) * 1.41f + 1e-9f);
                const float dTgt = juce::jmin (14.0f, kneeGr (dLv - dThr, 4.0f, 4.0f));
                s.dsGr = dTgt > s.dsGr ? dA * s.dsGr + (1 - dA) * dTgt : dR * s.dsGr + (1 - dR) * dTgt;
                y -= band * (1.0 - juce::Decibels::decibelsToGain (-s.dsGr));
                dsMax = juce::jmax (dsMax, s.dsGr);

                // compresseur (RMS + crête)
                s.cRms = cRms * s.cRms + (1 - cRms) * (float) (y * y);
                const float lv = juce::Decibels::gainToDecibels (juce::jmax (std::sqrt (s.cRms) * 1.41f, (float) std::abs (y) * 0.7f) + 1e-9f);
                const float tgt = juce::jmin (30.0f, kneeGr (lv - cThr, ratio, 6.0f));
                s.cGr = tgt > s.cGr ? cA * s.cGr + (1 - cA) * tgt : cR * s.cGr + (1 - cR) * tgt;
                y *= juce::Decibels::decibelsToGain (-s.cGr);
                grMax = juce::jmax (grMax, s.cGr);

                // chaleur (saturation douce, paire)
                if (warm > 0.001f)
                {
                    const double d = 1.0 + warm * 3.0;
                    y = std::tanh (y * d + warm * 0.08) / d * (1.0 + warm * 0.4) - std::tanh (warm * 0.08) / d;
                }
                float out = (float) y * outG;
                // sécurité anti-saturation douce au-dessus de -1 dBFS
                const float lim = 0.891f;
                if (std::abs (out) > lim) out = std::copysign (lim + (1.0f - lim) * std::tanh ((std::abs (out) - lim) / (1.0f - lim)), out);
                buffer.setSample (c, i, x + (out - x) * wet);
            }
        }
        grDb.store (grMax);
        dsDb.store (dsMax);
        for (int c = nCh; c < buffer.getNumChannels(); ++c) buffer.copyFrom (c, 0, buffer, 0, 0, n);
    }

    //--------------------------------------------------------------------------
    void startListening()
    {
        if (state.load() == Analysing) return;
        listenPos.store (0);
        state.store (Listening);
    }
    void stopListening()
    {
        if (state.load() != Listening) return;
        if (listenPos.load() > fs * 3.0) state.store (Analysing);
        else state.store (result.valid ? Done : Idle);
    }
    float listenProgress() const { return (float) juce::jlimit (0.0, 1.0, listenPos.load() / (fs * kListenSec)); }

    Result getResult() const { const juce::ScopedLock sl (resLock); return result; }
    int resultVersion() const { return resVersion.load(); }
    std::vector<Band> getEq() const { const juce::ScopedLock sl (resLock); return eqBands; }
    juce::String getError() const { const juce::ScopedLock sl (resLock); return error; }

    // pour l'interface
    static constexpr int kScope = 4096, kWave = 600;
    std::array<float, kScope> scope {};
    std::atomic<juce::uint32> scopeW { 0 };
    std::array<std::atomic<float>, kWave> wave {};
    std::atomic<int> waveW { 0 };
    std::array<std::atomic<bool>, kWave> waveListen {};
    std::atomic<float> grDb { 0 }, dsDb { 0 };
    std::atomic<int> state { Idle };
    double fs = 48000.0;
    juce::AudioProcessorValueTreeState apvts;

    void getStateInformation (juce::MemoryBlock& dest) override
    {
        auto st = apvts.copyState();
        juce::ValueTree r ("RESULT");
        {
            const juce::ScopedLock sl (resLock);
            r.setProperty ("valid", result.valid, nullptr);
            r.setProperty ("makeup", makeupDb.load(), nullptr);
            juce::String e;
            for (auto& b : eqBands) e << b.f << "," << b.q << "," << b.g << ";";
            r.setProperty ("eq", e, nullptr);
            for (auto* k : { "pitchTxt", "pitchSub", "toneTxt", "toneSub", "sibTxt", "sibSub", "compTxt", "compSub", "cutTxt", "cutSub" })
                r.setProperty (k, field (result, k), nullptr);
            if ((int) result.ltas.size() == kGrid)
            {
                juce::MemoryBlock mb (result.ltas.data(), result.ltas.size() * sizeof (float));
                r.setProperty ("ltas", mb.toBase64Encoding(), nullptr);
            }
        }
        st.appendChild (r, nullptr);
        if (auto xml = st.createXml()) copyXmlToBinary (*xml, dest);
    }

    void setStateInformation (const void* data, int size) override
    {
        auto xml = getXmlFromBinary (data, size);
        if (xml == nullptr) return;
        auto st = juce::ValueTree::fromXml (*xml);
        auto r = st.getChildWithName ("RESULT");
        if (r.isValid()) st.removeChild (r, nullptr);
        apvts.replaceState (st);
        if (r.isValid())
        {
            const juce::ScopedLock sl (resLock);
            result = {};
            result.valid = (bool) r.getProperty ("valid");
            makeupDb.store ((float) r.getProperty ("makeup"));
            eqBands.clear();
            for (auto& t : juce::StringArray::fromTokens (r.getProperty ("eq").toString(), ";", ""))
            {
                auto v = juce::StringArray::fromTokens (t, ",", "");
                if (v.size() == 3) eqBands.push_back ({ v[0].getFloatValue(), v[1].getFloatValue(), v[2].getFloatValue() });
            }
            result.eq = eqBands;
            for (auto* k : { "pitchTxt", "pitchSub", "toneTxt", "toneSub", "sibTxt", "sibSub", "compTxt", "compSub", "cutTxt", "cutSub" })
                field (result, k) = r.getProperty (k).toString();
            juce::MemoryBlock mb;
            if (mb.fromBase64Encoding (r.getProperty ("ltas").toString()) && mb.getSize() == (size_t) kGrid * sizeof (float))
            {
                result.ltas.resize ((size_t) kGrid);
                std::memcpy (result.ltas.data(), mb.getData(), mb.getSize());
            }
            if (result.valid) state.store (Done);
        }
        coefDirty.store (true);
        resVersion.fetch_add (1);
    }

private:
    static constexpr double kListenSec = 12.0;

    static juce::String& field (Result& r, const juce::String& k)
    {
        if (k == "pitchTxt") return r.pitchTxt; if (k == "pitchSub") return r.pitchSub;
        if (k == "toneTxt") return r.toneTxt;   if (k == "toneSub") return r.toneSub;
        if (k == "sibTxt") return r.sibTxt;     if (k == "sibSub") return r.sibSub;
        if (k == "compTxt") return r.compTxt;   if (k == "compSub") return r.compSub;
        if (k == "cutTxt") return r.cutTxt;     return r.cutSub;
    }
    static juce::String field (const Result& r, const juce::String& k) { return field (const_cast<Result&> (r), k); }

    float coefMs (float ms) const { return (float) std::exp (-1.0 / (juce::jmax (0.05, (double) ms) * 0.001 * fs)); }
    static float kneeGr (float over, float ratio, float knee)
    {
        const float slope = 1.0f - 1.0f / ratio;
        if (over <= -knee * 0.5f) return 0.0f;
        if (over >= knee * 0.5f) return over * slope;
        const float x = over + knee * 0.5f;
        return slope * x * x / (2.0f * knee);
    }

    void pushWave (float pk)
    {
        wavePeak = juce::jmax (wavePeak, pk);
        waveCount += 1;
        if (waveCount >= 4)   // ≈ 4 blocs par colonne
        {
            const int w = waveW.load();
            wave[(size_t) (w % kWave)].store (wavePeak);
            waveListen[(size_t) (w % kWave)].store (state.load() == Listening);
            waveW.store (w + 1);
            wavePeak = 0; waveCount = 0;
        }
    }

    struct BQ
    {
        Coefs k; double z1 = 0, z2 = 0;
        inline double process (double x) noexcept { const double y = k.b0 * x + z1; z1 = k.b1 * x - k.a1 * y + z2; z2 = k.b2 * x - k.a2 * y; return y; }
    };
    struct Chan
    {
        BQ hp1, hp2, ds1, ds2;
        std::array<BQ, kEqBands> eq;
        float dsRms = 0, dsGr = 0, cRms = 0, cGr = 0;
        void reset() { for (auto* b : { &hp1, &hp2, &ds1, &ds2 }) b->z1 = b->z2 = 0; for (auto& b : eq) b.z1 = b.z2 = 0; dsRms = dsGr = cRms = cGr = 0; }
    };

    static Coefs hpCoefs (double fs, double f, double q)
    {
        Coefs c; const double w = 2 * kPiD * f / fs, cs = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
        c.b0 = (1 + cs) / 2 / a0; c.b1 = -(1 + cs) / a0; c.b2 = (1 + cs) / 2 / a0; c.a1 = -2 * cs / a0; c.a2 = (1 - al) / a0; return c;
    }
    static Coefs lpCoefs (double fs, double f, double q)
    {
        Coefs c; const double w = 2 * kPiD * f / fs, cs = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
        c.b0 = (1 - cs) / 2 / a0; c.b1 = (1 - cs) / a0; c.b2 = (1 - cs) / 2 / a0; c.a1 = -2 * cs / a0; c.a2 = (1 - al) / a0; return c;
    }

    void updateCoefs()
    {
        std::vector<Band> bands;
        { const juce::SpinLock::ScopedTryLockType tl (eqSpin); if (tl.isLocked()) bands = eqAudio; else { coefDirty.store (true); return; } }
        const float amt = lastAmount = p ("amount") * 0.01f;
        const double lc = lastLowCut = p ("lowcut");
        for (auto& s : ch)
        {
            s.hp1.k = hpCoefs (fs, lc, 0.541); s.hp2.k = hpCoefs (fs, lc, 1.307);
            s.ds1.k = hpCoefs (fs, 5000.0, 0.6); s.ds2.k = lpCoefs (fs, juce::jmin (11000.0, fs * 0.45), 0.6);
            for (int b = 0; b < kEqBands; ++b)
                s.eq[(size_t) b].k = b < (int) bands.size() ? peakCoefs (fs, bands[(size_t) b].f, bands[(size_t) b].q, bands[(size_t) b].g * amt) : Coefs();
        }
    }

    void handleAsyncUpdate() override {}

    // fil d'analyse
    void run() override
    {
        while (! threadShouldExit())
        {
            wait (40);
            // suivi des paramètres qui demandent de recalculer les filtres
            if (std::abs (p ("amount") * 0.01f - lastAmount) > 1e-4f || std::abs (p ("lowcut") - lastLowCut) > 0.01) coefDirty.store (true);
            if (state.load() != Analysing) continue;

            const int len = juce::jmin (listenPos.load(), (int) listenBuf.size());
            std::vector<float> x (listenBuf.begin(), listenBuf.begin() + len);
            Result r;
            juce::String err;
            analyse (x, fs, (int) p ("style"), r, err);
            if (r.valid)
            {
                {
                    const juce::ScopedLock sl (resLock);
                    result = r; eqBands = r.eq; error.clear();
                }
                { const juce::SpinLock::ScopedLockType sl (eqSpin); eqAudio = r.eq; }
                makeupDb.store (r.makeup);
                pendingApply.store (true);
                state.store (Done);
            }
            else
            {
                { const juce::ScopedLock sl (resLock); error = err; }
                state.store (result.valid ? Done : Idle);
            }
            coefDirty.store (true);
            resVersion.fetch_add (1);
        }
    }

public:
    // applique les réglages trouvés aux paramètres (appelé par l'interface, fil des messages)
    void applyPending()
    {
        if (! pendingApply.exchange (false)) return;
        const auto r = getResult();
        auto set = [this] (const char* id, float v)
        {
            if (auto* prm = apvts.getParameter (id)) { prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 (v)); prm->endChangeGesture(); }
        };
        set ("lowcut", r.lowCut); set ("dess", r.dessThr); set ("compThr", r.compThr); set ("ratio", r.ratio);
        set ("attack", r.attackMs); set ("release", r.releaseMs); set ("out", 0.0f); set ("bypass", 0.0f);
    }
    bool hasPending() const { return pendingApply.load(); }

    // ---------------------------------------------------------------------------
    static void analyse (const std::vector<float>& x, double fs, int style, Result& r, juce::String& err)
    {
        const int N = 2048, hop = 1024;
        if ((int) x.size() < N * 8) { err = T("Pas assez de voix entendue : lance la lecture de la piste et réessaie."); return; }
        juce::dsp::FFT fft (11);
        std::vector<float> win ((size_t) N);
        for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * i / (N - 1));

        // niveau des trames
        std::vector<float> frameDb;
        for (size_t s = 0; s + N <= x.size(); s += hop)
        {
            double e = 0; for (int i = 0; i < N; ++i) e += x[s + i] * x[s + i];
            frameDb.push_back ((float) (10.0 * std::log10 (e / N + 1e-12)));
        }
        const float maxDb = *std::max_element (frameDb.begin(), frameDb.end());
        const float gate = juce::jmax (-50.0f, maxDb - 30.0f);

        std::vector<double> ltas ((size_t) N / 2, 0.0), ltasAll ((size_t) N / 2, 0.0);
        int nVoiced = 0;
        std::vector<float> f0s, act, sib;
        int nAct = 0;
        std::vector<cplx> in ((size_t) N), out ((size_t) N);
        const int ds = juce::jmax (1, (int) std::round (fs / 12000.0));   // YIN à ~12 kHz
        const double fsd = fs / ds;
        const int M = N / ds, tauMin = (int) (fsd / 1100.0), tauMax = juce::jmin (M / 2, (int) (fsd / 65.0));

        for (size_t fi = 0, s = 0; s + N <= x.size(); ++fi, s += hop)
        {
            if (frameDb[fi] < gate) continue;
            ++nAct;
            act.push_back (frameDb[fi]);
            for (int i = 0; i < N; ++i) in[(size_t) i] = x[s + i] * win[(size_t) i];
            fft.perform (in.data(), out.data(), false);
            double eTot = 0, eSib = 0;
            const double bin = fs / N;
            for (int k = 1; k < N / 2; ++k)
            {
                const double pw = std::norm (out[(size_t) k]);
                ltasAll[(size_t) k] += pw;
                eTot += pw;
                if (k * bin >= 5000 && k * bin <= 10000) eSib += pw;
            }
            sib.push_back ((float) (10.0 * std::log10 (eSib / (N * N * 0.25) + 1e-12)));

            // YIN (décimé, moyenne simple = passe-bas grossier)
            std::vector<float> y ((size_t) M);
            for (int i = 0; i < M; ++i) { float a = 0; for (int j = 0; j < ds; ++j) a += x[s + (size_t) (i * ds + j)]; y[(size_t) i] = a / ds; }
            std::vector<float> d ((size_t) tauMax + 1, 0.0f);
            for (int tau = 1; tau <= tauMax; ++tau)
            {
                double acc = 0;
                for (int i = 0; i + tau < M; ++i) { const double v = y[(size_t) i] - y[(size_t) (i + tau)]; acc += v * v; }
                d[(size_t) tau] = (float) acc;
            }
            double run = 0; int found = -1;
            std::vector<float> cm ((size_t) tauMax + 1, 1.0f);
            for (int tau = 1; tau <= tauMax; ++tau) { run += d[(size_t) tau]; cm[(size_t) tau] = (float) (d[(size_t) tau] * tau / (run + 1e-20)); }
            for (int tau = juce::jmax (2, tauMin); tau < tauMax; ++tau)
                if (cm[(size_t) tau] < 0.15f) { while (tau + 1 < tauMax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau; found = tau; break; }
            if (found > 0)
            {
                const float a = cm[(size_t) found - 1], b = cm[(size_t) found], c = cm[(size_t) found + 1];
                const float den = a - 2 * b + c;
                const float t = found + (std::abs (den) > 1e-9f ? 0.5f * (a - c) / den : 0.0f);
                f0s.push_back ((float) (fsd / t));
                ++nVoiced;   // trame chantée : sert au calcul de la tonalité (les « S » sont exclus)
                for (int k = 1; k < N / 2; ++k) ltas[(size_t) k] += std::norm (out[(size_t) k]);
            }
        }
        if (nAct < 20) { err = T("Signal trop faible ou trop court : monte le volume de la voix et réécoute."); return; }

        auto pct = [] (std::vector<float> v, float q) { if (v.empty()) return 0.0f; std::sort (v.begin(), v.end()); return v[(size_t) juce::jlimit (0, (int) v.size() - 1, (int) std::round (q * (v.size() - 1)))]; };

        // --- hauteur & coupe-bas
        r.f0 = pct (f0s, 0.5f);
        r.f0low = pct (f0s, 0.05f);
        const float voiced = f0s.size() / (float) nAct;
        if (f0s.size() < 10) { r.f0 = 0; r.f0low = 120; }
        r.lowCut = juce::jlimit (50.0f, 200.0f, std::round (r.f0low * 0.72f / 2.0f) * 2.0f);
        if (style == 4) r.lowCut = juce::jmin (r.lowCut, 90.0f);
        r.pitchTxt = r.f0 > 0 ? juce::String (juce::roundToInt (r.f0)) + " Hz" : "-";
        r.pitchSub = r.f0 <= 0 ? T("hauteur non détectée") : (r.f0 < 150 ? T("voix grave") : r.f0 < 260 ? T("voix médium") : T("voix aiguë"));
        if (voiced < 0.3f && r.f0 > 0) r.pitchSub << T(" (beaucoup de parlé)");
        r.cutTxt = juce::String (juce::roundToInt (r.lowCut)) + " Hz";
        r.cutSub = T("sous la note la plus basse");

        // --- tonalité : spectre moyen vs cible du style
        if (nVoiced >= 15) for (auto& v : ltas) v /= nVoiced;
        else { ltas = ltasAll; for (auto& v : ltas) v /= nAct; }
        Curve spec = powerToGrid (ltas, fs, N);
        const double ref = meanRange (spec, 300, 3000);
        for (auto& v : spec) v -= (float) ref;
        r.ltas = spec;
        r.target.resize ((size_t) kGrid);
        for (int i = 0; i < kGrid; ++i) r.target[(size_t) i] = styleTarget (style, gridFreq (i));
        FitParams fp;
        fp.lo = juce::jmax (120.0, (double) r.lowCut * 1.4); fp.hi = juce::jmin (12000.0, fs * 0.42);
        fp.maxBoost = 5.0; fp.maxCut = 7.0; fp.amount = 0.75; fp.maxBands = kEqBands;
        r.eq = fitBands (spec, r.target, fp, fs);
        const Band* up = nullptr; const Band* dn = nullptr;
        for (auto& b : r.eq) { if (b.g > 0 && (! up || b.g > up->g)) up = &b; if (b.g < 0 && (! dn || b.g < dn->g)) dn = &b; }
        auto fstr = [] (float f) { return f >= 1000 ? juce::String (f / 1000.0f, 1) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz"; };
        r.toneTxt = up ? T("Monte ") + fstr (up->f) : (dn ? T("Coupe ") + fstr (dn->f) : T("Équilibrée"));
        r.toneSub = (up && dn) ? T("coupe ") + fstr (dn->f) : (r.eq.empty() ? T("rien à corriger") : juce::String (r.eq.size()) + T(" bandes"));

        // --- sibilance
        const float sib95 = pct (sib, 0.95f), sibMed = pct (sib, 0.5f), lv50 = pct (act, 0.5f);
        r.sibDb = sib95;
        const float sibRel = sib95 - lv50;        // force des « S » par rapport à la voix
        r.dessThr = juce::jlimit (-50.0f, -6.0f, sib95 - (sibRel > -8 ? 7.0f : 4.0f));
        r.sibTxt = sibRel > -6 ? T("Forte") : sibRel > -12 ? T("Modérée") : T("Légère");
        r.sibSub = T("de-esser à ") + juce::String (juce::roundToInt (r.dessThr)) + " dB";
        juce::ignoreUnused (sibMed);

        // --- dynamique : seuil, ratio, attaque, relâchement
        const float p90 = pct (act, 0.9f), p10 = pct (act, 0.1f);
        const float dr = p90 - p10;
        const float styleRatio[] = { 2.5f, 3.5f, 4.0f, 3.0f, 4.5f, 3.0f };
        r.ratio = juce::jlimit (2.0f, 8.0f, styleRatio[juce::jlimit (0, 5, style)] + (dr - 12.0f) * 0.12f);
        // le seuil se place pour attraper les passages forts : ~6-8 dB de réduction sur les pics
        r.compThr = juce::jlimit (-50.0f, -3.0f, lv50 + 3.0f - juce::jlimit (0.0f, 6.0f, (dr - 8.0f) * 0.4f));
        // crête/RMS : consonnes très percussives -> attaque plus rapide
        double pk = 0, e = 0; for (float v : x) { pk = juce::jmax (pk, (double) std::abs (v)); e += v * v; }
        const double crest = 20.0 * std::log10 (pk / (std::sqrt (e / x.size()) + 1e-12) + 1e-12);
        r.attackMs = (float) juce::jlimit (2.0, 25.0, 30.0 - crest * 1.2);
        // débit des syllabes : montées d'enveloppe par seconde
        int onsets = 0;
        for (size_t i = 2; i < frameDb.size(); ++i)
            if (frameDb[i] > gate && frameDb[i] - frameDb[i - 2] > 6.0f && frameDb[i - 1] < frameDb[i]) ++onsets;
        const double secs = x.size() / fs;
        const double rate = juce::jmax (0.5, onsets / secs);
        r.releaseMs = (float) juce::jlimit (50.0, 350.0, 1000.0 / (rate * 2.2));
        r.compTxt = juce::String (r.ratio, 1) + ":1  " + juce::String (juce::roundToInt (r.compThr)) + " dB";
        r.compSub = T("attaque ") + juce::String (juce::roundToInt (r.attackMs)) + T(" ms, relâchement ") + juce::String (juce::roundToInt (r.releaseMs)) + " ms";

        // --- niveau de sortie : voix ramenée vers -18 dBFS RMS après compression
        const float grAt90 = kneeGr (p90 + 3.0f - r.compThr, r.ratio, 6.0f);
        r.makeup = juce::jlimit (-12.0f, 18.0f, -18.0f - (p90 - grAt90));
        r.valid = true;
    }

private:
    std::vector<float> listenBuf;
    std::atomic<int> listenPos { 0 };
    std::array<Chan, 2> ch;
    juce::LinearSmoothedValue<float> bypassMix { 1.0f }, dryMix { 1.0f };
    std::atomic<bool> coefDirty { true }, pendingApply { false };
    std::atomic<float> makeupDb { 0.0f };
    float lastAmount = -1; double lastLowCut = -1;
    float wavePeak = 0; int waveCount = 0;

    mutable juce::CriticalSection resLock;
    Result result;
    std::vector<Band> eqBands;
    juce::String error;
    std::atomic<int> resVersion { 0 };
    juce::SpinLock eqSpin;
    std::vector<Band> eqAudio;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Instinct)
};

//==============================================================================
namespace iu
{
    const juce::Colour bg (0xff0f1012), bg2 (0xff17191c), txt (0xffeceef1), dim (0xff8a9099), line (0x1effffff),
                       violet (0xffa98bff), amber (0xffffb347), blue (0xff6fb8ff);
    inline void text (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size, juce::Colour c, bool bold = false, juce::Justification j = juce::Justification::centredLeft)
    {
        g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        g.setColour (c);
        g.drawFittedText (s, r.toNearestInt(), j, 2, 0.9f);
    }
}

// Sphère de particules qui respire avec la voix
class Sphere : public juce::Component
{
public:
    explicit Sphere (Instinct& p) : proc (p), fft (10)
    {
        const int n = 1100;
        const float ga = juce::MathConstants<float>::pi * (3.0f - std::sqrt (5.0f));
        for (int i = 0; i < n; ++i)
        {
            const float y = 1.0f - 2.0f * (i + 0.5f) / n, rr = std::sqrt (1.0f - y * y), th = ga * i;
            pts.push_back ({ std::cos (th) * rr, y, std::sin (th) * rr });
        }
    }

    void tick()
    {
        // spectre des 1024 derniers échantillons -> 24 bandes
        std::array<float, 2048> buf {};
        const auto w = proc.scopeW.load();
        for (int i = 0; i < 1024; ++i)
        {
            const float hw = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * i / 1023.0f);
            buf[(size_t) i] = proc.scope[(size_t) ((w - 1024 + (juce::uint32) i) & (Instinct::kScope - 1))] * hw;
        }
        fft.performFrequencyOnlyForwardTransform (buf.data());
        float lvl = 0;
        for (int b = 0; b < 24; ++b)
        {
            const int k1 = (int) std::pow (2.0, 1.0 + b * 8.0 / 24.0), k2 = juce::jmax (k1 + 1, (int) std::pow (2.0, 1.0 + (b + 1) * 8.0 / 24.0));
            float m = 0; for (int k = k1; k < juce::jmin (512, k2); ++k) m = juce::jmax (m, buf[(size_t) k]);
            const float v = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (m / 64.0f + 1e-9f) + 60.0f) / 60.0f);
            bands[(size_t) b] = v > bands[(size_t) b] ? v : bands[(size_t) b] * 0.88f + v * 0.12f;
            lvl = juce::jmax (lvl, v);
        }
        level = level * 0.85f + lvl * 0.15f;
        angle += 0.004f + level * 0.02f;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const float cx = r.getCentreX(), cy = r.getCentreY(), R = juce::jmin (r.getWidth(), r.getHeight()) * 0.36f;
        const int st = proc.state.load();
        const auto tint = st == Instinct::Listening ? iu::violet : (st == Instinct::Analysing ? iu::amber : juce::Colours::white);
        { juce::ColourGradient gr (tint.withAlpha (0.06f + level * 0.10f), cx, cy, tint.withAlpha (0.0f), cx + R * 1.4f, cy, true); g.setGradientFill (gr); g.fillEllipse (juce::Rectangle<float> (R * 2.8f, R * 2.8f).withCentre ({ cx, cy })); }
        const float ca = std::cos (angle), sa = std::sin (angle), tilt = 0.35f, ct = std::cos (tilt), stt = std::sin (tilt);
        for (auto& p : pts)
        {
            const int band = juce::jlimit (0, 23, (int) ((p.y + 1.0f) * 12.0f));
            const float rad = 1.0f + bands[(size_t) band] * 0.22f + level * 0.06f;
            float x = p.x * ca - p.z * sa, z = p.x * sa + p.z * ca, y = p.y;
            const float y2 = y * ct - z * stt, z2 = y * stt + z * ct;
            x *= rad; const float yy = y2 * rad, zz = z2 * rad;
            const float depth = (zz + 1.3f) / 2.6f;
            const float sx = cx + x * R, sy = cy + yy * R, sz = 0.8f + depth * 1.6f;
            g.setColour (tint.withAlpha (juce::jlimit (0.05f, 1.0f, 0.15f + depth * 0.75f)));
            g.fillEllipse (sx - sz * 0.5f, sy - sz * 0.5f, sz, sz);
        }
    }

private:
    struct P { float x, y, z; };
    Instinct& proc;
    juce::dsp::FFT fft;
    std::vector<P> pts;
    std::array<float, 24> bands {};
    float level = 0, angle = 0;
};

// Courbe d'EQ trouvée
class EqView : public juce::Component
{
public:
    explicit EqView (Instinct& p) : proc (p) {}
    std::vector<Band> eq; float lowCut = 80, amount = 1;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (4);
        auto xOf = [&] (double f) { return r.getX() + r.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); };
        auto yOf = [&] (double db) { return r.getCentreY() - (float) db / 12.0f * r.getHeight() * 0.5f; };
        g.setColour (iu::line);
        g.drawHorizontalLine ((int) yOf (0), r.getX(), r.getRight());
        for (double f : { 100.0, 1000.0, 10000.0 }) g.drawVerticalLine ((int) xOf (f), r.getY(), r.getBottom());
        const double fs = proc.fs;
        juce::Path path;
        const Band* best = nullptr;
        for (auto& b : eq) if (! best || std::abs (b.g) > std::abs (best->g)) best = &b;
        for (int i = 0; i <= 300; ++i)
        {
            const double f = 20.0 * std::pow (1000.0, i / 300.0);
            double db = 0;
            for (auto& b : eq) db += magDb (peakCoefs (fs, b.f, b.q, b.g * amount), f, fs);
            const double w = f / lowCut; db += 10.0 * std::log10 (std::pow (w, 8.0) / (1.0 + std::pow (w, 8.0)) + 1e-12);
            const float x = xOf (f), y = juce::jlimit (r.getY(), r.getBottom(), yOf (db));
            if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
        for (auto& b : eq)
        {
            double db = 0;
            for (auto& o : eq) db += magDb (peakCoefs (fs, o.f, o.q, o.g * amount), b.f, fs);
            const float x = xOf (b.f), y = juce::jlimit (r.getY(), r.getBottom(), yOf (db));
            g.setColour (&b == best ? iu::amber : juce::Colours::white.withAlpha (0.6f));
            g.drawEllipse (x - 4, y - 4, 8, 8, 1.6f);
            if (&b == best)
                iu::text (g, (b.g > 0 ? "+" : "") + juce::String (b.g * amount, 1) + " dB  " + (b.f >= 1000 ? juce::String (b.f / 1000.0f, 1) + "k" : juce::String (juce::roundToInt (b.f))),
                          { x - 50, y - 22, 100, 14 }, 10.5f, iu::amber, true, juce::Justification::centred);
        }
    }
private:
    Instinct& proc;
};

class InstinctContent : public juce::Component, private juce::Timer
{
public:
    static constexpr int W = 1160, H = 720;

    explicit InstinctContent (Instinct& p) : proc (p), sphere (p), eqv (p)
    {
        setLookAndFeel (&lnf);
        addAndMakeVisible (sphere);
        addAndMakeVisible (eqv);

        auto pill = [this] (juce::TextButton& b, juce::Colour on)
        {
            b.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff23262b));
            b.setColour (juce::TextButton::buttonOnColourId, on);
            b.setColour (juce::TextButton::textColourOffId, iu::txt);
            b.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
            addAndMakeVisible (b);
        };
        pill (listen, iu::violet);
        listen.onClick = [this] { if (proc.state.load() == Instinct::Listening) proc.stopListening(); else proc.startListening(); };
        pill (compare, iu::amber);
        compare.setClickingTogglesState (false);
        compare.onClick = [this]
        {
            if (auto* prm = proc.apvts.getParameter ("bypass")) { prm->beginChangeGesture(); prm->setValueNotifyingHost (proc.p ("bypass") > 0.5f ? 0.0f : 1.0f); prm->endChangeGesture(); }
        };

        style.addItemList (kStyles, 1);
        addAndMakeVisible (style);
        styleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "style", style);

        auto knob = [this] (juce::Slider& s, const char* id, const char* look)
        {
            s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            s.setRotaryParameters (-0.75f * juce::MathConstants<float>::pi, 0.75f * juce::MathConstants<float>::pi, true);
            s.getProperties().set ("look", look);
            s.getProperties().set ("tick", iu::txt.toString());
            s.setPopupDisplayEnabled (true, true, this);
            addAndMakeVisible (s);
            atts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s));
            ids.push_back (id);
        };
        knob (kAmount, "amount", "chrome"); knob (kCut, "lowcut", "black"); knob (kDess, "dess", "black");
        knob (kComp, "compThr", "black"); knob (kRatio, "ratio", "grey"); knob (kAtt, "attack", "grey");
        knob (kRel, "release", "grey"); knob (kWarm, "warmth", "red"); knob (kOut, "out", "chrome"); knob (kMix, "mix", "black");

        sphere.setBounds (20, 70, 520, 470);
        listen.setBounds (580, 168, 150, 34);
        compare.setBounds (740, 168, 150, 34);
        style.setBounds (900, 22, 240, 28);
        eqv.setBounds (580, 220, 560, 120);
        int x = 60;
        for (auto* s : { &kAmount, &kCut, &kDess, &kComp, &kRatio, &kAtt, &kRel, &kWarm, &kOut, &kMix })
        {
            s->setBounds (x - 30, 622, 60, 60);
            x += 116;
        }
        startTimerHz (40);
    }
    ~InstinctContent() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override
    {
        using namespace iu;
        g.fillAll (bg);
        g.setColour (bg2);
        g.fillRoundedRectangle (juce::Rectangle<float> (8, 8, W - 16, H - 16), 14);
        g.setColour (line);
        g.drawRoundedRectangle (juce::Rectangle<float> (8, 8, W - 16, H - 16), 14, 1);
        text (g, "BAKR", { 30, 20, 90, 30 }, 24, txt, true);
        text (g, "INSTINCT", { 112, 24, 200, 24 }, 17, violet, true);
        text (g, T("assistant vocal : il écoute ta voix et règle la chaîne"), { 250, 26, 420, 20 }, 12, dim);
        text (g, "STYLE", { 850, 28, 46, 16 }, 10.5f, dim, true);

        // statut
        const int st = proc.state.load();
        juce::String title, sub;
        if (st == Instinct::Listening)
        {
            title = T("J'écoute...");
            const float pr = proc.listenProgress();
            sub = pr < 0.01f ? T("Lance la lecture de ta voix (FL Studio en lecture).")
                : pr < 0.3f ? T("Analyse de la hauteur de la voix...")
                : pr < 0.55f ? T("Mesure de la tonalité...")
                : pr < 0.8f ? T("Recherche des « S » et de la dynamique...")
                : T("Réglage du niveau de sortie...");
        }
        else if (st == Instinct::Analysing) { title = T("Analyse..."); sub = T("Calcul de la chaîne vocale."); }
        else if (st == Instinct::Done) { title = T("Chaîne prête"); sub = T("Ajuste avec les boutons en bas, ou réécoute un autre passage."); }
        else { title = T("Prêt à écouter"); sub = T("Clique ÉCOUTER puis joue 12 secondes de ta voix."); }
        const auto err = proc.getError();
        if (err.isNotEmpty() && st != Instinct::Listening && st != Instinct::Analysing) sub = err;
        text (g, title, { 580, 82, 560, 40 }, 30, txt, true);
        text (g, sub, { 580, 124, 560, 36 }, 13, err.isNotEmpty() && st != Instinct::Listening ? amber : dim);
        if (st == Instinct::Listening)
        {
            auto pb = juce::Rectangle<float> (900, 182, 240, 6);
            g.setColour (juce::Colour (0xff2a2d33)); g.fillRoundedRectangle (pb, 3);
            g.setColour (violet); g.fillRoundedRectangle (pb.withWidth (pb.getWidth() * proc.listenProgress()), 3);
        }

        // résultats
        const auto& r = res;
        struct Row { const char* lab; juce::String v, s; };
        const Row rows[] = {
            { "Hauteur", r.pitchTxt, r.pitchSub },
            { "Tonalité", r.toneTxt, r.toneSub },
            { "Sibilance", r.sibTxt, r.sibSub },
            { "Compression", r.compTxt, r.compSub },
            { "Coupe-bas", r.cutTxt, r.cutSub } };
        float y = 356;
        for (auto& row : rows)
        {
            text (g, T(row.lab), { 580, y, 130, 22 }, 13, dim);
            text (g, r.valid ? row.v : juce::String ("-"), { 710, y, 170, 22 }, 14, txt, true);
            text (g, r.valid ? row.s : juce::String(), { 880, y, 260, 22 }, 12, dim);
            y += 32;
        }
        // mètres de réduction
        auto meter = [&] (const juce::String& l, float v, float yy, juce::Colour c)
        {
            text (g, l, { 580, yy, 120, 14 }, 10.5f, dim, true);
            auto b = juce::Rectangle<float> (700, yy + 2, 440, 10);
            g.setColour (juce::Colour (0xff2a2d33)); g.fillRoundedRectangle (b, 4);
            g.setColour (c); g.fillRoundedRectangle (b.withWidth (b.getWidth() * juce::jlimit (0.0f, 1.0f, v / 15.0f)), 4);
        };
        meter ("COMPRESSION", grSm, 518, amber);
        meter ("DE-ESSER", dsSm, 536, blue);

        // forme d'onde défilante
        auto wb = juce::Rectangle<float> (20, 556, W - 40, 40);
        const int ww = proc.waveW.load();
        const float colW = wb.getWidth() / Instinct::kWave;
        for (int i = 0; i < Instinct::kWave; ++i)
        {
            const int idx = (ww - Instinct::kWave + i + Instinct::kWave * 1000) % Instinct::kWave;
            const float v = juce::jlimit (0.0f, 1.0f, std::sqrt (proc.wave[(size_t) idx].load()));
            const float h = juce::jmax (1.0f, v * wb.getHeight());
            g.setColour (proc.waveListen[(size_t) idx].load() ? violet.withAlpha (0.9f) : juce::Colours::white.withAlpha (0.55f));
            g.fillRect (wb.getX() + i * colW, wb.getCentreY() - h * 0.5f, juce::jmax (1.0f, colW - 0.4f), h);
        }

        // étiquettes des boutons
        const char* labs[] = { "INTENSITÉ", "COUPE-BAS", "DE-ESSER", "COMP SEUIL", "RATIO", "ATTAQUE", "RELÂCHEMENT", "CHALEUR", "SORTIE", "MIX" };
        int x = 60;
        for (size_t i = 0; i < ids.size(); ++i)
        {
            text (g, T(labs[i]), { (float) x - 55, 604, 110, 14 }, 10.5f, txt, true, juce::Justification::centred);
            text (g, proc.apvts.getParameter (ids[i])->getCurrentValueAsText(), { (float) x - 55, 684, 110, 14 }, 10.5f, dim, false, juce::Justification::centred);
            x += 116;
        }
    }

private:
    void timerCallback() override
    {
        proc.applyPending();
        sphere.tick();
        const int st = proc.state.load();
        listen.setButtonText (st == Instinct::Listening ? T("ARRÊTER") : (st == Instinct::Done ? T("RÉÉCOUTER") : T("ÉCOUTER")));
        listen.setToggleState (st == Instinct::Listening, juce::dontSendNotification);
        listen.setEnabled (st != Instinct::Analysing);
        const bool byp = proc.p ("bypass") > 0.5f;
        compare.setToggleState (byp, juce::dontSendNotification);
        compare.setButtonText (byp ? T("AVANT (original)") : T("APRÈS (traité)"));

        const int v = proc.resultVersion();
        if (v != lastV) { lastV = v; res = proc.getResult(); eqv.eq = proc.getEq(); }
        eqv.lowCut = proc.p ("lowcut"); eqv.amount = proc.p ("amount") * 0.01f;
        eqv.repaint();
        grSm = juce::jmax (proc.grDb.load(), grSm * 0.9f);
        dsSm = juce::jmax (proc.dsDb.load(), dsSm * 0.9f);
        repaint (juce::Rectangle<int> (560, 70, 600, 540));
        repaint (juce::Rectangle<int> (0, 550, W, 160));
    }

    Instinct& proc;
    bakr::LookAndFeel lnf;
    Sphere sphere;
    EqView eqv;
    juce::TextButton listen { T("ÉCOUTER") }, compare { T("APRÈS (traité)") };
    juce::ComboBox style;
    juce::Slider kAmount, kCut, kDess, kComp, kRatio, kAtt, kRel, kWarm, kOut, kMix;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> styleAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> atts;
    std::vector<const char*> ids;
    Instinct::Result res;
    int lastV = -1;
    float grSm = 0, dsSm = 0;
};

class InstinctEditor : public juce::AudioProcessorEditor
{
public:
    explicit InstinctEditor (Instinct& p) : AudioProcessorEditor (&p), content (p)
    {
        addAndMakeVisible (content);
        content.setBounds (0, 0, InstinctContent::W, InstinctContent::H);
        setResizable (true, true);
        setResizeLimits (InstinctContent::W / 2, InstinctContent::H / 2, InstinctContent::W * 2, InstinctContent::H * 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) InstinctContent::W / InstinctContent::H);
        setSize (InstinctContent::W, InstinctContent::H);
    }
    void resized() override { content.setTransform (juce::AffineTransform::scale ((float) getWidth() / InstinctContent::W)); }
private:
    InstinctContent content;
};

juce::AudioProcessorEditor* Instinct::createEditor() { return new InstinctEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Instinct(); }

#ifdef INSTINCT_TEST
int instinctState (juce::AudioProcessor* p) { return dynamic_cast<Instinct*> (p)->state.load(); }
void instinctListen (juce::AudioProcessor* p) { dynamic_cast<Instinct*> (p)->startListening(); }
void instinctApply (juce::AudioProcessor* p) { dynamic_cast<Instinct*> (p)->applyPending(); }
juce::String instinctReport (juce::AudioProcessor* p)
{
    auto r = dynamic_cast<Instinct*> (p)->getResult();
    juce::String s;
    s << "valid " << (int) r.valid << " err " << dynamic_cast<Instinct*> (p)->getError() << "\n"
      << "pitch " << r.pitchTxt << " (" << r.pitchSub << ")  low " << r.f0low << "\n"
      << "tone " << r.toneTxt << " / " << r.toneSub << "\n"
      << "sib " << r.sibTxt << " / " << r.sibSub << "\n"
      << "comp " << r.compTxt << " / " << r.compSub << "\n"
      << "cut " << r.cutTxt << "  makeup " << r.makeup << "\n";
    for (auto& b : r.eq) s << "  band " << b.f << " Hz q " << b.q << " g " << b.g << "\n";
    return s;
}
#endif
