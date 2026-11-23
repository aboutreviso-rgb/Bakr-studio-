// BAKR CASQUE PRO — calibration de casque + émulation d'enceintes et de pièces + outils de contrôle
// (pensé pour le beyerdynamic DT 880)
//   CALIBRATION : corrige la courbe du casque vers une cible (Studio neutre, Harman, Chaude) + réglages BASS / TREBLE.
//   ÉMULATION   : modèle de tête (retard, ombre, oreille), 5 enceintes, 6 pièces, angle et quantité de pièce.
//   CONTRÔLE    : Stéréo / Mono / Gauche / Droite / Side / Inverser, DIM -20 dB, LUFS court terme, crête, corrélation.
//   SAFE MODE   : coupe à l'export + limiteur doux à -1 dBFS.
// Zéro latence : filtres IIR, le son direct n'est jamais retardé.
#include "../../Common/BakrUI.h"
#include "../Calibre/CalibreDSP.h"

#define T(x) juce::String::fromUTF8 (x)
using namespace calib;

namespace
{
    using Pts = std::vector<std::pair<float, float>>;
    struct Phone { const char* name; Pts dev; };   // écart du casque à une réponse neutre (dB) — estimations moyennes

    const std::vector<Phone>& phones()
    {
        // DT 880 : grave qui manque, un peu trop de haut-grave / bas-médium, pic « beyer » vers 6 kHz
        static const Pts dt880 { { 20, -5.0f }, { 35, -3.5f }, { 60, -1.5f }, { 120, 0.5f }, { 250, 1.5f }, { 500, 0.5f }, { 1000, 0 }, { 2000, -0.5f }, { 3200, -1.5f }, { 4800, 2.0f }, { 5900, 7.0f }, { 7000, 2.5f }, { 8500, 0.5f }, { 10500, 2.5f }, { 13000, -1.0f }, { 16000, -3.0f }, { 20000, -6.0f } };
        static const std::vector<Phone> list {
            { "DT 880 (32 ohms)",           dt880 },
            { "DT 880 (250 ohms)",          dt880 },
            { "DT 880 (600 ohms)",          dt880 },
            { "DT 990 PRO (250 ohms)",      { { 20, -2.0f }, { 50, 1.0f }, { 100, 2.0f }, { 250, 0.5f }, { 1000, 0 }, { 3000, -1.5f }, { 5000, 2.0f }, { 6500, 5.0f }, { 8500, 8.0f }, { 10500, 4.0f }, { 13000, 1.0f }, { 16000, -2.0f }, { 20000, -6.0f } } },
            { "DT 770 PRO (80 / 250 ohms)", { { 20, 1.0f }, { 50, 3.0f }, { 100, 3.0f }, { 250, 1.0f }, { 600, -0.5f }, { 1000, 0 }, { 3000, -2.0f }, { 5000, 1.0f }, { 6000, 3.0f }, { 8500, 7.0f }, { 10500, 3.0f }, { 14000, 0.0f }, { 20000, -5.0f } } },
            { "Aucune correction (plat)",   { } },
        };
        return list;
    }
    juce::StringArray phoneNames() { juce::StringArray s; for (auto& p : phones()) s.add (p.name); return s; }

    const juce::StringArray kTargets { "Studio neutre", "Harman (grave +)", "Chaude (douce)" };
    inline float targetDb (int t, double f)
    {
        auto lowShelf = [f] (double fc, double g) { return g / (1.0 + std::exp (std::log2 (f / fc) * 3.0)); };
        auto tilt = [f] (double from, double dbOct) { return f > from ? dbOct * std::log2 (f / from) : 0.0; };
        switch (t)
        {
            case 1:  return (float) (lowShelf (105.0, 4.5) + tilt (2000.0, -0.6));
            case 2:  return (float) (lowShelf (120.0, 2.0) + tilt (1000.0, -0.9));
            default: return 0.0f;
        }
    }

    // enceintes virtuelles : couleur (dB) + coupe-bas naturel
    struct Speaker { const char* name; Pts eq; float lowCut; bool mono; };
    const std::vector<Speaker>& speakers()
    {
        static const std::vector<Speaker> s {
            { "Neutre (enceintes parfaites)", { }, 0, false },
            { "Moniteurs 8\" (style HS8)",    { { 30, -6 }, { 45, -2 }, { 60, 0 }, { 2000, 0.8f }, { 4000, 1.2f }, { 10000, 0.5f }, { 20000, -1.5f } }, 0, false },
            { "Moniteur blanc (style NS-10)", { { 60, -9 }, { 100, -4 }, { 200, -1 }, { 800, 1.5f }, { 1500, 4.0f }, { 3000, 2.0f }, { 8000, -1.5f }, { 16000, -5 } }, 55, false },
            { "Petit cube mono",              { { 100, -12 }, { 200, -4 }, { 500, 1 }, { 1500, 4.0f }, { 3000, 2.0f }, { 6000, -3 }, { 9000, -10 }, { 14000, -20 } }, 140, true },
            { "Hi-fi salon",                  { { 40, 3.0f }, { 80, 3.5f }, { 200, 1.0f }, { 1000, 0 }, { 3000, -0.5f }, { 10000, 2.0f }, { 16000, 2.5f } }, 0, false },
        };
        return s;
    }
    juce::StringArray speakerNames() { juce::StringArray s; for (auto& p : speakers()) s.add (p.name); return s; }

    // pièces : taille (échelle des réflexions), RT60, niveau des réflexions, mode grave, brillance
    struct RoomDef { const char* name; float size, rt, er, bassDb, bassF, bright; };
    const std::vector<RoomDef>& rooms()
    {
        static const std::vector<RoomDef> r {
            { "Studio traité",  1.00f, 0.25f, 0.45f,  0.0f,  80, 6500 },
            { "Grand studio",   1.80f, 0.45f, 0.40f,  1.0f,  60, 8000 },
            { "Salon",          1.30f, 0.60f, 0.55f,  2.0f, 120, 9000 },
            { "Voiture",        0.35f, 0.12f, 0.65f,  5.0f,  70, 5000 },
            { "Club",           2.60f, 1.00f, 0.35f,  4.0f,  55, 5500 },
            { "Chambre",        0.80f, 0.40f, 0.60f,  2.5f, 110, 7000 },
        };
        return r;
    }
    juce::StringArray roomNames() { juce::StringArray s; for (auto& p : rooms()) s.add (juce::String::fromUTF8 (p.name)); return s; }

    const juce::StringArray kMonitor { T("STÉRÉO"), "MONO", "GAUCHE", "DROITE", "SIDE", "INVERSER" };

