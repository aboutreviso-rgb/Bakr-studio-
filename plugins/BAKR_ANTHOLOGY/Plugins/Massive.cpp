// BAKR MASSIVE — égaliseur passif stéréo à lampes, 4 bandes par canal (inspiré d'un célèbre EQ passif de mastering)
// Par bande : GAIN 0-20 dB, BOOST / OUT / CUT, SHELF / BELL, BANDWIDTH, FREQUENCY (11 crans).
// Par canal : IN, GAIN -6..+4 (étage à lampe), LOW PASS, HIGH PASS.  LINK : le canal 2 suit le canal 1.
#include "../Common/BakrUI.h"
using namespace bakr;

namespace
{
    // crans de fréquence de chaque bande (comme l'appareil d'origine)
    const float kFreq[4][11] = {
        { 22, 33, 47, 68, 100, 150, 220, 330, 470, 680, 1000 },
        { 82, 120, 180, 270, 390, 560, 820, 1200, 1800, 2700, 3900 },
        { 220, 330, 470, 680, 1000, 1500, 2200, 3300, 4700, 6800, 10000 },
        { 560, 820, 1200, 1800, 2700, 3900, 5600, 8200, 12000, 16000, 27000 } };
    const char* kFreqTxt[4][11] = {
        { "22", "33", "47", "68", "100", "150", "220", "330", "470", "680", "1K" },
        { "82", "120", "180", "270", "390", "560", "820", "1K2", "1K8", "2K7", "3K9" },
        { "220", "330", "470", "680", "1K", "1K5", "2K2", "3K3", "4K7", "6K8", "10K" },
        { "560", "820", "1K2", "1K8", "2K7", "3K9", "5K6", "8K2", "12K", "16K", "27K" } };
    const char* kRange[4] = { "22 - 1K", "82 - 3K9", "220 - 10K", "560 - 27K" };
    const float kLp[6] = { 0, 18000, 12000, 9000, 7500, 6000 };
    const float kHp[6] = { 0, 22, 39, 68, 120, 220 };

    juce::String id (const char* base, int c, int b = -1) { return juce::String (base) + juce::String (c) + (b >= 0 ? juce::String (b) : juce::String()); }

    // identifiants permanents (les presets et l'interface gardent des const char*)
    const char* keep (const juce::String& s)
    {
        static std::vector<std::unique_ptr<juce::String>> pool;
        static juce::CriticalSection lock;
        const juce::ScopedLock sl (lock);
        for (auto& e : pool) if (*e == s) return e->toRawUTF8();
        pool.push_back (std::make_unique<juce::String> (s));
        return pool.back()->toRawUTF8();
    }
    const char* pid (const char* base, int c, int b = -1) { return keep (id (base, c, b)); }
}

class Massive : public BaseProcessor
{
public:
    Massive() : BaseProcessor ("BAKR MASSIVE", layout(), presets())
    {
        for (int c = 0; c < 2; ++c)
        {
            auto& q = prm[(size_t) c];
            for (int b = 0; b < 4; ++b)
            {
                q.g[(size_t) b]  = apvts.getRawParameterValue (pid ("g", c + 1, b));
                q.m[(size_t) b]  = apvts.getRawParameterValue (pid ("m", c + 1, b));
                q.sh[(size_t) b] = apvts.getRawParameterValue (pid ("sh", c + 1, b));
                q.bw[(size_t) b] = apvts.getRawParameterValue (pid ("bw", c + 1, b));
                q.f[(size_t) b]  = apvts.getRawParameterValue (pid ("f", c + 1, b));
            }
            q.in = apvts.getRawParameterValue (pid ("in", c + 1));
            q.gain = apvts.getRawParameterValue (pid ("gain", c + 1));
            q.lp = apvts.getRawParameterValue (pid ("lp", c + 1));
            q.hp = apvts.getRawParameterValue (pid ("hp", c + 1));
        }
    }

