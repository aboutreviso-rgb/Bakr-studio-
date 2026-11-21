// BAKR CALIBRE — calibration automatique des enceintes (pensé pour les Yamaha HS8)
// Une seule DLL, deux rôles :
//   ENCEINTES : sur le Master. Joue les balayages, calcule et applique la correction (EQ + niveau + délai G/D).
//   MICRO     : sur la piste du micro. Écoute la pièce et envoie le son au Master (sortie coupée : pas de larsen).
#include "../../Common/BakrUI.h"
#include "CalibreDSP.h"
#include <thread>

using namespace calib;

namespace
{
    // Lien entre les deux instances (même DLL, même processus FL Studio)
    struct Link
    {
        std::atomic<bool>  recording { false };
        std::atomic<int>   recPos { 0 };
        std::atomic<int>   writers { 0 };
        std::atomic<int>   capacity { 0 };
        std::atomic<float*> buffer { nullptr };
        std::vector<float> storage;
        std::atomic<juce::uint32> micBeat { 0 }, masterBeat { 0 };
        std::atomic<float> micPeak { 0.0f };

        static Link& get() { static Link l; return l; }
    };

    juce::uint32 nowMs() { return juce::Time::getMillisecondCounter(); }

    constexpr int kMaxPositions = 9;
    constexpr int kMaxBands = 10;
    const juce::StringArray kMicTypes { "Micro de mesure (plat)", "Takstar TAK35", "Statique voix (AT2020, NT1, C-1...)", "Dynamique (SM58)", "Dynamique broadcast (SM7B)", "Fichier de calibration (.txt)" };
    const juce::StringArray kTargets { "Plat (neutre)", "Studio (pente douce)", "Basses+ (Afro / Rap)" };
}