    constexpr int kCalBands = 10, kSpkBands = 6;
    const char* const kWatched[] = { "model", "calAmt", "target", "bass", "treble", "speaker", "roomType" };
}

//==============================================================================
class Casque : public juce::AudioProcessor, private juce::AsyncUpdater, private juce::AudioProcessorValueTreeState::Listener
{
public:
    Casque()
        : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          apvts (*this, nullptr, "BAKR_CASQUE", layout())
    {
        for (auto* id : kWatched) apvts.addParameterListener (id, this);
        triggerAsyncUpdate();
    }
    ~Casque() override { for (auto* id : kWatched) apvts.removeParameterListener (id, this); cancelPendingUpdate(); }

    static juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        using namespace bakr;
        Layout l;
        addChoice (l, "model", "Casque", phoneNames(), 1);
        addBool (l, "calOn", "Calibration", true);
        addFloat (l, "calAmt", "Force de la calibration", 0, 100, 100, "%", 0, 0);
        addChoice (l, "target", "Cible", kTargets, 0);
        addFloat (l, "bass", "Bass", -6, 6, 0, "dB");
        addFloat (l, "treble", "Treble", -6, 6, 0, "dB");
        addBool (l, "emuOn", "Emulation enceintes", true);
        addChoice (l, "speaker", "Enceintes", speakerNames(), 1);
        addChoice (l, "roomType", "Type de piece", roomNames(), 0);
        addFloat (l, "angle", "Angle des enceintes", 30, 120, 60, "deg", 0, 0);
        addFloat (l, "room", "Piece", 0, 100, 25, "%", 0, 0);
        addChoice (l, "monitor", "Ecoute", kMonitor, 0);
        addBool (l, "dim", "Dim -20 dB", false);
        addBool (l, "safe", "Safe mode", true);
        addFloat (l, "out", "Volume", -24, 6, 0, "dB");
        addBool (l, "bypass", "Bypass", false);
        return l;
    }
    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    const juce::String getName() const override { return "BAKR CASQUE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
    }

    void prepareToPlay (double sampleRate, int) override
    {
        fs = sampleRate;
        for (auto& c : ch) c.reset();
        for (auto& d : itd) d.assign (1024, 0.0f);
        itdW = 0;
        for (auto& b : erBuf) b.assign (16384, 0.0f);
        erW = 0;
        for (auto& f : fdn) { f.buf.assign (16384, 0.0f); f.w = 0; f.lp = 0; }
        for (auto* s : { &calMix, &emuMix, &dry, &dimG }) s->reset (sampleRate, 0.05);
        calMix.setCurrentAndTargetValue (0); emuMix.setCurrentAndTargetValue (0); dry.setCurrentAndTargetValue (0); dimG.setCurrentAndTargetValue (1);
        lastAngle = -1; lastRoomType = -1;
        // pondération K (LUFS) : pré-filtre aigu + passe-haut RLB
        for (auto& k : kw) { k.a.z1 = k.a.z2 = k.b.z1 = k.b.z2 = 0; k.a.k = shelfCoefs (fs, 1681.97, 4.0, true); k.b.k = hpCoefs (fs, 38.13, 0.5); }
        lufsBlock = (int) (0.1 * fs); lufsAcc = 0; lufsN = 0; lufsHist.fill (0.0); lufsIdx = 0;
        corrXY = corrXX = corrYY = 0;
        triggerAsyncUpdate();
    }
    void releaseResources() override {}

    //--------------------------------------------------------------------------
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals nd;
        const int n = buffer.getNumSamples();
        if (n <= 0 || buffer.getNumChannels() < 2) return;

        if (calDirty.load())
        {
            const juce::SpinLock::ScopedTryLockType tl (calSpin);
            if (tl.isLocked())
            {
                // fondu de 40 ms entre l'ancienne et la nouvelle correction : aucun clic quand on change un réglage
                oldNb = calAudio.nb; oldPre = calAudio.preamp;
                calAudio = calPending; calDirty.store (false);
                xfLen = juce::jmax (1, (int) (0.04 * fs)); xfPos = 0;
                for (auto& c : ch)
                {
                    c.calOld = c.cal;
                    for (int b = 0; b < kCalBands; ++b) { c.cal[(size_t) b].k = calAudio.k[b]; c.cal[(size_t) b].z1 = c.cal[(size_t) b].z2 = 0; }
                    bool spkChanged = false;
                    for (int b = 0; b < kSpkBands; ++b) spkChanged = spkChanged || std::memcmp (&c.spk[(size_t) b].k, &calAudio.spk[b], sizeof (Coefs)) != 0;
                    if (spkChanged || std::memcmp (&c.spkHp1.k, &calAudio.hp1, sizeof (Coefs)) != 0)
                    {
                        for (int b = 0; b < kSpkBands; ++b) { c.spk[(size_t) b].k = calAudio.spk[b]; c.spk[(size_t) b].z1 = c.spk[(size_t) b].z2 = 0; }
                        c.spkHp1.k = calAudio.hp1; c.spkHp2.k = calAudio.hp2;
                        c.spkHp1.z1 = c.spkHp1.z2 = c.spkHp2.z1 = c.spkHp2.z2 = 0;
                    }
                }
            }
        }
        const float ang = p ("angle");
        const int rt = (int) p ("roomType");
        if (std::abs (ang - lastAngle) > 0.01f || rt != lastRoomType) { lastAngle = ang; lastRoomType = rt; setupEmulation (ang, rt); }

        const bool exporting = isNonRealtime() && p ("safe") > 0.5f;
        const bool off = p ("bypass") > 0.5f || exporting;
        dry.setTargetValue (off ? 1.0f : 0.0f);
        calMix.setTargetValue (p ("calOn") > 0.5f ? 1.0f : 0.0f);
        emuMix.setTargetValue (p ("emuOn") > 0.5f ? 1.0f : 0.0f);
        dimG.setTargetValue (p ("dim") > 0.5f ? 0.1f : 1.0f);
        const float room = p ("room") * 0.01f;
        const float outG = juce::Decibels::decibelsToGain (p ("out"));
        const float pre = calAudio.preamp;
        const bool safe = p ("safe") > 0.5f;
        const int mon = exporting ? 0 : (int) p ("monitor");
        const bool spkMono = calAudio.spkMono;

        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);
        float pk = 0, emuLvl = 0;
        for (int i = 0; i < n; ++i)
        {
            const float xl = L[i], xr = R[i];
            const float cm = calMix.getNextValue(), em = emuMix.getNextValue(), dm = dry.getNextValue(), dg = dimG.getNextValue();

            // 0. mode d'écoute (contrôle du mix)
            float il = xl, ir = xr;
            switch (mon)
            {
                case 1: il = ir = 0.5f * (xl + xr); break;
                case 2: ir = il = xl; break;
                case 3: il = ir = xr; break;
                case 4: il = 0.5f * (xl - xr); ir = -il; break;
                case 5: std::swap (il, ir); break;
                default: break;
            }

            // 1. émulation d'enceintes + pièce
            double l = il, r = ir;
            if (em > 1.0e-4f)
            {
                double sl = il, sr = ir;
                if (spkMono) sl = sr = 0.5 * (il + ir);
                sl = ch[0].spkHp2.process (ch[0].spkHp1.process (sl)); sr = ch[1].spkHp2.process (ch[1].spkHp1.process (sr));
                for (int b = 0; b < kSpkBands; ++b) { sl = ch[0].spk[(size_t) b].process (sl); sr = ch[1].spk[(size_t) b].process (sr); }

                itd[0][(size_t) itdW] = (float) sl; itd[1][(size_t) itdW] = (float) sr;
                const float dl = readFrac (itd[0], itdSamples), dr = readFrac (itd[1], itdSamples);
                itdW = (itdW + 1) & 1023;
                // oreille opposée : retard (ITD), niveau plus bas, ombre de la tête, creux de pavillon
                const double cL = ch[0].notch.process (ch[0].shadow2.process (ch[0].shadow.process (dr))) * contraGain;
                const double cR = ch[1].notch.process (ch[1].shadow2.process (ch[1].shadow.process (dl))) * contraGain;
                double el = ch[0].pinna.process (sl) + cL, er = ch[1].pinna.process (sr) + cR;
                el *= normGain; er *= normGain;

                if (room > 0.001f)
                {
                    // réflexions précoces stéréo (chaque enceinte se réfléchit sur les murs)
                    erBuf[0][(size_t) erW] = (float) el; erBuf[1][(size_t) erW] = (float) er;
                    double rl = 0, rr = 0;
                    for (int t = 0; t < 8; ++t)
                    {
                        const auto& tp = taps[(size_t) t];
                        const float vl = erBuf[0][(size_t) ((erW - tp.d) & 16383)], vr = erBuf[1][(size_t) ((erW - tp.d) & 16383)];
                        if (tp.cross) { rl += vr * tp.g; rr += vl * tp.g; } else { rl += vl * tp.g; rr += vr * tp.g; }
                    }
                    erW = (erW + 1) & 16383;
                    // réverbération : FDN 4 lignes amortie
                    float o[4];
                    for (int k = 0; k < 4; ++k) o[k] = fdn[(size_t) k].buf[(size_t) ((fdn[(size_t) k].w - fdn[(size_t) k].len) & 16383)];
                    const float h = 0.5f;
                    const float mm[4] = { h * (o[0] + o[1] + o[2] + o[3]), h * (o[0] - o[1] + o[2] - o[3]), h * (o[0] + o[1] - o[2] - o[3]), h * (o[0] - o[1] - o[2] + o[3]) };
                    for (int k = 0; k < 4; ++k)
                    {
                        auto& f = fdn[(size_t) k];
                        f.lp += damp * (mm[k] * f.g - f.lp);
                        f.buf[(size_t) f.w] = f.lp + (float) (k < 2 ? rl : rr) * 0.3f;
                        f.w = (f.w + 1) & 16383;
                    }
                    double wl = rl * erGain + (o[0] + o[2] * 0.5) * revGain;
                    double wr = rr * erGain + (o[1] + o[3] * 0.5) * revGain;
                    wl = ch[0].roomBass.process (ch[0].roomLp.process (wl)); wr = ch[1].roomBass.process (ch[1].roomLp.process (wr));
                    // la pièce colore aussi le son direct (modes graves)
                    const double bl = ch[0].directBass.process (el), br = ch[1].directBass.process (er);
                    el = el + (bl - el) * room; er = er + (br - er) * room;
                    el = el * (1.0 - room * 0.2) + wl * room * 0.6;
                    er = er * (1.0 - room * 0.2) + wr * room * 0.6;
                }
                l = il + (el - il) * em; r = ir + (er - ir) * em;
                emuLvl = juce::jmax (emuLvl, (float) std::abs (cL));
            }

            // 2. calibration du casque (+ cible + bass / treble)
            if (cm > 1.0e-4f)
            {
                double cl = l, cr = r;
                for (int b = 0; b < calAudio.nb; ++b) { cl = ch[0].cal[(size_t) b].process (cl); cr = ch[1].cal[(size_t) b].process (cr); }
                if (xfPos < xfLen)
                {
                    double ol0 = l, or0 = r;
                    for (int b = 0; b < oldNb; ++b) { ol0 = ch[0].calOld[(size_t) b].process (ol0); or0 = ch[1].calOld[(size_t) b].process (or0); }
                    const double t = (double) ++xfPos / xfLen;
                    const double gNew = 1.0, gOld = oldPre / juce::jmax (1.0e-6f, calAudio.preamp);   // même volume pendant le fondu
                    cl = cl * gNew * t + ol0 * gOld * (1.0 - t);
                    cr = cr * gNew * t + or0 * gOld * (1.0 - t);
                }
                l += (cl - l) * cm; r += (cr - r) * cm;
            }

            const float g = outG * dg * (1.0f + (pre - 1.0f) * cm);   // la marge de sécurité ne s'applique qu'avec la calibration
            float ol = (float) l * g, orr = (float) r * g;
            if (safe) { ol = soft (ol); orr = soft (orr); }
            L[i] = ol + (xl - ol) * dm;
            R[i] = orr + (xr - orr) * dm;
            pk = juce::jmax (pk, std::abs (L[i]), std::abs (R[i]));

            // mesures sur le MIX d'entrée (ce qui sera exporté)
            const double kl = kw[0].b.process (kw[0].a.process (xl)), kr = kw[1].b.process (kw[1].a.process (xr));
            lufsAcc += kl * kl + kr * kr;
            if (++lufsN >= lufsBlock)
            {
                lufsHist[(size_t) lufsIdx] = lufsAcc / lufsN; lufsIdx = (lufsIdx + 1) % 30; lufsAcc = 0; lufsN = 0;
                double s = 0; for (double v : lufsHist) s += v;
                lufs.store ((float) (-0.691 + 10.0 * std::log10 (s / 30.0 + 1e-12)));
            }
            corrXY = corrXY * 0.9995 + (double) xl * xr; corrXX = corrXX * 0.9995 + (double) xl * xl; corrYY = corrYY * 0.9995 + (double) xr * xr;
            inPeak = juce::jmax (inPeak, std::abs (xl), std::abs (xr));
        }
        correlation.store ((float) (corrXX * corrYY > 1e-12 ? corrXY / std::sqrt (corrXX * corrYY) : 1.0));
        mixPeak.store (juce::jmax (inPeak, mixPeak.load())); inPeak = 0;
        outPeak.store (juce::jmax (pk, outPeak.load()));
        crossLevel.store (juce::jmax (emuLvl, crossLevel.load()));
        isExporting.store (exporting);
    }

    struct View { Curve original, corr, result, target; };
    View getView() const { const juce::ScopedLock sl (viewLock); return view; }
    int viewVersion() const { return viewVer.load(); }

    std::atomic<float> outPeak { 0 }, mixPeak { 0 }, crossLevel { 0 }, lufs { -70.0f }, correlation { 1.0f };
    std::atomic<bool> isExporting { false };
    double fs = 48000.0;
    juce::AudioProcessorValueTreeState apvts;

    void getStateInformation (juce::MemoryBlock& d) override { if (auto x = apvts.copyState().createXml()) copyXmlToBinary (*x, d); }
    void setStateInformation (const void* data, int size) override
    {
        if (auto x = getXmlFromBinary (data, size)) apvts.replaceState (juce::ValueTree::fromXml (*x));
        triggerAsyncUpdate();
    }