    static Layout layout()
    {
        Layout l;
        for (int c = 1; c <= 2; ++c)
        {
            for (int b = 0; b < 4; ++b)
            {
                juce::StringArray fr; for (auto* t : kFreqTxt[b]) fr.add (t);
                addFloat (l, pid ("g", c, b), keep ("Ch" + juce::String (c) + " B" + juce::String (b + 1) + " Gain"), 0, 20, 0, "dB");
                addChoice (l, pid ("m", c, b), keep ("Ch" + juce::String (c) + " B" + juce::String (b + 1) + " Mode"), { "CUT", "OUT", "BOOST" }, 1);
                addBool (l, pid ("sh", c, b), keep ("Ch" + juce::String (c) + " B" + juce::String (b + 1) + " Shelf"), b == 0 || b == 3);
                addFloat (l, pid ("bw", c, b), keep ("Ch" + juce::String (c) + " B" + juce::String (b + 1) + " Bandwidth"), 0, 10, 5, "");
                addChoice (l, pid ("f", c, b), keep ("Ch" + juce::String (c) + " B" + juce::String (b + 1) + " Freq"), fr, 5);
            }
            addBool (l, pid ("in", c), keep ("Ch" + juce::String (c) + " In"), true);
            addFloat (l, pid ("gain", c), keep ("Ch" + juce::String (c) + " Gain"), -6, 4, 0, "dB");
            addChoice (l, pid ("lp", c), keep ("Ch" + juce::String (c) + " Low Pass"), { "OFF", "18K", "12K", "9K", "7K5", "6K" }, 0);
            addChoice (l, pid ("hp", c), keep ("Ch" + juce::String (c) + " High Pass"), { "OFF", "22", "39", "68", "120", "220" }, 0);
        }
        addBool (l, "link", "Link", true);
        addBool (l, "power", "Power", true);
        addChoice (l, "ms", "Mode", { "STEREO", "MID/SIDE" }, 0);
        addFloat (l, "color", "Couleur lampe", 0, 100, 30, "%", 0, 0);
        addBool (l, "autoGain", "Auto gain", false);
        addBool (l, "analog", "Tolerances analogiques", true);
        addBool (l, "steps", "Crans 0.5 dB", false);
        return l;
    }

    // presets : réglages du canal 1 (le canal 2 suit grâce à LINK)
    static PresetDef P (const juce::String& name, std::vector<std::tuple<int, float, int, bool, float, int>> bands, float gain = 0, int lp = 0, int hp = 0)
    {
        PresetDef d { name, {} };
        for (int b = 0; b < 4; ++b) d.values.push_back ({ pid ("m", 1, b), 1.0f });
        for (auto& [b, g, mode, shelf, bw, f] : bands)
        {
            d.values.push_back ({ pid ("g", 1, b), g });
            d.values.push_back ({ pid ("m", 1, b), (float) mode });
            d.values.push_back ({ pid ("sh", 1, b), shelf ? 1.0f : 0.0f });
            d.values.push_back ({ pid ("bw", 1, b), bw });
            d.values.push_back ({ pid ("f", 1, b), (float) f });
        }
        d.values.push_back ({ "gain1", gain }); d.values.push_back ({ "lp1", (float) lp }); d.values.push_back ({ "hp1", (float) hp });
        d.values.push_back ({ "link", 1.0f });
        return d;
    }

