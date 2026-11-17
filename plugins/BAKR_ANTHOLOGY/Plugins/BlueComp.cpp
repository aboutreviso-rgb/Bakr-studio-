// BAKR BLUE COMP — compresseur VCA polyvalent, attaque/relâchement manuels ou "programme"
#include "../Common/BakrUI.h"
using namespace bakr;

class BlueComp : public BaseProcessor
{
public:
    BlueComp() : BaseProcessor ("BAKR BLUE COMP", layout(), presets()) {}

    static Layout layout()
    {
        Layout l;
        addFloat (l, "gain", "Gain", 0, 30, 0, "dB");
        addFloat (l, "ratio", "Ratio", 1, 20, 4, ":1", 4);
        addFloat (l, "thr", "Threshold", -40, 0, -15, "dB");
        addFloat (l, "mix", "Dry/Wet", 0, 100, 100, "%", 0, 0);
        addFloat (l, "att", "Attack", 0.1f, 100, 10, "ms", 10);
        addFloat (l, "rel", "Release", 30, 3000, 300, "ms", 300, 0);
        addChoice (l, "arMode", "Attack/Release", { "Manual", "Fixed" }, 0);
        addChoice (l, "schpf", "Sidechain HPF", { "OFF", "60", "90", "150", "300" }, 0);
        addChoice (l, "meter", "Meter", { "GR", "Input", "Output" }, 0);
        addBool (l, "on", "Comp On", true);
        addBool (l, "vintage", "Vintage", true);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            { "Voix Afrobeats",          { { "thr", -20 }, { "ratio", 4 }, { "att", 5 }, { "rel", 120 }, { "gain", 6 }, { "schpf", 2 } } },
            { "Voix douce (programme)",  { { "thr", -18 }, { "ratio", 3 }, { "arMode", 1 }, { "gain", 5 } } },
            { "Basse reggae controlee",  { { "thr", -18 }, { "ratio", 5 }, { "att", 20 }, { "rel", 250 }, { "gain", 5 } } },
            { "Caisse claire claquante", { { "thr", -16 }, { "ratio", 6 }, { "att", 15 }, { "rel", 80 }, { "gain", 6 } } },
            { "Bus batterie parallele",  { { "thr", -28 }, { "ratio", 10 }, { "att", 1 }, { "rel", 100 }, { "gain", 12 }, { "mix", 40 } } },
            { "Piano / guitare",         { { "thr", -16 }, { "ratio", 2.5f }, { "arMode", 1 }, { "gain", 3 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        for (auto& s : st) { s.hp.reset(); s.rms.reset(); s.xf.prepare (fsOS, 0.4f, 22000.0f, 0.5f); }
        env.reset(); envSlow.reset();
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const bool on = p ("on") > 0.5f, vintage = p ("vintage") > 0.5f, fixed = p ("arMode") > 0.5f;
        const float thr = p ("thr"), ratio = p ("ratio"), makeup = gain (p ("gain"));
        const float atk = coefMs (fixed ? 8.0f : p ("att"), fs);
        const float rel = coefMs (fixed ? 150.0f : p ("rel"), fs);
        const float atkSlow = coefMs (60.0f, fs), relSlow = coefMs (1500.0f, fs);
        const float hps[] = { 0, 60, 90, 150, 300 };
        const float hpf = hps[(int) p ("schpf")];
        for (auto& s : st) { if (hpf > 0) s.hp.highPass (fs, hpf, 0.707); else s.hp.identity(); }
        const float rmsC = coefMs (3.0f, fs);

        const int n = (int) b.getNumSamples();
        float maxGr = 0;
        for (int i = 0; i < n; ++i)
        {
            float lvl = 0;
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                const float sc = s.hp.process (b.getChannelPointer ((size_t) c)[i]);
                lvl = juce::jmax (lvl, s.rms.process (sc, rmsC) * 1.41f, std::abs (sc) * 0.6f);
            }
            float gr = 0;
            if (on)
            {
                const float target = kneeGr (dB (lvl) - thr, ratio, 6.0f);
                gr = env.process (target, atk, rel);
                if (fixed) gr = juce::jmax (gr, envSlow.process (target, atkSlow, relSlow));
            }
            maxGr = juce::jmax (maxGr, gr);
            const float g = gain (-gr) * (on ? makeup : 1.0f);
            for (int c = 0; c < numCh; ++c)
            {
                float& x = b.getChannelPointer ((size_t) c)[i];
                x = vintage ? st[(size_t) c].xf.process (softClip (x * g, 0.15f)) : x * g;
            }
        }
        reportGr (0, maxGr); reportGr (1, maxGr);
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1050; u.h = 330;
        const auto txt = juce::Colour (0xfff4f6fa), dim = juce::Colour (0xffc9d6ee), acc = juce::Colour (0xffffe08a);
        u.theme = { juce::Colour (0xff2f63c4), juce::Colour (0xff1c428f), txt, dim, acc };
        u.boxes = { { 760, 30, 250, 150, juce::Colour (0x22000000), juce::Colour (0x40ffffff) } };
        u.texts = {
            { "BAKR", 60, 236, 160, 36, 30, txt, true, true },
            { "BLUE COMP", 60, 270, 160, 18, 14, acc },
            { "COMPRESSOR  BC-1", 60, 288, 160, 14, 10, dim },
        };
        u.controls = {
            { Ctl::Knob, "gain", "GAIN", 140, 96, 70, "black", {}, "0", "30" },
            { Ctl::Knob, "ratio", "RATIO", 270, 96, 70, "black", {}, "1", "20" },
            { Ctl::Knob, "thr", "THRESHOLD", 400, 96, 70, "black", {}, "-40", "0" },
            { Ctl::Knob, "mix", "DRY/WET", 525, 96, 58, "black", {}, "DRY", "WET" },
            { Ctl::Selector, "meter", "METER", 650, 96, 44, "black", { "GR", "IN", "OUT" } },

            { Ctl::Knob, "att", "ATTACK", 290, 245, 64, "black", {}, ".1", "100" },
            { Ctl::Knob, "rel", "RELEASE", 410, 245, 64, "black", {}, "30", "3s" },
            { Ctl::Selector, "arMode", "ATTACK / RELEASE", 535, 250, 46, "black", { "MANUAL", "FIXED" } },
            { Ctl::Selector, "schpf", "SIDECHAIN HPF", 680, 250, 46, "black", { "OFF", "60", "90", "150", "300" } },
            { Ctl::Lamp, "on", "ON", 820, 250, 34, "", {}, {}, {}, juce::Colour (0xffff3b2f) },
            { Ctl::Lever, "vintage", "VINTAGE", 930, 250, 30 },
        };
        u.meters = { { 772, 38, 226, 134, Src::Selectable, "OUTPUT", "amber", "meter" } };
        u.presetArea = { 60, 196, 180, 24 };
        return new Editor (*this, u);
    }

private:
    struct Ch { Biquad hp; Rms rms; Transformer xf; };
    std::array<Ch, 2> st;
    Env env, envSlow;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BlueComp(); }
