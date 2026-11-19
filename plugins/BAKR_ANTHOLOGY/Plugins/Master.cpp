// BAKR MASTER — chaîne de studio analogique pour le Master : console > bande > glue > ton > limiteur, LUFS
#include "../Common/BakrUI.h"
using namespace bakr;

class Master : public BaseProcessor
{
public:
    Master() : BaseProcessor ("BAKR MASTER", layout(), presets()) {}

    static Layout layout()
    {
        Layout l;
        addChoice (l, "console", "Console", { "Clean", "Classique", "Vintage" }, 1);
        addFloat (l, "drive", "Console Drive", 0, 100, 50, "%", 0, 0);
        addBool (l, "noise", "Bruit", false);
        addBool (l, "tapeOn", "Bande", true);
        addChoice (l, "speed", "Vitesse", { "15 ips", "30 ips" }, 0);
        addFloat (l, "tapeSat", "Saturation bande", 0, 100, 30, "%", 0, 0);
        addBool (l, "glueOn", "Glue", true);
        addFloat (l, "thr", "Glue Threshold", -30, 0, -12, "dB");
        addChoice (l, "ratio", "Glue Ratio", { "2", "4", "10" }, 0);
        addChoice (l, "att", "Glue Attack", { "0.1", "0.3", "1", "3", "10", "30" }, 4);
        addChoice (l, "rel", "Glue Release", { "0.1", "0.3", "0.6", "1.2", "AUTO" }, 4);
        addChoice (l, "schpf", "Glue S/C Filter", { "OFF", "60", "90", "150" }, 0);
        addFloat (l, "makeup", "Glue Makeup", 0, 12, 0, "dB");
        addFloat (l, "glueMix", "Glue Mix", 0, 100, 100, "%", 0, 0);
        addFloat (l, "low", "Grave (80 Hz)", -6, 6, 0, "dB");
        addFloat (l, "mid", "Medium (1.5 kHz)", -6, 6, 0, "dB");
        addFloat (l, "air", "Air (14 kHz)", -6, 6, 0, "dB");
        addFloat (l, "width", "Largeur", 0, 200, 100, "%", 0, 0);
        addBool (l, "bassMono", "Basses mono", false);
        addFloat (l, "limGain", "Limiter Gain", 0, 18, 0, "dB");
        addFloat (l, "ceiling", "Plafond", -6, 0, -1, "dB");
        addBool (l, "lufsReset", "Reset LUFS", false);
        addBool (l, "power", "Power", true);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            { "Colle douce",                   { { "thr", -10 }, { "tapeSat", 20 } } },
            { "Mix analogique chaud",          { { "console", 2 }, { "drive", 60 }, { "tapeSat", 40 }, { "thr", -14 }, { "makeup", 1.5f }, { "low", 0.8f }, { "air", 0.8f }, { "limGain", 3 } } },
            { "Beat rap / trap punchy",        { { "console", 1 }, { "drive", 60 }, { "thr", -16 }, { "att", 4 }, { "rel", 1 }, { "schpf", 2 }, { "makeup", 2 }, { "low", 1 }, { "bassMono", 1 }, { "limGain", 5 } } },
            { "RnB doux et large",             { { "console", 2 }, { "tapeSat", 35 }, { "thr", -12 }, { "air", 1.2f }, { "width", 120 }, { "limGain", 3 } } },
            { "Master fort (streaming)",       { { "drive", 60 }, { "thr", -16 }, { "makeup", 2 }, { "bassMono", 1 }, { "limGain", 7 }, { "ceiling", -1 } } },
            { "Vintage bande",                 { { "console", 2 }, { "drive", 65 }, { "tapeSat", 70 }, { "air", -1 }, { "limGain", 2 } } },
            { "Juste la console",              { { "tapeOn", 0 }, { "glueOn", 0 }, { "drive", 55 } } },
            { "Transparent",                   { { "console", 0 }, { "tapeOn", 0 }, { "thr", -8 } } },
            { "Mandingue ivoirien",            { { "console", 2 }, { "tapeSat", 45 }, { "thr", -18 }, { "att", 5 }, { "glueMix", 50 }, { "makeup", 3 }, { "low", 0.8f }, { "limGain", 3 } } },
            { "Afrobeats (Nigeria)",           { { "drive", 60 }, { "tapeSat", 35 }, { "thr", -14 }, { "att", 5 }, { "rel", 4 }, { "makeup", 2 }, { "mid", 0.5f }, { "air", 1 }, { "bassMono", 1 }, { "limGain", 6 } } },
            { "Reggae",                        { { "console", 2 }, { "tapeSat", 50 }, { "thr", -14 }, { "schpf", 3 }, { "makeup", 2 }, { "low", 1.5f }, { "air", -1 }, { "limGain", 4 } } },
            { "Amapiano (log drum)",           { { "thr", -14 }, { "schpf", 3 }, { "makeup", 2 }, { "air", 1.2f }, { "width", 125 }, { "bassMono", 1 }, { "limGain", 5 } } },
            { "Coupe-decale",                  { { "drive", 65 }, { "thr", -16 }, { "ratio", 1 }, { "att", 3 }, { "rel", 1 }, { "makeup", 3 }, { "mid", 1 }, { "air", 1 }, { "limGain", 7 } } },
            { "Afro-pop / Afro RnB",           { { "console", 2 }, { "tapeSat", 30 }, { "thr", -12 }, { "air", 1.5f }, { "width", 120 }, { "limGain", 4 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        osRate = fsOS;
        for (auto& s : st)
        {
            for (auto* f : { &s.bump, &s.hfTape, &s.scHp, &s.low, &s.mid, &s.air, &s.monoL, &s.kShelf, &s.kHp }) f->reset();
            s.xf.prepare (fsOS, 0.7f, 21000.0f, 0.8f); s.xfLight.prepare (fsOS, 0.3f, 23000.0f, 0.4f);
            s.rms.reset();
            s.kShelf.highShelf (fsOS, 1681.0, 4.0, 0.707);
            s.kHp.highPass (fsOS, 38.0, 0.5);
        }
        env.reset(); envSlow.reset(); limEnv.reset();
        resetLufs();
    }

    void resetLufs()
    {
        shortMs = 0; blockAcc = 0; blockCount = 0; blocks.clear(); blocks.reserve (40000);
        extra[0].store (-70.0f); extra[1].store (-70.0f);
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const bool rst = p ("lufsReset") > 0.5f;
        if (rst != lastReset) { lastReset = rst; resetLufs(); }

        const int console = (int) p ("console");
        const float drive = p ("drive") * 0.01f;
        const bool noise = p ("noise") > 0.5f, tapeOn = p ("tapeOn") > 0.5f, glueOn = p ("glueOn") > 0.5f, bassMono = p ("bassMono") > 0.5f;
        const bool fast30 = p ("speed") > 0.5f;
        const float tapeSat = p ("tapeSat") * 0.01f;
        const float ratios[] = { 2, 4, 10 }, atks[] = { 0.1f, 0.3f, 1, 3, 10, 30 }, rels[] = { 100, 300, 600, 1200 }, hps[] = { 0, 60, 90, 150 };
        const float ratio = ratios[(int) p ("ratio")], thr = p ("thr"), makeup = gain (p ("makeup")), gMix = p ("glueMix") * 0.01f;
        const float atk = coefMs (atks[(int) p ("att")], fs);
        const int relI = (int) p ("rel");
        const bool autoRel = relI == 4;
        const float rel = coefMs (autoRel ? 150.0f : rels[relI], fs), relSlow = coefMs (1500.0f, fs), atkSlow = coefMs (80.0f, fs);
        const float hpf = hps[(int) p ("schpf")];
        const float width = p ("width") * 0.01f;
        const float limGain = gain (p ("limGain")), ceil = gain (p ("ceiling"));
        const float limAtk = coefMs (0.05f, fs), limRel = coefMs (80.0f, fs);
        for (auto& s : st)
        {
            s.bump.lowShelf (fs, fast30 ? 40.0 : 55.0, tapeOn ? (fast30 ? 0.8 : 1.5) : 0.0, 1.3);
            s.hfTape.highShelf (fs, fast30 ? 16000.0 : 10000.0, tapeOn ? (fast30 ? -0.5 : -1.5) - tapeSat * 1.5 : 0.0, 0.7);
            if (hpf > 0) s.scHp.highPass (fs, hpf, 0.707); else s.scHp.identity();
            s.low.lowShelf (fs, 80.0, p ("low"), 0.7);
            s.mid.peak (fs, 1500.0, 0.7, p ("mid"));
            s.air.highShelf (fs, 14000.0, p ("air"), 0.7);
            s.monoL.lowPass (fs, 120.0, 0.707);
        }
        const float rmsC = coefMs (1.0f, fs);
        const float shortC = 1.0f - std::exp (-1.0f / (3.0f * (float) fs));
        const int blockLen = (int) (0.4 * fs);

        const int n = (int) b.getNumSamples();
        float maxGr = 0, maxLim = 0;
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { b.getChannelPointer (0)[i], numCh > 1 ? b.getChannelPointer (1)[i] : 0.0f };
            float lvl = 0;
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                float y = x[c];
                // CONSOLE : la chaleur monte avec le niveau
                if (console == 0)      y = softClip (y, 0.05f * drive);
                else if (console == 1) y = s.xfLight.process (softClip (y, 0.1f + 0.35f * drive));
                else                   y = s.xf.process (triode (y, 0.15f + 0.5f * drive, 0.18f));
                if (noise) y += (rng.nextFloat() * 2.0f - 1.0f) * 3.0e-5f + 1.5e-5f * std::sin (humPh);
                // BANDE
                if (tapeOn)
                {
                    y = s.bump.process (y);
                    const float a = 0.6f + 1.2f * tapeSat;
                    y = (std::tanh (a * y + 0.05f * tapeSat) - std::tanh (0.05f * tapeSat)) / a;
                    y = s.hfTape.process (y);
                }
                x[c] = y;
                const float sc = s.scHp.process (y);
                lvl = juce::jmax (lvl, s.rms.process (sc, rmsC) * 1.41f, std::abs (sc) * 0.8f);
            }
            humPh += juce::MathConstants<float>::twoPi * 50.0f / (float) fs;
            if (humPh > juce::MathConstants<float>::twoPi) humPh -= juce::MathConstants<float>::twoPi;

            // GLUE (compresseur de bus, stéréo lié)
            float gr = 0;
            if (glueOn)
            {
                const float target = kneeGr (dB (lvl) - thr, ratio, 2.0f);
                gr = env.process (target, atk, rel);
                if (autoRel) gr = juce::jmax (gr, envSlow.process (target, atkSlow, relSlow));
            }
            maxGr = juce::jmax (maxGr, gr);
            const float gG = gain (-gr) * makeup;
            for (int c = 0; c < numCh; ++c)
                x[c] = glueOn ? x[c] * (1.0f - gMix) + x[c] * gG * gMix : x[c];

            // TON
            for (int c = 0; c < numCh; ++c)
            {
                auto& s = st[(size_t) c];
                x[c] = s.air.process (s.mid.process (s.low.process (x[c])));
            }
            if (numCh > 1)
            {
                if (bassMono)
                {
                    const float lo0 = st[0].monoL.process (x[0]), lo1 = st[1].monoL.process (x[1]);
                    const float loS = 0.5f * (lo0 - lo1);
                    x[0] -= loS; x[1] += loS;
                }
                const float mm = 0.5f * (x[0] + x[1]), ss = 0.5f * (x[0] - x[1]) * width;
                x[0] = mm + ss; x[1] = mm - ss;
            }

            // LIMITER (0 latence) : enveloppe crête + sécurité douce au plafond
            float pk = 0;
            for (int c = 0; c < numCh; ++c) { x[c] *= limGain; pk = juce::jmax (pk, std::abs (x[c])); }
            const float over = juce::jmax (0.0f, dB (pk) - dB (ceil));
            const float lg = limEnv.process (over, limAtk, limRel);
            maxLim = juce::jmax (maxLim, lg);
            const float gL = gain (-lg);
            float ms = 0;
            for (int c = 0; c < numCh; ++c)
            {
                float y = x[c] * gL;
                const float k = y / ceil;
                y = ceil * (std::abs (k) < 0.9f ? k : (k > 0 ? 1.0f : -1.0f) * (0.9f + 0.1f * std::tanh ((std::abs (k) - 0.9f) * 10.0f)));
                b.getChannelPointer ((size_t) c)[i] = y;
                // LUFS (pondération K)
                auto& s = st[(size_t) c];
                const float kw = s.kHp.process (s.kShelf.process (y));
                ms += kw * kw;
            }
            if (numCh == 1) ms *= 2.0f;
            shortMs += shortC * (ms - shortMs);
            blockAcc += ms; ++blockCount;
            if (blockCount >= blockLen)
            {
                const double e = blockAcc / blockCount;
                blockAcc = 0; blockCount = 0;
                const float lufsBlock = -0.691f + 10.0f * std::log10 ((float) e + 1e-12f);
                if (lufsBlock > -70.0f && blocks.size() < blocks.capacity()) blocks.push_back ((float) e);
                if (! blocks.empty())
                {
                    double sum = 0; for (float v : blocks) sum += v;
                    const float rel = -0.691f + 10.0f * std::log10 ((float) (sum / blocks.size()) + 1e-12f) - 10.0f;
                    double s2 = 0; int n2 = 0;
                    for (float v : blocks) if (-0.691f + 10.0f * std::log10 (v + 1e-12f) > rel) { s2 += v; ++n2; }
                    if (n2 > 0) extra[1].store (-0.691f + 10.0f * std::log10 ((float) (s2 / n2) + 1e-12f));
                }
            }
        }
        extra[0].store (juce::jmax (-70.0f, -0.691f + 10.0f * std::log10 (shortMs + 1e-12f)));
        extra[2].store (maxLim);
        reportGr (0, maxGr); reportGr (1, maxGr);
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1400; u.h = 440;
        const auto txt = juce::Colour (0xffeef0f3), dim = juce::Colour (0xffa4abb4), acc = juce::Colour (0xffffb347);
        u.theme = { juce::Colour (0xff3a3f46), juce::Colour (0xff1c1f23), txt, dim, acc };
        u.theme.texture = 1; u.theme.earColour = juce::Colour (0xff474d55);
        const auto box = juce::Colour (0x28000000), line = juce::Colour (0x30ffffff);
        u.boxes = { { 48, 40, 200, 360, box, line }, { 258, 40, 170, 360, box, line }, { 438, 40, 400, 360, box, line },
                    { 848, 40, 240, 360, box, line }, { 1098, 40, 130, 360, box, line }, { 1238, 40, 128, 360, box, line } };
        u.texts = {
            { "BAKR", 48, 404, 120, 30, 24, txt, true, true, juce::Justification::centredLeft },
            { "MASTER  -  analog studio chain", 130, 410, 300, 20, 11, dim, true, true, juce::Justification::centredLeft },
            { "CONSOLE", 48, 46, 200, 16, 12, acc }, { "BANDE", 258, 46, 170, 16, 12, acc }, { "GLUE", 438, 46, 400, 16, 12, acc },
            { "TON", 848, 46, 240, 16, 12, acc }, { "LIMITER", 1098, 46, 130, 16, 12, acc }, { "NIVEAU", 1238, 46, 128, 16, 12, acc },
            { "IN", 1244, 296, 30, 12, 9, dim, true, false, juce::Justification::centredLeft },
            { "OUT", 1244, 322, 30, 12, 9, dim, true, false, juce::Justification::centredLeft },
            { "LIMIT GR", 1098, 336, 130, 12, 9, dim },
        };
        const auto red = juce::Colour (0xffff4a3d), amber = juce::Colour (0xffffb347);
        u.controls = {
            { Ctl::Selector, "console", "TYPE", 148, 125, 50, "black", { "CLEAN", "CLASSIQUE", "VINTAGE" } },
            { Ctl::Knob, "drive", "DRIVE", 148, 250, 66, "chrome", {}, "0", "100" },
            { Ctl::Button, "noise", "BRUIT", 148, 350, 60, "", {}, {}, {}, amber },

            { Ctl::Button, "tapeOn", "IN", 343, 92, 56, "", {}, {}, {}, red },
            { Ctl::Radio, "speed", "15 IPS", 308, 150, 60, "", {}, {}, {}, amber, 0, {}, 0 },
            { Ctl::Radio, "speed", "30 IPS", 378, 150, 60, "", {}, {}, {}, amber, 0, {}, 1 },
            { Ctl::Knob, "tapeSat", "SATURATION", 343, 280, 66, "black", {}, "0", "100" },

            { Ctl::Button, "glueOn", "IN", 790, 90, 56, "", {}, {}, {}, red },
            { Ctl::Knob, "thr", "THRESHOLD", 500, 248, 58, "grey", {}, "-30", "0" },
            { Ctl::Selector, "ratio", "RATIO", 605, 252, 42, "grey", { "2", "4", "10" } },
            { Ctl::Selector, "att", "ATTACK", 700, 252, 42, "grey", { ".1", ".3", "1", "3", "10", "30" } },
            { Ctl::Selector, "rel", "RELEASE", 790, 252, 42, "grey", { ".1", ".3", ".6", "1.2", "A" } },
            { Ctl::Selector, "schpf", "S/C FILTRE", 540, 355, 40, "grey", { "OFF", "60", "90", "150" } },
            { Ctl::Knob, "makeup", "MAKEUP", 650, 352, 44, "grey", {}, "0", "+12" },
            { Ctl::Knob, "glueMix", "MIX", 760, 352, 44, "grey", {}, "DRY", "WET" },

            { Ctl::Knob, "low", "GRAVE", 908, 125, 54, "black", {}, "-6", "+6" },
            { Ctl::Knob, "mid", "MEDIUM", 1028, 125, 54, "black", {}, "-6", "+6" },
            { Ctl::Knob, "air", "AIR", 908, 250, 54, "black", {}, "-6", "+6" },
            { Ctl::Knob, "width", "LARGEUR", 1028, 250, 54, "black", {}, "0", "200%" },
            { Ctl::Button, "bassMono", "BASSES MONO", 968, 352, 96, "", {}, {}, {}, amber },

            { Ctl::Knob, "limGain", "GAIN", 1163, 125, 64, "red", {}, "0", "+18" },
            { Ctl::Knob, "ceiling", "PLAFOND", 1163, 255, 54, "black", {}, "-6", "0" },

            { Ctl::Button, "lufsReset", "RESET", 1302, 200, 62, "", {}, {}, {}, juce::Colour (0xffd0d0d0) },
            { Ctl::Lamp, "power", "", 1302, 370, 28, "", {}, {}, {}, red },
        };
        u.meters = {
            { 458, 62, 190, 112, Src::GrMax, "GLUE GR", "cream" },
            { 1110, 350, 106, 14, Src::Extra2, "", "bar" },
            { 1246, 70, 112, 48, Src::Extra0, "COURT", "readout", nullptr, "LU" },
            { 1246, 126, 112, 48, Src::Extra1, "INTEGRE", "readout", nullptr, "LU" },
            { 1272, 294, 88, 14, Src::InMax, "", "led" },
            { 1272, 320, 88, 14, Src::OutMax, "", "led" },
        };
        return new Editor (*this, u);
    }

private:
    struct Ch { Biquad bump, hfTape, scHp, low, mid, air, monoL, kShelf, kHp; Transformer xf, xfLight; Rms rms; };
    std::array<Ch, 2> st;
    Env env, envSlow, limEnv;
    juce::Random rng { 3 };
    float humPh = 0;
    double osRate = 88200.0;
    float shortMs = 0; double blockAcc = 0; int blockCount = 0;
    std::vector<float> blocks;
    bool lastReset = false;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Master(); }
