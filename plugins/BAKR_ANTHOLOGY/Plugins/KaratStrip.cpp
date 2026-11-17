// BAKR KARAT STRIP — tranche de console : préampli + EQ 4 bandes + compresseur, 3 modèles
#include "../Common/BakrUI.h"
using namespace bakr;

class KaratStrip : public BaseProcessor
{
public:
    KaratStrip() : BaseProcessor ("BAKR KARAT STRIP", layout(), presets()) {}

    static Layout layout()
    {
        Layout l;
        addChoice (l, "model", "Model", { "Brit", "Punch", "Clean" }, 0);
        addBool (l, "preOn", "Pre In", true);
        addBool (l, "eqOn", "EQ In", true);
        addBool (l, "compOn", "Comp In", true);
        addFloat (l, "trim", "Input Trim", -24, 24, 0, "dB");
        addFloat (l, "drive", "Drive", 0, 100, 30, "%", 0, 0);
        addChoice (l, "hpf", "HPF", { "OFF", "40", "80", "120", "200", "300" }, 0);
        addChoice (l, "lfF", "LF Freq", { "35", "60", "110", "220" }, 1);
        addFloat (l, "lfG", "LF Gain", -15, 15, 0, "dB");
        addFloat (l, "mfF", "MF Freq", 200, 8000, 1500, "Hz", 1200, 0);
        addFloat (l, "mfG", "MF Gain", -15, 15, 0, "dB");
        addChoice (l, "mfQ", "MF Q", { "Wide", "Narrow" }, 0);
        addChoice (l, "hfF", "HF Freq", { "8K", "12K", "16K" }, 1);
        addFloat (l, "hfG", "HF Gain", -15, 15, 0, "dB");
        addFloat (l, "thr", "Threshold", -40, 0, -12, "dB");
        addChoice (l, "ratio", "Ratio", { "1.5", "2", "3", "4", "6", "10" }, 2);
        addFloat (l, "att", "Attack", 0.3f, 80, 8, "ms", 8);
        addFloat (l, "rel", "Release", 50, 1500, 250, "ms", 300, 0);
        addBool (l, "auto", "Auto Release", false);
        addFloat (l, "makeup", "Makeup", 0, 20, 0, "dB");
        addFloat (l, "out", "Output", -24, 24, 0, "dB");
        addFloat (l, "mix", "Mix", 0, 100, 100, "%", 0, 0);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            { "Voix Afrobeats brillante",  { { "hpf", 2 }, { "mfF", 3000 }, { "mfG", 2 }, { "hfG", 3 }, { "thr", -18 }, { "ratio", 3 }, { "att", 5 }, { "rel", 150 }, { "makeup", 4 } } },
            { "Voix mandingue chaude",     { { "drive", 55 }, { "hpf", 2 }, { "lfG", 2 }, { "mfF", 400 }, { "mfG", -2 }, { "hfG", 2 }, { "thr", -16 }, { "ratio", 2 }, { "makeup", 3 } } },
            { "Kick afro (Punch)",         { { "model", 1 }, { "lfF", 1 }, { "lfG", 4 }, { "mfF", 350 }, { "mfG", -4 }, { "mfQ", 1 }, { "hfF", 0 }, { "hfG", 3 }, { "thr", -14 }, { "ratio", 3 }, { "att", 20 }, { "rel", 120 }, { "makeup", 3 } } },
            { "Basse reggae",              { { "drive", 50 }, { "hpf", 1 }, { "lfF", 0 }, { "lfG", 3 }, { "mfF", 800 }, { "mfG", 2 }, { "thr", -18 }, { "ratio", 3 }, { "att", 15 }, { "rel", 300 }, { "makeup", 4 } } },
            { "Caisse claire / Shaker",    { { "model", 1 }, { "hpf", 3 }, { "mfF", 5000 }, { "mfG", 3 }, { "thr", -15 }, { "ratio", 4 }, { "att", 2 }, { "rel", 90 }, { "makeup", 4 } } },
            { "Guitare highlife",          { { "hpf", 2 }, { "mfF", 2500 }, { "mfG", 2 }, { "thr", -14 }, { "ratio", 1 }, { "makeup", 2 } } },
            { "Bus mix (Clean)",           { { "model", 2 }, { "drive", 15 }, { "lfG", 1 }, { "hfG", 1.5f }, { "thr", -10 }, { "ratio", 1 }, { "att", 30 }, { "auto", 1 }, { "makeup", 1 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        for (auto& s : st)
        {
            s.hp1.reset(); s.hp2.reset(); s.lf.reset(); s.mf.reset(); s.hf.reset();
            s.xf.prepare (fsOS, 0.6f, 20000.0f, 0.6f);
            s.xfOut.prepare (fsOS, 0.25f, 24000.0f, 0.2f);
        }
        env.reset(); envSlow.reset();
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const int model = (int) p ("model");
        const bool preOn = p ("preOn") > 0.5f, eqOn = p ("eqOn") > 0.5f, compOn = p ("compOn") > 0.5f, autoRel = p ("auto") > 0.5f;
        const float trim = gain (p ("trim")), drive = p ("drive") * 0.01f, out = gain (p ("out") + p ("makeup") * (compOn ? 1.0f : 0.0f));

        // --- coefficients EQ (selon le modèle)
        const float hpfs[] = { 0, 40, 80, 120, 200, 300 }, lfs[] = { 35, 60, 110, 220 }, hfs[] = { 8000, 12000, 16000 };
        const float hpf = hpfs[(int) p ("hpf")];
        const double lfQ = model == 0 ? 0.95 : (model == 1 ? 0.707 : 0.6);
        const float mfG = p ("mfG");
        double mfQ = p ("mfQ") > 0.5f ? 2.2 : 0.75;
        if (model == 1) mfQ *= 1.0 + std::abs (mfG) / 9.0;   // Q proportionnel (style américain)
        if (model == 0) mfQ *= 0.85;                           // plus large et musical (style anglais)
        for (auto& s : st)
        {
            if (hpf > 0) { s.hp1.highPass (fs, hpf, 0.54); s.hp2.highPass (fs, hpf, 1.31); } else { s.hp1.identity(); s.hp2.identity(); }
            s.lf.lowShelf (fs, lfs[(int) p ("lfF")], p ("lfG"), lfQ);
            s.mf.peak (fs, p ("mfF"), mfQ, mfG);
            s.hf.highShelf (fs, hfs[(int) p ("hfF")], p ("hfG"), model == 0 ? 0.8 : 0.707);
        }

        // --- compresseur
        const float ratios[] = { 1.5f, 2, 3, 4, 6, 10 };
        const float ratio = ratios[(int) p ("ratio")], thr = p ("thr");
        const float knee = model == 0 ? 10.0f : (model == 1 ? 3.0f : 6.0f);
        const float atk = coefMs (p ("att"), fs), rel = coefMs (p ("rel"), fs);
        const float relFast = coefMs (100.0f, fs), relSlow = coefMs (1200.0f, fs), atkSlow = coefMs (60.0f, fs);
        const float rmsC = coefMs (model == 1 ? 0.8f : 3.0f, fs);

        const int n = (int) b.getNumSamples();
        float maxGr = 0;
        for (int i = 0; i < n; ++i)
        {
            float lvl = 0;
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                float& x = b.getChannelPointer ((size_t) c)[i];
                float y = x * trim;
                if (preOn)
                {
                    if (model == 0)      y = s.xf.process (triode (y, 0.15f + 0.6f * drive, 0.12f));
                    else if (model == 1) y = softClip (y, 0.1f + 0.8f * drive);
                    else                 y = softClip (y, 0.25f * drive);
                }
                if (eqOn)
                    y = s.hf.process (s.mf.process (s.lf.process (s.hp2.process (s.hp1.process (y)))));
                x = y;
                lvl = juce::jmax (lvl, s.rms.process (y, rmsC) * 1.41f, std::abs (y) * (model == 1 ? 0.95f : 0.7f));
            }
            float gr = 0;
            if (compOn)
            {
                const float target = kneeGr (dB (lvl) - thr, ratio, knee);
                if (autoRel)
                    gr = juce::jmax (env.process (target, atk, relFast), envSlow.process (target, atkSlow, relSlow));
                else
                    gr = env.process (target, atk, rel);
            }
            maxGr = juce::jmax (maxGr, gr);
            const float g = gain (-gr) * out;
            for (int c = 0; c < numCh; ++c)
            {
                float& x = b.getChannelPointer ((size_t) c)[i];
                x = preOn && model != 2 ? st[(size_t) c].xfOut.process (x * g) : x * g;
            }
        }
        reportGr (0, maxGr); reportGr (1, maxGr);
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1200; u.h = 420;
        const auto gold = juce::Colour (0xffe1b65a), txt = juce::Colour (0xffe9e4d8);
        u.theme = { juce::Colour (0xff35373b), juce::Colour (0xff1c1d20), txt, juce::Colour (0xffa29d92), gold };
        const auto box = juce::Colour (0x33000000), line = juce::Colour (0x44e1b65a);
        u.boxes = { { 48, 110, 190, 292, box, line }, { 248, 110, 472, 292, box, line },
                    { 730, 110, 250, 292, box, line }, { 990, 110, 172, 292, box, line } };
        u.texts = {
            { "BAKR", 470, 34, 260, 36, 32, gold, true, true },
            { "KARAT STRIP", 470, 68, 260, 18, 13, txt },
            { "ANALOG CONSOLE ANTHOLOGY", 470, 86, 260, 14, 9, juce::Colour (0xffa29d92) },
            { "PREAMP", 48, 114, 190, 16, 12, gold }, { "EQUALIZER", 248, 114, 472, 16, 12, gold },
            { "COMPRESSOR", 730, 114, 250, 16, 12, gold }, { "METERS", 990, 114, 172, 16, 12, gold },
        };
        const auto lit = gold;
        u.controls = {
            { Ctl::Button, "preOn", "PRE", 85, 62, 54, "", {}, {}, {}, lit },
            { Ctl::Button, "eqOn", "EQ", 150, 62, 54, "", {}, {}, {}, lit },
            { Ctl::Button, "compOn", "COMP", 215, 62, 54, "", {}, {}, {}, lit },
            { Ctl::Knob, "trim", "INPUT TRIM", 360, 66, 50, "black", {}, "-24", "+24" },
            { Ctl::Selector, "model", "MODEL", 800, 86, 40, "black", { "BRIT", "PUNCH", "CLEAN" }, {}, {}, juce::Colour (0xffff3b2f), 54.0f },
            { Ctl::Knob, "out", "OUTPUT", 920, 66, 50, "black", {}, "-24", "+24" },

            { Ctl::Knob, "drive", "DRIVE", 143, 205, 76, "chrome", {}, "0", "MAX" },
            { Ctl::Selector, "hpf", "HIGH PASS", 143, 340, 46, "black", { "OFF", "40", "80", "120", "200", "300" } },

            { Ctl::Knob, "lfG", "LF GAIN", 310, 205, 60, "red", {}, "-15", "+15" },
            { Ctl::Selector, "lfF", "LF FREQ", 310, 340, 44, "blue", { "35", "60", "110", "220" } },
            { Ctl::Knob, "mfG", "MF GAIN", 430, 205, 60, "red", {}, "-15", "+15" },
            { Ctl::Knob, "mfF", "MF FREQ", 430, 340, 44, "blue", {}, "200", "8K" },
            { Ctl::Selector, "mfQ", "MF Q", 545, 272, 38, "black", { "WIDE", "NARR" } },
            { Ctl::Knob, "hfG", "HF GAIN", 655, 205, 60, "red", {}, "-15", "+15" },
            { Ctl::Selector, "hfF", "HF FREQ", 655, 340, 44, "blue", { "8K", "12K", "16K" } },

            { Ctl::Knob, "thr", "THRESHOLD", 795, 205, 60, "black", {}, "-40", "0" },
            { Ctl::Selector, "ratio", "RATIO", 912, 205, 46, "black", { "1.5", "2", "3", "4", "6", "10" } },
            { Ctl::Knob, "att", "ATTACK", 795, 340, 46, "black", {}, ".3", "80" },
            { Ctl::Knob, "rel", "RELEASE", 912, 340, 46, "black", {}, "50", "1.5s" },
            { Ctl::Button, "auto", "AUTO", 855, 278, 46, "", {}, {}, {}, lit },

            { Ctl::Knob, "makeup", "MAKEUP", 1035, 360, 42, "black", {}, "0", "+20" },
            { Ctl::Knob, "mix", "MIX", 1118, 360, 42, "black", {}, "DRY", "WET" },
        };
        u.meters = { { 1000, 136, 152, 92, Src::GrMax, "COMP GR", "cream" },
                     { 1000, 236, 152, 92, Src::OutMax, "OUTPUT", "cream" } };
        u.presetArea = { 990, 52, 172, 24 };
        return new Editor (*this, u);
    }

private:
    struct Ch { Biquad hp1, hp2, lf, mf, hf; Transformer xf, xfOut; Rms rms; };
    std::array<Ch, 2> st;
    Env env, envSlow;

    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KaratStrip(); }