    static std::vector<PresetDef> presets()
    {
        // (bande, gain, mode 0=CUT 1=OUT 2=BOOST, shelf, bandwidth, cran de fréquence)
        return {
            { "Init", {} },
            P ("Master - Air & poids (classique)",  { { 0, 2.5f, 2, true, 6, 3 }, { 1, 1.5f, 0, false, 4, 4 }, { 3, 3.0f, 2, true, 5, 9 } }, 0, 0, 1),
            P ("Master - Afrobeats brillant",       { { 0, 3.0f, 2, true, 6, 3 }, { 1, 2.0f, 0, false, 4, 4 }, { 2, 1.5f, 2, false, 3, 8 }, { 3, 3.5f, 2, true, 5, 9 } }, 0, 0, 2),
            P ("Master - Coupe-decale punch",       { { 0, 3.5f, 2, false, 4, 4 }, { 1, 2.5f, 0, false, 5, 5 }, { 2, 2.0f, 2, false, 4, 7 }, { 3, 3.0f, 2, true, 5, 10 } }, 0, 0, 2),
            P ("Master - Mandingue chaud",          { { 0, 2.0f, 2, true, 6, 4 }, { 1, 1.5f, 2, false, 3, 3 }, { 2, 1.5f, 0, false, 5, 6 }, { 3, 2.0f, 2, true, 4, 9 } }, 0, 1, 1),
            P ("Master - Reggae rond",              { { 0, 3.0f, 2, true, 7, 3 }, { 1, 2.0f, 0, false, 4, 5 }, { 3, 1.5f, 2, true, 4, 8 } }, 0, 2, 1),
            P ("Mastering doux (2 dB max)",         { { 0, 1.0f, 2, true, 6, 3 }, { 1, 1.0f, 0, false, 3, 4 }, { 3, 2.0f, 2, true, 5, 10 } }),
            P ("Voix - Air & presence",             { { 1, 2.5f, 0, false, 5, 3 }, { 2, 2.0f, 2, false, 3, 8 }, { 3, 4.0f, 2, true, 5, 9 } }, 0, 0, 4),
            P ("Voix - Chaleur",                    { { 1, 2.0f, 2, false, 3, 2 }, { 2, 2.0f, 0, false, 5, 7 }, { 3, 2.0f, 2, true, 4, 8 } }, 0, 0, 3),
            P ("Basse - Poids sub",                 { { 0, 4.0f, 2, false, 5, 2 }, { 1, 3.0f, 0, false, 5, 4 }, { 2, 2.0f, 2, false, 4, 4 } }, 0, 4, 1),
            P ("Kick - Claque",                     { { 0, 3.0f, 2, false, 6, 3 }, { 1, 4.0f, 0, false, 6, 5 }, { 2, 3.0f, 2, false, 6, 7 } }, 0, 0, 1),
            P ("Bus instru - Ouvrir",               { { 1, 2.0f, 0, false, 4, 4 }, { 3, 3.0f, 2, true, 5, 8 } }, 0, 0, 2),
            P ("Pultec style (creux sous la bosse)",{ { 0, 4.0f, 2, true, 9, 4 }, { 1, 2.0f, 0, false, 4, 4 }, { 3, 2.5f, 2, true, 8, 9 } }),
        };
    }

    void prepareCore (double, int, int) override
    {
        for (auto& c : ch) { for (auto& b : c.band) b.reset(); c.hp1.reset(); c.hp2.reset(); c.lp.reset(); c.xf.reset(); }
        lastKey.fill (-1.0f);
    }

    // bande -> filtre : passif = le Q s'élargit quand le gain est faible
public:
    static void setBand (Biquad& bq, double fs, int b, float g, int mode, bool shelf, float bw, int fi, bool steps = false, double fMul = 1.0, double gMul = 1.0)
    {
        if (steps) g = std::round (g * 2.0f) * 0.5f;   // version mastering : crans de 0.5 dB
        if (mode == 1 || g < 0.05f) { bq.identity(); return; }
        const double gain = (mode == 2 ? g : -g) * gMul;
        const double f = juce::jmin ((double) kFreq[b][juce::jlimit (0, 10, fi)] * fMul, fs * 0.45);
        const double t = bw / 10.0;
        if (shelf)
        {
            const double q = 0.45 + 1.4 * t;          // BANDWIDTH haut = bosse / creux autour du plateau
            if (b <= 1) bq.lowShelf (fs, f, gain, q); else bq.highShelf (fs, f, gain, q);
        }
        else
        {
            double q = 0.35 * std::pow (10.0, t * 1.05);   // 0.35 (large) -> ~3.9 (étroit)
            q *= 0.65 + 0.35 * juce::jlimit (0.0, 1.0, g / 12.0);
            bq.peak (fs, f, q, gain);
        }
    }