//==============================================================================
class Calibre : public juce::AudioProcessor, private juce::AudioProcessorValueTreeState::Listener,
                private juce::AsyncUpdater, private juce::Thread
{
public:
    enum State { Idle = 0, Armed, Playing, Analysing };

    struct Display
    {
        bool valid = false;
        int positions = 0;
        Curve meas[2], corr[2], target;
        float trimDb[2] { 0, 0 };
        double delayMs[2] { 0, 0 };
        float outGainDb = 0;
        juce::String room = "-", trim = "-", lrText, bandsText[2];
    };

    Calibre()
        : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          Thread ("BAKR CALIBRE analyse"),
          apvts (*this, nullptr, "BAKR_CALIBRE", layout())
    {
        for (auto* id : { "amount", "maxBoost", "maxCut", "loLim", "hiLim", "target", "micType", "levelMatch", "delayMatch" })
            apvts.addParameterListener (id, this);
        startThread();
    }

    ~Calibre() override
    {
        for (auto* id : { "amount", "maxBoost", "maxCut", "loLim", "hiLim", "target", "micType", "levelMatch", "delayMatch" })
            apvts.removeParameterListener (id, this);
        stopThread (3000);
        cancelPendingUpdate();
    }

    static juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        using namespace bakr;
        Layout l;
        addChoice (l, "role", "Role", { "ENCEINTES (Master)", "MICRO (piste du micro)" }, 0);
        addBool (l, "corrOn", "Correction", true);
        addFloat (l, "amount", "Quantite", 0, 100, 90, "%", 0, 0);
        addFloat (l, "maxBoost", "Boost max", 0, 6, 3, "dB");
        addFloat (l, "maxCut", "Cut max", 0, 18, 12, "dB");
        addFloat (l, "loLim", "Freq min", 20, 120, 40, "Hz", 0, 0);
        addFloat (l, "hiLim", "Freq max", 100, 20000, 500, "Hz", 1000, 0);
        addChoice (l, "target", "Cible", kTargets, 1);
        addChoice (l, "micType", "Micro", kMicTypes, 1);
        addFloat (l, "level", "Niveau du test", -40, -6, -20, "dBFS");
        addBool (l, "levelMatch", "Niveau G/D", true);
        addBool (l, "delayMatch", "Delai G/D", true);
        addBool (l, "exportBypass", "Bypass a l'export", true);
        return l;
    }

    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool isMicRole() const { return p ("role") > 0.5f; }

    //--------------------------------------------------------------------------
    const juce::String getName() const override { return "BAKR CALIBRE"; }
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
        return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
            && l.getMainInputChannelSet() == out;
    }

    void prepareToPlay (double sampleRate, int) override
    {
        const bool changed = std::abs (sampleRate - fs.load()) > 1.0;
        fs.store (sampleRate);
        for (auto& c : chans) { c.reset(); }
        wet.reset (sampleRate, 0.03);
        wet.setCurrentAndTargetValue (0.0f);
        if (changed) triggerAsyncUpdate();
    }
    void releaseResources() override {}

    //--------------------------------------------------------------------------
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals nd;
        auto& link = Link::get();
        const int n = buffer.getNumSamples();
        const int nCh = juce::jmin (2, buffer.getNumChannels());
        if (n <= 0 || nCh <= 0) return;

        if (isMicRole()) { micProcess (buffer, n, nCh); return; }
        link.masterBeat.store (nowMs());

        // nouvelles corrections calculées ?
        if (corrDirty.load())
        {
            const juce::SpinLock::ScopedTryLockType tl (corrLock);
            if (tl.isLocked())
            {
                active = pending;
                corrDirty.store (false);
                for (int c = 0; c < 2; ++c)
                    for (int b = 0; b < kMaxBands; ++b) chans[(size_t) c].bq[(size_t) b].setCoefs (active.coefs[c][b]);
            }
        }

        int st = state.load();
        if (st == Armed)
        {
            noiseOn.store (false);
            link.recPos.store (0);
            link.recording.store (true);
            playPos = 0;
            state.store (Playing);
            st = Playing;
        }

        if (st == Playing || st == Analysing)
        {
            const float lvl = juce::Decibels::decibelsToGain (p ("level"));
            float* L = buffer.getWritePointer (0);
            float* R = nCh > 1 ? buffer.getWritePointer (1) : nullptr;
            for (int i = 0; i < n; ++i)
            {
                float l = 0, r = 0;
                if (st == Playing && playPos < playTotal)
                {
                    const int seg = playPos / segLen, ofs = playPos % segLen;
                    if (seg < numSpk && ofs < (int) sweep.size())
                    {
                        const float s = sweep[(size_t) ofs] * lvl;
                        if (seg == 0) l = s; else r = s;
                    }
                    if (++playPos >= playTotal)
                    {
                        link.recording.store (false);
                        state.store (Analysing);
                        st = Analysing;
                    }
                }
                if (R != nullptr) { L[i] = l; R[i] = r; }
                else L[i] = l + r;
            }
            progress.store (playTotal > 0 ? (float) playPos / (float) playTotal : 0.0f);
            for (int c = nCh; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
            return;
        }

        if (noiseOn.load())
        {
            const float lvl = juce::Decibels::decibelsToGain (p ("level") - 4.0f);
            for (int i = 0; i < n; ++i)
            {
                const float v = pink.next() * lvl;
                for (int c = 0; c < nCh; ++c) buffer.setSample (c, i, v);
            }
            return;
        }

        // --- correction
        const bool on = p ("corrOn") > 0.5f && active.valid && ! (isNonRealtime() && p ("exportBypass") > 0.5f);
        wet.setTargetValue (on ? 1.0f : 0.0f);
        if (! wet.isSmoothing() && wet.getCurrentValue() < 1.0e-4f)
        {
            // correction coupée : on fait tout de même avancer les lignes de délai (pas de clic au retour)
            for (int c = 0; c < nCh; ++c)
                for (int i = 0; i < n; ++i) chans[(size_t) c].pushDelay (buffer.getSample (c, i));
            return;
        }

        float* data[2] = { buffer.getWritePointer (0), nCh > 1 ? buffer.getWritePointer (1) : nullptr };
        for (int i = 0; i < n; ++i)
        {
            const float m = wet.getNextValue();
            for (int c = 0; c < nCh; ++c)
            {
                auto& ch = chans[(size_t) c];
                const float x = data[c][i];
                double y = ch.pushDelay (x);
                const int d = juce::jlimit (0, Chan::kDelaySize - 1, active.delaySamples[c]);
                y = ch.readDelay (d) * active.gain[c];
                for (int b = 0; b < active.numBands[c]; ++b) y = ch.bq[(size_t) b].process (y);
                data[c][i] = x + ((float) y - x) * m;
            }
        }
    }

    void micProcess (juce::AudioBuffer<float>& buffer, int n, int nCh)
    {
        auto& link = Link::get();
        link.micBeat.store (nowMs());
        float pk = 0;
        const float* L = buffer.getReadPointer (0);
        const float* R = nCh > 1 ? buffer.getReadPointer (1) : nullptr;

        // le micro peut arriver sur une seule voie : on garde la plus forte
        float eL = 0, eR = 0;
        for (int i = 0; i < n; ++i) { eL = juce::jmax (eL, std::abs (L[i])); if (R) eR = juce::jmax (eR, std::abs (R[i])); }
        const bool both = R != nullptr && eR > eL * 0.5f && eL > eR * 0.5f;
        const float* src = (R != nullptr && eR > eL) ? R : L;
        pk = juce::jmax (eL, eR);

        link.writers.fetch_add (1);
        if (link.recording.load())
        {
            float* dst = link.buffer.load();
            const int cap = link.capacity.load();
            int pos = link.recPos.load();
            for (int i = 0; i < n && pos < cap; ++i)
                dst[pos++] = both ? 0.5f * (L[i] + R[i]) : src[i];
            link.recPos.store (pos);
        }
        link.writers.fetch_sub (1);

        link.micPeak.store (juce::jmax (pk, link.micPeak.load()));
        buffer.clear();   // rien ne sort de la piste du micro
    }

    //--------------------------------------------------------------------------
    // Interface -> mesure
    juce::String startMeasurement()
    {
        if (state.load() != Idle) return juce::String (juce::CharPointer_UTF8 ("Une mesure est d\xc3\xa9j\xc3\xa0 en cours."));
        {
            const juce::ScopedLock sl (dataLock);
            if ((int) positions.size() >= kMaxPositions) return "9 positions maximum : clique EFFACER pour recommencer.";
        }
        if (! micConnected()) return juce::String (juce::CharPointer_UTF8 ("Micro non d\xc3\xa9tect\xc3\xa9 : mets un 2e BAKR CALIBRE en mode MICRO sur la piste du micro."));

        const double rate = fs.load();
        auto& link = Link::get();
        for (int t = 0; t < 200 && (link.recording.load() || link.writers.load() > 0); ++t) juce::Thread::sleep (2);

        const int nOut = juce::jmax (1, juce::jmin (2, getMainBusNumOutputChannels()));
        numSpk = nOut;
        sweep = makeSweep (rate, 5.0);
        segLen = (int) sweep.size() + (int) (1.5 * rate);
        playTotal = numSpk * segLen + (int) (0.5 * rate);
        measureFs = rate;
        link.buffer.store (nullptr);
        link.capacity.store (0);
        link.storage.assign ((size_t) playTotal + (size_t) rate, 0.0f);
        link.buffer.store (link.storage.data());
        link.capacity.store ((int) link.storage.size());
        progress.store (0.0f);
        setStatus (juce::String (juce::CharPointer_UTF8 ("Mesure en cours : ne bouge pas, silence dans la pi\xc3\xa8" "ce...")), false);
        state.store (Armed);
        return {};
    }

    void clearAll()
    {
        if (state.load() != Idle) return;
        { const juce::ScopedLock sl (dataLock); positions.clear(); }
        setStatus (juce::String (juce::CharPointer_UTF8 ("Mesures effac\xc3\xa9" "es.")), false);
        recompute();
    }
    void removeLast()
    {
        if (state.load() != Idle) return;
        { const juce::ScopedLock sl (dataLock); if (! positions.empty()) positions.pop_back(); }
        setStatus (juce::String (juce::CharPointer_UTF8 ("Derni\xc3\xa8re position supprim\xc3\xa9" "e.")), false);
        recompute();
    }

    bool micConnected() const { return nowMs() - Link::get().micBeat.load() < 1500; }
    bool masterConnected() const { return nowMs() - Link::get().masterBeat.load() < 1500; }

    void setStatus (const juce::String& s, bool err)
    {
        const juce::ScopedLock sl (statusLock);
        status = s; statusError = err;
    }
    juce::String getStatus (bool& err) const
    {
        const juce::ScopedLock sl (statusLock);
        err = statusError; return status;
    }

    void setCustomMic (const Curve& c, const juce::String& name)
    {
        { const juce::ScopedLock sl (dataLock); customMic = c; customMicName = name; }
        recompute();
    }
    juce::String getCustomMicName() const { const juce::ScopedLock sl (dataLock); return customMicName; }

    Display getDisplay() const { const juce::ScopedLock sl (dataLock); return display; }
    int getDisplayVersion() const { return displayVersion.load(); }

    std::atomic<int> state { Idle };
    std::atomic<float> progress { 0.0f };
    std::atomic<bool> noiseOn { false };
    juce::AudioProcessorValueTreeState apvts;

    //--------------------------------------------------------------------------
    void getStateInformation (juce::MemoryBlock& dest) override
    {
        auto st = apvts.copyState();
        juce::ValueTree cal ("CALIB");
        {
            const juce::ScopedLock sl (dataLock);
            for (auto& pos : positions)
            {
                juce::ValueTree pt ("POS");
                for (int c = 0; c < 2; ++c)
                {
                    juce::MemoryBlock mb (pos.ch[c].magDb.data(), pos.ch[c].magDb.size() * sizeof (float));
                    pt.setProperty ("mag" + juce::String (c), mb.toBase64Encoding(), nullptr);
                    pt.setProperty ("arr" + juce::String (c), pos.ch[c].arrivalSec, nullptr);
                }
                cal.appendChild (pt, nullptr);
            }
            if ((int) customMic.size() == kGrid)
            {
                juce::MemoryBlock mb (customMic.data(), customMic.size() * sizeof (float));
                cal.setProperty ("mic", mb.toBase64Encoding(), nullptr);
                cal.setProperty ("micName", customMicName, nullptr);
            }
        }
        st.appendChild (cal, nullptr);
        if (auto xml = st.createXml()) copyXmlToBinary (*xml, dest);
    }

    void setStateInformation (const void* data, int size) override
    {
        auto xml = getXmlFromBinary (data, size);
        if (xml == nullptr) return;
        auto st = juce::ValueTree::fromXml (*xml);
        if (! st.isValid()) return;
        auto cal = st.getChildWithName ("CALIB");
        if (cal.isValid()) st.removeChild (cal, nullptr);
        apvts.replaceState (st);
        if (cal.isValid())
        {
            auto readCurve = [] (const juce::String& b64)
            {
                juce::MemoryBlock mb;
                Curve c;
                if (mb.fromBase64Encoding (b64) && mb.getSize() == (size_t) kGrid * sizeof (float))
                {
                    c.resize ((size_t) kGrid);
                    std::memcpy (c.data(), mb.getData(), mb.getSize());
                }
                return c;
            };
            const juce::ScopedLock sl (dataLock);
            positions.clear();
            for (auto pt : cal)
            {
                Measure m; m.ok = true;
                bool good = true;
                for (int c = 0; c < 2; ++c)
                {
                    m.ch[c].magDb = readCurve (pt.getProperty ("mag" + juce::String (c)).toString());
                    m.ch[c].arrivalSec = (double) pt.getProperty ("arr" + juce::String (c));
                    good = good && (int) m.ch[c].magDb.size() == kGrid;
                }
                if (good && (int) positions.size() < kMaxPositions) positions.push_back (m);
            }
            customMic = readCurve (cal.getProperty ("mic").toString());
            customMicName = cal.getProperty ("micName").toString();
        }
        triggerAsyncUpdate();
    }

