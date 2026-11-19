// BAKR TUBE — saturation à lampes Stéréo ou Mid/Side, 0 latence
#include "../Common/BakrUI.h"
using namespace bakr;

class Tube : public BaseProcessor
{
public:
    Tube() : BaseProcessor ("BAKR TUBE", layout(), presets()) {}

    static void addSection (Layout& l, const juce::String& p, const juce::String& nm)
    {
        auto id = [&] (const char* s) { return (p + s); };
        auto keep = [] (const juce::String& s) { static juce::StringArray pool; pool.add (s); return pool[pool.size() - 1].toRawUTF8(); };
        addFloat (l, keep (id ("pent")), keep (nm + " Pentode"), 0, 100, 0, "%", 0, 0);
        addFloat (l, keep (id ("tri")), keep (nm + " Triode"), 0, 100, 30, "%", 0, 0);
        addFloat (l, keep (id ("alt")), keep (nm + " Alt Tube"), 0, 100, 0, "%", 0, 0);
        addFloat (l, keep (id ("sat")), keep (nm + " Saturation"), 0, 100, 0, "%", 0, 0);
        addChoice (l, keep (id ("shape")), keep (nm + " Sat Shape"), { "Bande", "Plein", "Creux" }, 1);
        addFloat (l, keep (id ("freq")), keep (nm + " Sat Freq"), 40, 12000, 700, "Hz", 800, 0);
        addFloat (l, keep (id ("q")), keep (nm + " Sat Q"), 0.3f, 6.0f, 1.0f, "", 1.0f, 2);
        addBool (l, keep (id ("satOn")), keep (nm + " Sat In"), true);
        addBool (l, keep (id ("solo")), keep (nm + " Sat Solo"), false);
        addFloat (l, keep (id ("air")), keep (nm + " Air"), 0, 100, 0, "%", 0, 0);
        addBool (l, keep (id ("airOn")), keep (nm + " Air On"), true);
        addFloat (l, keep (id ("out")), keep (nm + " Output"), -18, 18, 0, "dB");
        addBool (l, keep (id ("byp")), keep (nm + " Bypass"), false);
    }