    // réponse d'un biquad (dB) — pour l'auto gain et la courbe affichée
    static double magDb (const Biquad& q, double f, double fs)
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * f / fs;
        const std::complex<double> z1 = std::polar (1.0, -w), z2 = z1 * z1;
        const auto num = (double) q.b0 + (double) q.b1 * z1 + (double) q.b2 * z2, den = 1.0 + (double) q.a1 * z1 + (double) q.a2 * z2;
        return 20.0 * std::log10 (std::abs (num) / std::abs (den) + 1e-12);
    }
    static constexpr int kCurve = 160;
    static double curveFreq (int i) { return 20.0 * std::pow (1000.0, i / (double) (kCurve - 1)); }

    // courbes des 2 canaux (pour l'affichage), calculées sur le fil des messages
    void computeCurves (std::array<std::array<float, kCurve>, 2>& out)
    {
        const double fs = 96000.0;
        const bool ms = p ("ms") > 0.5f, link = p ("link") > 0.5f && ! ms;
        for (int c = 0; c < 2; ++c)
        {
            const auto& q = prm[(size_t) (link ? 0 : c)];
            std::array<Biquad, 7> f;
            const bool tol = p ("analog") > 0.5f && c == 1 && ! ms;
            for (int b = 0; b < 4; ++b)
                setBand (f[(size_t) b], fs, b, q.g[(size_t) b]->load(), (int) q.m[(size_t) b]->load(), q.sh[(size_t) b]->load() > 0.5f,
                         q.bw[(size_t) b]->load(), (int) q.f[(size_t) b]->load(), p ("steps") > 0.5f, tol ? 1.012 : 1.0, tol ? 0.97 : 1.0);
            const float hp = kHp[juce::jlimit (0, 5, (int) q.hp->load())], lp = kLp[juce::jlimit (0, 5, (int) q.lp->load())];
            if (hp > 0) { f[4].highPass (fs, hp, 0.54); f[5].highPass (fs, hp, 1.31); }
            if (lp > 0) f[6].lowPass (fs, lp, 0.6);
            const bool in = q.in->load() > 0.5f;
            for (int i = 0; i < kCurve; ++i)
            {
                double db = 0;
                if (in) for (auto& b : f) db += magDb (b, curveFreq (i), fs);
                out[(size_t) c][(size_t) i] = (float) db;
            }
        }
    }

    void updateFilters (double fs)
    {
        const bool ms = p ("ms") > 0.5f;
        const bool link = p ("link") > 0.5f && ! ms;
        std::array<float, 96> key {};
        int k = 0;
        for (int c = 0; c < 2; ++c)
        {
            const auto& q = prm[(size_t) (link ? 0 : c)];
            for (int b = 0; b < 4; ++b)
            {
                key[(size_t) k++] = q.g[(size_t) b]->load(); key[(size_t) k++] = q.m[(size_t) b]->load();
                key[(size_t) k++] = q.sh[(size_t) b]->load(); key[(size_t) k++] = q.bw[(size_t) b]->load();
                key[(size_t) k++] = q.f[(size_t) b]->load();
            }
            key[(size_t) k++] = q.lp->load(); key[(size_t) k++] = q.hp->load();
        }
        key[(size_t) k++] = (float) fs;
        key[(size_t) k++] = (ms ? 1.0f : 0.0f) + (p ("analog") > 0.5f ? 2.0f : 0.0f) + (p ("steps") > 0.5f ? 4.0f : 0.0f) + (link ? 8.0f : 0.0f);
        if (key == lastKey) return;
        lastKey = key;
        for (int c = 0; c < 2; ++c)
        {
            const auto& q = prm[(size_t) (link ? 0 : c)];
            auto& s = ch[(size_t) c];
            // tolérances analogiques : le canal droit n'est jamais tout à fait identique au gauche (comme un vrai appareil)
            const bool tol = p ("analog") > 0.5f && c == 1 && ! ms;
            for (int b = 0; b < 4; ++b)
                setBand (s.band[(size_t) b], fs, b, q.g[(size_t) b]->load(), (int) q.m[(size_t) b]->load(),
                         q.sh[(size_t) b]->load() > 0.5f, q.bw[(size_t) b]->load(), (int) q.f[(size_t) b]->load(),
                         p ("steps") > 0.5f, tol ? 1.012 : 1.0, tol ? 0.97 : 1.0);
            const float hp = kHp[juce::jlimit (0, 5, (int) q.hp->load())], lp = kLp[juce::jlimit (0, 5, (int) q.lp->load())];
            if (hp > 0) { s.hp1.highPass (fs, hp, 0.54); s.hp2.highPass (fs, hp, 1.31); } else { s.hp1.identity(); s.hp2.identity(); }
            if (lp > 0) s.lp.lowPass (fs, juce::jmin ((double) lp, fs * 0.45), 0.6); else s.lp.identity();   // 12 dB/oct doux
            s.xf.prepare (fs, 0.2f + 0.5f * p ("color") * 0.01f, 24000.0f, 0.4f);

            // auto gain : compense le volume moyen ajouté / enlevé par l'EQ (100 Hz - 8 kHz) -> comparaison honnête
            double avg = 0; int na = 0;
            for (int i = 0; i < 40; ++i)
            {
                const double fq = 100.0 * std::pow (80.0, i / 39.0);
                double db = 0;
                for (auto& b : s.band) db += magDb (b, fq, fs);
                avg += db; ++na;
            }
            s.autoComp = (float) std::pow (10.0, -(avg / na) / 20.0);
        }
    }

    void processCore (juce::dsp::AudioBlock<float>& blk, double fs) override
    {
        updateFilters (fs);
        const int nCh = juce::jmin (2, (int) blk.getNumChannels()), n = (int) blk.getNumSamples();
        const bool ms = p ("ms") > 0.5f && nCh == 2;
        const bool link = p ("link") > 0.5f && ! ms;
        const bool ag = p ("autoGain") > 0.5f;
        const float drive = 0.04f + 0.5f * p ("color") * 0.01f;
        float* X[2] = { blk.getChannelPointer (0), nCh > 1 ? blk.getChannelPointer (1) : nullptr };

        // MID / SIDE : canal 1 = centre (voix, kick, basse), canal 2 = côtés (largeur, reverb)
        if (ms)
            for (int i = 0; i < n; ++i) { const float l = X[0][i], r = X[1][i]; X[0][i] = 0.5f * (l + r); X[1][i] = 0.5f * (l - r); }

        for (int c = 0; c < nCh; ++c)
        {
            const auto& q = prm[(size_t) (link ? 0 : c)];
            const bool in = q.in->load() > 0.5f;
            auto& s = ch[(size_t) c];
            const float g = gain (q.gain->load()) * (ag && in ? s.autoComp : 1.0f);
            float* x = X[c];
            for (int i = 0; i < n; ++i)
            {
                float y = x[i];
                if (in)
                {
                    y = s.hp2.process (s.hp1.process (y));
                    for (auto& b : s.band) y = b.process (y);
                    y = s.lp.process (y);
                }
                y *= g;
                if (! ms) { y = triode (y, drive, 0.12f); y = s.xf.process (y); }
                x[i] = y;
            }
        }
        if (ms)
            for (int i = 0; i < n; ++i)
            {
                const float m = X[0][i], sd = X[1][i];
                // la lampe et le transfo travaillent sur G/D (comme l'appareil en sortie)
                X[0][i] = ch[0].xf.process (triode (m + sd, drive, 0.12f));
                X[1][i] = ch[1].xf.process (triode (m - sd, drive, 0.12f));
            }
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1340; u.h = 610;
        const auto txt = juce::Colour (0xffeef0f6), dim = juce::Colour (0xffc9cbe0), acc = juce::Colour (0xff7fd0ff);
        u.theme = { juce::Colour (0xff7a7da3), juce::Colour (0xff585b80), txt, dim, acc };
        u.theme.texture = 2;
        u.theme.earColour = juce::Colour (0xff4b4e6e);
        const auto pill = juce::Colour (0xff121316), pline = juce::Colour (0x40ffffff), lcd = juce::Colour (0xff0b0c0e);
        const auto green = juce::Colour (0xff4bff6a), red = juce::Colour (0xffff3b2f), blue = juce::Colour (0xff3ea6ff);

        const float bx[2][4] = { { 100, 232, 364, 496 }, { 844, 976, 1108, 1240 } };
        for (int c = 0; c < 2; ++c)
            for (int b = 0; b < 4; ++b)
            {
                const float x = bx[c][b];
                u.boxes.push_back ({ x - 60, 10, 120, 88, lcd, pline, 8 });                 // afficheur du haut
                u.boxes.push_back ({ x - 54, 104, 108, 350, pill, pline, 54 });             // colonne noire arrondie
                u.texts.push_back ({ kRange[b], x - 58, 14, 116, 14, 11.5f, txt });
                u.texts.push_back ({ "BANDWIDTH", x - 54, 268, 108, 12, 9.5f, txt });
                u.texts.push_back ({ "FREQUENCY HZ", x - 54, 330, 108, 12, 9.5f, txt });
                const int cc = c + 1;
                u.controls.push_back ({ Ctl::Selector, pid ("m", cc, b), "", x - 20, 72, 26, "black", { "CUT", "OUT", "BOOST" } });
                u.controls.push_back ({ Ctl::Lever, pid ("sh", cc, b), "SHELF", x + 36, 76, 18 });
                u.controls.push_back ({ Ctl::Knob, pid ("g", cc, b), "", x, 168, 72, "black", {}, "0", "20" });
                u.texts.push_back ({ "dB", x - 10, 208, 20, 12, 9.5f, txt });
                u.controls.push_back ({ Ctl::Knob, pid ("bw", cc, b), "", x, 248, 40, "black", {}, "", "" });
                juce::StringArray fr; for (auto* t : kFreqTxt[b]) fr.add (t);
                u.controls.push_back ({ Ctl::Selector, pid ("f", cc, b), "", x, 395, 54, "black", fr });
            }
        // centre
        u.texts.push_back ({ "BAKR", 600, 12, 140, 26, 24, txt, false });
        u.texts.push_back ({ "MASSIVE", 600, 36, 140, 22, 18, txt, false });
        u.texts.push_back ({ "PASSIVE STEREO EQUALIZER", 580, 446, 180, 14, 10, txt });
        u.controls.push_back ({ Ctl::Lamp, "power", "POWER", 670, 112, 26, "", {}, {}, {}, red });
        u.controls.push_back ({ Ctl::Button, "in1", "IN", 616, 112, 30, "", {}, {}, {}, blue });
        u.controls.push_back ({ Ctl::Button, "in2", "IN", 724, 112, 30, "", {}, {}, {}, blue });
        u.controls.push_back ({ Ctl::Knob, "gain1", "GAIN", 616, 190, 48, "black", {}, "-6", "+4" });
        u.controls.push_back ({ Ctl::Knob, "gain2", "GAIN", 724, 190, 48, "black", {}, "-6", "+4" });
        u.controls.push_back ({ Ctl::Lever, "link", "LINK", 670, 200, 22 });
        u.controls.push_back ({ Ctl::Selector, "lp1", "LOW PASS", 616, 290, 46, "black", { "OFF", "18K", "12K", "9K", "7K5", "6K" } });
        u.controls.push_back ({ Ctl::Selector, "lp2", "LOW PASS", 724, 290, 46, "black", { "OFF", "18K", "12K", "9K", "7K5", "6K" } });
        u.controls.push_back ({ Ctl::Selector, "hp1", "HIGH PASS", 616, 385, 46, "black", { "OFF", "22", "39", "68", "120", "220" } });
        u.controls.push_back ({ Ctl::Selector, "hp2", "HIGH PASS", 724, 385, 46, "black", { "OFF", "22", "39", "68", "120", "220" } });
        juce::ignoreUnused (green);
        // bande du bas : courbe, mode, couleur, options, VU
        u.boxes.push_back ({ 40, 474, 1260, 126, juce::Colour (0xff121316), pline, 8 });
        u.controls.push_back ({ Ctl::Selector, "ms", "MODE", 950, 532, 40, "black", { "STEREO", "M/S" } });
        u.controls.push_back ({ Ctl::Knob, "color", "COULEUR", 1050, 532, 44, "red", {}, "0", "MAX" });
        u.controls.push_back ({ Ctl::Button, "autoGain", "AUTO GAIN", 930, 584, 54, "", {}, {}, {}, blue });
        u.controls.push_back ({ Ctl::Button, "analog", "ANALOG", 995, 584, 54, "", {}, {}, {}, green });
        u.controls.push_back ({ Ctl::Button, "steps", "0.5 dB", 1060, 584, 54, "", {}, {}, {}, juce::Colour (0xffffb347) });
        u.meters.push_back ({ 1110, 486, 88, 70, Src::OutL, "G / MID", "amber" });
        u.meters.push_back ({ 1206, 486, 88, 70, Src::OutR, "D / SIDE", "amber" });
        return new MassiveEditor (*this, u);
    }

    // éditeur + courbe de réponse
    class Curve : public juce::Component, private juce::Timer
    {
    public:
        explicit Curve (Massive& m) : mp (m) { mp.computeCurves (cv); startTimerHz (15); setInterceptsMouseClicks (false, false); }
        void paint (juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (6, 8);
            auto xOf = [&] (double f) { return r.getX() + r.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); };
            auto yOf = [&] (double db) { return r.getCentreY() - (float) juce::jlimit (-21.0, 21.0, db) / 20.0f * r.getHeight() * 0.5f; };
            g.setFont (juce::FontOptions (9.5f));
            for (double f : { 30.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 })
            {
                g.setColour (juce::Colours::white.withAlpha (0.08f)); g.drawVerticalLine ((int) xOf (f), r.getY(), r.getBottom());
                g.setColour (juce::Colours::white.withAlpha (0.35f));
                g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f), juce::Rectangle<float> (xOf (f) - 14, r.getBottom() - 10, 28, 10), juce::Justification::centred);
            }
            for (int db : { -12, -6, 0, 6, 12 })
            {
                g.setColour (juce::Colours::white.withAlpha (db == 0 ? 0.25f : 0.07f)); g.drawHorizontalLine ((int) yOf (db), r.getX(), r.getRight());
                g.setColour (juce::Colours::white.withAlpha (0.35f));
                g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (r.getX() + 2, yOf (db) - 6, 24, 12), juce::Justification::centredLeft);
            }
            const juce::Colour col[2] = { juce::Colour (0xff6fd0ff), juce::Colour (0xffffb347) };
            for (int c = 1; c >= 0; --c)
            {
                juce::Path pth;
                for (int i = 0; i < kCurve; ++i)
                {
                    const float x = xOf (curveFreq (i)), y = yOf (cv[(size_t) c][(size_t) i]);
                    if (i == 0) pth.startNewSubPath (x, y); else pth.lineTo (x, y);
                }
                g.setColour (col[c].withAlpha (c == 0 ? 0.95f : 0.75f));
                g.strokePath (pth, juce::PathStrokeType (c == 0 ? 2.2f : 1.6f, juce::PathStrokeType::curved));
            }
            const bool ms = mp.p ("ms") > 0.5f;
            g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
            g.setColour (col[0]); g.drawText (ms ? "MID" : "GAUCHE", juce::Rectangle<float> (r.getRight() - 150, r.getY(), 70, 14), juce::Justification::centredRight);
            g.setColour (col[1]); g.drawText (ms ? "SIDE" : "DROITE", juce::Rectangle<float> (r.getRight() - 75, r.getY(), 70, 14), juce::Justification::centredRight);
        }
    private:
        void timerCallback() override { mp.computeCurves (cv); repaint(); }
        Massive& mp;
        std::array<std::array<float, kCurve>, 2> cv {};
    };

    class MassiveEditor : public Editor
    {
    public:
        MassiveEditor (Massive& m, UiSpec u) : Editor (m, u), curve (m), spW (u.w)
        {
            addAndMakeVisible (curve);
            resized();
        }
        void resized() override
        {
            Editor::resized();
            const float s = (float) getWidth() / (float) spW;
            curve.setBounds (48, 478, 830, 118);
            curve.setTransform (juce::AffineTransform::scale (s));
        }
    private:
        Curve curve;
        int spW;
    };

private:
    struct Ch
    {
        std::array<Biquad, 4> band;
        Biquad hp1, hp2, lp;
        Transformer xf;
        float autoComp = 1.0f;
    };
    std::array<Ch, 2> ch;
    struct Ptrs { std::array<std::atomic<float>*, 4> g {}, m {}, sh {}, bw {}, f {}; std::atomic<float>* in = nullptr, *gain = nullptr, *lp = nullptr, *hp = nullptr; };
    std::array<Ptrs, 2> prm;
    std::array<float, 96> lastKey {};
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Massive(); }
