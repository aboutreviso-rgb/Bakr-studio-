// BAKR FET — limiteur FET (ratios 4 / 8 / 12 / 20 / ALL), 0 latence
#include "../Common/BakrUI.h"
using namespace bakr;

class Fet : public BaseProcessor
{
public:
    Fet() : BaseProcessor ("BAKR FET", layout(), presets()) {}

    static Layout layout()
    {
        Layout l;
        addFloat (l, "input", "Input", 0, 48, 18, "dB");
        addFloat (l, "output", "Output", -24, 24, 0, "dB");
        addFloat (l, "attack", "Attack", 20, 800, 200, "us", 150, 0);
        addFloat (l, "release", "Release", 50, 1100, 300, "ms", 300, 0);
        addChoice (l, "ratio", "Ratio", { "4", "8", "12", "20", "ALL" }, 0);
        addFloat (l, "mix", "Mix", 0, 100, 100, "%", 0, 0);
        addChoice (l, "meter", "Meter", { "GR", "Input", "Output" }, 0);
        addBool (l, "power", "Power", true);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            { "Voix rap / lead en avant",       { { "input", 24 }, { "output", 0 }, { "attack", 60 }, { "release", 150 }, { "ratio", 0 } } },
            { "Kick",                           { { "input", 22 }, { "output", 2 }, { "attack", 600 }, { "release", 120 }, { "ratio", 1 } } },
            { "Caisse claire",                  { { "input", 26 }, { "output", 0 }, { "attack", 400 }, { "release", 90 }, { "ratio", 1 } } },
            { "Room batterie (ALL)",            { { "input", 30 }, { "output", -4 }, { "attack", 20 }, { "release", 300 }, { "ratio", 4 }, { "mix", 45 } } },
            { "Basse",                          { { "input", 22 }, { "output", 2 }, { "attack", 300 }, { "release", 250 }, { "ratio", 0 } } },
            { "Afro - Afrobeats (voix)",        { { "input", 22 }, { "output", 2 }, { "attack", 100 }, { "release", 180 }, { "ratio", 0 } } },
            { "Afro - Log drum (Amapiano)",     { { "input", 24 }, { "output", 1 }, { "attack", 700 }, { "release", 200 }, { "ratio", 1 } } },
            { "Afro - Coupe-decale (voix)",     { { "input", 28 }, { "output", -2 }, { "attack", 40 }, { "release", 100 }, { "ratio", 1 } } },
            { "Afro - Mandingue (djembe)",      { { "input", 24 }, { "output", 0 }, { "attack", 500 }, { "release", 120 }, { "ratio", 0 } } },
            { "Afro - Afro-pop parallele",      { { "input", 30 }, { "output", -2 }, { "attack", 80 }, { "release", 250 }, { "ratio", 4 }, { "mix", 40 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        for (auto& s : st) { s.xf.prepare (fsOS, 0.35f, 26000.0f, 0.4f); s.bright.highShelf (fsOS, 6000.0, 0.8, 0.7); }
        env.reset();
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const float in = gain (p ("input") - 18.0f), out = gain (p ("output") + 3.0f);
        const int r = (int) p ("ratio");
        const bool all = r == 4;
        const float ratios[] = { 4, 8, 12, 20, 40 };
        const float ratio = ratios[r];
        const float atk = coefMs (p ("attack") * 0.001f * (all ? 0.5f : 1.0f), fs);
        const float rel = coefMs (p ("release"), fs);
        constexpr float thr = -20.0f;

        const int n = (int) b.getNumSamples();
        float maxGr = 0;
        for (int i = 0; i < n; ++i)
        {
            float lvl = 0;
            for (int c = 0; c < numCh; ++c)
            {
                float& x = b.getChannelPointer ((size_t) c)[i];
                x *= in;
                lvl = juce::jmax (lvl, std::abs (x));
            }
            float target = kneeGr (dB (lvl) - thr, ratio, all ? 0.0f : 2.0f);
            if (all) target *= 1.15f;   // le mode ALL sur-compresse et "explose"
            const float gr = env.process (target, atk, rel);
            maxGr = juce::jmax (maxGr, gr);
            const float g = gain (-gr);
            // grain du FET : augmente avec la compression
            const float grain = juce::jlimit (0.0f, 1.0f, gr / 18.0f) * (all ? 0.9f : 0.5f);
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                float& x = b.getChannelPointer ((size_t) c)[i];
                float y = x * g;
                y = triode (y, 0.08f + grain * 0.6f, 0.08f);          // FET + classe A
                y = s.xf.process (softClip (y, 0.1f));                 // transfo de sortie
                x = s.bright.process (y) * out;
            }
        }
        reportGr (0, maxGr); reportGr (1, maxGr);
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1050; u.h = 340;
        const auto txt = juce::Colour (0xffeceef1), dim = juce::Colour (0xffa8adb4), acc = juce::Colour (0xff6fc2ff);
        u.theme = { juce::Colour (0xff262729), juce::Colour (0xff111213), txt, dim, acc };
        u.theme.texture = 2; u.theme.earColour = juce::Colour (0xff9da2a8);
        // bandeau aluminium brossé en bas, comme les limiteurs FET classiques
        u.boxes = { { 34, 262, 982, 78, juce::Colour (0xffb8bcc1), juce::Colour (0xff6d7177), 0.0f },
                    { 540, 30, 84, 220, juce::Colour (0x22000000), juce::Colour (0x30ffffff) } };
        const auto ink = juce::Colour (0xff1c1d20);
        u.texts = {
            { "BAKR", 60, 272, 140, 34, 30, ink, true, true, juce::Justification::centredLeft },
            { "FET  LIMITING AMPLIFIER", 60, 304, 260, 18, 12, juce::Colour (0xff3a3d42), true, false, juce::Justification::centredLeft },
            { "RATIO", 540, 34, 84, 14, 11, txt },
            { "METER", 640, 283, 56, 14, 11, ink, true, false, juce::Justification::centredRight },
        };
        const auto lit = juce::Colour (0xffffffff);
        u.controls = {
            { Ctl::Knob, "input", "INPUT", 150, 135, 116, "black", {}, "0", "48" },
            { Ctl::Knob, "output", "OUTPUT", 330, 135, 116, "black", {}, "-24", "+24" },
            { Ctl::Knob, "attack", "ATTACK", 468, 95, 54, "black", {}, "20us", "800us" },
            { Ctl::Knob, "release", "RELEASE", 468, 205, 54, "black", {}, "50", "1.1s" },
            { Ctl::Radio, "ratio", "4",   582, 72, 58, "", {}, {}, {}, lit, 0, {}, 0 },
            { Ctl::Radio, "ratio", "8",   582, 108, 58, "", {}, {}, {}, lit, 0, {}, 1 },
            { Ctl::Radio, "ratio", "12",  582, 144, 58, "", {}, {}, {}, lit, 0, {}, 2 },
            { Ctl::Radio, "ratio", "20",  582, 180, 58, "", {}, {}, {}, lit, 0, {}, 3 },
            { Ctl::Radio, "ratio", "ALL", 582, 216, 58, "", {}, {}, {}, juce::Colour (0xffff4a3d), 0, {}, 4 },
            { Ctl::Radio, "meter", "GR",  735, 290, 58, "", {}, {}, {}, lit, 0, {}, 0 },
            { Ctl::Radio, "meter", "IN",  810, 290, 58, "", {}, {}, {}, lit, 0, {}, 1 },
            { Ctl::Radio, "meter", "OUT", 885, 290, 58, "", {}, {}, {}, lit, 0, {}, 2 },
            { Ctl::Knob, "mix", "MIX", 972, 120, 46, "black", {}, "DRY", "WET" },
            { Ctl::Lever, "power", "POWER", 972, 220, 30 },
        };
        u.meters = { { 650, 34, 270, 210, Src::Selectable, "OUTPUT", "cream", "meter" } };
        return new Editor (*this, u);
    }

private:
    struct Ch { Transformer xf; Biquad bright; };
    std::array<Ch, 2> st;
    Env env;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Fet(); }