private:
    static float soft (float x)
    {
        const float lim = 0.891f;   // -1 dBFS
        const float a = std::abs (x);
        return a <= lim ? x : std::copysign (lim + (1.0f - lim) * std::tanh ((a - lim) / (1.0f - lim)), x);
    }
    float readFrac (const std::vector<float>& b, float d) const
    {
        const int di = (int) d; const float fr = d - di;
        const float a = b[(size_t) ((itdW - di) & 1023)], c = b[(size_t) ((itdW - di - 1) & 1023)];
        return a + (c - a) * fr;
    }

    struct BQ
    {
        Coefs k; double z1 = 0, z2 = 0;
        inline double process (double x) noexcept { const double y = k.b0 * x + z1; z1 = k.b1 * x - k.a1 * y + z2; z2 = k.b2 * x - k.a2 * y; return y; }
    };
    struct Chan
    {
        std::array<BQ, kCalBands> cal, calOld;
        std::array<BQ, kSpkBands> spk;
        BQ spkHp1, spkHp2, shadow, shadow2, notch, pinna, roomLp, roomBass, directBass;
        void reset()
        {
            for (auto& b : cal) b.z1 = b.z2 = 0;
            for (auto& b : calOld) b.z1 = b.z2 = 0;
            for (auto& b : spk) b.z1 = b.z2 = 0;
            for (auto* b : { &spkHp1, &spkHp2, &shadow, &shadow2, &notch, &pinna, &roomLp, &roomBass, &directBass }) b->z1 = b->z2 = 0;
        }
    };
    struct CalSet { Coefs k[kCalBands]; int nb = 0; float preamp = 1.0f; Coefs spk[kSpkBands]; Coefs hp1, hp2; bool spkMono = false; };
    struct Line { std::vector<float> buf; int w = 0, len = 1000; float g = 0.7f, lp = 0; };
    struct Tap { int d = 1; float g = 0; bool cross = false; };
    struct KW { BQ a, b; };

    static Coefs shelfCoefs (double fs, double f, double g, bool high)
    {
        Coefs c; const double A = std::pow (10.0, g / 40.0), w = 2 * kPiD * f / fs, cs = std::cos (w), sn = std::sin (w), al = sn / 2 * std::sqrt (2.0);
        const double sA = 2 * std::sqrt (A) * al;
        if (high)
        {
            const double a0 = (A + 1) - (A - 1) * cs + sA;
            c.b0 = A * ((A + 1) + (A - 1) * cs + sA) / a0; c.b1 = -2 * A * ((A - 1) + (A + 1) * cs) / a0; c.b2 = A * ((A + 1) + (A - 1) * cs - sA) / a0;
            c.a1 = 2 * ((A - 1) - (A + 1) * cs) / a0; c.a2 = ((A + 1) - (A - 1) * cs - sA) / a0;
        }
        else
        {
            const double a0 = (A + 1) + (A - 1) * cs + sA;
            c.b0 = A * ((A + 1) - (A - 1) * cs + sA) / a0; c.b1 = 2 * A * ((A - 1) - (A + 1) * cs) / a0; c.b2 = A * ((A + 1) - (A - 1) * cs - sA) / a0;
            c.a1 = -2 * ((A - 1) + (A + 1) * cs) / a0; c.a2 = ((A + 1) + (A - 1) * cs - sA) / a0;
        }
        return c;
    }
    static Coefs lpCoefs (double fs, double f, double q)
    {
        Coefs c; const double w = 2 * kPiD * f / fs, cs = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
        c.b0 = (1 - cs) / 2 / a0; c.b1 = (1 - cs) / a0; c.b2 = (1 - cs) / 2 / a0; c.a1 = -2 * cs / a0; c.a2 = (1 - al) / a0; return c;
    }
    static Coefs hpCoefs (double fs, double f, double q)
    {
        Coefs c; const double w = 2 * kPiD * f / fs, cs = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
        c.b0 = (1 + cs) / 2 / a0; c.b1 = -(1 + cs) / a0; c.b2 = (1 + cs) / 2 / a0; c.a1 = -2 * cs / a0; c.a2 = (1 - al) / a0; return c;
    }

    // modèle de tête (enceintes à ±angle/2) + pièce choisie
    void setupEmulation (float angleDeg, int roomType)
    {
        const double az = juce::degreesToRadians (angleDeg * 0.5), a = 0.0875, c = 343.0;
        itdSamples = (float) juce::jlimit (0.0, 1000.0, a / c * (az + std::sin (az)) * fs);    // Woodworth
        const double s = std::sin (az);
        contraGain = (float) juce::Decibels::decibelsToGain (-2.5 - 3.5 * s);
        const double shadowDb = -(4.0 + 10.0 * s);
        const auto& rd = rooms()[(size_t) juce::jlimit (0, (int) rooms().size() - 1, roomType)];
        for (auto& cc : ch)
        {
            cc.shadow.k = shelfCoefs (fs, 900.0 + 700.0 * (1.0 - s), shadowDb, true);
            cc.shadow2.k = lpCoefs (fs, juce::jmin (fs * 0.45, 9000.0 - 3000.0 * s), 0.6);
            cc.notch.k = peakCoefs (fs, juce::jmin (fs * 0.45, 8500.0 + 2500.0 * (1.0 - s)), 2.5, -4.0 * s);   // creux de pavillon
            cc.pinna.k = peakCoefs (fs, 3000.0, 1.2, 1.0 + 1.5 * s);
            cc.roomLp.k = lpCoefs (fs, juce::jmin (fs * 0.45, (double) rd.bright), 0.6);
            cc.roomBass.k = peakCoefs (fs, rd.bassF, 1.4, rd.bassDb);
            cc.directBass.k = peakCoefs (fs, rd.bassF, 2.0, rd.bassDb * 0.6);
        }
        normGain = (float) (1.0 / (1.0 + contraGain * 0.75));
        // réflexions : sol, plafond, murs latéraux, mur arrière, coins
        static const float baseMs[8] = { 3.1f, 5.3f, 6.7f, 8.9f, 11.6f, 14.2f, 17.9f, 23.5f };
        static const float baseG[8]  = { 0.55f, 0.45f, 0.42f, 0.36f, 0.30f, 0.26f, 0.20f, 0.15f };
        static const bool cross[8]   = { false, false, true, false, true, true, false, true };
        for (int t = 0; t < 8; ++t)
            taps[(size_t) t] = { (int) juce::jlimit (1.0, 16000.0, baseMs[t] * rd.size * 0.001 * fs), baseG[t], cross[t] };
        const int lens[4] = { 1031, 1327, 1523, 1871 };
        for (int k = 0; k < 4; ++k)
        {
            fdn[(size_t) k].len = juce::jlimit (64, 16000, (int) (lens[k] * rd.size * fs / 48000.0));
            fdn[(size_t) k].g = (float) std::pow (10.0, -3.0 * fdn[(size_t) k].len / (rd.rt * fs));
        }
        erGain = rd.er; revGain = 0.35f + 0.4f * juce::jlimit (0.0f, 1.0f, rd.rt);
        damp = juce::jlimit (0.2f, 0.7f, rd.bright / 12000.0f);
    }

    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override
    {
        const int mi = juce::jlimit (0, (int) phones().size() - 1, (int) p ("model"));
        const auto& ph = phones()[(size_t) mi];
        const float amt = p ("calAmt") * 0.01f, bass = p ("bass"), treble = p ("treble");
        const int tgt = (int) p ("target");
        Curve dev ((size_t) kGrid, 0.0f), target ((size_t) kGrid, 0.0f);
        for (int i = 0; i < kGrid; ++i)
        {
            const double f = gridFreq (i);
            dev[(size_t) i] = ph.dev.empty() ? 0.0f : interpPoints (ph.dev, f);
            // cible + goûts (bass / treble en plateaux doux)
            target[(size_t) i] = targetDb (tgt, f) + (float) (bass / (1.0 + std::exp (std::log2 (f / 110.0) * 2.5)))
                                                   + (float) (treble / (1.0 + std::exp (-std::log2 (f / 7000.0) * 2.5)));
        }
        FitParams fp; fp.lo = 20; fp.hi = 18000; fp.maxBoost = 8; fp.maxCut = 12; fp.amount = amt; fp.maxBands = kCalBands;
        auto bands = fitBands (dev, target, fp, fs);
        CalSet cs;
        cs.nb = juce::jmin (kCalBands, (int) bands.size());
        for (int b = 0; b < cs.nb; ++b) cs.k[b] = peakCoefs (fs, bands[(size_t) b].f, bands[(size_t) b].q, bands[(size_t) b].g);

        // enceintes virtuelles
        const auto& sp = speakers()[(size_t) juce::jlimit (0, (int) speakers().size() - 1, (int) p ("speaker"))];
        {
            Curve scol ((size_t) kGrid, 0.0f), zero ((size_t) kGrid, 0.0f);
            for (int i = 0; i < kGrid; ++i) scol[(size_t) i] = sp.eq.empty() ? 0.0f : -interpPoints (sp.eq, gridFreq (i));   // fitBands corrige -> on lui donne l'inverse
            FitParams sf; sf.lo = 30; sf.hi = 16000; sf.maxBoost = 6; sf.maxCut = 14; sf.amount = 1.0; sf.maxBands = kSpkBands;
            auto sb = sp.eq.empty() ? std::vector<Band>() : fitBands (scol, zero, sf, fs);
            for (int b = 0; b < kSpkBands; ++b) cs.spk[b] = b < (int) sb.size() ? peakCoefs (fs, sb[(size_t) b].f, sb[(size_t) b].q, sb[(size_t) b].g) : Coefs();
            if (sp.lowCut > 0) { cs.hp1 = hpCoefs (fs, sp.lowCut, 0.54); cs.hp2 = hpCoefs (fs, sp.lowCut, 1.31); }
            cs.spkMono = sp.mono;
        }

        View v; v.original = dev; v.target = target; v.corr = bandsCurve (bands, fs); v.result.resize ((size_t) kGrid);
        float mx = 0;
        for (int i = 0; i < kGrid; ++i)
        {
            v.result[(size_t) i] = dev[(size_t) i] + v.corr[(size_t) i];
            if (gridFreq (i) >= 30 && gridFreq (i) <= 16000) mx = juce::jmax (mx, v.corr[(size_t) i]);
        }
        cs.preamp = juce::Decibels::decibelsToGain (-mx * 0.6f);   // le reste est tenu par le limiteur doux (SAFE MODE)
        { const juce::SpinLock::ScopedLockType sl (calSpin); calPending = cs; calDirty.store (true); }
        { const juce::ScopedLock sl (viewLock); view = v; }
        viewVer.fetch_add (1);
    }

    std::array<Chan, 2> ch;
    std::array<std::vector<float>, 2> itd;
    int itdW = 0;
    float itdSamples = 10, contraGain = 0.4f, normGain = 0.8f, lastAngle = -1;
    int lastRoomType = -1;
    std::array<std::vector<float>, 2> erBuf; int erW = 0;
    std::array<Tap, 8> taps;
    std::array<Line, 4> fdn;
    float erGain = 0.45f, revGain = 0.5f, damp = 0.45f;
    int oldNb = 0, xfLen = 1, xfPos = 1;
    float oldPre = 1.0f;
    juce::LinearSmoothedValue<float> calMix, emuMix, dry, dimG;
    juce::SpinLock calSpin;
    CalSet calPending, calAudio;
    std::atomic<bool> calDirty { false };
    mutable juce::CriticalSection viewLock;
    View view;
    std::atomic<int> viewVer { 0 };
    // mesures
    std::array<KW, 2> kw;
    int lufsBlock = 4800, lufsN = 0, lufsIdx = 0;
    double lufsAcc = 0;
    std::array<double, 30> lufsHist {};
    double corrXY = 0, corrXX = 0, corrYY = 0;
    float inPeak = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Casque)
};