private:
    //--------------------------------------------------------------------------
    struct BQ
    {
        Coefs k; double z1 = 0, z2 = 0;
        void setCoefs (const Coefs& c) { k = c; }
        inline double process (double x) noexcept
        {
            const double y = k.b0 * x + z1;
            z1 = k.b1 * x - k.a1 * y + z2;
            z2 = k.b2 * x - k.a2 * y;
            return y;
        }
    };
    struct Chan
    {
        static constexpr int kDelaySize = 8192;
        std::array<BQ, kMaxBands> bq;
        std::array<float, kDelaySize> dl {};
        int w = 0;
        void reset() { for (auto& b : bq) b.z1 = b.z2 = 0; dl.fill (0.0f); w = 0; }
        inline float pushDelay (float x) noexcept { dl[(size_t) w] = x; w = (w + 1) & (kDelaySize - 1); return x; }
        inline float readDelay (int d) const noexcept { return dl[(size_t) ((w - 1 - d) & (kDelaySize - 1))]; }
    };
    struct CorrSet
    {
        bool valid = false;
        Coefs coefs[2][kMaxBands];
        int numBands[2] { 0, 0 };
        float gain[2] { 1, 1 };
        int delaySamples[2] { 0, 0 };
    };

    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { recompute(); }

    // Fil d'analyse : attend la fin d'un balayage puis calcule
    void run() override
    {
        while (! threadShouldExit())
        {
            wait (40);
            if (state.load() != Analysing) continue;
            auto& link = Link::get();
            for (int t = 0; t < 500 && link.writers.load() > 0; ++t) juce::Thread::sleep (1);
            const int len = juce::jmin (link.recPos.load(), link.capacity.load());
            std::vector<float> rec;
            if (float* b = link.buffer.load(); b != nullptr && len > 0) rec.assign (b, b + len);

            setStatus ("Analyse...", false);
            Measure m;
            if (rec.empty()) m.error = juce::String (juce::CharPointer_UTF8 ("Aucun son re\xc3\xa7u du micro : v\xc3\xa9rifie le 2e BAKR CALIBRE (mode MICRO) sur la piste du micro."));
            else m = analyse (rec, sweep, segLen, measureFs, numSpk);

            if (m.ok)
            {
                int count;
                { const juce::ScopedLock sl (dataLock); positions.push_back (m); count = (int) positions.size(); }
                recompute();
                juce::String s = "Position " + juce::String (count) + juce::String (juce::CharPointer_UTF8 (" mesur\xc3\xa9" "e (pic micro ")) + juce::String (juce::roundToInt (m.recPeakDb)) + " dBFS).";
                if (m.warning.isNotEmpty()) s << "  " << m.warning;
                else if (count < 3) s << juce::String (juce::CharPointer_UTF8 ("  D\xc3\xa9place le micro de 20-30 cm et mesure encore."));
                else s << juce::String (juce::CharPointer_UTF8 ("  Calibration pr\xc3\xaate !"));
                setStatus (s, m.warning.isNotEmpty());
            }
            else setStatus (m.error, true);
            state.store (Idle);
        }
    }

    // Moyenne des positions -> cible -> EQ automatique -> envoi au fil audio
    void recompute()
    {
        const double rate = fs.load();
        FitParams fp;
        fp.amount = p ("amount") * 0.01;
        fp.maxBoost = p ("maxBoost");
        fp.maxCut = p ("maxCut");
        fp.lo = p ("loLim");
        fp.hi = juce::jmax ((double) p ("hiLim"), fp.lo * 1.5);
        fp.maxBands = kMaxBands;
        const int tgt = (int) p ("target");
        const int micT = (int) p ("micType");
        const bool lvlM = p ("levelMatch") > 0.5f, dlyM = p ("delayMatch") > 0.5f;

        Display d;
        CorrSet cs;
        {
            const juce::ScopedLock sl (dataLock);
            d.positions = (int) positions.size();
            d.target.resize ((size_t) kGrid);
            for (int i = 0; i < kGrid; ++i) d.target[(size_t) i] = targetAt (tgt, gridFreq (i));

            if (! positions.empty())
            {
                const Curve mic = micCurve (micT, customMic);
                double ref[2], arr[2] = { 0, 0 };
                std::vector<Band> bands[2];
                for (int c = 0; c < 2; ++c)
                {
                    Curve avg ((size_t) kGrid);
                    for (int i = 0; i < kGrid; ++i)
                    {
                        double s = 0;
                        for (auto& pos : positions) s += std::pow (10.0, pos.ch[c].magDb[(size_t) i] / 10.0);
                        avg[(size_t) i] = (float) (10.0 * std::log10 (s / positions.size() + 1e-30)) - mic[(size_t) i];
                    }
                    for (auto& pos : positions) arr[c] += pos.ch[c].arrivalSec / positions.size();
                    ref[c] = meanRange (avg, 200.0, 2000.0);
                    for (auto& v : avg) v -= (float) ref[c];
                    d.meas[c] = avg;
                    bands[c] = fitBands (avg, d.target, fp, rate);
                    d.corr[c] = bandsCurve (bands[c], rate);
                }

                // niveau et délai gauche/droite
                const double diff = ref[0] - ref[1];
                if (lvlM && std::abs (diff) > 0.1)
                    d.trimDb[diff > 0 ? 0 : 1] = (float) -juce::jlimit (0.0, 6.0, std::abs (diff));
                const double dt = arr[0] - arr[1];
                if (dlyM && std::abs (dt) * 1000.0 > 0.02)
                    d.delayMs[dt > 0 ? 1 : 0] = juce::jlimit (0.0, 20.0, std::abs (dt) * 1000.0);

                // gain de sortie automatique : jamais plus fort qu'avant (anti-saturation)
                float mx = 0;
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < kGrid; ++i)
                        if (gridFreq (i) >= 20.0 && gridFreq (i) <= 20000.0) mx = juce::jmax (mx, d.corr[c][(size_t) i] + d.trimDb[c]);
                d.outGainDb = -mx;

                cs.valid = true;
                for (int c = 0; c < 2; ++c)
                {
                    cs.numBands[c] = juce::jmin (kMaxBands, (int) bands[c].size());
                    for (int b = 0; b < cs.numBands[c]; ++b)
                        cs.coefs[c][b] = peakCoefs (rate, bands[c][(size_t) b].f, bands[c][(size_t) b].q, bands[c][(size_t) b].g);
                    cs.gain[c] = juce::Decibels::decibelsToGain (d.trimDb[c] + d.outGainDb);
                    cs.delaySamples[c] = (int) std::round (d.delayMs[c] * 0.001 * rate);

                    juce::String t;
                    for (auto& b : bands[c])
                        t << (b.f >= 1000 ? juce::String (b.f / 1000.0f, 1) + "k" : juce::String ((int) std::round (b.f))) << " "
                          << (b.g > 0 ? "+" : "") << juce::String (b.g, 1) << "   ";
                    d.bandsText[c] = t.isEmpty() ? juce::String (juce::CharPointer_UTF8 ("aucune correction n\xc3\xa9" "cessaire")) : t;
                }

                // conseils pour les switchs à l'arrière des HS8
                Curve both ((size_t) kGrid);
                for (int i = 0; i < kGrid; ++i) both[(size_t) i] = 0.5f * (d.meas[0][(size_t) i] + d.meas[1][(size_t) i]);
                const double mid = meanRange (both, 500.0, 2000.0);
                const double low = meanRange (both, 80.0, 400.0) - mid;
                const double top = meanRange (both, 3000.0, 10000.0) - mid;
                d.room = low > 4.0 ? "-4 dB" : (low > 2.0 ? "-2 dB" : "0 dB");
                d.trim = top > 1.5 ? "-1 dB" : (top < -1.5 ? "+1 dB" : "0 dB");
                const double cm = std::abs (dt) * 34300.0;
                d.lrText = "G/D : " + juce::String (std::abs (diff), 1) + " dB " + (std::abs (diff) < 0.3 ? "" : (diff > 0 ? "(gauche plus forte)" : "(droite plus forte)"))
                         + "   distance : " + juce::String (juce::roundToInt (cm)) + " cm " + (cm < 2 ? "" : (dt > 0 ? "(gauche plus loin)" : "(droite plus loin)"));
                d.valid = true;
            }
            display = d;
        }
        {
            const juce::SpinLock::ScopedLockType sl (corrLock);
            pending = cs;
            corrDirty.store (true);
        }
        displayVersion.fetch_add (1);
    }

    //--------------------------------------------------------------------------
    std::atomic<double> fs { 48000.0 };
    std::array<Chan, 2> chans;
    juce::LinearSmoothedValue<float> wet;
    Pink pink;

    // mesure
    std::vector<float> sweep;
    int segLen = 1, playTotal = 0, playPos = 0, numSpk = 2;
    double measureFs = 48000.0;

    // résultats
    mutable juce::CriticalSection dataLock, statusLock;
    std::vector<Measure> positions;
    Curve customMic;
    juce::String customMicName;
    Display display;
    std::atomic<int> displayVersion { 0 };
    juce::String status { juce::String (juce::CharPointer_UTF8 ("Pr\xc3\xaat. Suis les \xc3\xa9tapes 1 - 2 - 3.")) };
    bool statusError = false;

    juce::SpinLock corrLock;
    CorrSet pending, active;
    std::atomic<bool> corrDirty { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Calibre)
};

