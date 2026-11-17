// BAKR VARIMU ST — compresseur/limiteur stéréo à lampes variable-mu, 6 constantes de temps
#include "../Common/BakrUI.h"
using namespace bakr;

class VariMuSt : public BaseProcessor
{
public:
    VariMuSt() : BaseProcessor ("BAKR VARIMU ST", layout(), presets()) {}

    static Layout layout()
    {
        Layout l;
        addChoice (l, "mode", "Mode", { "L/R", "Link", "M/S" }, 1);
        const juce::StringArray tcs { "1", "2", "3", "4", "5", "6" };
        addFloat (l, "inL", "Input L/M", -10, 20, 0, "dB");
        addFloat (l, "thrL", "Threshold L/M", -40, 0, -18, "dB");
        addChoice (l, "tcL", "Time Constant L/M", tcs, 1);
        addFloat (l, "inR", "Input R/S", -10, 20, 0, "dB");
        addFloat (l, "thrR", "Threshold R/S", -40, 0, -18, "dB");
        addChoice (l, "tcR", "Time Constant R/S", tcs, 1);
        addFloat (l, "color", "Color", 0, 100, 40, "%", 0, 0);
        addFloat (l, "out", "Output", -20, 20, 0, "dB");
        addFloat (l, "mix", "Mix", 0, 100, 100, "%", 0, 0);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            { "Master glue",              { { "thrL", -14 }, { "tcL", 4 }, { "color", 35 }, { "out", 3 } } },
            { "Bus batterie afro",        { { "inL", 6 }, { "thrL", -20 }, { "tcL", 1 }, { "color", 60 }, { "mix", 60 } } },
            { "Voix en avant",            { { "mode", 0 }, { "inL", 4 }, { "thrL", -20 }, { "tcL", 2 }, { "inR", 4 }, { "thrR", -20 }, { "tcR", 2 }, { "color", 50 }, { "out", 5 } } },
            { "Bus instru mandingue",     { { "thrL", -16 }, { "tcL", 3 }, { "color", 45 }, { "out", 6 } } },
            { "Stereo M/S (centre fort)", { { "mode", 2 }, { "thrL", -18 }, { "tcL", 2 }, { "thrR", -10 }, { "tcR", 2 }, { "out", 5 } } },
            { "Limiteur lent (TC 6)",     { { "inL", 6 }, { "thrL", -12 }, { "tcL", 5 }, { "color", 40 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        for (auto& s : st) { s.e1.reset(); s.e2.reset(); s.e3.reset(); s.rms.reset(); s.xf.prepare (fsOS, 0.55f, 21000.0f, 0.7f); }
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const int mode = numCh < 2 ? 0 : (int) p ("mode");
        const bool link = mode == 1, ms = mode == 2;
        const float color = p ("color") * 0.01f, out = gain (p ("out"));

        // constantes de temps (attaque ms, relâchement ms) façon limiteur 670
        static const float tcAtk[6]  = { 0.2f, 0.2f, 0.4f, 0.4f, 0.4f, 0.2f };
        static const float tcRel1[6] = { 300, 800, 2000, 5000, 2000, 300 };
        struct Coefs { float a1, r1, a2, r2, a3, r3; int stages; };
        auto make = [&] (int tc)
        {
            Coefs k { coefMs (tcAtk[tc], fs), coefMs (tcRel1[tc], fs), 0, 0, 0, 0, 1 };
            if (tc == 4) { k.a2 = coefMs (40, fs); k.r2 = coefMs (10000, fs); k.stages = 2; }
            if (tc == 5) { k.a2 = coefMs (40, fs); k.r2 = coefMs (10000, fs); k.a3 = coefMs (400, fs); k.r3 = coefMs (25000, fs); k.stages = 3; }
            return k;
        };
        const char* inId[2] = { "inL", "inR" }; const char* thrId[2] = { "thrL", "thrR" }; const char* tcId[2] = { "tcL", "tcR" };
        float inG[2], thr[2]; Coefs k[2];
        for (int c = 0; c < 2; ++c)
        {
            const int src = link ? 0 : c;
            inG[c] = gain (p (inId[src])); thr[c] = p (thrId[src]); k[c] = make ((int) p (tcId[src]));
        }
        const float rmsC = coefMs (0.5f, fs);
        const float drive = 0.2f + 0.5f * color, bias = 0.04f + 0.18f * color;

        const int n = (int) b.getNumSamples();
        float mg[2] = { 0, 0 };
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { b.getChannelPointer (0)[i], numCh > 1 ? b.getChannelPointer (1)[i] : 0.0f };
            if (ms) { const float m = 0.5f * (x[0] + x[1]), s = 0.5f * (x[0] - x[1]); x[0] = m; x[1] = s; }

            float lvl[2];
            for (int c = 0; c < numCh; ++c)
            {
                x[c] *= inG[c];
                lvl[c] = dB (juce::jmax (st[(size_t) c].rms.process (x[c], rmsC) * 1.41f, std::abs (x[c]) * 0.8f));
            }
            if (link && numCh > 1) lvl[0] = lvl[1] = juce::jmax (lvl[0], lvl[1]);

            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                const float target = variMuGr (lvl[c] - thr[c]);
                float gr = s.e1.process (target, k[c].a1, k[c].r1);
                if (k[c].stages >= 2) gr = juce::jmax (gr, s.e2.process (target, k[c].a2, k[c].r2));
                if (k[c].stages >= 3) gr = juce::jmax (gr, s.e3.process (target, k[c].a3, k[c].r3));
                mg[c] = juce::jmax (mg[c], gr);
                // étage push-pull : surtout impair, un peu de pair selon COLOR
                float y = x[c] * gain (-gr);
                y = triode (softClip (y, drive * 0.5f), drive * 0.6f, bias);
                x[c] = s.xf.process (y);
            }
            if (ms) { const float m = x[0], s = x[1]; x[0] = m + s; x[1] = m - s; }
            b.getChannelPointer (0)[i] = x[0] * out;
            if (numCh > 1) b.getChannelPointer (1)[i] = x[1] * out;
        }
        reportGr (0, mg[0]); reportGr (1, numCh > 1 ? mg[1] : mg[0]);
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1150; u.h = 440;
        const auto txt = juce::Colour (0xfff0eee8), dim = juce::Colour (0xffc5c3bc), acc = juce::Colour (0xffffc14d);
        u.theme = { juce::Colour (0xff6f7377), juce::Colour (0xff3a3d41), txt, dim, acc };
        u.boxes = {
            { 450, 30, 250, 220, juce::Colour (0xff1b1712), juce::Colour (0xff9a9690), 10.0f },
            { 462, 142, 226, 100, juce::Colour (0xff120f0b), juce::Colour (0x30ffffff), 6.0f, 1 },
            { 70, 262, 360, 162, juce::Colour (0x22000000), juce::Colour (0x40ffffff) },
            { 720, 262, 360, 162, juce::Colour (0x22000000), juce::Colour (0x40ffffff) },
        };
        u.texts = {
            { "BAKR", 450, 36, 250, 34, 30, txt, true, true },
            { "VARIMU ST", 450, 68, 250, 18, 14, acc },
            { "stereo tube limiting amplifier", 450, 86, 250, 14, 10, dim, false, true },
            { "LEFT  /  MID", 70, 266, 360, 16, 12, acc }, { "RIGHT  /  SIDE", 720, 266, 360, 16, 12, acc },
            { "L / M", 90, 24, 300, 14, 11, txt }, { "R / S", 760, 24, 300, 14, 11, txt },
        };
        u.controls = {

            { Ctl::Knob, "inL", "INPUT", 140, 350, 66, "black", {}, "-10", "+20" },
            { Ctl::Knob, "thrL", "THRESHOLD", 250, 350, 66, "black", {}, "-40", "0" },
            { Ctl::Selector, "tcL", "TIME CONST", 365, 352, 48, "black", { "1", "2", "3", "4", "5", "6" } },

            { Ctl::Selector, "mode", "MODE", 515, 318, 44, "black", { "L/R", "LINK", "M/S" } },
            { Ctl::Knob, "color", "COLOR", 635, 318, 44, "bakelite", {}, "CLEAN", "HOT" },
            { Ctl::Knob, "out", "OUTPUT", 535, 398, 40, "black", {}, "-20", "+20" },
            { Ctl::Knob, "mix", "MIX", 615, 398, 40, "black", {}, "DRY", "WET" },

            { Ctl::Knob, "inR", "INPUT", 790, 350, 66, "black", {}, "-10", "+20" },
            { Ctl::Knob, "thrR", "THRESHOLD", 900, 350, 66, "black", {}, "-40", "0" },
            { Ctl::Selector, "tcR", "TIME CONST", 1015, 352, 48, "black", { "1", "2", "3", "4", "5", "6" } },
        };
        u.meters = { { 90, 40, 300, 192, Src::GrL, "GAIN REDUCTION", "cream" },
                     { 760, 40, 300, 192, Src::GrR, "GAIN REDUCTION", "cream" } };
        u.presetArea = { 480, 108, 190, 24 };
        return new Editor (*this, u);
    }

private:
    struct Ch { Env e1, e2, e3; Rms rms; Transformer xf; };
    std::array<Ch, 2> st;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VariMuSt(); }
