// BAKR LEVELER — compresseur opto à lampe (niveleur optique), 0 latence
#include "../Common/BakrUI.h"
using namespace bakr;

class Leveler : public BaseProcessor
{
public:
    Leveler() : BaseProcessor ("BAKR LEVELER", layout(), presets()) {}

    static Layout layout()
    {
        Layout l;
        addFloat (l, "red", "Reduction", 0, 100, 40, "", 0, 0);
        addFloat (l, "gain", "Gain", 0, 100, 40, "", 0, 0);
        addChoice (l, "mode", "Compress / Limit", { "Compress", "Limit" }, 0);
        addFloat (l, "accent", "Accent", 0, 100, 0, "%", 0, 0);
        addFloat (l, "mix", "Mix", 0, 100, 100, "%", 0, 0);
        addChoice (l, "meter", "Meter", { "Reduc.", "Entree", "Sortie" }, 0);
        addBool (l, "power", "Power", true);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            { "Voix lead",                   { { "red", 48 }, { "gain", 46 } } },
            { "Rap tenu",                    { { "red", 58 }, { "gain", 52 }, { "mode", 1 } } },
            { "Basse",                       { { "red", 55 }, { "gain", 50 } } },
            { "Guitare acoustique",          { { "red", 42 }, { "gain", 42 }, { "accent", 30 } } },
            { "Colle de bus",                { { "red", 32 }, { "gain", 36 }, { "mix", 70 } } },
            { "Afro - Afrobeats (Nigeria)",  { { "red", 50 }, { "gain", 48 }, { "accent", 35 } } },
            { "Afro - Amapiano",             { { "red", 44 }, { "gain", 44 }, { "accent", 20 }, { "mix", 85 } } },
            { "Afro - Coupe-decale",         { { "red", 60 }, { "gain", 54 }, { "mode", 1 }, { "accent", 25 } } },
            { "Afro - Mandingue",            { { "red", 46 }, { "gain", 50 } } },
            { "Afro - Reggae",               { { "red", 50 }, { "gain", 52 }, { "accent", 10 } } },
            { "Afro - Afro-pop / RnB",       { { "red", 44 }, { "gain", 44 }, { "accent", 30 }, { "mix", 80 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        for (auto& s : st) { s.rms.reset(); s.accent.reset(); s.xf.prepare (fsOS, 0.6f, 21000.0f, 0.8f); s.dc.setLP (fsOS, 6.0); s.dc.reset(); }
        cell.reset();
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const float red = p ("red"), gainK = p ("gain") * 0.01f;
        const bool limit = p ("mode") > 0.5f;
        // GAIN : volume de sortie ET pousse la lampe
        const float makeup = gain (-11.0f + gainK * 30.0f);
        const float drive = 0.15f + 0.55f * gainK;
        for (auto& s : st) s.accent.highShelf (fs, 2500.0, p ("accent") * 0.14, 0.7);
        const float rmsC = coefMs (5.0f, fs);

        const int n = (int) b.getNumSamples();
        float maxGr = 0;
        for (int i = 0; i < n; ++i)
        {
            float lvl = 0;
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                const float sc = s.accent.process (b.getChannelPointer ((size_t) c)[i]);
                lvl = juce::jmax (lvl, s.rms.process (sc, rmsC) * 1.41f, std::abs (sc) * 0.7f);
            }
            const float over = dB (lvl) + red * 0.42f - 12.0f;
            const float target = kneeGr (over, limit ? 10.0f : 3.0f, limit ? 6.0f : 18.0f);
            const float gr = cell.process (target, fs, 10.0f, 1.0f);
            maxGr = juce::jmax (maxGr, gr);
            const float g = gain (-gr);
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                float& x = b.getChannelPointer ((size_t) c)[i];
                float y = x * g * makeup;
                y = triode (y * 0.7f, drive, 0.2f) / 0.7f;
                y = s.dc.hp (y);
                x = s.xf.process (y) * gain (-2.0f * gainK);
            }
        }
        reportGr (0, maxGr); reportGr (1, maxGr);
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1000; u.h = 380;
        const auto txt = juce::Colour (0xff1d1f22), dim = juce::Colour (0xff42454a), acc = juce::Colour (0xffb3261e);
        u.theme = { juce::Colour (0xffa9adb1), juce::Colour (0xff7c8085), txt, dim, acc };
        u.theme.texture = 1; u.theme.earColour = juce::Colour (0xff8d9196);
        u.boxes = { { 50, 290, 900, 74, juce::Colour (0x18000000), juce::Colour (0x40000000) } };
        u.texts = {
            { "BAKR", 60, 22, 170, 40, 34, txt, true, true, juce::Justification::centredLeft },
            { "LEVELER", 60, 58, 170, 20, 15, acc, true, false, juce::Justification::centredLeft },
            { "opto tube leveling amplifier", 60, 78, 220, 16, 11, dim, false, true, juce::Justification::centredLeft },
        };
        u.controls = {
            { Ctl::Knob, "gain", "GAIN", 175, 185, 120, "black", {}, "0", "100" },
            { Ctl::Knob, "red", "PEAK REDUCTION", 825, 185, 120, "black", {}, "0", "100" },
            { Ctl::Selector, "mode", "", 175, 330, 40, "black", { "COMPRESS", "LIMIT" } },
            { Ctl::Knob, "accent", "", 370, 330, 40, "black", {}, "ACCENT", "HF" },
            { Ctl::Knob, "mix", "", 500, 330, 40, "black", {}, "DRY", "WET" },
            { Ctl::Selector, "meter", "", 650, 330, 40, "black", { "REDUC", "IN", "OUT" } },
            { Ctl::Lamp, "power", "", 825, 330, 34, "", {}, {}, {}, juce::Colour (0xffff3b2f) },
        };
        u.texts.push_back ({ "MIX", 470, 296, 60, 14, 11, txt });
        u.texts.push_back ({ "METER", 690, 296, 60, 14, 11, txt });
        u.texts.push_back ({ "POWER", 860, 322, 60, 14, 11, txt, true, false, juce::Justification::centredLeft });
        u.meters = { { 330, 40, 340, 230, Src::Selectable, "OUTPUT", "cream", "meter" } };
        return new Editor (*this, u);
    }

private:
    struct Ch { Rms rms; Biquad accent; Transformer xf; OnePole dc; };
    std::array<Ch, 2> st;
    OptoCell cell;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Leveler(); }