//==============================================================================
// Interface
namespace ui
{
    const juce::Colour bg0 (0xff2c3036), bg1 (0xff15171a), panel (0x30000000), line (0x26ffffff),
                       txt (0xffeef0f3), dim (0xff9aa2ac), amber (0xffffb347), green (0xff5fe08a),
                       cyanL (0xff4fc3f7), pinkR (0xffff7aa8), red (0xffff4a3d);

    inline void text (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size, juce::Colour c,
                      bool bold = true, juce::Justification j = juce::Justification::centredLeft)
    {
        g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        g.setColour (c);
        g.drawFittedText (s, r.toNearestInt(), j, 3, 0.9f);
    }
}

class Graph : public juce::Component
{
public:
    explicit Graph (Calibre& p) : proc (p) {}
    int view = 2;   // 0 = gauche, 1 = droite, 2 = les deux
    Calibre::Display d;
    bool showCorr = true;

    float xOf (double f) const { return plot.getX() + plot.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); }
    float yOf (double db) const { return plot.getCentreY() - plot.getHeight() * 0.5f * (float) (db / 18.0); }

    void paint (juce::Graphics& g) override
    {
        using namespace ui;
        auto r = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0d0f11));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (line);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        plot = r.reduced (40, 26).withTrimmedBottom (4).withTrimmedRight (-12);

        // zone corrigée
        const float lo = juce::jlimit (plot.getX(), plot.getRight(), xOf (proc.p ("loLim")));
        const float hi = juce::jlimit (plot.getX(), plot.getRight(), xOf (juce::jmax (proc.p ("hiLim"), proc.p ("loLim") * 1.5f)));
        g.setColour (amber.withAlpha (0.06f));
        g.fillRect (juce::Rectangle<float> (lo, plot.getY(), hi - lo, plot.getHeight()));

        g.setFont (juce::FontOptions (10.0f));
        for (double f : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
        {
            const float x = xOf (f);
            g.setColour (line); g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
            g.setColour (dim);
            g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (x - 20, plot.getBottom() + 3, 40, 12), juce::Justification::centred);
        }
        for (int db = -18; db <= 18; db += 6)
        {
            const float y = yOf (db);
            g.setColour (db == 0 ? juce::Colours::white.withAlpha (0.22f) : line);
            g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
            g.setColour (dim);
            g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (r.getX() + 4, y - 6, 30, 12), juce::Justification::centredRight);
        }

        if (! d.valid)
        {
            text (g, juce::String (juce::CharPointer_UTF8 ("Pas encore de mesure.\nSuis les \xc3\xa9tapes 1 - 2 - 3 \xc3\xa0 droite.")), plot.reduced (40), 18, dim, true, juce::Justification::centred);
            drawCurve (g, d.target, juce::Colours::white.withAlpha (0.6f), 1.5f, true);
            return;
        }

        g.saveState();
        g.reduceClipRegion (plot.toNearestInt());
        drawCurve (g, d.target, juce::Colours::white.withAlpha (0.65f), 1.5f, true);
        for (int c = 0; c < 2; ++c)
        {
            if (view != 2 && view != c) continue;
            const auto col = c == 0 ? cyanL : pinkR;
            drawCurve (g, d.meas[c], col.withAlpha (view == 2 ? 0.75f : 0.9f), 1.6f, false);
            if (showCorr)
            {
                Curve res (d.meas[c]);
                for (int i = 0; i < kGrid; ++i) res[(size_t) i] += d.corr[c][(size_t) i];
                drawCurve (g, d.corr[c], amber, 2.0f, false, true);
                drawCurve (g, res, green.withAlpha (view == 2 ? 0.75f : 1.0f), 2.2f, false);
            }
        }
        g.restoreState();

        // légende
        auto lg = juce::Rectangle<float> (plot.getX() + 8, plot.getY() + 6, 600, 14);
        auto item = [&] (const juce::String& s, juce::Colour c)
        {
            g.setColour (c); g.fillRect (lg.getX(), lg.getCentreY() - 1.5f, 14.0f, 3.0f);
            ui::text (g, s, lg.withTrimmedLeft (18).withWidth (110), 10.5f, ui::txt, false);
            lg.translate (118, 0);
        };
        if (view != 1) item ("Mesure gauche", cyanL);
        if (view != 0) item ("Mesure droite", pinkR);
        item ("Cible", juce::Colours::white.withAlpha (0.65f));
        if (showCorr) { item ("Correction", amber); item (juce::String (juce::CharPointer_UTF8 ("R\xc3\xa9sultat")), green); }
    }

