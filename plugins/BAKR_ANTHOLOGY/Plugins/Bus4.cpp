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
        const auto txt = juce::Colour (0xfff0f2f5), dim = juce::Colour (0xffb9c1cb), acc = juce::Colour (0xffff4a3d);
        u.theme = { juce::Colour (0xff47505c), juce::Colour (0xff2b3139), txt, dim, acc };
        u.theme.texture = 2; u.theme.earColour = juce::Colour (0xff3a4048);
        u.boxes = { { 96, 22, 214, 300, juce::Colour (0x22000000), juce::Colour (0x30ffffff) },
                    { 326, 22, 520, 300, juce::Colour (0x18000000), juce::Colour (0x30ffffff) },
                    { 856, 22, 110, 300, juce::Colour (0x22000000), juce::Colour (0x30ffffff) } };
        u.texts = {
            { "BUS 4", 38, 20, 56, 300, 46, juce::Colour (0xff7d8794), false, false, juce::Justification::centred, true },
            { "BAKR", 856, 36, 110, 34, 28, txt, true, true },
            { "BUS 4", 856, 68, 110, 18, 14, acc },
            { "console bus", 856, 86, 110, 14, 10, dim, false, true },
            { "compressor", 856, 100, 110, 14, 10, dim, false, true },
            { "OUT", 98, 210, 32, 14, 10, dim },
        };
        const auto red = juce::Colour (0xffff3b2f), white = juce::Colour (0xfffff6d6);
        u.controls = {
            { Ctl::Button, "in", "IN", 160, 176, 50, "", {}, {}, {}, red },
            { Ctl::Button, "color", "COLOR", 240, 176, 50, "", {}, {}, {}, white },
            { Ctl::Knob, "trim", "PRE TRIM", 203, 285, 56, "red", {}, "-12", "+24" },
            { Ctl::Knob, "thr", "THRESHOLD", 410, 112, 80, "grey", {}, "-30", "+6" },
            { Ctl::Knob, "makeup", "MAKEUP", 545, 112, 64, "grey", {}, "0", "+20" },
            { Ctl::Selector, "ratio", "RATIO", 680, 112, 50, "grey", { "2", "4", "10" } },
            { Ctl::Knob, "mix", "MIX", 790, 112, 46, "grey", {}, "DRY", "WET" },
            { Ctl::Selector, "att", "ATTACK", 410, 262, 50, "grey", { ".1", ".3", "1", "3", "10", "30" } },
            { Ctl::Selector, "rel", "RELEASE", 545, 262, 50, "grey", { ".1", ".3", ".6", "1.2", "A" } },
            { Ctl::Selector, "schpf", "S/C HPF", 680, 262, 46, "grey", { "OFF", "60", "90", "150", "250" } },
        };
        u.meters = { { 106, 30, 194, 118, Src::GrMax, "COMPRESSION", "white" },
                     { 128, 208, 176, 18, Src::OutMax, "", "led" } };
        u.presetArea = { 858, 130, 106, 24 };
        return new Editor (*this, u);
    }

private:
    struct Ch { Biquad hp; Rms rms; Transformer xf; };
    std::array<Ch, 2> st;
    Env env, envSlow;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Bus4(); }