//==============================================================================
namespace cu
{
    const juce::Colour bg (0xff1b1d21), dark (0xff111214), txt (0xffe9ebee), dim (0xff8c929b),
                       orange (0xffff9a2e), pink (0xffff6fa3), cyan (0xff5fd2ff), green (0xff58d68d), red (0xffff5a4a), line (0x22ffffff);
    inline void text (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size, juce::Colour c, bool bold = false, juce::Justification j = juce::Justification::centred)
    {
        g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        g.setColour (c); g.drawFittedText (s, r.toNearestInt(), j, 2, 0.9f);
    }
    inline void disc (juce::Graphics& g, juce::Point<float> c, float r)
    {
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (juce::Rectangle<float> (r * 2 + 16, r * 2 + 16).withCentre (c.translated (0, 4)));
        juce::ColourGradient gr (juce::Colour (0xff3a3e45), c.x, c.y - r, juce::Colour (0xff15171a), c.x, c.y + r, false);
        g.setGradientFill (gr); g.fillEllipse (juce::Rectangle<float> (r * 2 + 12, r * 2 + 12).withCentre (c));
        g.setColour (dark); g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
        g.setColour (line); g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c), 1.0f);
    }
}

class PowerButton : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (2);
        const bool on = getToggleState();
        g.setColour (juce::Colour (0xff2b2e33)); g.fillEllipse (r);
        g.setColour (on ? cu::orange : juce::Colour (0xff4a4e55)); g.drawEllipse (r.reduced (1), 2.0f);
        auto c = r.getCentre(); const float rad = r.getWidth() * 0.22f;
        juce::Path pth; pth.addCentredArc (c.x, c.y, rad, rad, 0, 0.7f, juce::MathConstants<float>::twoPi - 0.7f, true);
        g.setColour (on ? cu::orange : (over ? cu::txt : cu::dim));
        g.strokePath (pth, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (c.x, c.y - rad - 2, c.x, c.y - 1, 2.0f);
    }
};

