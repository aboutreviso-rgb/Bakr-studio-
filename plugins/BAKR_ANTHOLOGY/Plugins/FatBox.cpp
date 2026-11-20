// BAKR FATBOX — processeur 2 canaux : transfo (TRANNY), chaleur bande (WARMTH),
// 3 compresseurs combinables (TRACKING / BUSS / SPANK), LF SAT, liaison des canaux. 0 latence.
#include "../Common/BakrUI.h"
using namespace bakr;

class FatBox : public BaseProcessor
{
public:
    FatBox() : BaseProcessor ("BAKR FATBOX", layout(), presets()) {}

    static const char* pid (int ch, const char* nm)
    {
        static juce::StringArray pool;
        const juce::String s = juce::String (nm) + (ch == 0 ? "A" : "B");
        const int i = pool.indexOf (s);
        if (i >= 0) return pool[i].toRawUTF8();
        pool.add (s);
        return pool[pool.size() - 1].toRawUTF8();
    }

    static Layout layout()
    {
        Layout l;
        for (int c = 0; c < 2; ++c)
        {
            const juce::String n = c == 0 ? " 1" : " 2";
            auto nm = [&] (const char* s) { static juce::StringArray pool; pool.add (juce::String (s) + n); return pool[pool.size() - 1].toRawUTF8(); };
            addFloat (l, pid (c, "in"), nm ("Input"), -12, 30, 6, "dB");
            addFloat (l, pid (c, "out"), nm ("Output"), -24, 12, 0, "dB");
            addFloat (l, pid (c, "warm"), nm ("Warmth"), 0, 100, 30, "%", 0, 0);
            addBool (l, pid (c, "tran"), nm ("Tranny"), true);
            addFloat (l, pid (c, "tranLv"), nm ("Tranny Level"), 0, 100, 50, "%", 0, 0);
            addBool (l, pid (c, "lfsat"), nm ("LF Sat"), false);
            addBool (l, pid (c, "buss"), nm ("Buss Comp"), false);
            addBool (l, pid (c, "track"), nm ("Tracking Comp"), true);
            addBool (l, pid (c, "spank"), nm ("Spank"), false);
            addFloat (l, pid (c, "thr"), nm ("Threshold"), -30, 0, -12, "dB");
            addChoice (l, pid (c, "att"), nm ("Attack"), { "0.2 ms", "1 ms", "5 ms", "20 ms" }, 2);
            addChoice (l, pid (c, "rel"), nm ("Release"), { "0.05 s", "0.2 s", "0.5 s", "2 s" }, 1);
            addChoice (l, pid (c, "hp"), nm ("HP Side Filt"), { "OFF", "60", "150", "240", "480" }, 0);
            addBool (l, pid (c, "byp"), nm ("Bypass"), false);
        }
        addBool (l, "linkComp", "Link Compress", true);
        addBool (l, "linkCtrl", "Link Controls", true);
        addFloat (l, "mix", "Mix", 0, 100, 100, "%", 0, 0);
        addBool (l, "power", "Power", true);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        auto list = rawPresets();
        // les deux canaux reçoivent les mêmes réglages
        for (auto& pr : list)
        {
            const auto vals = pr.values;
            for (auto& [id, v] : vals)
            {
                juce::String s (id);
                if (s.endsWith ("A")) pr.values.push_back ({ pid (1, s.dropLastCharacters (1).toRawUTF8()), v });
            }
        }
        return list;
    }

