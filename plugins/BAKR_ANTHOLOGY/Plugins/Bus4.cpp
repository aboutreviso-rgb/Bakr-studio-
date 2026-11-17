// BAKR BUS4 — compresseur de bus de console (VCA), colle le mix
#include "../Common/BakrUI.h"
using namespace bakr;

class Bus4 : public BaseProcessor
{
public:
    Bus4() : BaseProcessor ("BAKR BUS4", layout(), presets()) {}

    static Layout layout()
    {
        Layout l;
        addFloat (l, "trim", "Pre Trim", -12, 24, 0, "dB");
        addFloat (l, "thr", "Threshold", -30, 6, -12, "dB");
        addChoice (l, "ratio", "Ratio", { "2", "4", "10" }, 1);
        addChoice (l, "att", "Attack", { "0.1", "0.3", "1", "3", "10", "30" }, 3);
        addChoice (l, "rel", "Release", { "0.1", "0.3", "0.6", "1.2", "AUTO" }, 4);
        addFloat (l, "makeup", "Makeup", 0, 20, 0, "dB");
        addChoice (l, "schpf", "SC HPF", { "OFF", "60", "90", "150", "250" }, 0);
        addBool (l, "in", "Comp In", true);
        addBool (l, "color", "Console Color", true);
        addFloat (l, "mix", "Mix", 0, 100, 100, "%", 0, 0);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            { "Mix bus glue (2 dB)",       { { "thr", -10 }, { "ratio", 0 }, { "att", 4 }, { "rel", 4 }, { "makeup", 2 } } },
            { "Master Afrobeats",           { { "thr", -12 }, { "ratio", 0 }, { "att", 3 }, { "rel", 4 }, { "schpf", 2 }, { "makeup", 2.5f } } },
            { "Batterie qui cogne",         { { "thr", -18 }, { "ratio", 1 }, { "att", 4 }, { "rel", 0 }, { "makeup", 5 } } },
            { "Batterie parallele (NY)",    { { "thr", -24 }, { "ratio", 2 }, { "att", 1 }, { "rel", 1 }, { "makeup", 8 }, { "mix", 40 } } },
            { "Bus instru mandingue",       { { "thr", -14 }, { "ratio", 0 }, { "att", 4 }, { "rel", 2 }, { "schpf", 1 }, { "makeup", 2 } } },
            { "Amapiano / log drum",        { { "thr", -14 }, { "ratio", 1 }, { "att", 5 }, { "rel", 4 }, { "schpf", 3 }, { "makeup", 3 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        for (auto& s : st) { s.hp.reset(); s.rms.reset(); s.xf.prepare (fsOS, 0.25f, 24000.0f, 0.3f); }
        env.reset(); envSlow.reset();
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const float trim = gain (p ("trim")), thr = p ("thr"), makeup = gain (p ("makeup"));
        const float ratios[] = { 2, 4, 10 }, atks[] = { 0.1f, 0.3f, 1, 3, 10, 30 }, rels[] = { 100, 300, 600, 1200 }, hps[] = { 0, 60, 90, 150, 250 };
        const float ratio = ratios[(int) p ("ratio")];
        const float atk = coefMs (atks[(int) p ("att")], fs);
        const int relIdx = (int) p ("rel");
        const bool autoRel = relIdx == 4;
        const float rel = coefMs (autoRel ? 150.0f : rels[relIdx], fs), relSlow = coefMs (1500.0f, fs), atkSlow = coefMs (80.0f, fs);
        const float hpf = hps[(int) p ("schpf")];
        const bool on = p ("in") > 0.5f, color = p ("color") > 0.5f;
        for (auto& s : st) { if (hpf > 0) s.hp.highPass (fs, hpf, 0.707); else s.hp.identity(); }
        const float rmsC = coefMs (1.0f, fs);

        const int n = (int) b.getNumSamples();
        float maxGr = 0;
        for (int i = 0; i < n; ++i)
        {
            float lvl = 0;
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                float& x = b.getChannelPointer ((size_t) c)[i];
                x *= trim;
                if (color) x = softClip (x, 0.12f);
                const float sc = s.hp.process (x);
                lvl = juce::jmax (lvl, s.rms.process (sc, rmsC) * 1.41f, std::abs (sc) * 0.85f);
            }
            float gr = 0;
            if (on)
            {
                const float target = kneeGr (dB (lvl) - thr, ratio, 2.0f);
                gr = env.process (target, atk, rel);
                if (autoRel) gr = juce::jmax (gr, envSlow.process (target, atkSlow, relSlow));
            }
            maxGr = juce::jmax (maxGr, gr);
            const float g = gain (-gr) * (on ? makeup : 1.0f);
            for (int c = 0; c < numCh; ++c)
            {
                float& x = b.getChannelPointer ((size_t) c)[i];
                x = color ? st[(size_t) c].xf.process (x * g) : x * g;
            }
        }
        reportGr (0, maxGr); reportGr (1, maxGr);
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1000; u.h = 340;
        const auto txt = juce::Colour (0xfff2f2f0), dim = juce::Colour (0xffd2d4d6), acc = juce::Colour (0xffffffff);
        u.theme = { juce::Colour (0xff7d8186), juce::Colour (0xff5c6065), txt, dim, acc };
        u.boxes = { { 46, 34, 190, 90, juce::Colour (0x22000000), juce::Colour (0x40ffffff) },
                    { 520, 34, 446, 292, juce::Colour (0x18000000), juce::Colour (0x30ffffff) } };
        u.texts = {
            { "BAKR", 46, 38, 190, 40, 36, txt, true, true },
            { "BUS 4", 46, 78, 190, 22, 18, juce::Colour (0xffff4a3d) },
            { "console bus compressor", 46, 100, 190, 16, 11, dim, false, true },
            { "BUS COMPRESSOR", 262, 300, 246, 18, 12, txt },
        };
        const auto red = juce::Colour (0xffff3b2f), white = juce::Colour (0xfff7f3d8);
        u.controls = {
            { Ctl::Knob, "trim", "PRE TRIM", 140, 255, 66, "red", {}, "-12", "+24" },
            { Ctl::Button, "in", "IN", 315, 235, 62, "", {}, {}, {}, red },
            { Ctl::Button, "color", "COLOR", 405, 235, 62, "", {}, {}, {}, white },
            { Ctl::Knob, "thr", "THRESHOLD", 600, 112, 76, "grey", {}, "-30", "+6" },
            { Ctl::Knob, "makeup", "MAKEUP", 735, 112, 64, "grey", {}, "0", "+20" },
            { Ctl::Selector, "ratio", "RATIO", 875, 112, 50, "grey", { "2", "4", "10" } },
            { Ctl::Selector, "att", "ATTACK", 585, 262, 50, "grey", { ".1", ".3", "1", "3", "10", "30" } },
            { Ctl::Selector, "rel", "RELEASE", 712, 262, 50, "grey", { ".1", ".3", ".6", "1.2", "A" } },
            { Ctl::Selector, "schpf", "S/C HPF", 832, 262, 46, "grey", { "OFF", "60", "90", "150", "250" } },
            { Ctl::Knob, "mix", "MIX", 925, 262, 40, "grey", {}, "", "" },
        };
        u.meters = { { 270, 32, 230, 156, Src::GrMax, "COMPRESSION", "white" } };
        u.presetArea = { 52, 140, 178, 24 };
        return new Editor (*this, u);
    }

private:
    struct Ch { Biquad hp; Rms rms; Transformer xf; };
    std::array<Ch, 2> st;
    Env env, envSlow;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Bus4(); }