class SwitchButton : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics& g, bool, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (2);
        const bool on = getToggleState();
        g.setColour (on ? cu::orange.withAlpha (0.85f) : juce::Colour (0xff3a3d43)); g.fillRoundedRectangle (r, r.getHeight() / 2);
        const float d = r.getHeight() - 4;
        g.setColour (cu::txt); g.fillEllipse (on ? r.getRight() - d - 2 : r.getX() + 2, r.getY() + 2, d, d);
    }
};

class CasqueContent : public juce::Component, private juce::Timer
{
public:
    static constexpr int W = 1000, H = 760;
    explicit CasqueContent (Casque& p) : proc (p)
    {
        setLookAndFeel (&lnf);
        auto bAtt = [this] (juce::Button& b, const char* id) { addAndMakeVisible (b); batts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, b)); };
        bAtt (calOn, "calOn"); bAtt (emuOn, "emuOn"); bAtt (safe, "safe");
        auto pill = [] (juce::TextButton& b)
        {
            b.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b2e33));
            b.setColour (juce::TextButton::buttonOnColourId, cu::orange);
            b.setColour (juce::TextButton::textColourOffId, cu::txt);
            b.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        };
        pill (bypass); bypass.setClickingTogglesState (true); bAtt (bypass, "bypass");
        pill (dimB); dimB.setClickingTogglesState (true); bAtt (dimB, "dim");
        for (int i = 0; i < kMonitor.size(); ++i)
        {
            auto* b = monBtns.add (new juce::TextButton (kMonitor[i]));
            pill (*b);
            b->onClick = [this, i] { if (auto* prm = proc.apvts.getParameter ("monitor")) { prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i)); prm->endChangeGesture(); } };
            addAndMakeVisible (b);
        }

        auto combo = [this] (juce::ComboBox& c, const juce::StringArray& items, const char* id)
        {
            c.addItemList (items, 1); addAndMakeVisible (c);
            catts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, c));
        };
        combo (model, phoneNames(), "model"); combo (target, kTargets, "target");
        combo (speaker, speakerNames(), "speaker"); combo (roomType, roomNames(), "roomType");

        auto knob = [this] (juce::Slider& s, const char* id, const char* look)
        {
            s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            s.setRotaryParameters (-0.75f * juce::MathConstants<float>::pi, 0.75f * juce::MathConstants<float>::pi, true);
            s.getProperties().set ("look", look); s.getProperties().set ("tick", cu::txt.toString());
            s.setPopupDisplayEnabled (true, true, this);
            addAndMakeVisible (s);
            satts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s));
            if (auto* prm = proc.apvts.getParameter (id)) s.setDoubleClickReturnValue (true, proc.apvts.getParameterRange (id).convertFrom0to1 (prm->getDefaultValue()));
        };
        knob (room, "room", "black"); knob (calAmt, "calAmt", "grey"); knob (vol, "out", "chrome");
        knob (bass, "bass", "black"); knob (treble, "treble", "black");

        for (auto* b : { &angDown, &angUp })
        {
            b->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b2e33));
            b->setColour (juce::TextButton::textColourOffId, cu::txt);
            addAndMakeVisible (*b);
        }
        angDown.onClick = [this] { stepAngle (-5); };
        angUp.onClick = [this] { stepAngle (5); };

        calOn.setBounds (318, 340, 44, 44);
        emuOn.setBounds (888, 340, 44, 44);
        bypass.setBounds (840, 18, 130, 28);
        model.setBounds (30, 450, 210, 26); target.setBounds (250, 450, 160, 26);
        calAmt.setBounds (60, 506, 56, 56); bass.setBounds (180, 506, 56, 56); treble.setBounds (300, 506, 56, 56);
        speaker.setBounds (590, 450, 220, 26); roomType.setBounds (820, 450, 150, 26);
        room.setBounds (760, 500, 64, 64);
        angDown.setBounds (690, 252, 30, 30); angUp.setBounds (840, 252, 30, 30);
        safe.setBounds (474, 300, 52, 24);
        for (int i = 0; i < monBtns.size(); ++i) monBtns[i]->setBounds (430 + (i % 2) * 72, 362 + (i / 2) * 30, 68, 26);
        dimB.setBounds (430, 458, 140, 26);
        vol.setBounds (465, 520, 70, 70);
        startTimerHz (30);
    }
    ~CasqueContent() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override
    {
        using namespace cu;
        g.fillAll (bg);
        { juce::ColourGradient gr (juce::Colour (0xff2a2d33), 0, 0, bg, 0, (float) H * 0.6f, false); g.setGradientFill (gr); g.fillRect (getLocalBounds()); }
        text (g, "BAKR", { 24, 16, 80, 30 }, 22, txt, true, juce::Justification::centredLeft);
        text (g, "CASQUE PRO", { 96, 20, 180, 24 }, 16, orange, true, juce::Justification::centredLeft);
        text (g, T("calibration • enceintes & pièces virtuelles • contrôle du mix"), { 290, 18, 420, 26 }, 12.5f, dim);

        const juce::Point<float> cL (210, 215), cR (780, 215);
        const float R = 150;
        disc (g, cL, R);
        auto plot = juce::Rectangle<float> (cL.x - 115, cL.y - 78, 230, 128);
        g.setColour (line);
        for (double f : { 100.0, 1000.0, 10000.0 }) { const float x = xOf (plot, f); g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom()); }
        g.drawHorizontalLine ((int) yOf (plot, 0), plot.getX(), plot.getRight());
        drawCurve (g, plot, v.original, juce::Colours::white.withAlpha (0.45f), 1.4f, true);
        drawCurve (g, plot, v.target, green.withAlpha (0.6f), 1.2f, true);
        if (proc.p ("calOn") > 0.5f)
        {
            drawCurve (g, plot, v.corr, pink, 1.8f, false);
            drawCurve (g, plot, v.result, cyan, 2.0f, false);
        }
        auto legend = [&] (juce::Colour c, const juce::String& s, float x, float y)
        { g.setColour (c); g.fillEllipse (x, y + 5, 6, 6); text (g, s, { x + 10, y, 80, 16 }, 10.5f, txt, false, juce::Justification::centredLeft); };
        legend (juce::Colours::white.withAlpha (0.6f), "Original", cL.x - 92, cL.y + 58);
        legend (green, "Cible", cL.x + 6, cL.y + 58);
        legend (pink, "Correction", cL.x - 92, cL.y + 78);
        legend (cyan, T("Résultat"), cL.x + 6, cL.y + 78);
        text (g, "HEADPHONE CALIBRATION", { 40, 410, 390, 20 }, 15, txt, true);
        text (g, "CASQUE", { 30, 434, 210, 14 }, 10, dim, true, juce::Justification::centredLeft);
        text (g, "CIBLE", { 250, 434, 160, 14 }, 10, dim, true, juce::Justification::centredLeft);
        const char* kl[] = { "FORCE", "BASS", "TREBLE" }; const char* ki[] = { "calAmt", "bass", "treble" };
        for (int i = 0; i < 3; ++i)
        {
            const float x = 88.0f + i * 120.0f;
            text (g, kl[i], { x - 50, 564, 100, 14 }, 10.5f, txt, true);
            text (g, proc.apvts.getParameter (ki[i])->getCurrentValueAsText(), { x - 50, 578, 100, 14 }, 10.5f, dim);
        }

        disc (g, cR, R);
        const float ang = proc.p ("angle"), half = juce::degreesToRadians (ang * 0.5f);
        const bool emu = proc.p ("emuOn") > 0.5f;
        for (int s = -1; s <= 1; s += 2)
        {
            const float a = s * half;
            juce::Path wedge;
            const float r1 = 98, r2 = 132, w = 0.26f;
            wedge.addCentredArc (cR.x, cR.y, r2, r2, 0, a - w, a + w, true);
            wedge.addCentredArc (cR.x, cR.y, r1, r1, 0, a + w, a - w, false);
            wedge.closeSubPath();
            juce::ColourGradient gr (orange.withAlpha (emu ? 0.95f : 0.25f), cR.x, cR.y - r2, orange.withAlpha (emu ? 0.35f : 0.08f), cR.x, cR.y - r1, false);
            g.setGradientFill (gr); g.fillPath (wedge);
        }
        g.setColour (txt.withAlpha (0.85f));
        g.drawEllipse (cR.x - 18, cR.y - 52, 36, 42, 2.0f);
        g.fillEllipse (cR.x - 23, cR.y - 36, 6, 12); g.fillEllipse (cR.x + 17, cR.y - 36, 6, 12);
        juce::Path nose; nose.addTriangle (cR.x - 5, cR.y - 52, cR.x + 5, cR.y - 52, cR.x, cR.y - 59); g.fillPath (nose);
        text (g, juce::String (juce::roundToInt (ang)) + T("°"), { cR.x - 40, cR.y + 22, 80, 40 }, 24, txt, true);
        text (g, "LOUDSPEAKER ANGLE", { cR.x - 100, cR.y + 62, 200, 16 }, 11, dim);
        g.setColour (orange.withAlpha (0.25f + 0.75f * juce::jlimit (0.0f, 1.0f, xfeed * 3.0f)));
        g.fillEllipse (cR.x - 3, cR.y + 86, 6, 6);
        text (g, "LOUDSPEAKER EMULATION", { 570, 410, 390, 20 }, 15, txt, true);
        text (g, "ENCEINTES", { 590, 434, 220, 14 }, 10, dim, true, juce::Justification::centredLeft);
        text (g, T("PIÈCE"), { 820, 434, 150, 14 }, 10, dim, true, juce::Justification::centredLeft);
        text (g, "ROOM", { 742, 486, 100, 14 }, 10.5f, txt, true);
        text (g, "DRY", { 720, 560, 40, 14 }, 10, dim); text (g, "WET", { 824, 560, 40, 14 }, 10, dim);
        text (g, T("Compare ton mix sur d'autres enceintes et pièces."), { 590, 506, 160, 40 }, 10, dim, false, juce::Justification::topLeft);

        text (g, "SAFE MODE", { 430, 280, 140, 16 }, 11, dim, true);
        text (g, T("coupe à l'export, limite -1 dB"), { 405, 326, 190, 14 }, 9.5f, dim);
        text (g, T("ÉCOUTE"), { 430, 346, 140, 14 }, 10, dim, true);
        text (g, "VOLUME", { 450, 504, 100, 14 }, 10.5f, txt, true);
        text (g, proc.apvts.getParameter ("out")->getCurrentValueAsText(), { 450, 592, 100, 14 }, 10.5f, dim);

        auto box = juce::Rectangle<float> (20, 620, W - 40, 118);
        g.setColour (juce::Colour (0x40000000)); g.fillRoundedRectangle (box, 8);
        g.setColour (line); g.drawRoundedRectangle (box, 8, 1);
        text (g, T("MESURES DU MIX (avant correction = ce qui sera exporté)"), { 36, 628, 500, 14 }, 10, dim, true, juce::Justification::centredLeft);
        text (g, lufsV > -69 ? juce::String (lufsV, 1) : juce::String ("-"), { 36, 650, 150, 40 }, 30, txt, true, juce::Justification::centredLeft);
        text (g, "LUFS court terme (3 s)", { 36, 692, 200, 14 }, 10, dim, false, juce::Justification::centredLeft);
        text (g, T("streaming ≈ -14  •  club ≈ -8"), { 36, 708, 220, 14 }, 9.5f, dim, false, juce::Justification::centredLeft);
        const float pkDb = juce::Decibels::gainToDecibels (peakV, -60.0f);
        text (g, (pkDb > -59 ? juce::String (pkDb, 1) : juce::String ("-")) + " dB", { 270, 650, 160, 40 }, 30, pkDb > -0.3f ? red : txt, true, juce::Justification::centredLeft);
        text (g, T("crête (rouge = sature)"), { 270, 692, 200, 14 }, 10, dim, false, juce::Justification::centredLeft);
        auto cb = juce::Rectangle<float> (520, 662, 420, 12);
        g.setColour (juce::Colour (0xff2a2d33)); g.fillRoundedRectangle (cb, 6);
        const float cxp = cb.getCentreX() + cb.getWidth() * 0.5f * juce::jlimit (-1.0f, 1.0f, corrV);
        g.setColour (corrV < 0 ? red : (corrV < 0.3f ? orange : green));
        g.fillRoundedRectangle (juce::Rectangle<float> (juce::jmin (cb.getCentreX(), cxp), cb.getY(), std::abs (cxp - cb.getCentreX()) + 2, cb.getHeight()), 6);
        g.setColour (txt); g.drawVerticalLine ((int) cb.getCentreX(), cb.getY() - 3, cb.getBottom() + 3);
        text (g, "-1", { cb.getX() - 4, cb.getBottom() + 4, 30, 14 }, 10, dim, false, juce::Justification::centredLeft);
        text (g, "0", { cb.getCentreX() - 15, cb.getBottom() + 4, 30, 14 }, 10, dim);
        text (g, "+1", { cb.getRight() - 26, cb.getBottom() + 4, 30, 14 }, 10, dim, false, juce::Justification::centredRight);
        text (g, T("CORRÉLATION ") + juce::String (corrV, 2) + (corrV < 0 ? T("  problème de phase : vérifie en MONO") : juce::String()), { 520, 640, 420, 16 }, 10.5f, corrV < 0 ? red : txt, true, juce::Justification::centredLeft);
        text (g, T("Mets BAKR CASQUE en DERNIER sur le Master, seulement pour écouter au casque."), { 520, 704, 420, 16 }, 10, dim, false, juce::Justification::centredLeft);
        if (proc.isExporting.load()) text (g, T("EXPORT : correction coupée"), { 520, 718, 420, 16 }, 10.5f, orange, true, juce::Justification::centredLeft);
    }