private:
    void drawCurve (juce::Graphics& g, const Curve& c, juce::Colour col, float w, bool dashed, bool fill = false)
    {
        if ((int) c.size() != kGrid) return;
        juce::Path path;
        bool started = false;
        for (int i = 0; i < kGrid; ++i)
        {
            const double f = gridFreq (i);
            if (f < 18.0 || f > 21000.0) continue;
            const float x = xOf (f), y = yOf (juce::jlimit (-24.0f, 24.0f, c[(size_t) i]));
            if (! started) { path.startNewSubPath (x, y); started = true; } else path.lineTo (x, y);
        }
        if (fill)
        {
            juce::Path f (path);
            f.lineTo (xOf (21000.0), yOf (0)); f.lineTo (xOf (18.0), yOf (0)); f.closeSubPath();
            g.setColour (col.withAlpha (0.12f)); g.fillPath (f);
        }
        g.setColour (col);
        if (dashed)
        {
            juce::Path dp;
            const float dl[] = { 6.0f, 4.0f };
            juce::PathStrokeType (w).createDashedStroke (dp, path, dl, 2);
            g.fillPath (dp);
        }
        else g.strokePath (path, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    Calibre& proc;
    juce::Rectangle<float> plot;
};

//==============================================================================
class Content : public juce::Component, private juce::Timer
{
public:
    static constexpr int W = 1180, H = 700;

    explicit Content (Calibre& p) : proc (p), graph (p)
    {
        setLookAndFeel (&lnf);
        auto styleBtn = [this] (juce::TextButton& b, juce::Colour on = ui::amber)
        {
            b.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff23262b));
            b.setColour (juce::TextButton::buttonOnColourId, on);
            b.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffdedede));
            b.setColour (juce::TextButton::textColourOnId, juce::Colour (0xff141414));
            addAndMakeVisible (b);
        };

        // rôle
        for (auto* b : { &roleSpk, &roleMic }) { styleBtn (*b); b->setClickingTogglesState (false); }
        roleSpk.onClick = [this] { setRole (0); };
        roleMic.onClick = [this] { setRole (1); };

        addAndMakeVisible (graph);
        for (auto* b : { &viewL, &viewR, &viewLR }) { styleBtn (*b); b->setClickingTogglesState (false); }
        viewL.onClick  = [this] { graph.view = 0; graph.repaint(); };
        viewR.onClick  = [this] { graph.view = 1; graph.repaint(); };
        viewLR.onClick = [this] { graph.view = 2; graph.repaint(); };

        // étape 1 : micro
        micType.addItemList (kMicTypes, 1);
        addAndMakeVisible (micType);
        micAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "micType", micType);
        styleBtn (loadCal);
        loadCal.onClick = [this] { chooseCalFile(); };

        // étape 2 : niveau
        level.setLookAndFeel (&plainLnf);
        plainLnf.setColour (juce::Slider::thumbColourId, ui::amber);
        plainLnf.setColour (juce::Slider::trackColourId, ui::amber.withAlpha (0.6f));
        plainLnf.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff0b0c0e));
        level.setSliderStyle (juce::Slider::LinearHorizontal);
        level.setTextBoxStyle (juce::Slider::TextBoxRight, false, 78, 20);
        level.setColour (juce::Slider::textBoxTextColourId, ui::txt);
        level.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        level.getProperties().set ("tick", ui::txt.toString());
        addAndMakeVisible (level);
        levelAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "level", level);
        styleBtn (noise, juce::Colour (0xffe0e0e0));
        noise.setClickingTogglesState (false);
        noise.onClick = [this] { proc.noiseOn.store (! proc.noiseOn.load()); };

        // étape 3 : mesure
        styleBtn (measure, ui::green);
        measure.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2f6b45));
        measure.onClick = [this]
        {
            const auto err = proc.startMeasurement();
            if (err.isNotEmpty()) proc.setStatus (err, true);
        };
        styleBtn (undo); undo.onClick = [this] { proc.removeLast(); };
        styleBtn (clear); clear.onClick = [this] { proc.clearAll(); };

        // réglages de correction
        auto knob = [this] (juce::Slider& s, const char* id, const char* look)
        {
            s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            s.setRotaryParameters (-0.75f * juce::MathConstants<float>::pi, 0.75f * juce::MathConstants<float>::pi, true);
            s.getProperties().set ("look", look);
            s.getProperties().set ("tick", ui::txt.toString());
            s.setPopupDisplayEnabled (true, true, this);
            addAndMakeVisible (s);
            atts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s));
            if (auto* prm = proc.apvts.getParameter (id))
                s.setDoubleClickReturnValue (true, proc.apvts.getParameterRange (id).convertFrom0to1 (prm->getDefaultValue()));
        };
        knob (kAmount, "amount", "chrome");
        knob (kBoost, "maxBoost", "black");
        knob (kCut, "maxCut", "black");
        knob (kLo, "loLim", "grey");
        knob (kHi, "hiLim", "grey");
        target.addItemList (kTargets, 1);
        addAndMakeVisible (target);
        targetAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "target", target);

        auto toggle = [this] (juce::ToggleButton& b, const char* id, juce::Colour light)
        {
            b.getProperties().set ("kind", "button");
            b.getProperties().set ("light", light.toString());
            addAndMakeVisible (b);
            btnAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, b));
        };
        toggle (corrOn, "corrOn", ui::green);
        toggle (lvlM, "levelMatch", ui::amber);
        toggle (dlyM, "delayMatch", ui::amber);
        toggle (expB, "exportBypass", ui::amber);
        corrOn.setTooltip ("Compare avec / sans correction");
        expB.setTooltip (juce::String (juce::CharPointer_UTF8 ("Coupe la correction pendant l'export du morceau (le mix export\xc3\xa9 ne doit pas \xc3\xaatre corrig\xc3\xa9)")));

        layoutAll();
        startTimerHz (30);
    }

    ~Content() override { level.setLookAndFeel (nullptr); setLookAndFeel (nullptr); }

    void layoutAll()
    {
        roleSpk.setBounds (720, 16, 210, 30);
        roleMic.setBounds (940, 16, 210, 30);

        graph.setBounds (20, 70, 760, 420);
        viewL.setBounds (630, 76, 44, 22); viewR.setBounds (676, 76, 44, 22); viewLR.setBounds (722, 76, 52, 22);

        const int X = 800, Wd = 360;
        micType.setBounds (X + 14, 136, Wd - 28 - 100, 24);
        loadCal.setBounds (X + Wd - 110, 136, 96, 24);
        level.setBounds (X + 14, 218, Wd - 28 - 120, 24);
        noise.setBounds (X + Wd - 130, 218, 116, 24);
        measure.setBounds (X + 14, 296, Wd - 28, 40);
        undo.setBounds (X + 14, 342, (Wd - 34) / 2, 24);
        clear.setBounds (X + 20 + (Wd - 34) / 2, 342, (Wd - 34) / 2, 24);

        const int ky = 548, ks = 62;
        int kx = 70;
        for (auto* s : { &kAmount, &kBoost, &kCut, &kLo, &kHi })
        {
            s->setBounds (kx - ks / 2, ky - ks / 2, ks, ks);
            kx += 92;
        }
        target.setBounds (500, 530, 170, 24);
        corrOn.setBounds (500, 572, 170, 34);
        lvlM.setBounds (690, 524, 88, 28);
        dlyM.setBounds (690, 560, 88, 28);
        expB.setBounds (690, 596, 88, 28);

        // mode micro
        micInfo = { 140, 120, 900, 440 };
    }

    void paint (juce::Graphics& g) override
    {
        using namespace ui;
        auto r = getLocalBounds().toFloat();
        g.setGradientFill (juce::ColourGradient (bg0, 0, 0, bg1, 0, (float) H, false));
        g.fillAll();
        // texture brossée légère
        juce::Random rnd (11);
        for (int y = 0; y < H; y += 2)
        {
            g.setColour (juce::Colours::white.withAlpha (0.008f + 0.012f * rnd.nextFloat()));
            g.drawHorizontalLine (y, 0, (float) W);
        }
        text (g, "BAKR", { 22, 10, 120, 36 }, 30, txt);
        text (g, "CALIBRE", { 112, 14, 200, 30 }, 22, amber);
        text (g, "calibration automatique des enceintes  -  Yamaha HS8", { 250, 18, 460, 24 }, 12.5f, dim, false);

        const bool mic = proc.isMicRole();
        if (mic) { paintMicMode (g); return; }

        // panneau des étapes
        const float X = 800, Wd = 360;
        auto box = [&] (juce::Rectangle<float> b) { g.setColour (panel); g.fillRoundedRectangle (b, 6); g.setColour (line); g.drawRoundedRectangle (b, 6, 1); };
        box ({ X, 70, Wd, 420 });
        auto step = [&] (int n, const juce::String& t, float y)
        {
            g.setColour (amber); g.fillEllipse (X + 12, y, 22, 22);
            text (g, juce::String (n), { X + 12, y, 22, 22 }, 13, juce::Colours::black, true, juce::Justification::centred);
            text (g, t, { X + 42, y, Wd - 50, 22 }, 13, txt);
        };
        step (1, "MICRO", 80);
        const bool conn = proc.micConnected();
        g.setColour (conn ? green : red);
        g.fillEllipse (X + 120, 86, 10, 10);
        text (g, conn ? juce::String (juce::CharPointer_UTF8 ("connect\xc3\xa9")) : juce::String (juce::CharPointer_UTF8 ("non d\xc3\xa9tect\xc3\xa9 (voir notice)")), { X + 136, 80, 210, 22 }, 11.5f, conn ? green : red, false);
        // vumètre micro
        auto mb = juce::Rectangle<float> (X + 14, 110, Wd - 28, 16);
        g.setColour (juce::Colour (0xff0b0c0e)); g.fillRoundedRectangle (mb, 3);
        const float frac = juce::jlimit (0.0f, 1.0f, (micDb + 60.0f) / 60.0f);
        auto fill = mb.reduced (2).withWidth ((mb.getWidth() - 4) * frac);
        { juce::ColourGradient gr (green, mb.getX(), 0, red, mb.getRight(), 0, false); gr.addColour (0.75, amber); g.setGradientFill (gr); }
        g.fillRoundedRectangle (fill, 2);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        for (float db : { -40.0f, -20.0f, -6.0f })
        {
            const float x = mb.getX() + 2 + (mb.getWidth() - 4) * (db + 60.0f) / 60.0f;
            g.drawVerticalLine ((int) x, mb.getY(), mb.getBottom());
        }
        text (g, micDb > -99 ? juce::String (juce::roundToInt (micDb)) + " dB" : "-", mb.withTrimmedLeft (mb.getWidth() - 50), 10, txt, false);
        const auto cn = proc.getCustomMicName();
        text (g, (int) proc.p ("micType") == 5 ? (cn.isEmpty() ? juce::String (juce::CharPointer_UTF8 ("aucun fichier charg\xc3\xa9")) : "fichier : " + cn) : "type de micro (corrige sa couleur)", { X + 14, 162, Wd - 28, 16 }, 10.5f, dim, false);

        step (2, "NIVEAU DU TEST", 186);
        text (g, juce::String (juce::CharPointer_UTF8 ("Pendant le bruit rose, le vum\xc3\xa8tre micro doit \xc3\xaatre vers -20 dB (orange).")), { X + 14, 246, Wd - 28, 28 }, 10.5f, dim, false);

        step (3, juce::String (juce::CharPointer_UTF8 ("MESURE (3 \xc3\xa0 5 positions)")), 270);
        const int st = proc.state.load();
        if (st == Calibre::Playing || st == Calibre::Armed || st == Calibre::Analysing)
        {
            auto pb = juce::Rectangle<float> (X + 14, 374, Wd - 28, 8);
            g.setColour (juce::Colour (0xff0b0c0e)); g.fillRoundedRectangle (pb, 3);
            g.setColour (green); g.fillRoundedRectangle (pb.withWidth (pb.getWidth() * proc.progress.load()), 3);
        }
        bool err = false;
        const auto s = proc.getStatus (err);
        text (g, s, { X + 14, 386, Wd - 28, 50 }, 11.5f, err ? amber : txt, false, juce::Justification::topLeft);
        text (g, juce::String (juce::CharPointer_UTF8 ("Micro \xc3\xa0 hauteur des oreilles, \xc3\xa0 ta place. TAK35 : face avant (logo TAK35) tourn\xc3\xa9" "e vers le milieu des enceintes.")), { X + 14, 444, Wd - 28, 36 }, 10.5f, dim, false, juce::Justification::topLeft);

        // bas : réglages + résultats
        box ({ 20, 500, 760, 182 });
        box ({ 800, 500, 360, 182 });
        text (g, "CORRECTION", { 32, 504, 200, 18 }, 12, amber);
        int kx = 70;
        for (auto& t : juce::StringArray { juce::String (juce::CharPointer_UTF8 ("QUANTIT\xc3\x89")), "BOOST MAX", "CUT MAX", "FREQ MIN", "FREQ MAX" })
        {
            text (g, t, { (float) kx - 45, 582, 90, 14 }, 10.5f, txt, true, juce::Justification::centred);
            kx += 92;
        }
        kx = 70;
        for (auto* id : { "amount", "maxBoost", "maxCut", "loLim", "hiLim" })
        {
            auto* prm = proc.apvts.getParameter (id);
            text (g, prm->getCurrentValueAsText(), { (float) kx - 45, 597, 90, 14 }, 10.5f, dim, false, juce::Justification::centred);
            kx += 92;
        }
        text (g, "CIBLE", { 500, 512, 170, 16 }, 10.5f, txt);
        text (g, juce::String (juce::CharPointer_UTF8 ("Freq max : 500 Hz conseill\xc3\xa9 avec un micro voix (au-dessus, il colore la mesure).")), { 32, 620, 740, 16 }, 10.5f, dim, false);
        const auto& dd = graph.d;
        if (dd.valid)
        {
            text (g, "Gauche : " + dd.bandsText[0], { 32, 640, 740, 16 }, 10.5f, cyanL, false);
            text (g, "Droite : " + dd.bandsText[1], { 32, 658, 740, 16 }, 10.5f, pinkR, false);
        }

        text (g, juce::String (juce::CharPointer_UTF8 ("R\xc3\x89GLAGES HS8 CONSEILL\xc3\x89S (arri\xc3\xa8re)")), { 812, 504, 340, 18 }, 12, amber);
        auto sw = [&] (const juce::String& name, const juce::String& val, float y)
        {
            text (g, name, { 812, y, 160, 22 }, 12, txt);
            auto vb = juce::Rectangle<float> (972, y, 90, 22);
            g.setColour (juce::Colour (0xff0b0c0e)); g.fillRoundedRectangle (vb, 4);
            text (g, val, vb, 13, dd.valid ? amber : dim, true, juce::Justification::centred);
        };
        sw ("ROOM CONTROL", dd.valid ? dd.room : "-", 530);
        sw ("HIGH TRIM", dd.valid ? dd.trim : "-", 558);
        text (g, dd.valid ? dd.lrText : juce::String (juce::CharPointer_UTF8 ("\xc3\x89" "cart gauche/droite : -")), { 812, 588, 340, 30 }, 10.5f, txt, false, juce::Justification::topLeft);
        juce::String info;
        if (dd.valid)
            info << "Positions : " << dd.positions << "   Sortie auto : " << juce::String (dd.outGainDb, 1) << " dB\n"
                 << "Si tu changes un switch : EFFACER puis refais les mesures.";
        else info = juce::String (juce::CharPointer_UTF8 ("Mets les 2 HS8 au m\xc3\xaame volume (bouton LEVEL), switchs \xc3\xa0 0, puis mesure."));
        text (g, info, { 812, 622, 340, 54 }, 10.5f, dim, false, juce::Justification::topLeft);
        juce::ignoreUnused (r);
    }

    void paintMicMode (juce::Graphics& g)
    {
        using namespace ui;
        auto b = micInfo.toFloat();
        g.setColour (panel); g.fillRoundedRectangle (b, 8);
        g.setColour (line); g.drawRoundedRectangle (b, 8, 1);
        text (g, "MODE MICRO", b.removeFromTop (60).reduced (24, 10), 26, amber);
        const bool m = proc.masterConnected();
        g.setColour (m ? green : red);
        g.fillEllipse (b.getX() + 24, b.getY() + 6, 14, 14);
        text (g, m ? juce::String (juce::CharPointer_UTF8 ("Reli\xc3\xa9 au BAKR CALIBRE du Master")) : "BAKR CALIBRE (mode ENCEINTES) introuvable sur le Master",
              { b.getX() + 46, b.getY(), 800, 26 }, 15, m ? green : red);
        auto mb = juce::Rectangle<float> (b.getX() + 24, b.getY() + 50, b.getWidth() - 48, 26);
        g.setColour (juce::Colour (0xff0b0c0e)); g.fillRoundedRectangle (mb, 4);
        const float frac = juce::jlimit (0.0f, 1.0f, (micDb + 60.0f) / 60.0f);
        { juce::ColourGradient gr (green, mb.getX(), 0, red, mb.getRight(), 0, false); gr.addColour (0.75, amber); g.setGradientFill (gr); }
        g.fillRoundedRectangle (mb.reduced (3).withWidth ((mb.getWidth() - 6) * frac), 3);
        text (g, "Niveau micro : " + (micDb > -99 ? juce::String (juce::roundToInt (micDb)) + " dBFS" : juce::String ("silence")), { mb.getX(), mb.getBottom() + 4, 400, 18 }, 12, txt, false);
        text (g,
              juce::String (juce::CharPointer_UTF8 ("Cette piste \xc3\xa9" "coute la pi\xc3\xa8" "ce pendant les mesures. Elle ne laisse sortir aucun son (pas de larsen).\n\n" "1. Sur cette piste du mixeur, choisis l'entr\xc3\xa9" "e de ton micro (en haut \xc3\xa0 droite du mixeur).\n" "2. Sur la piste Master, ajoute un autre BAKR CALIBRE en mode ENCEINTES.\n" "3. Ouvre le BAKR CALIBRE du Master et lance les mesures depuis l\xc3\xa0.\n\n" "Apr\xc3\xa8s la calibration, tu peux enlever ce plugin (ou le laisser, il ne fait rien d'autre).")),
              { b.getX() + 24, b.getY() + 110, b.getWidth() - 48, 200 }, 14, txt, false, juce::Justification::topLeft);
    }

    void resized() override {}

