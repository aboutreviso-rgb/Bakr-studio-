// BAKR TAK35 PRO — rend le micro Takstar TAK35 plus professionnel :
// gate de bruit, correction de la couleur du TAK35, 9 modeles de micros de studio,
// proximite, preampli (lampe / transfo), de-esser, compresseur vocal, air.
// LATENCE 0 : traitement direct, sans suréchantillonnage ni anticipation (lookahead).
#include "../Common/BakrUI.h"
using namespace bakr;

namespace
{
    enum BandType { Peak, LowShelf, HighShelf };
    struct Band { BandType t; float f, g, q; };
    enum PreType { Clean, Tube, Xfmr };
    struct Model
    {
        const char* name;
        std::vector<Band> bands;
        PreType pre;
        float baseDrive;   // couleur propre au modele (0..1)
        float lowCut;      // coupe-bas naturel du modele (Hz, 0 = aucun)
    };

    // Profils inspires des micros celebres (courbes moyennes, pas des copies exactes)
    const std::vector<Model>& models()
    {
        static const std::vector<Model> m {
            { "TAK35 naturel",   { },                                                                                  Clean, 0.00f, 0 },
            { "Studio 87",       { { Peak, 180, 0.8f, 0.8f }, { Peak, 3200, -0.6f, 1.2f }, { Peak, 9500, 2.2f, 0.7f }, { HighShelf, 16000, -1.0f, 0.7f } }, Xfmr, 0.25f, 40 },
            { "Lampe 47",        { { Peak, 220, 2.0f, 0.7f }, { Peak, 2800, 1.5f, 0.9f }, { Peak, 7000, -0.8f, 1.0f }, { HighShelf, 13000, -2.0f, 0.7f } }, Tube, 0.45f, 0 },
            { "Lampe 12",        { { Peak, 150, 0.6f, 0.8f }, { Peak, 5000, 1.2f, 0.9f }, { HighShelf, 11000, 3.5f, 0.7f } },  Tube, 0.35f, 30 },
            { "Brillant 800",    { { LowShelf, 120, -1.5f, 0.7f }, { Peak, 4800, 2.2f, 0.8f }, { HighShelf, 8000, 4.0f, 0.7f } }, Tube, 0.30f, 70 },
            { "Studio 103",      { { Peak, 250, -0.6f, 0.9f }, { Peak, 8500, 3.8f, 1.0f }, { HighShelf, 15000, -0.5f, 0.7f } }, Xfmr, 0.15f, 50 },
            { "Broadcast 7",     { { Peak, 200, 2.0f, 0.7f }, { Peak, 4200, 2.0f, 1.0f }, { HighShelf, 10000, -4.5f, 0.7f } }, Xfmr, 0.35f, 60 },
            { "Ruban 121",       { { Peak, 140, 2.2f, 0.7f }, { Peak, 1200, 0.8f, 0.8f }, { HighShelf, 6000, -6.0f, 0.6f } },  Xfmr, 0.40f, 0 },
            { "Afro Lead",       { { LowShelf, 140, -2.0f, 0.7f }, { Peak, 380, -1.5f, 1.0f }, { Peak, 3500, 2.5f, 0.9f }, { HighShelf, 10000, 3.5f, 0.7f } }, Tube, 0.30f, 90 },
        };
        return m;
    }

    // Inverse de la couleur estimee du TAK35 (aigus +2..+3.5 dB entre 4 et 12 kHz, grave qui tombe sous 50 Hz)
    const std::vector<Band> tak35Fix {
        { Peak, 4200, -1.2f, 1.1f }, { Peak, 8500, -3.0f, 0.75f }, { HighShelf, 17000, 2.0f, 0.7f }, { LowShelf, 45, 2.0f, 0.7f } };
}

class TakPro : public BaseProcessor
{
public:
    static constexpr int kBands = 6;

    TakPro() : BaseProcessor ("BAKR TAK35 PRO", layout(), presets(), false) {}   // latence 0 (pas de suréchantillonnage)

