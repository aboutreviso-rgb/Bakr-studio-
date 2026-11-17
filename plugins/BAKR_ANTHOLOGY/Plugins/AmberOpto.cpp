// BAKR AMBER OPTO — compresseur opto à lampes (style teletronix)
#include "../Common/BakrUI.h"
using namespace bakr;

class AmberOpto : public BaseProcessor
{
public:
    AmberOpto() : BaseProcessor ("BAKR AMBER OPTO", layout(), presets()) {}

    static Layout layout()
    {
        Layout l;
        addFloat (l, "trim", "Input Trim", -12, 12, 0, "dB");
        addFloat (l, "peak", "Peak Reduction", 0, 100, 40, "", 0, 0);
        addFloat (l, "gain", "Gain", -10, 30, 0, "dB");
        addChoice (l, "mode", "Mode", { "Compress", "Limit" }, 0);
        addChoice (l, "attack", "Attack", { "Fast", "Slow" }, 0);
        addChoice (l, "release", "Release", { "Fast", "Med", "Slow" }, 1);
        addFloat (l, "emph", "Emphasis", 0, 100, 0, "%", 0, 0);
        addChoice (l, "model", "Model", { "Tube", "Clean" }, 0);
        addFloat (l, "mix", "Mix", 0, 100, 100, "%", 0, 0);
        addChoice (l, "meter", "Meter", { "GR", "Input", "Output" }, 0);
        addBool (l, "on", "Power", true);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            { "Voix lead douce",          { { "peak", 45 }, { "gain", 1.5f }, { "release", 1 } } },
            { "Voix Afrobeats",           { { "peak", 55 }, { "gain", 3 }, { "emph", 30 }, { "attack", 0 } } },
            { "Voix mandingue chaude",    { { "peak", 50 }, { "gain", 2.5f }, { "release", 2 }, { "trim", 2 } } },
            { "Basse ronde",              { { "peak", 60 }, { "gain", 4 }, { "release", 2 }, { "attack", 1 } } },
            { "Limiteur voix",            { { "peak", 60 }, { "gain", 4 }, { "mode", 1 } } },
            { "Bus choeurs (parallele)",  { { "peak", 70 }, { "gain", 9 }, { "mix", 55 } } },
            { "Guitare / Kora",           { { "peak", 40 }, { "gain", 4 }, { "emph", 40 } } },
            { "Propre (Clean)",           { { "peak", 45 }, { "gain", 5 }, { "model", 1 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        for (auto& s : st)
        {
            s.opto.reset(); s.rms.reset(); s.emph.reset();
            s.xfTube.prepare (fsOS, 0.6f, 20000.0f, 0.8f);
            s.xfClean.prepare (fsOS, 0.15f, 24000.0f, 0.3f);
        }
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const float trim = gain (p ("trim")), makeup = gain (p ("gain"));
        const float peak = p ("peak"), emph = p ("emph");
        const bool limit = p ("mode") > 0.5f, slowA = p ("attack") > 0.5f, tube = p ("model") < 0.5f, on = p ("on") > 0.5f;
        const int rel = (int) p ("release");
        const float relScale = rel == 0 ? 0.5f : (rel == 1 ? 1.0f : 2.2f);
        const float rmsC = coefMs (5.0f, fs);
        for (auto& s : st) s.emph.highShelf (fs, 3000.0, emph * 0.12, 0.7);

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
                const float sc = s.emph.process (x);
                lvl = juce::jmax (lvl, s.rms.process (sc, rmsC) * 1.41f, std::abs (sc) * 0.7f);
            }
            const float over = dB (lvl) + peak * 0.45f - 10.0f;
            const float target = kneeGr (over, limit ? 10.0f : 3.0f, limit ? 6.0f : 18.0f);
            const float gr = on ? st[0].opto.process (target, fs, slowA ? 20.0f : 8.0f, relScale) : 0.0f;
            maxGr = juce::jmax (maxGr, gr);
            const float g = gain (-gr);
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                float& x = b.getChannelPointer ((size_t) c)[i];
                float y = x * g;
                y = tube ? s.xfTube.process (triode (y * 0.8f, 0.45f) / 0.8f) : s.xfClean.process (y);
                x = y * makeup;
            }
        }
        reportGr (0, maxGr); reportGr (1, maxGr);
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1000; u.h = 360;
        u.theme = { juce::Colour (0xff3f5f6b), juce::Colour (0xff2b434c), juce::Colour (0xfff3ead2), juce::Colour (0xffc9c2ae), juce::Colour (0xffffd34a) };
        const auto cream = juce::Colour (0xfff3ead2), ink = juce::Colour (0xff2b2416);
        u.boxes = { { 34, 238, 932, 122, juce::Colour (0xffd8b546), juce::Colour (0xff8d6f1e), 0.0f },
                    { 760, 40, 190, 92, juce::Colour (0x22000000), juce::Colour (0x55ffffff), 6.0f } };
        u.texts = {
            { "BAKR", 760, 44, 190, 40, 34, cream, true, true },
            { "AMBER OPTO", 760, 82, 190, 24, 18, juce::Colour (0xffffd34a), true },
            { "opto leveling amplifier", 760, 106, 190, 18, 11, cream, false, true },
            { "TUBE OPTICAL COMPRESSOR  -  BAKR AUDIO PLUGINS", 60, 336, 600, 18, 11, ink, true, false, juce::Justification::centredLeft },
        };
        u.controls = {
            { Ctl::Knob, "trim", "INPUT TRIM", 140, 125, 92, "black", {}, "-12", "+12" },
            { Ctl::Selector, "mode", "MODE", 280, 90, 42, "black", { "COMP", "LIMIT" } },
            { Ctl::Selector, "meter", "METER", 280, 182, 42, "black", { "GR", "IN", "OUT" } },
            { Ctl::Lamp, "on", "POWER", 700, 82, 34, "", {}, {}, {}, juce::Colour (0xffff3b2f) },
            { Ctl::Selector, "model", "MODEL", 700, 180, 42, "black", { "TUBE", "CLEAN" } },
            { Ctl::Knob, "gain", "GAIN", 170, 292, 74, "cream", {}, "-10", "+30" },
            { Ctl::Knob, "peak", "PEAK REDUCTION", 360, 292, 74, "cream", {}, "0", "100" },
            { Ctl::Selector, "attack", "ATTACK", 520, 298, 40, "black", { "FAST", "SLOW" } },
            { Ctl::Selector, "release", "RELEASE", 650, 298, 40, "black", { "FAST", "MED", "SLOW" } },
            { Ctl::Knob, "emph", "EMPHASIS", 780, 298, 46, "black", {}, "0", "HF" },
            { Ctl::Knob, "mix", "MIX", 895, 298, 46, "black", {}, "DRY", "WET" },
        };
        for (auto& c : u.controls)
            if (c.y > 238) c.labelColour = ink;
        u.meters = { { 350, 32, 290, 190, Src::Selectable, "OUTPUT", "cream", "meter" } };
        u.presetArea = { 762, 196, 188, 22 };
        auto* e = new Editor (*this, u);
        return e;
    }

private:
    struct Ch { OptoCell opto; Rms rms; Biquad emph; Transformer xfTube, xfClean; };
    std::array<Ch, 2> st;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AmberOpto(); }