private:
    void setRole (int r)
    {
        if (auto* prm = proc.apvts.getParameter ("role"))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) r));
            prm->endChangeGesture();
        }
    }

    void chooseCalFile()
    {
        chooser = std::make_unique<juce::FileChooser> ("Fichier de calibration du micro", juce::File(), "*.txt;*.cal;*.frd");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (! f.existsAsFile()) return;
            juce::String err;
            auto c = loadCalFile (f, err);
            if (c.empty()) { proc.setStatus (err, true); return; }
            proc.setCustomMic (c, f.getFileName());
            if (auto* prm = proc.apvts.getParameter ("micType")) prm->setValueNotifyingHost (prm->convertTo0to1 (5.0f));
            proc.setStatus (juce::String (juce::CharPointer_UTF8 ("Fichier de calibration charg\xc3\xa9 : ")) + f.getFileName(), false);
        });
    }

    void timerCallback() override
    {
        const bool mic = proc.isMicRole();
        roleSpk.setToggleState (! mic, juce::dontSendNotification);
        roleMic.setToggleState (mic, juce::dontSendNotification);
        for (juce::Component* c : std::initializer_list<juce::Component*> { &graph, &viewL, &viewR, &viewLR, &micType, &loadCal, &level, &noise, &measure, &undo, &clear,
                                  &kAmount, &kBoost, &kCut, &kLo, &kHi, &target, &corrOn, &lvlM, &dlyM, &expB })
            c->setVisible (! mic);

        // vumètre micro (lu dans le lien partagé, avec retombée)
        auto& link = Link::get();
        const float pk = link.micPeak.exchange (0.0f);
        const float db = (proc.micConnected() && pk > 0) ? juce::Decibels::gainToDecibels (pk, -100.0f) : -100.0f;
        micDb = db > micDb ? db : juce::jmax (-100.0f, micDb - 1.5f);

        const int st = proc.state.load();
        const bool busy = st != Calibre::Idle;
        measure.setEnabled (! busy);
        undo.setEnabled (! busy); clear.setEnabled (! busy);
        noise.setToggleState (proc.noiseOn.load(), juce::dontSendNotification);
        const auto d = proc.getDisplay();
        measure.setButtonText (busy ? "MESURE EN COURS..." : "MESURER  (position " + juce::String (juce::jmin (9, d.positions + 1)) + ")");
        viewL.setToggleState (graph.view == 0, juce::dontSendNotification);
        viewR.setToggleState (graph.view == 1, juce::dontSendNotification);
        viewLR.setToggleState (graph.view == 2, juce::dontSendNotification);

        const int v = proc.getDisplayVersion();
        if (v != lastVersion) { lastVersion = v; graph.d = d; graph.repaint(); }
        if (++slow % 3 == 0) graph.repaint();
        repaint (juce::Rectangle<int> (800, 70, 360, 420));
        repaint (juce::Rectangle<int> (20, 500, 1140, 182));
        if (mic) repaint();
    }

    Calibre& proc;
    bakr::LookAndFeel lnf;
    juce::LookAndFeel_V4 plainLnf;
    Graph graph;
    juce::TextButton roleSpk { "ENCEINTES (Master)" }, roleMic { "MICRO (piste du micro)" };
    juce::TextButton viewL { "G" }, viewR { "D" }, viewLR { "G+D" };
    juce::ComboBox micType, target;
    juce::TextButton loadCal { "Charger .txt" }, noise { "BRUIT ROSE" }, measure { "MESURER" }, undo { juce::String (juce::CharPointer_UTF8 ("Annuler la derni\xc3\xa8re")) }, clear { "EFFACER tout" };
    juce::Slider level, kAmount, kBoost, kCut, kLo, kHi;
    juce::ToggleButton corrOn { "CORRECTION" }, lvlM { "NIVEAU G/D" }, dlyM { juce::String (juce::CharPointer_UTF8 ("D\xc3\x89LAI G/D")) }, expB { "BYPASS EXPORT" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> micAtt, targetAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> atts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btnAtts;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> micInfo;
    float micDb = -100.0f;
    int lastVersion = -1, slow = 0;
};

