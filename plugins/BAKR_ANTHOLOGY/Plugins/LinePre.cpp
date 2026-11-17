// BAKR LINE PRE — préampli classe A à transfo + EQ 3 bandes à inductance + HPF
#include "../Common/BakrUI.h"
using namespace bakr;

class LinePre : public BaseProcessor
{
public:
    LinePre() : BaseProcessor ("BAKR LINE PRE", layout(), presets()) {}

    static Layout layout()
    {
        Layout l;
        addFloat (l, "pre", "Preamp Gain", -10, 30, 0, "dB");
        addBool (l, "mic", "Mic (+20 dB)", false);
        addBool (l, "phase", "Phase", false);
        addBool (l, "eqOn", "EQ In", true);
        addChoice (l, "hpf", "HPF", { "OFF", "50", "80", "160", "300" }, 0);
        addChoice (l, "lfF", "LF Freq", { "OFF", "35", "60", "110", "220" }, 2);
        addFloat (l, "lfG", "LF Gain", -16, 16, 0, "dB");
        addChoice (l, "mfF", "MF Freq", { "360", "700", "1.6K", "3.2K", "4.8K", "7.2K" }, 3);
        addFloat (l, "mfG", "MF Gain", -18, 18, 0, "dB");
        addFloat (l, "hfG", "HF Gain (12k)", -16, 16, 0, "dB");
        addFloat (l, "out", "Output", -30, 12, 0, "dB");
        addFloat (l, "mix", "Mix", 0, 100, 100, "%", 0, 0);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            { "Voix chaude (pousser)",       { { "pre", 12 }, { "out", -6 }, { "hpf", 2 }, { "mfF", 3 }, { "mfG", 2 }, { "hfG", 3 } } },
            { "Voix Afrobeats air",          { { "pre", 8 }, { "out", -3 }, { "hpf", 2 }, { "lfF", 3 }, { "lfG", -2 }, { "mfF", 4 }, { "mfG", 2 }, { "hfG", 5 } } },
            { "Kick afro gros",              { { "pre", 10 }, { "out", -5 }, { "lfF", 2 }, { "lfG", 6 }, { "mfF", 0 }, { "mfG", -5 }, { "hfG", 2 } } },
            { "Basse reggae ronde",          { { "pre", 14 }, { "out", -6 }, { "lfF", 1 }, { "lfG", 4 }, { "mfF", 1 }, { "mfG", 3 } } },
            { "Balafon / Kora brillant",     { { "pre", 6 }, { "out", -2 }, { "hpf", 3 }, { "mfF", 4 }, { "mfG", 3 }, { "hfG", 4 } } },
            { "Saturation forte (Mic)",      { { "mic", 1 }, { "pre", 10 }, { "out", -10 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        for (auto& s : st)
        {
            s.hp1.reset(); s.hp2.reset(); s.lf.reset(); s.mf.reset(); s.hf.reset();
            s.xfIn.prepare (fsOS, 0.75f, 20000.0f, 0.8f);
            s.xfOut.prepare (fsOS, 0.4f, 22000.0f, 0.4f);
        }
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const bool mic = p ("mic") > 0.5f, eqOn = p ("eqOn") > 0.5f;
        const float pre = gain (p ("pre") + (mic ? 20.0f : 0.0f)) * (p ("phase") > 0.5f ? -1.0f : 1.0f);
        const float out = gain (p ("out"));
        const float hps[] = { 0, 50, 80, 160, 300 }, lfs[] = { 0, 35, 60, 110, 220 }, mfs[] = { 360, 700, 1600, 3200, 4800, 7200 };
        const float hpf = hps[(int) p ("hpf")], lfF = lfs[(int) p ("lfF")];
        for (auto& s : st)
        {
            if (hpf > 0) { s.hp1.highPass (fs, hpf, 0.5); s.hp2.highPass (fs, hpf, 1.0); } else { s.hp1.identity(); s.hp2.identity(); }
            if (lfF > 0) s.lf.lowShelf (fs, lfF, p ("lfG"), 0.95); else s.lf.identity();
            s.mf.peak (fs, mfs[(int) p ("mfF")], 1.4, p ("mfG"));
            s.hf.highShelf (fs, 12000.0, p ("hfG"), 0.707);
        }

        const int n = (int) b.getNumSamples();
        for (int c = 0; c < numCh; ++c)
        {
            auto& s = st[(size_t) c];
            float* d = b.getChannelPointer ((size_t) c);
            for (int i = 0; i < n; ++i)
            {
                // préampli classe A : la saturation monte avec le gain (le signal est plus chaud)
                float y = s.xfIn.process (triode (d[i] * pre * 0.7f, 0.35f, 0.1f) / 0.7f);
                if (eqOn) y = s.hf.process (s.mf.process (s.lf.process (s.hp2.process (s.hp1.process (y)))));
                y = s.xfOut.process (softClip (y * 0.8f, 0.3f) / 0.8f);
                d[i] = y * out;
            }
        }
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1000; u.h = 360;
        const auto txt = juce::Colour (0xffeceae4), dim = juce::Colour (0xffa9a7a2), acc = juce::Colour (0xffff6a4d);
        u.theme = { juce::Colour (0xff2a2b2e), juce::Colour (0xff121315), txt, dim, acc };
        const auto box = juce::Colour (0x26000000), line = juce::Colour (0x30ffffff);
        u.boxes = { { 46, 34, 300, 150, box, line }, { 46, 194, 300, 154, box, line },
                    { 356, 34, 600, 314, box, line }
                  };
        u.texts = {
            { "BAKR", 60, 44, 150, 34, 30, txt, true, true },
            { "LINE PRE", 60, 76, 150, 18, 14, acc, true, false, juce::Justification::centredLeft },
            { "class A preamp + EQ", 60, 94, 160, 14, 10, dim, false, true, juce::Justification::centredLeft },
            { "INPUT", 56, 136, 60, 14, 10, dim, true, false, juce::Justification::centredLeft },
            { "OUTPUT", 56, 160, 60, 14, 10, dim, true, false, juce::Justification::centredLeft },
            { "HF", 400, 42, 80, 16, 13, txt }, { "MID", 520, 42, 80, 16, 13, txt },
            { "LOW", 640, 42, 80, 16, 13, txt }, { "FILTER", 760, 42, 80, 16, 13, txt },
            { "12 kHz", 400, 300, 80, 14, 10, dim },
        };
        const auto red = juce::Colour (0xffff3b2f), amber = juce::Colour (0xffffb43a);
        u.controls = {
            { Ctl::Knob, "pre", "PREAMP", 285, 92, 74, "red", {}, "-10", "+30" },
            { Ctl::Button, "mic", "MIC", 85, 230, 52, "", {}, {}, {}, red },
            { Ctl::Button, "phase", "PHASE", 150, 230, 52, "", {}, {}, {}, amber },
            { Ctl::Button, "eqOn", "EQ IN", 215, 230, 52, "", {}, {}, {}, amber },
            { Ctl::Knob, "out", "OUTPUT", 150, 305, 52, "grey", {}, "-30", "+12" },
            { Ctl::Knob, "mix", "MIX", 280, 305, 44, "grey", {}, "DRY", "WET" },

            { Ctl::Knob, "hfG", "GAIN", 440, 130, 66, "red", {}, "-16", "+16" },
            { Ctl::Knob, "mfG", "GAIN", 560, 130, 66, "red", {}, "-18", "+18" },
            { Ctl::Selector, "mfF", "FREQ", 560, 270, 54, "blue", { "360", "700", "1.6K", "3.2K", "4.8K", "7.2K" } },
            { Ctl::Knob, "lfG", "GAIN", 680, 130, 66, "red", {}, "-16", "+16" },
            { Ctl::Selector, "lfF", "FREQ", 680, 270, 54, "blue", { "OFF", "35", "60", "110", "220" } },
            { Ctl::Selector, "hpf", "HIGH PASS", 800, 200, 56, "grey", { "OFF", "50", "80", "160", "300" } },
        };
        u.meters = { { 110, 132, 226, 18, Src::InMax, "", "led" },
                     { 110, 156, 226, 18, Src::OutMax, "", "led" } };
        u.presetArea = { 760, 300, 186, 24 };
        return new Editor (*this, u);
    }

private:
    struct Ch { Biquad hp1, hp2, lf, mf, hf; Transformer xfIn, xfOut; };
    std::array<Ch, 2> st;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LinePre(); }