private:
    static float xOf (juce::Rectangle<float> r, double f) { return r.getX() + r.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); }
    static float yOf (juce::Rectangle<float> r, double db) { return r.getCentreY() - (float) db / 12.0f * r.getHeight() * 0.5f; }
    void drawCurve (juce::Graphics& g, juce::Rectangle<float> r, const Curve& c, juce::Colour col, float w, bool dotted)
    {
        if ((int) c.size() != kGrid) return;
        juce::Path pth; bool st = false;
        for (int i = 0; i < kGrid; ++i)
        {
            const double f = gridFreq (i);
            if (f < 20 || f > 20000) continue;
            const float x = xOf (r, f), y = juce::jlimit (r.getY(), r.getBottom(), yOf (r, c[(size_t) i]));
            if (! st) { pth.startNewSubPath (x, y); st = true; } else pth.lineTo (x, y);
        }
        g.setColour (col);
        if (dotted) { juce::Path d; const float dl[] = { 2.0f, 3.0f }; juce::PathStrokeType (w).createDashedStroke (d, pth, dl, 2); g.fillPath (d); }
        else g.strokePath (pth, juce::PathStrokeType (w, juce::PathStrokeType::curved));
    }
    void stepAngle (int d)
    {
        if (auto* prm = proc.apvts.getParameter ("angle"))
        {
            const float val = juce::jlimit (30.0f, 120.0f, proc.p ("angle") + (float) d);
            prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 (val)); prm->endChangeGesture();
        }
    }
    void timerCallback() override
    {
        const int vv = proc.viewVersion();
        if (vv != lastV) { lastV = vv; v = proc.getView(); }
        xfeed = juce::jmax (proc.crossLevel.exchange (0.0f), xfeed * 0.85f);
        const int mon = (int) proc.p ("monitor");
        for (int i = 0; i < monBtns.size(); ++i) monBtns[i]->setToggleState (i == mon, juce::dontSendNotification);
        lufsV = proc.lufs.load();
        peakV = juce::jmax (proc.mixPeak.exchange (0.0f), peakV * 0.97f);
        corrV = corrV * 0.8f + proc.correlation.load() * 0.2f;
        repaint();
    }

    Casque& proc;
    bakr::LookAndFeel lnf;
    PowerButton calOn, emuOn;
    SwitchButton safe;
    juce::TextButton bypass { "BYPASS" }, dimB { "DIM -20 dB" };
    juce::TextButton angDown { juce::String::fromUTF8 ("\xe2\x97\x80") }, angUp { juce::String::fromUTF8 ("\xe2\x96\xb6") };
    juce::OwnedArray<juce::TextButton> monBtns;
    juce::ComboBox model, target, speaker, roomType;
    juce::Slider room, calAmt, vol, bass, treble;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> catts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> batts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> satts;
    Casque::View v;
    int lastV = -1;
    float xfeed = 0, lufsV = -70, peakV = 0, corrV = 1;
};

class CasqueEditor : public juce::AudioProcessorEditor
{
public:
    explicit CasqueEditor (Casque& p) : AudioProcessorEditor (&p), content (p)
    {
        addAndMakeVisible (content);
        content.setBounds (0, 0, CasqueContent::W, CasqueContent::H);
        setResizable (true, true);
        setResizeLimits (CasqueContent::W / 2, CasqueContent::H / 2, CasqueContent::W * 2, CasqueContent::H * 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) CasqueContent::W / CasqueContent::H);
        setSize (CasqueContent::W, CasqueContent::H);
    }
    void resized() override { content.setTransform (juce::AffineTransform::scale ((float) getWidth() / CasqueContent::W)); }
private:
    CasqueContent content;
};

juce::AudioProcessorEditor* Casque::createEditor() { return new CasqueEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Casque(); }