class CalibreEditor : public juce::AudioProcessorEditor
{
public:
    explicit CalibreEditor (Calibre& p) : AudioProcessorEditor (&p), content (p)
    {
        addAndMakeVisible (content);
        content.setBounds (0, 0, Content::W, Content::H);
        setResizable (true, true);
        setResizeLimits (Content::W / 2, Content::H / 2, Content::W * 2, Content::H * 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) Content::W / Content::H);
        setSize (Content::W, Content::H);
    }
    void resized() override
    {
        const float s = (float) getWidth() / (float) Content::W;
        content.setTransform (juce::AffineTransform::scale (s));
    }
private:
    Content content;
    juce::TooltipWindow tips { this, 600 };
};

juce::AudioProcessorEditor* Calibre::createEditor() { return new CalibreEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Calibre(); }

#ifdef CALIBRE_TEST
// accès pour l'outil de test
Calibre* calibreCast (juce::AudioProcessor* p) { return dynamic_cast<Calibre*> (p); }
juce::String calibreStart (juce::AudioProcessor* p) { return calibreCast (p)->startMeasurement(); }
int calibreState (juce::AudioProcessor* p) { return calibreCast (p)->state.load(); }
juce::String calibreReport (juce::AudioProcessor* p)
{
    auto* c = calibreCast (p);
    bool e; auto s = c->getStatus (e);
    auto d = c->getDisplay();
    juce::String r;
    r << "status: " << s << "\n" << "room " << d.room << " trim " << d.trim << "\n" << d.lrText << "\n"
      << "L: " << d.bandsText[0] << "\nR: " << d.bandsText[1] << "\n"
      << "trim " << d.trimDb[0] << "/" << d.trimDb[1] << " delay " << d.delayMs[0] << "/" << d.delayMs[1] << " out " << d.outGainDb << "\n";
    for (double f : { 30.0, 45.0, 55.0, 60.0, 80.0, 100.0, 150.0, 300.0, 1000.0, 5000.0 })
    {
        const int i = (int) std::round (gridPos (f));
        if (d.valid) r << "  " << f << " Hz: meas " << juce::String (d.meas[0][(size_t) i], 1) << " corr " << juce::String (d.corr[0][(size_t) i], 1) << "\n";
    }
    return r;
}
#endif