    static std::vector<PresetDef> rawPresets()
    {
        auto A = [] (const char* nm) { return pid (0, nm); };
        return {
            { "Init", {} },
            { "Bus batterie (Buss)",          { { A ("buss"), 1 }, { A ("track"), 0 }, { A ("in"), 10 }, { A ("warm"), 40 }, { A ("out"), -1 } } },
            { "Voix lead (Tracking + Warmth)",{ { A ("in"), 8 }, { A ("warm"), 45 }, { A ("thr"), -16 }, { A ("att"), 1 }, { A ("hp"), 2 }, { A ("out"), 0 } } },
            { "Basse grasse (LF Sat)",        { { A ("in"), 10 }, { A ("lfsat"), 1 }, { A ("tranLv"), 75 }, { A ("thr"), -14 }, { A ("att"), 3 }, { A ("out"), -1 } } },
            { "Room ecrasee (Spank)",         { { A ("spank"), 1 }, { A ("in"), 14 }, { A ("warm"), 60 }, { A ("out"), -5 }, { "mix", 50 } } },
            { "Master doux (Buss)",           { { A ("buss"), 1 }, { A ("track"), 0 }, { A ("in"), 3 }, { A ("warm"), 20 }, { A ("thr"), -8 }, { A ("rel"), 3 }, { A ("hp"), 1 } } },
            { "Afro - Afrobeats (voix)",      { { A ("in"), 8 }, { A ("warm"), 40 }, { A ("thr"), -16 }, { A ("att"), 1 }, { A ("hp"), 2 }, { A ("out"), 0 } } },
            { "Afro - Amapiano (log drum)",   { { A ("in"), 10 }, { A ("lfsat"), 1 }, { A ("warm"), 30 }, { A ("thr"), -12 }, { A ("att"), 3 }, { A ("out"), -1 } } },
            { "Afro - Coupe-decale (bus)",    { { A ("buss"), 1 }, { A ("spank"), 1 }, { A ("track"), 0 }, { A ("in"), 10 }, { A ("warm"), 50 }, { A ("out"), -3 }, { "mix", 70 } } },
            { "Afro - Mandingue (percus)",    { { A ("in"), 8 }, { A ("warm"), 35 }, { A ("thr"), -14 }, { A ("att"), 2 }, { A ("rel"), 0 }, { A ("out"), 0 } } },
            { "Afro - Reggae (basse)",        { { A ("in"), 10 }, { A ("lfsat"), 1 }, { A ("tranLv"), 80 }, { A ("warm"), 45 }, { A ("rel"), 2 }, { A ("out"), -1 } } },
            { "Afro - Afro-pop (master)",     { { A ("buss"), 1 }, { A ("track"), 0 }, { A ("in"), 4 }, { A ("warm"), 25 }, { A ("thr"), -10 }, { A ("rel"), 3 }, { A ("hp"), 1 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        for (auto& s : st)
        {
            s.hfSplit.reset(); s.hp.reset(); s.lfSplit.reset(); s.rms.reset(); s.dc.reset();
            s.dc.setLP (fsOS, 6.0);
            s.xf.prepare (fsOS, 0.5f, 22000.0f, 0.6f);
            for (auto& e : s.env) e.reset();
            s.warmEnv.reset();
        }
    }

    struct Prm { float in, out, warm, tranLv; bool tran, lfsat, buss, track, spank, byp; float thr; int att, rel, hp; };
    Prm read (int c)
    {
        auto P = [&] (const char* nm) { return p (pid (c, nm)); };
        return { gain (P ("in")), gain (P ("out")), P ("warm") * 0.01f, P ("tranLv") * 0.01f,
                 P ("tran") > 0.5f, P ("lfsat") > 0.5f, P ("buss") > 0.5f, P ("track") > 0.5f, P ("spank") > 0.5f, P ("byp") > 0.5f,
                 P ("thr"), (int) P ("att"), (int) P ("rel"), (int) P ("hp") };
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const bool linkCtrl = p ("linkCtrl") > 0.5f, linkComp = p ("linkComp") > 0.5f && numCh > 1;
        Prm k[2] = { read (0), read (linkCtrl ? 0 : 1) };
        const float atks[] = { 0.2f, 1.0f, 5.0f, 20.0f }, rels[] = { 50, 200, 500, 2000 }, hps[] = { 0, 60, 150, 240, 480 };
        struct Co { float a, r; } co[2][3];
        for (int c = 0; c < 2; ++c)
        {
            auto& s = st[(size_t) c];
            const float hp = hps[k[c].hp];
            if (hp > 0) s.hp.highPass (fs, hp, 0.707); else s.hp.identity();
            s.hfSplit.setLP (fs, 3500.0);
            s.lfSplit.setLP (fs, 120.0);
            s.xf.amount = 0.2f + 0.8f * k[c].tranLv;
            const float a = atks[k[c].att], r = rels[k[c].rel];
            co[c][0] = { coefMs (a, fs), coefMs (r, fs) };                 // TRACKING
            co[c][1] = { coefMs (a * 3.0f, fs), coefMs (r * 2.0f, fs) };   // BUSS (plus lent, plus doux)
            co[c][2] = { coefMs (0.1f, fs), coefMs (60.0f, fs) };          // SPANK (très rapide)
        }
        const float rmsC = coefMs (2.0f, fs), warmC = coefMs (30.0f, fs);

        const int n = (int) b.getNumSamples();
        float mg[2] = { 0, 0 }, mw[2] = { 0, 0 };
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { b.getChannelPointer (0)[i], numCh > 1 ? b.getChannelPointer (1)[i] : 0.0f };
            float lvl[2] = { 0, 0 };
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                const Prm& q = k[c];
                if (q.byp) continue;
                float y = x[c] * q.in;
                // TRANNY : transfo + LF SAT (sature surtout les graves)
                if (q.tran)
                {
                    if (q.lfsat)
                    {
                        const float lf = s.lfSplit.lp (y);
                        y = (y - lf) + softClip (lf * 1.6f, 0.5f + q.tranLv) / 1.6f;
                    }
                    y = s.xf.process (y);
                }
                // WARMTH : compression douce des aigus + saturation façon bande
                if (q.warm > 0.0f)
                {
                    const float lowPart = s.hfSplit.lp (y);
                    const float hf = y - lowPart;
                    const float g = 1.0f + 6.0f * q.warm;
                    const float hfSat = std::tanh (hf * g) / g;
                    const float red = dB (std::abs (hf) + 1e-6f) - dB (std::abs (hfSat) + 1e-6f);
                    s.warmEnv.process (red, warmC, warmC);
                    mw[c] = juce::jmax (mw[c], s.warmEnv.v);
                    y = softClip (lowPart + hfSat, 0.25f * q.warm);
                }
                x[c] = y;
                const float sc = s.hp.process (y);
                lvl[c] = juce::jmax (s.rms.process (sc, rmsC) * 1.41f, std::abs (sc) * 0.8f);
            }
            if (linkComp) lvl[0] = lvl[1] = juce::jmax (lvl[0], lvl[1]);

            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                const Prm& q = k[c];
                if (q.byp) continue;
                const float ld = dB (lvl[c]);
                float gr = 0;
                if (q.track) gr += s.env[0].process (kneeGr (ld - q.thr, 3.0f, 6.0f), co[c][0].a, co[c][0].r);
                if (q.buss)  gr += s.env[1].process (kneeGr (ld - q.thr - 4.0f, 2.0f, 10.0f), co[c][1].a, co[c][1].r);
                if (q.spank) gr += s.env[2].process (kneeGr (ld - q.thr - 6.0f, 20.0f, 1.0f), co[c][2].a, co[c][2].r);
                gr = juce::jmin (gr, 40.0f);
                mg[c] = juce::jmax (mg[c], gr);
                float y = x[c] * gain (-gr);
                // Spank ajoute du grain quand il écrase
                if (q.spank) y = softClip (y, juce::jlimit (0.0f, 0.6f, gr / 30.0f));
                x[c] = s.dc.hp (y) * q.out;
            }
            b.getChannelPointer (0)[i] = x[0];
            if (numCh > 1) b.getChannelPointer (1)[i] = x[1];
        }
        reportGr (0, mg[0]); reportGr (1, numCh > 1 ? mg[1] : mg[0]);
        extra[0].store (mw[0] * 2.0f); extra[1].store ((numCh > 1 ? mw[1] : mw[0]) * 2.0f);
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1440; u.h = 430;
        const auto txt = juce::Colour (0xffeceef0), dim = juce::Colour (0xffa3a8ae), acc = juce::Colour (0xffffb347);
        u.theme = { juce::Colour (0xff222325), juce::Colour (0xff0e0f10), txt, dim, acc };
        u.theme.texture = 2; u.theme.earColour = juce::Colour (0xff2b2c2f);
        const auto rack = juce::Colour (0x0cffffff), rline = juce::Colour (0x30ffffff), sep = juce::Colour (0xff050505);
        u.boxes = {
            { 40, 10, 1360, 196, rack, rline, 4.0f }, { 40, 220, 1360, 200, rack, rline, 4.0f },
            { 34, 210, 1372, 6, sep, sep, 0.0f },
            { 682, 14, 76, 400, juce::Colour (0x22000000), juce::Colour (0x20ffffff), 4.0f },
        };
        u.texts = {
            { "BAKR", 682, 362, 76, 24, 20, acc, true, true },
            { "FATBOX", 682, 384, 76, 16, 11, txt },
            { "LINK", 682, 238, 76, 14, 10, dim },
        };
        const auto red = juce::Colour (0xffff4a3d), amber = juce::Colour (0xffffc23a), green = juce::Colour (0xff45d06a);
        for (int c = 0; c < 2; ++c)
        {
            const float ox = c == 0 ? 44.0f : 760.0f;
            const juce::String chn = c == 0 ? "CH 1" : "CH 2";
            u.texts.push_back ({ chn, ox + 6, 186, 60, 14, 10, dim, true, false, juce::Justification::centredLeft });
            u.texts.push_back ({ "COMP", ox + 150, 44, 60, 12, 9, dim, true, false, juce::Justification::centredLeft });
            u.texts.push_back ({ "WARMTH", ox + 150, 74, 60, 12, 9, dim, true, false, juce::Justification::centredLeft });
            u.texts.push_back ({ "COMPRESSOR CONTROLS", ox + 150, 228, 320, 14, 10, dim });
            u.controls.push_back ({ Ctl::Knob, pid (c, "in"), "INPUT", ox + 72, 112, 100, "chrome", {}, "-12", "+30" });
            u.controls.push_back ({ Ctl::Button, pid (c, "buss"), "BUSS", ox + 180, 140, 58, "", {}, {}, {}, red });
            u.controls.push_back ({ Ctl::Button, pid (c, "track"), "TRACK", ox + 248, 140, 58, "", {}, {}, {}, red });
            u.controls.push_back ({ Ctl::Button, pid (c, "spank"), "SPANK", ox + 316, 140, 58, "", {}, {}, {}, red });
            u.controls.push_back ({ Ctl::Knob, pid (c, "warm"), "WARMTH", ox + 405, 118, 62, "chrome", {}, "0", "MAX" });
            u.controls.push_back ({ Ctl::Button, pid (c, "tran"), "TRANNY", ox + 492, 92, 60, "", {}, {}, {}, amber });
            u.controls.push_back ({ Ctl::Button, pid (c, "byp"), "BYPASS", ox + 492, 146, 60, "", {}, {}, {}, juce::Colour (0xffe0e0e0) });
            u.controls.push_back ({ Ctl::Knob, pid (c, "out"), "OUTPUT", ox + 575, 112, 100, "chrome", {}, "-24", "+12" });

            u.controls.push_back ({ Ctl::Knob, pid (c, "thr"), "THRESHOLD", ox + 72, 330, 86, "chrome", {}, "-30", "0" });
            u.controls.push_back ({ Ctl::Selector, pid (c, "hp"), "HP SIDE FILT", ox + 190, 330, 44, "black", { "OFF", "60", "150", "240", "480" } });
            u.controls.push_back ({ Ctl::Selector, pid (c, "att"), "ATTACK", ox + 300, 330, 44, "black", { ".2", "1", "5", "20" } });
            u.controls.push_back ({ Ctl::Selector, pid (c, "rel"), "RELEASE", ox + 405, 330, 44, "black", { ".05", ".2", ".5", "2s" } });
            u.controls.push_back ({ Ctl::Button, pid (c, "lfsat"), "LF SAT", ox + 492, 345, 60, "", {}, {}, {}, red });
            u.controls.push_back ({ Ctl::Knob, pid (c, "tranLv"), "TRANNY LEVEL", ox + 575, 330, 86, "chrome", {}, "0", "MAX" });

            u.meters.push_back ({ ox + 196, 42, 160, 14, c == 0 ? Src::GrL : Src::GrR, "", "bar" });
            u.meters.push_back ({ ox + 196, 72, 160, 14, c == 0 ? Src::Extra0 : Src::Extra1, "", "bar" });
        }
        u.controls.push_back ({ Ctl::Lever, "power", "POWER", 720, 66, 30 });
        u.controls.push_back ({ Ctl::Knob, "mix", "MIX", 720, 160, 40, "black", {}, "DRY", "WET" });
        u.controls.push_back ({ Ctl::Button, "linkComp", "COMP", 720, 272, 56, "", {}, {}, {}, green });
        u.controls.push_back ({ Ctl::Button, "linkCtrl", "CTRL", 720, 318, 56, "", {}, {}, {}, green });
        return new Editor (*this, u);
    }

private:
    struct Ch { OnePole hfSplit, lfSplit, dc; Biquad hp; Rms rms; Transformer xf; Env env[3]; Env warmEnv; };
    std::array<Ch, 2> st;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FatBox(); }