    static Layout layout()
    {
        Layout l;
        juce::StringArray names;
        for (auto& m : models()) names.add (m.name);
        addChoice (l, "model", "Modele", names, 1);
        addFloat (l, "fix", "Correction TAK35", 0, 100, 100, "%", 0, 0);
        addFloat (l, "char", "Intensite", 0, 150, 100, "%", 0, 0);
        addFloat (l, "prox", "Proximite", -6, 6, 0, "dB");
        addFloat (l, "drive", "Preampli", 0, 100, 25, "%", 0, 0);
        addChoice (l, "hpf", "Coupe-bas", { "OFF", "60", "80", "100", "120", "150" }, 2);
        addFloat (l, "gate", "Gate", -80, -30, -80, "dB", -55, 0);
        addFloat (l, "dess", "De-esser", 0, 100, 30, "%", 0, 0);
        addFloat (l, "comp", "Compression", 0, 100, 35, "%", 0, 0);
        addFloat (l, "air", "Air", 0, 6, 1.5f, "dB");
        addFloat (l, "out", "Sortie", -18, 12, 0, "dB");
        addFloat (l, "mix", "Mix", 0, 100, 100, "%", 0, 0);
        addBool (l, "power", "Power", true);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            // ---- VOIX LEAD : devant, claire, presente
            { "LEAD - Pro polyvalente (87)",      { { "model", 1 }, { "drive", 25 }, { "prox", 0.5f }, { "dess", 35 }, { "comp", 45 }, { "air", 2.0f }, { "hpf", 2 } } },
            { "LEAD - Afrobeats",                 { { "model", 8 }, { "drive", 30 }, { "prox", 0.5f }, { "dess", 45 }, { "comp", 55 }, { "air", 2.5f }, { "hpf", 4 } } },
            { "LEAD - Coupe-decale",              { { "model", 4 }, { "char", 120 }, { "drive", 35 }, { "dess", 45 }, { "comp", 60 }, { "air", 2.5f }, { "hpf", 4 } } },
            { "LEAD - Mandingue",                 { { "model", 2 }, { "drive", 35 }, { "prox", 1.0f }, { "dess", 30 }, { "comp", 45 }, { "air", 1.5f }, { "hpf", 2 } } },
            { "LEAD - Reggae",                    { { "model", 3 }, { "drive", 40 }, { "prox", 1.5f }, { "dess", 30 }, { "comp", 45 }, { "air", 1.0f }, { "hpf", 2 } } },
            { "LEAD - R&B douce (lampe)",         { { "model", 3 }, { "drive", 30 }, { "prox", 1.0f }, { "dess", 40 }, { "comp", 50 }, { "air", 2.5f }, { "hpf", 3 } } },
            { "LEAD - Rap",                       { { "model", 6 }, { "drive", 35 }, { "prox", 1.5f }, { "dess", 35 }, { "comp", 60 }, { "air", 2.0f }, { "hpf", 3 } } },
            { "LEAD - Chaude (47)",               { { "model", 2 }, { "drive", 40 }, { "prox", 1.5f }, { "dess", 25 }, { "comp", 45 }, { "air", 0.5f }, { "hpf", 1 } } },
            { "LEAD - Brillante (800)",           { { "model", 4 }, { "drive", 30 }, { "dess", 50 }, { "comp", 50 }, { "air", 2.0f }, { "hpf", 3 } } },
            // ---- BACKING : soutient le lead sans le cacher (moins de grave, plus serre)
            { "BACKING - Standard",               { { "model", 1 }, { "char", 80 }, { "drive", 20 }, { "prox", -2.0f }, { "dess", 55 }, { "comp", 65 }, { "air", 1.0f }, { "hpf", 5 } } },
            { "BACKING - Afro (repetitions)",     { { "model", 8 }, { "char", 80 }, { "drive", 25 }, { "prox", -2.5f }, { "dess", 55 }, { "comp", 65 }, { "air", 1.5f }, { "hpf", 5 } } },
            { "BACKING - Doubles du lead",        { { "model", 4 }, { "char", 70 }, { "drive", 15 }, { "prox", -3.0f }, { "dess", 65 }, { "comp", 70 }, { "air", 0.5f }, { "hpf", 5 } } },
            { "BACKING - Adlibs / reponses",      { { "model", 6 }, { "char", 110 }, { "drive", 45 }, { "prox", -1.0f }, { "dess", 45 }, { "comp", 70 }, { "air", 1.0f }, { "hpf", 5 } } },
            { "BACKING - Sombre en fond",         { { "model", 7 }, { "char", 80 }, { "drive", 30 }, { "prox", -3.0f }, { "dess", 40 }, { "comp", 60 }, { "air", 0 }, { "hpf", 5 } } },
            // ---- CHOEURS : doux, aerien, se fondent ensemble
            { "CHOEURS - Doux et aeriens",        { { "model", 3 }, { "char", 90 }, { "drive", 15 }, { "prox", -3.0f }, { "dess", 55 }, { "comp", 55 }, { "air", 3.0f }, { "hpf", 5 } } },
            { "CHOEURS - Gospel / eglise",        { { "model", 2 }, { "char", 80 }, { "drive", 25 }, { "prox", -1.5f }, { "dess", 45 }, { "comp", 50 }, { "air", 2.0f }, { "hpf", 4 } } },
            { "CHOEURS - Afro (refrain en groupe)",{ { "model", 8 }, { "char", 75 }, { "drive", 20 }, { "prox", -3.0f }, { "dess", 60 }, { "comp", 60 }, { "air", 2.5f }, { "hpf", 5 } } },
            { "CHOEURS - Mandingue (reponse)",    { { "model", 2 }, { "char", 70 }, { "drive", 20 }, { "prox", -2.5f }, { "dess", 45 }, { "comp", 55 }, { "air", 1.5f }, { "hpf", 5 } } },
            { "CHOEURS - Harmonies serrees",      { { "model", 5 }, { "char", 70 }, { "drive", 10 }, { "prox", -4.0f }, { "dess", 65 }, { "comp", 65 }, { "air", 2.0f }, { "hpf", 5 } } },
            // ---- AUTRES
            { "Parole / podcast",                 { { "model", 6 }, { "gate", -50 }, { "drive", 20 }, { "dess", 35 }, { "comp", 60 }, { "air", 1.0f }, { "hpf", 4 } } },
            { "Chambre bruyante (gate)",          { { "model", 1 }, { "gate", -48 }, { "drive", 20 }, { "dess", 30 }, { "comp", 40 }, { "hpf", 4 } } },
        };
    }

    void prepareCore (double fsOS, int, int) override
    {
        fsCache = fsOS;
        for (auto& s : st)
        {
            for (auto& b : s.eq) b.reset();
            for (auto& b : s.fix) b.reset();
            s.hp1.reset(); s.hp2.reset(); s.prox.reset(); s.air.reset(); s.dsBand.reset(); s.dsBand2.reset();
            s.dc.reset(); s.dc.setLP (fsOS, 8.0);
            s.xf.prepare (fsOS, 0.5f, 21000.0f, 0.5f); s.xf.reset();
            s.gateEnv.reset(); s.compEnv.reset(); s.dsEnv.reset(); s.rmsC.reset(); s.rmsG.reset(); s.rmsD.reset();
            s.hold = 0;
        }
        lastKey.fill (-1.0f);
    }

    static void setBand (Biquad& bq, double fs, const Band& b, float scale)
    {
        const double g = b.g * scale;
        if (std::abs (g) < 0.01) { bq.identity(); return; }
        const double f = juce::jmin ((double) b.f, fs * 0.45);
        switch (b.t)
        {
            case Peak:      bq.peak (fs, f, b.q, g); break;
            case LowShelf:  bq.lowShelf (fs, f, g, b.q); break;
            case HighShelf: bq.highShelf (fs, f, g, b.q); break;
        }
    }

    void updateFilters (double fs)
    {
        const int mi = juce::jlimit (0, (int) models().size() - 1, (int) p ("model"));
        const float fix = p ("fix") * 0.01f, ch = p ("char") * 0.01f, prox = p ("prox"), air = p ("air");
        const int hpi = (int) p ("hpf");
        const std::array<float, 7> key { (float) mi, fix, ch, prox, air, (float) hpi, (float) fs };
        if (key == lastKey) return;
        lastKey = key;

        const auto& m = models()[(size_t) mi];
        const float hps[] = { 0, 60, 80, 100, 120, 150 };
        const float hp = juce::jmax (hps[hpi], hpi == 0 ? 0.0f : m.lowCut);
        for (auto& s : st)
        {
            for (int b = 0; b < kBands; ++b)
            {
                if (b < (int) m.bands.size()) setBand (s.eq[(size_t) b], fs, m.bands[(size_t) b], ch);
                else s.eq[(size_t) b].identity();
            }
            for (size_t b = 0; b < s.fix.size(); ++b) setBand (s.fix[b], fs, tak35Fix[b], fix);
            if (hp > 0) { s.hp1.highPass (fs, hp, 0.54); s.hp2.highPass (fs, hp, 1.31); }
            else { s.hp1.identity(); s.hp2.identity(); }
            if (std::abs (prox) > 0.01f) s.prox.lowShelf (fs, 160.0, prox, 0.6); else s.prox.identity();
            if (air > 0.01f) s.air.highShelf (fs, juce::jmin (12000.0, fs * 0.4), air, 0.6); else s.air.identity();
            s.dsBand.highPass (fs, 5500.0, 0.6);
            s.dsBand2.lowPass (fs, juce::jmin (11000.0, fs * 0.45), 0.6);
            s.xf.prepare (fs, 0.3f + 0.7f * m.baseDrive, 21000.0f, 0.6f);
        }
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        updateFilters (fs);
        const int nCh = juce::jmin (2, (int) b.getNumChannels());
        const int n = (int) b.getNumSamples();
        const auto& m = models()[(size_t) juce::jlimit (0, (int) models().size() - 1, (int) p ("model"))];

        const float gateThr = p ("gate");
        const bool gateOn = gateThr > -79.5f;
        const float drv = juce::jlimit (0.0f, 0.85f, m.baseDrive * 0.5f + p ("drive") * 0.01f * 0.7f);
        const float dess = p ("dess") * 0.01f, comp = p ("comp") * 0.01f;
        const float dsThr = -14.0f - 26.0f * dess;
        const float cThr = -6.0f - 24.0f * comp, cRatio = 1.5f + 2.5f * comp;
        const float makeup = gain (comp * 9.0f);
        const float outG = gain (p ("out"));

        const float cRms = coefMs (8.0f, fs), cA = coefMs (4.0f, fs), cR = coefMs (140.0f, fs);
        const float gRms = coefMs (3.0f, fs), gA = coefMs (1.0f, fs), gR = coefMs (120.0f, fs);
        const float dRms = coefMs (1.5f, fs), dA = coefMs (0.5f, fs), dR = coefMs (60.0f, fs);
        const int holdSamples = (int) (0.05 * fs);

        float mg[2] = { 0, 0 }, md = 0, mgate = 0;
        for (int c = 0; c < nCh; ++c)
        {
            auto& s = st[(size_t) c];
            float* x = b.getChannelPointer ((size_t) c);
            for (int i = 0; i < n; ++i)
            {
                float y = x[i];
                // 1. coupe-bas (12 dB/oct, 2 etages -> 24 dB/oct)
                y = s.hp2.process (s.hp1.process (y));

                // 2. gate / expandeur doux (souffle du micro, bruit de la piece)
                if (gateOn)
                {
                    const float lv = dB (s.rmsG.process (y, gRms) * 1.41f + 1e-9f);
                    float target = 0.0f;
                    if (lv > gateThr) s.hold = holdSamples;
                    else if (s.hold > 0) --s.hold;
                    else target = juce::jmin (30.0f, (gateThr - lv) * 2.0f);
                    const float gr = s.gateEnv.process (target, gA, gR);
                    mgate = juce::jmax (mgate, gr);
                    y *= gain (-gr);
                }

                // 3. correction TAK35, 4. modele de micro, 5. proximite
                for (auto& f : s.fix) y = f.process (y);
                for (auto& f : s.eq) y = f.process (y);
                y = s.prox.process (y);

                // 6. preampli
                if (drv > 0.001f)
                {
                    if (m.pre == Tube) y = triode (y * (1.0f + drv), drv, 0.18f) / (1.0f + drv * 0.6f);
                    else if (m.pre == Xfmr) { y = s.xf.process (y * (1.0f + drv * 0.5f)) / (1.0f + drv * 0.3f); }
                    else y = softClip (y, drv * 0.3f);
                }

                // 7. de-esser (bande 5.5 - 11 kHz)
                if (dess > 0.001f)
                {
                    const float band = s.dsBand2.process (s.dsBand.process (y));
                    const float lv = dB (s.rmsD.process (band, dRms) * 1.41f + 1e-9f);
                    const float gr = s.dsEnv.process (juce::jmin (14.0f, kneeGr (lv - dsThr, 4.0f, 4.0f)), dA, dR);
                    md = juce::jmax (md, gr);
                    y -= band * (1.0f - gain (-gr));
                }

                // 8. compresseur vocal (style opto rapide)
                if (comp > 0.001f)
                {
                    const float lv = dB (juce::jmax (s.rmsC.process (y, cRms) * 1.41f, std::abs (y) * 0.7f) + 1e-9f);
                    const float gr = s.compEnv.process (juce::jmin (30.0f, kneeGr (lv - cThr, cRatio, 8.0f)), cA, cR);
                    mg[c] = juce::jmax (mg[c], gr);
                    y *= gain (-gr) * makeup;
                }

                // 9. air + sortie
                y = s.air.process (y);
                x[i] = s.dc.hp (y) * outG;
            }
        }
        reportGr (0, mg[0]);
        reportGr (1, nCh > 1 ? mg[1] : mg[0]);
        extra[0].store (md);
        extra[1].store (mgate);
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1240; u.h = 440;
        const auto txt = juce::Colour (0xffeef0f3), dim = juce::Colour (0xffa0a7b0), acc = juce::Colour (0xffffb347);
        u.theme = { juce::Colour (0xff2b2f35), juce::Colour (0xff121417), txt, dim, acc };
        u.theme.earColour = juce::Colour (0xff24272b);
        const auto rack = juce::Colour (0x10ffffff), rline = juce::Colour (0x30ffffff);
        const auto blue = juce::Colour (0xff6fd0ff), red = juce::Colour (0xffff4a3d), green = juce::Colour (0xff45d06a);
        u.boxes = {
            { 40, 12, 380, 416, rack, rline, 6.0f },     // micro
            { 432, 12, 520, 200, rack, rline, 6.0f },    // couleur
            { 432, 222, 520, 206, rack, rline, 6.0f },   // dynamique
            { 964, 12, 236, 416, rack, rline, 6.0f },    // sortie
        };
        u.texts = {
            { "BAKR", 56, 22, 90, 28, 24, txt, true, false, juce::Justification::centredLeft },
            { "TAK35 PRO", 140, 26, 160, 22, 17, acc, true, false, juce::Justification::centredLeft },
            { "MODELE DE MICRO", 56, 62, 348, 14, 11, dim },
            { "COULEUR", 444, 18, 200, 14, 11, dim, true, false, juce::Justification::centredLeft },
            { "DYNAMIQUE", 444, 228, 200, 14, 11, dim, true, false, juce::Justification::centredLeft },
            { "pour Takstar TAK35  -  LATENCE 0", 56, 404, 348, 14, 10.5f, dim },
            { "DE-ESS", 868, 236, 70, 12, 9, dim },
            { "GATE", 868, 298, 70, 12, 9, dim },
            { "COMP GR", 976, 30, 212, 12, 9, dim },
            { "IN / OUT", 976, 66, 212, 12, 9, dim },
        };
        // 9 modeles en grille 3 x 3
        const char* shortNames[] = { "TAK35", "STUDIO 87", "LAMPE 47", "LAMPE 12", "BRILLANT 800", "STUDIO 103", "BROADCAST 7", "RUBAN 121", "AFRO LEAD" };
        for (int i = 0; i < 9; ++i)
        {
            const float cx = 108.0f + (float) (i % 3) * 122.0f, cy = 110.0f + (float) (i / 3) * 62.0f;
            u.controls.push_back ({ Ctl::Radio, "model", shortNames[i], cx, cy, 110, "", {}, {}, {}, i == 8 ? acc : blue, 0, {}, i });
        }
        u.controls.push_back ({ Ctl::Knob, "fix", "CORRECTION TAK35", 130, 330, 70, "chrome", {}, "0", "100" });
        u.controls.push_back ({ Ctl::Knob, "char", "INTENSITE", 300, 330, 70, "chrome", {}, "0", "150" });

        u.controls.push_back ({ Ctl::Knob, "prox", "PROXIMITE", 510, 122, 66, "black", {}, "LOIN", "PRES" });
        u.controls.push_back ({ Ctl::Knob, "drive", "PREAMPLI", 640, 122, 66, "red", {}, "0", "MAX" });
        u.controls.push_back ({ Ctl::Selector, "hpf", "COUPE-BAS", 765, 122, 50, "black", { "OFF", "60", "80", "100", "120", "150" } });
        u.controls.push_back ({ Ctl::Knob, "air", "AIR", 885, 122, 60, "black", {}, "0", "+6" });

        u.controls.push_back ({ Ctl::Knob, "gate", "GATE", 510, 336, 66, "grey", {}, "OFF", "-30" });
        u.controls.push_back ({ Ctl::Knob, "dess", "DE-ESSER", 640, 336, 66, "black", {}, "0", "MAX" });
        u.controls.push_back ({ Ctl::Knob, "comp", "COMPRESSION", 765, 336, 66, "chrome", {}, "0", "MAX" });

        u.controls.push_back ({ Ctl::Knob, "out", "SORTIE", 1082, 210, 86, "chrome", {}, "-18", "+12" });
        u.controls.push_back ({ Ctl::Knob, "mix", "MIX", 1030, 350, 48, "black", {}, "DRY", "WET" });
        u.controls.push_back ({ Ctl::Lever, "power", "POWER", 1140, 350, 30 });

        u.meters = {
            { 976, 46, 212, 14, Src::GrMax, "", "bar" },
            { 976, 80, 212, 16, Src::InMax, "", "led" },
            { 976, 104, 212, 16, Src::OutMax, "", "led" },
            { 846, 252, 92, 12, Src::Extra0, "", "bar" },
            { 846, 314, 92, 12, Src::Extra1, "", "bar" },
        };
        u.presetArea = { 980, 140, 204, 24 };
        juce::ignoreUnused (red, green);
        return new Editor (*this, u);
    }

private:
    struct Ch
    {
        std::array<Biquad, kBands> eq;
        std::array<Biquad, 4> fix;
        Biquad hp1, hp2, prox, air, dsBand, dsBand2;
        OnePole dc;
        Transformer xf;
        Env gateEnv, compEnv, dsEnv;
        Rms rmsC, rmsG, rmsD;
        int hold = 0;
    };
    std::array<Ch, 2> st;
    double fsCache = 96000.0;
    std::array<float, 7> lastKey {};
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TakPro(); }