    static Layout layout()
    {
        Layout l;
        addChoice (l, "mode", "Mode", { "Stereo", "M/S" }, 0);
        addBool (l, "link", "Link S to M", false);
        addSection (l, "m_", "ST/M");
        addSection (l, "s_", "S");
        addFloat (l, "density", "Density", -100, 100, 0, "", 0, 0);
        addChoice (l, "calib", "Calibration", { "Dark", "Normal", "Bright" }, 1);
        addFloat (l, "mono", "Mono Maker", 10, 300, 10, "Hz", 80, 0);
        addFloat (l, "width", "Stereo Width", 0, 400, 100, "%", 0, 0);
        addFloat (l, "mix", "Parallel Mix", 0, 100, 100, "%", 0, 0);
        addFloat (l, "output", "Output", -18, 18, 0, "dB");
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            { "Chaleur douce",                      { { "m_tri", 35 } } },
            { "Master chaud (glue)",                { { "m_tri", 40 }, { "density", 35 }, { "mono", 120 } } },
            { "Master M/S : centre chaud, cotes aeres", { { "mode", 1 }, { "m_tri", 40 }, { "s_tri", 10 }, { "s_air", 40 }, { "mono", 120 }, { "width", 115 } } },
            { "Master brillant (air)",              { { "m_tri", 25 }, { "m_air", 45 }, { "calib", 2 } } },
            { "Voix lead chaude",                   { { "m_tri", 45 }, { "m_pent", 15 }, { "m_air", 30 } } },
            { "Voix rap epaisse",                   { { "m_pent", 45 }, { "m_tri", 25 }, { "density", 25 } } },
            { "Batterie punch",                     { { "m_pent", 50 }, { "density", -45 } } },
            { "Basse grasse",                       { { "m_tri", 50 }, { "m_sat", 45 }, { "m_shape", 0 }, { "m_freq", 700 } } },
            { "Beat lo-fi (sale)",                  { { "m_alt", 70 }, { "m_sat", 60 }, { "calib", 0 } } },
            { "Parallele epais",                    { { "m_tri", 70 }, { "m_pent", 40 }, { "mix", 50 } } },
            { "Afro voix - Afrobeats lead (Nigeria)",   { { "m_tri", 45 }, { "m_pent", 15 }, { "m_air", 40 }, { "calib", 2 } } },
            { "Afro voix - Afrobeats autotune (brillant)", { { "m_tri", 30 }, { "m_pent", 20 }, { "m_air", 55 }, { "calib", 2 } } },
            { "Afro voix - Afrobeats douce (Afro RnB)", { { "m_tri", 45 }, { "m_air", 30 } } },
            { "Afro voix - Ad-libs / backs",        { { "m_tri", 35 }, { "m_pent", 25 }, { "m_air", 35 }, { "width", 130 } } },
            { "Afro voix - Amapiano (aeree)",       { { "m_tri", 30 }, { "m_air", 55 } } },
            { "Afro voix - Coupe-decale (atalaku)", { { "m_pent", 50 }, { "m_tri", 20 }, { "m_sat", 30 }, { "m_shape", 0 }, { "m_freq", 2500 } } },
            { "Afro voix - Mandingue (griot)",      { { "m_tri", 50 }, { "m_sat", 20 }, { "m_shape", 0 }, { "m_freq", 2200 } } },
            { "Afro voix - Reggae / dancehall",     { { "m_tri", 50 }, { "calib", 0 } } },
            { "Afro voix - Afro-drill / rap",       { { "m_pent", 55 }, { "m_alt", 25 }, { "density", 20 } } },
            { "Afro - Afrobeats (master)",          { { "mode", 1 }, { "m_tri", 35 }, { "s_air", 35 }, { "density", 25 }, { "mono", 120 }, { "width", 110 } } },
            { "Afro - Amapiano (log drum / bus)",   { { "m_tri", 30 }, { "m_sat", 40 }, { "m_shape", 0 }, { "m_freq", 90 }, { "mono", 140 } } },
            { "Afro - Coupe-decale (bus)",          { { "m_pent", 40 }, { "m_tri", 25 }, { "density", -20 } } },
            { "Afro - Mandingue (chaleur naturelle)", { { "m_tri", 40 }, { "density", -15 } } },
            { "Afro - Reggae (vintage)",            { { "m_tri", 45 }, { "m_alt", 20 }, { "calib", 0 }, { "mono", 110 } } },
            { "Afro - Afro-pop / RnB",              { { "mode", 1 }, { "m_tri", 35 }, { "s_air", 40 }, { "width", 120 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        for (auto& s : st)
        {
            for (auto* f : { &s.bp, &s.air, &s.pre, &s.post, &s.monoL }) f->reset();
            s.dc.setLP (fsOS, 6.0); s.dc.reset();
            s.denseRms.reset(); s.fast.reset(); s.slow.reset();
        }
        denseEnv.reset();
    }

    struct Sec { float pent, tri, alt, sat, freq, q, air, out; int shape; bool satOn, solo, airOn, byp; };
    Sec readSec (const char* pre)
    {
        auto P = [&] (const char* nm) { return p ((juce::String (pre) + nm).toRawUTF8()); };
        return { P ("pent") * 0.01f, P ("tri") * 0.01f, P ("alt") * 0.01f, P ("sat") * 0.01f, P ("freq"), P ("q"),
                 P ("air") * 0.01f, gain (P ("out")), (int) P ("shape"), P ("satOn") > 0.5f, P ("solo") > 0.5f, P ("airOn") > 0.5f, P ("byp") > 0.5f };
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const bool ms = numCh > 1 && p ("mode") > 0.5f;
        const Sec m = readSec ("m_");
        const Sec s = ms ? (p ("link") > 0.5f ? m : readSec ("s_")) : m;
        const int calib = (int) p ("calib");
        const float density = p ("density") * 0.01f, width = p ("width") * 0.01f, out = gain (p ("output"));
        const float monoF = p ("mono");
        const double emph = calib == 0 ? -3.0 : (calib == 2 ? 3.0 : 0.0);

        for (int c = 0; c < 2; ++c)
        {
            const Sec& k = c == 0 ? m : s;
            auto& ch = st[(size_t) c];
            ch.bp.peak (fs, k.freq, k.q, 0.0); // remplacé juste après par un passe-bande
            {
                const double w = juce::MathConstants<double>::twoPi * juce::jlimit (20.0, fs * 0.45, (double) k.freq) / fs;
                const double al = std::sin (w) / (2.0 * k.q);
                ch.bp.set (al, 0.0, -al, 1.0 + al, -2.0 * std::cos (w), 1.0 - al);
            }
            ch.air.highShelf (fs, 16000.0, k.airOn ? k.air * 8.0 : 0.0, 0.7);
            ch.pre.highShelf (fs, 3000.0, emph, 0.6);
            ch.post.highShelf (fs, 3000.0, -emph, 0.6);
            if (monoF > 15.0f) ch.monoL.lowPass (fs, monoF, 0.707); else ch.monoL.identity();
        }
        const float rmsC = coefMs (20.0f, fs), fastC = coefMs (1.0f, fs), slowC = coefMs (40.0f, fs);

        const int n = (int) b.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { b.getChannelPointer (0)[i], numCh > 1 ? b.getChannelPointer (1)[i] : 0.0f };
            if (ms) { const float mm = 0.5f * (x[0] + x[1]), ss = 0.5f * (x[0] - x[1]); x[0] = mm; x[1] = ss; }

            for (int c = 0; c < numCh; ++c)
            {
                const Sec& k = (ms && c == 1) ? s : m;
                auto& ch = st[(size_t) c];
                if (k.byp) continue;
                float y = ch.pre.process (x[c]);
                // DENSITY : + colle (compression douce) / - attaques en avant
                const float lvl = ch.denseRms.process (y, rmsC);
                if (density > 0.0f)
                {
                    const float grD = kneeGr (dB (lvl * 1.41f) + 20.0f, 1.0f + density * 3.0f, 12.0f);
                    y *= gain (-grD);
                }
                else if (density < 0.0f)
                {
                    const float a = std::abs (y);
                    ch.fast.process (a, fastC, fastC); ch.slow.process (a, slowC, slowC);
                    const float tr = juce::jlimit (0.0f, 12.0f, dB (ch.fast.v + 1e-6f) - dB (ch.slow.v + 1e-6f));
                    y *= gain (tr * -density * 0.6f);
                }
                if (k.pent > 0.0f) y = softClip (y, k.pent * 0.9f);
                if (k.tri > 0.0f)  y = triode (y, k.tri * 0.8f, 0.22f);
                if (k.alt > 0.0f)  y = softClip (triode (y, k.alt * 0.7f, 0.38f), k.alt * 0.4f);
                if (k.satOn && (k.sat > 0.0f || k.solo))
                {
                    const float band = ch.bp.process (y);
                    const float d = 0.2f + k.sat * 1.6f;
                    if (k.solo)            y = softClip (band, d);
                    else if (k.shape == 0) y = (y - band) + softClip (band, d);
                    else if (k.shape == 1) y = softClip (y, k.sat);
                    else                   y = band + softClip (y - band, d);
                }
                y = ch.dc.hp (ch.post.process (y));
                y = ch.air.process (y);
                x[c] = y * k.out;
            }
            if (ms) { const float mm = x[0], ss = x[1]; x[0] = mm + ss; x[1] = mm - ss; }
            if (numCh > 1)
            {
                // mono maker sous la fréquence choisie + largeur
                if (monoF > 15.0f)
                {
                    const float lo0 = st[0].monoL.process (x[0]);
                    const float lo1 = st[1].monoL.process (x[1]);
                    const float loS = 0.5f * (lo0 - lo1);
                    x[0] -= loS; x[1] += loS;
                }
                const float mm = 0.5f * (x[0] + x[1]), ss = 0.5f * (x[0] - x[1]) * width;
                x[0] = mm + ss; x[1] = mm - ss;
            }
            b.getChannelPointer (0)[i] = x[0] * out;
            if (numCh > 1) b.getChannelPointer (1)[i] = x[1] * out;
        }
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1200; u.h = 500;
        const auto txt = juce::Colour (0xffe9edf2), dim = juce::Colour (0xff9aa3ad), acc = juce::Colour (0xff6fd0ff);
        u.theme = { juce::Colour (0xff2c3138), juce::Colour (0xff15181c), txt, dim, acc };
        u.theme.texture = 1; u.theme.earColour = juce::Colour (0xff3a4048);
        const auto box = juce::Colour (0x26000000), line = juce::Colour (0x30ffffff);
        u.boxes = { { 140, 30, 740, 150, box, line }, { 140, 190, 740, 150, box, line },
                    { 48, 350, 1118, 136, box, line }, { 48, 30, 84, 310, box, line },
                    { 890, 152, 276, 188, juce::Colour (0xff0d0c0a), line, 6.0f, 1 } };
        u.texts = {
            { "ST / M", 150, 34, 80, 14, 11, acc, true, false, juce::Justification::centredLeft },
            { "S", 150, 194, 80, 14, 11, acc, true, false, juce::Justification::centredLeft },
            { "BAKR", 60, 360, 150, 36, 30, txt, true, true, juce::Justification::centredLeft },
            { "TUBE", 60, 394, 150, 20, 16, acc, true, false, juce::Justification::centredLeft },
            { "mid / side tube saturation", 60, 414, 180, 14, 10, dim, false, true, juce::Justification::centredLeft },
            { "MODE", 48, 36, 84, 14, 11, txt },
        };
        auto row = [&] (const char* pre, float y)
        {
            auto id = [pre] (const char* s) { static juce::StringArray pool; pool.add (juce::String (pre) + s); return pool[pool.size() - 1].toRawUTF8(); };
            const auto red = juce::Colour (0xffff4a3d), blue = juce::Colour (0xff4fb4ff);
            u.controls.push_back ({ Ctl::Knob, id ("pent"), "PENTODE", 200, y, 52, "black", {}, "0", "100" });
            u.controls.push_back ({ Ctl::Knob, id ("tri"), "TRIODE", 275, y, 52, "black", {}, "0", "100" });
            u.controls.push_back ({ Ctl::Knob, id ("alt"), "ALT TUBE", 350, y, 52, "black", {}, "0", "100" });
            u.controls.push_back ({ Ctl::Knob, id ("sat"), "SATURATION", 430, y, 52, "red", {}, "0", "100" });
            u.controls.push_back ({ Ctl::Selector, id ("shape"), "", 505, y - 4, 36, "black", { "BANDE", "PLEIN", "CREUX" } });
            u.controls.push_back ({ Ctl::Knob, id ("freq"), "SAT FREQ", 580, y, 46, "teal", {}, "40", "12k" });
            u.controls.push_back ({ Ctl::Knob, id ("q"), "Q", 645, y, 40, "black", {}, ".3", "6" });
            u.controls.push_back ({ Ctl::Knob, id ("air"), "AIR", 715, y, 46, "blue", {}, "0", "+8" });
            u.controls.push_back ({ Ctl::Knob, id ("out"), "OUTPUT", 800, y, 52, "black", {}, "-18", "+18" });
            u.controls.push_back ({ Ctl::Button, id ("satOn"), "SAT IN", 455, y + 52, 52, "", {}, {}, {}, red });
            u.controls.push_back ({ Ctl::Button, id ("solo"), "SOLO", 520, y + 52, 52, "", {}, {}, {}, juce::Colour (0xffffc23a) });
            u.controls.push_back ({ Ctl::Button, id ("airOn"), "AIR", 715, y + 52, 52, "", {}, {}, {}, blue });
            u.controls.push_back ({ Ctl::Button, id ("byp"), "BYPASS", 800, y + 52, 58, "", {}, {}, {}, juce::Colour (0xffd0d0d0) });
        };
        row ("m_", 95.0f);
        row ("s_", 255.0f);
        u.controls.push_back ({ Ctl::Radio, "mode", "STEREO", 90, 80, 64, "", {}, {}, {}, juce::Colour (0xff6fd0ff), 0, {}, 0 });
        u.controls.push_back ({ Ctl::Radio, "mode", "M/S", 90, 122, 64, "", {}, {}, {}, juce::Colour (0xff6fd0ff), 0, {}, 1 });
        u.controls.push_back ({ Ctl::Button, "link", "LINK", 90, 255, 64, "", {}, {}, {}, juce::Colour (0xffffc23a) });
        u.controls.push_back ({ Ctl::Knob, "density", "DENSITY", 330, 425, 60, "chrome", {}, "-", "+" });
        u.controls.push_back ({ Ctl::Selector, "calib", "CALIBRATION", 470, 430, 44, "black", { "DARK", "NORMAL", "BRIGHT" } });
        u.controls.push_back ({ Ctl::Knob, "mono", "MONO MAKER", 620, 425, 54, "black", {}, "OFF", "300" });
        u.controls.push_back ({ Ctl::Knob, "width", "STEREO WIDTH", 760, 425, 54, "black", {}, "0", "400%" });
        u.controls.push_back ({ Ctl::Knob, "mix", "PARALLEL MIX", 900, 425, 54, "black", {}, "DRY", "WET" });
        u.controls.push_back ({ Ctl::Knob, "output", "OUTPUT", 1060, 425, 64, "chrome", {}, "-18", "+18" });
        u.meters = { { 892, 34, 134, 108, Src::InMax, "INPUT", "cream" },
                     { 1032, 34, 134, 108, Src::OutMax, "OUTPUT", "cream" } };
        return new Editor (*this, u);
    }

private:
    struct Ch { Biquad bp, air, pre, post, monoL; OnePole dc; Rms denseRms; Env fast, slow; };
    std::array<Ch, 2> st;
    Env denseEnv;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Tube(); }
