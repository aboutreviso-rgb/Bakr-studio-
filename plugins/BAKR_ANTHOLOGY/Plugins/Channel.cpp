// BAKR CHANNEL — tranche de console : entrée, filtres, EQ 4 bandes, compresseur 5 caractères, gate,
// grain analogique, clipper, sortie. 0 latence.
#include "../Common/BakrUI.h"
using namespace bakr;

class Channel : public BaseProcessor
{
public:
    Channel() : BaseProcessor ("BAKR CHANNEL", layout(), presets()) {}

    static Layout layout()
    {
        Layout l;
        addBool (l, "power", "Power", true);
        addChoice (l, "mode", "Mode", { "Stereo", "Mono (G+D)", "Mono (gauche)", "Mono (droite)" }, 0);
        addFloat (l, "inTrim", "In Trim", -24, 24, 0, "dB");
        addBool (l, "phase", "Phase", false);
        addBool (l, "pre", "Pre", false);
        addFloat (l, "drive", "Drive", 0, 100, 30, "%", 0, 0);
        addFloat (l, "hpf", "HPF", 16, 500, 16, "Hz", 80, 0);
        addFloat (l, "lpf", "LPF", 3000, 22000, 22000, "Hz", 9000, 0);
        addFloat (l, "hfG", "HF Gain", -18, 18, 0, "dB");
        addFloat (l, "hfF", "HF Freq", 1500, 16000, 8000, "Hz", 6000, 0);
        addBool (l, "hfBell", "HF Bell", false);
        addFloat (l, "hmG", "HMF Gain", -18, 18, 0, "dB");
        addFloat (l, "hmF", "HMF Freq", 600, 7000, 2500, "Hz", 2200, 0);
        addFloat (l, "hmQ", "HMF Q", 0.4f, 4.0f, 1.0f, "", 1.0f, 2);
        addFloat (l, "lmG", "LMF Gain", -18, 18, 0, "dB");
        addFloat (l, "lmF", "LMF Freq", 200, 2500, 600, "Hz", 600, 0);
        addFloat (l, "lmQ", "LMF Q", 0.4f, 4.0f, 1.0f, "", 1.0f, 2);
        addFloat (l, "lfG", "LF Gain", -18, 18, 0, "dB");
        addFloat (l, "lfF", "LF Freq", 30, 450, 100, "Hz", 100, 0);
        addBool (l, "lfBell", "LF Bell", false);
        addBool (l, "modern", "Moderne", false);
        addBool (l, "eqOn", "EQ In", true);
        addChoice (l, "cType", "Comp Type", { "VCA", "FET", "OPTO", "TUBE", "CLEAN" }, 0);
        addFloat (l, "ratio", "Ratio", 1, 21, 3, ":1", 4, 1);
        addFloat (l, "thr", "Seuil", -50, 0, -10, "dB");
        addFloat (l, "rel", "Release", 50, 2000, 250, "ms", 300, 0);
        addBool (l, "fast", "Fast Attack", false);
        addFloat (l, "cMix", "Comp Mix", 0, 100, 100, "%", 0, 0);
        addFloat (l, "gThr", "Gate Seuil", -80, 0, -80, "dB");
        addFloat (l, "gRange", "Gate Range", 0, 80, 40, "dB");
        addFloat (l, "gRel", "Gate Release", 20, 2000, 200, "ms", 200, 0);
        addBool (l, "gFast", "Gate Fast", false);
        addBool (l, "gExp", "Expandeur", false);
        addBool (l, "dynOn", "Dyn In", true);
        addBool (l, "dynPre", "Dyn avant EQ", false);
        addBool (l, "fltSc", "Filtres -> S/C", false);
        addBool (l, "scListen", "Ecoute S/C", false);
        addChoice (l, "grain", "Grain", { "Console", "Chaud", "Bande" }, 0);
        addFloat (l, "grainAmt", "Grain Dosage", 0, 100, 50, "%", 0, 0);
        addBool (l, "clipOn", "Clipper", false);
        addFloat (l, "clipGain", "Clip Gain", 0, 18, 0, "dB");
        addFloat (l, "clipCeil", "Plafond", -12, 0, -0.3f, "dB");
        addFloat (l, "clipCurve", "Courbe", 0, 100, 30, "%", 0, 0);
        addBool (l, "clipListen", "Ecoute Clip", false);
        addFloat (l, "width", "Largeur", 0, 200, 100, "%", 0, 0);
        addFloat (l, "pan", "Pan", -100, 100, 0, "", 0, 0);
        addFloat (l, "fader", "Fader", -60, 12, 0, "dB");
        addFloat (l, "outTrim", "Out Trim", -12, 12, 0, "dB");
        addBool (l, "mute", "Mute", false);
        return l;
    }

    static std::vector<PresetDef> presets()
    {
        return {
            { "Init", {} },
            { "Voix lead (VCA)",                 { { "hpf", 90 }, { "lmG", -2 }, { "lmF", 350 }, { "hmG", 2 }, { "hmF", 3500 }, { "hfG", 2.5f }, { "hfF", 12000 }, { "thr", -18 }, { "ratio", 3 } } },
            { "Voix rap (FET)",                  { { "hpf", 100 }, { "hmG", 3 }, { "hmF", 3000 }, { "cType", 1 }, { "thr", -20 }, { "ratio", 4 }, { "rel", 120 }, { "fast", 1 } } },
            { "Kick",                            { { "lfG", 4 }, { "lfF", 60 }, { "lfBell", 1 }, { "lmG", -4 }, { "lmF", 350 }, { "hmG", 3 }, { "hmF", 4000 }, { "thr", -14 }, { "ratio", 4 } } },
            { "Caisse claire (gate)",            { { "hpf", 80 }, { "hmG", 3 }, { "hmF", 5000 }, { "thr", -16 }, { "ratio", 4 }, { "gThr", -40 }, { "gRange", 30 } } },
            { "Bus instru (TUBE)",               { { "cType", 3 }, { "thr", -12 }, { "ratio", 2 }, { "grain", 1 }, { "grainAmt", 60 } } },
            { "Afro - Afrobeats (Nigeria)",      { { "hpf", 90 }, { "lmG", -2 }, { "lmF", 300 }, { "hmG", 2 }, { "hmF", 3500 }, { "hfG", 4 }, { "hfF", 12000 }, { "cType", 2 }, { "thr", -20 }, { "ratio", 3 }, { "grain", 1 } } },
            { "Afro - Amapiano (voix)",          { { "hpf", 90 }, { "hfG", 3 }, { "hfF", 12000 }, { "cType", 2 }, { "thr", -16 }, { "ratio", 2.5f }, { "cMix", 80 }, { "width", 120 } } },
            { "Afro - Log drum (Amapiano)",      { { "lfG", 3 }, { "lfF", 70 }, { "lfBell", 1 }, { "lmG", -3 }, { "lmF", 400 }, { "thr", -14 }, { "ratio", 4 }, { "clipOn", 1 }, { "clipGain", 2 } } },
            { "Afro - Coupe-decale (voix)",      { { "hpf", 110 }, { "pre", 1 }, { "drive", 55 }, { "hmG", 3.5f }, { "hmF", 2500 }, { "cType", 1 }, { "thr", -22 }, { "ratio", 6 }, { "rel", 100 }, { "fast", 1 } } },
            { "Afro - Mandingue (voix griot)",   { { "hpf", 80 }, { "hmG", 2.5f }, { "hmF", 2200 }, { "cType", 3 }, { "thr", -16 }, { "ratio", 2.5f }, { "grain", 1 } } },
            { "Afro - Percussions (djembe, shaker)", { { "hpf", 60 }, { "hmG", 2 }, { "hmF", 4000 }, { "cType", 1 }, { "thr", -14 }, { "ratio", 4 }, { "rel", 90 } } },
            { "Afro - Balafon / kora",           { { "hpf", 100 }, { "lmG", -2 }, { "lmF", 400 }, { "hfG", 2.5f }, { "hfF", 10000 }, { "cType", 2 }, { "thr", -14 }, { "ratio", 2 } } },
            { "Afro - Reggae (voix)",            { { "hpf", 80 }, { "lpf", 14000 }, { "lmG", 1.5f }, { "lmF", 250 }, { "cType", 2 }, { "thr", -16 }, { "ratio", 3 }, { "grain", 2 }, { "grainAmt", 60 } } },
            { "Afro - Afro-pop / RnB (voix)",    { { "hpf", 90 }, { "hfG", 4.5f }, { "hfF", 12000 }, { "cType", 0 }, { "thr", -20 }, { "ratio", 3 }, { "cMix", 75 }, { "grain", 1 } } },
        };
    }

    void prepareCore (double fsOS, int, int nCh) override
    {
        numCh = nCh;
        for (auto& s : st)
        {
            for (auto* f : { &s.hp, &s.lp, &s.hf, &s.hm, &s.lm, &s.lf, &s.scHp, &s.scLp }) f->reset();
            s.hp1.reset(); s.scHp1.reset();
            s.xfCon.prepare (fsOS, 0.4f, 23000.0f, 0.5f);
            s.xfWarm.prepare (fsOS, 0.5f, 17000.0f, 0.6f);
            s.xfTape.prepare (fsOS, 0.5f, 15000.0f, 1.2f);
            s.dc.setLP (fsOS, 6.0); s.dc.reset();
        }
        env.reset(); gEnv.reset(); opto.reset(); rms.reset();
    }

    void processCore (juce::dsp::AudioBlock<float>& b, double fs) override
    {
        const int mode = numCh < 2 ? 1 : (int) p ("mode");
        const bool mono = mode > 0;
        const float inG = gain (p ("inTrim")) * (p ("phase") > 0.5f ? -1.0f : 1.0f);
        const bool preOn = p ("pre") > 0.5f; const float drive = p ("drive") * 0.01f;
        const float hpfF = p ("hpf"), lpfF = p ("lpf");
        const bool hpOn = hpfF > 17.0f, lpOn = lpfF < 21500.0f;
        const bool modern = p ("modern") > 0.5f, eqOn = p ("eqOn") > 0.5f;
        auto bellQ = [modern] (double q, double g) { return modern ? q * (1.0 + std::abs (g) / 9.0) : q; };
        for (auto& s : st)
        {
            if (hpOn) { s.hp.highPass (fs, hpfF, 1.0); s.hp1.setLP (fs, hpfF); s.scHp.highPass (fs, hpfF, 1.0); s.scHp1.setLP (fs, hpfF); }
            else      { s.hp.identity(); s.scHp.identity(); }
            if (lpOn) { s.lp.lowPass (fs, lpfF, 0.707); s.scLp.lowPass (fs, lpfF, 0.707); } else { s.lp.identity(); s.scLp.identity(); }
            if (p ("hfBell") > 0.5f) s.hf.peak (fs, p ("hfF"), bellQ (0.9, p ("hfG")), p ("hfG")); else s.hf.highShelf (fs, p ("hfF"), p ("hfG"), 0.707);
            s.hm.peak (fs, p ("hmF"), bellQ (p ("hmQ"), p ("hmG")), p ("hmG"));
            s.lm.peak (fs, p ("lmF"), bellQ (p ("lmQ"), p ("lmG")), p ("lmG"));
            if (p ("lfBell") > 0.5f) s.lf.peak (fs, p ("lfF"), bellQ (0.9, p ("lfG")), p ("lfG")); else s.lf.lowShelf (fs, p ("lfF"), p ("lfG"), 0.707);
        }
        const int cType = (int) p ("cType");
        const float ratioP = p ("ratio"), ratio = ratioP >= 20.5f ? 1000.0f : ratioP, thr = p ("thr");
        const bool fast = p ("fast") > 0.5f;
        const float atkMs = cType == 1 ? (fast ? 0.1f : 1.0f) : (fast ? 1.0f : 10.0f);
        const float atk = coefMs (atkMs, fs), rel = coefMs (p ("rel"), fs);
        const float cMix = p ("cMix") * 0.01f;
        const float gThr = p ("gThr"), gRange = p ("gRange");
        const bool gateOn = gThr > -79.5f, gExp = p ("gExp") > 0.5f;
        const float gAtk = coefMs (p ("gFast") > 0.5f ? 0.1f : 2.0f, fs), gRel = coefMs (p ("gRel"), fs);
        const bool dynOn = p ("dynOn") > 0.5f, dynPre = p ("dynPre") > 0.5f, fltSc = p ("fltSc") > 0.5f, scListen = p ("scListen") > 0.5f;
        const int grainT = (int) p ("grain"); const float grainAmt = p ("grainAmt") * 0.01f;
        const bool clipOn = p ("clipOn") > 0.5f, clipListen = p ("clipListen") > 0.5f;
        const float clipGain = gain (p ("clipGain")), ceil = gain (p ("clipCeil")), curve = p ("clipCurve") * 0.01f;
        const float width = p ("width") * 0.01f, pan = p ("pan") * 0.01f;
        const float outG = (p ("mute") > 0.5f ? 0.0f : 1.0f) * gain (p ("fader") <= -59.5f ? -150.0f : p ("fader")) * gain (p ("outTrim"));
        const float rmsC = coefMs (cType == 1 ? 0.5f : 3.0f, fs);
        const float knee = cType == 3 ? 16.0f : (cType == 1 ? 0.0f : (cType == 4 ? 6.0f : 4.0f));
        const float panL = std::cos ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.41421f;
        const float panR = std::sin ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f) * 1.41421f;

        const int n = (int) b.getNumSamples();
        float maxGr = 0, maxClip = 0;
        const int nProc = mono ? 1 : numCh;
        for (int i = 0; i < n; ++i)
        {
            float x[2] = { b.getChannelPointer (0)[i], numCh > 1 ? b.getChannelPointer (1)[i] : 0.0f };
            if (mode == 1) x[0] = 0.5f * (x[0] + x[1]);
            else if (mode == 3) x[0] = x[1];

            float sc[2] = { 0, 0 };
            float lvl = 0;
            for (int c = 0; c < nProc; ++c)
            {
                auto& s = st[(size_t) c];
                float y = x[c] * inG;
                if (preOn) y = s.dc.hp (triode (y, 0.1f + 0.7f * drive, 0.15f));
                const float yIn = y;
                if (! fltSc) y = s.lp.process (s.hp1.hp (s.hp.process (y)));
                // détecteur : avec les filtres si FILTRES -> S/C
                sc[c] = fltSc ? s.scLp.process (s.scHp1.hp (s.scHp.process (yIn))) : y;
                lvl = juce::jmax (lvl, rms.process (sc[c], rmsC) * 1.41f, std::abs (sc[c]) * (cType == 1 ? 0.95f : 0.6f));
                if (eqOn && ! dynPre) y = s.lf.process (s.lm.process (s.hm.process (s.hf.process (y))));
                x[c] = y;
            }
            // dynamique
            float g = 1.0f;
            if (dynOn)
            {
                const float ld = dB (lvl);
                const float target = kneeGr (ld - thr, cType == 3 ? juce::jmin (ratio, 2.0f + juce::jmax (0.0f, ld - thr) * 0.3f) : ratio, knee);
                float gr = cType == 2 ? opto.process (target, fs, fast ? 4.0f : 10.0f, p ("rel") / 500.0f) : env.process (target, atk, rel);
                maxGr = juce::jmax (maxGr, gr);
                const float gc = gain (-gr);
                g = (1.0f - cMix) + gc * cMix;
                if (gateOn)
                {
                    const float under = juce::jmax (0.0f, gThr - ld);
                    const float gTarget = gExp ? juce::jmin (gRange, under * 1.0f) : (under > 0.0f ? gRange : 0.0f);
                    g *= gain (-gEnv.process (gTarget, gRel, gAtk));
                }
            }
            for (int c = 0; c < nProc; ++c)
            {
                auto& s = st[(size_t) c];
                float y = scListen ? sc[c] : x[c] * g;
                if (eqOn && dynPre && ! scListen) y = s.lf.process (s.lm.process (s.hm.process (s.hf.process (y))));
                // grain : réagit au niveau
                if (grainAmt > 0.0f)
                {
                    if (grainT == 0)      y = s.xfCon.process (softClip (y, 0.15f * grainAmt));
                    else if (grainT == 1) y = s.xfWarm.process (triode (y, 0.4f * grainAmt, 0.2f));
                    else                  y = s.xfTape.process ((std::tanh ((1.0f + grainAmt) * y + 0.04f) - std::tanh (0.04f)) / (1.0f + grainAmt));
                }
                x[c] = y;
            }
            // sortie : mono -> pan ; stéréo -> largeur + balance
            float o[2];
            if (mono || numCh < 2) { o[0] = x[0] * panL; o[1] = x[0] * panR; }
            else
            {
                const float mm = 0.5f * (x[0] + x[1]), ss = 0.5f * (x[0] - x[1]) * width;
                o[0] = (mm + ss) * juce::jmin (1.0f, 1.0f - pan); o[1] = (mm - ss) * juce::jmin (1.0f, 1.0f + pan);
            }
            for (int c = 0; c < numCh; ++c)
            {
                float y = (numCh < 2 ? x[0] : o[c]) * outG;
                if (clipOn)
                {
                    const float pre = y * clipGain;
                    const float k = pre / ceil;
                    float yc;
                    if (curve < 0.01f) yc = juce::jlimit (-1.0f, 1.0f, k);
                    else
                    {
                        const float kn = 1.0f - curve * 0.6f;
                        const float a = std::abs (k);
                        yc = a <= kn ? k : (k > 0 ? 1.0f : -1.0f) * (kn + (1.0f - kn) * std::tanh ((a - kn) / (1.0f - kn)));
                    }
                    yc *= ceil;
                    maxClip = juce::jmax (maxClip, dB (std::abs (pre) + 1e-9f) - dB (std::abs (yc) + 1e-9f));
                    y = clipListen ? pre - yc : yc;
                }
                b.getChannelPointer ((size_t) c)[i] = y;
            }
        }
        extra[0].store (maxClip);
        reportGr (0, maxGr); reportGr (1, maxGr);
    }

    juce::AudioProcessorEditor* createEditor() override
    {
        UiSpec u;
        u.w = 1500; u.h = 480;
        const auto txt = juce::Colour (0xffeef0f3), dim = juce::Colour (0xffa6adb6), acc = juce::Colour (0xffffd27a);
        u.theme = { juce::Colour (0xff484e56), juce::Colour (0xff2a2e33), txt, dim, acc };
        u.theme.texture = 2; u.theme.earColour = juce::Colour (0xff3a3f46);
        const auto box = juce::Colour (0x26000000), line = juce::Colour (0x30ffffff);
        u.boxes = { { 46, 30, 164, 440, box, line }, { 218, 30, 112, 440, box, line }, { 338, 30, 362, 440, box, line },
                    { 708, 30, 312, 440, box, line }, { 1028, 30, 200, 440, box, line }, { 1236, 30, 230, 440, box, line } };
        u.texts = {
            { "ENTREE", 46, 36, 164, 14, 11, acc }, { "FILTRES", 218, 36, 112, 14, 11, acc }, { "EQUALIZER", 338, 36, 362, 14, 11, acc },
            { "DYNAMIQUE", 708, 36, 312, 14, 11, acc }, { "GRAIN  /  CLIPPER", 1028, 36, 200, 14, 11, acc }, { "SORTIE", 1236, 36, 230, 14, 11, acc },
            { "GATE / EXPANDEUR", 708, 318, 312, 12, 9, dim },
            { "CLIP", 1036, 438, 40, 12, 9, dim, true, false, juce::Justification::centredLeft },
            { "BAKR CHANNEL", 1250, 446, 200, 18, 13, txt, true, true },
        };
        const auto red = juce::Colour (0xffff4a3d), amber = juce::Colour (0xffffc23a), green = juce::Colour (0xff45d06a), white = juce::Colour (0xfff0f0e8);
        u.controls = {
            { Ctl::Lamp, "power", "", 72, 68, 26, "", {}, {}, {}, red },
            { Ctl::Selector, "mode", "MODE", 128, 140, 44, "black", { "ST", "G+D", "G", "D" } },
            { Ctl::Knob, "inTrim", "IN TRIM", 128, 250, 54, "grey", {}, "-24", "+24" },
            { Ctl::Button, "phase", "PHASE", 92, 325, 50, "", {}, {}, {}, amber },
            { Ctl::Button, "pre", "PRE", 164, 325, 50, "", {}, {}, {}, red },
            { Ctl::Knob, "drive", "DRIVE", 128, 410, 48, "red", {}, "0", "MAX" },

            { Ctl::Knob, "hpf", "HPF", 274, 120, 52, "grey", {}, "OUT", "500" },
            { Ctl::Knob, "lpf", "LPF", 274, 245, 52, "grey", {}, "3k", "OUT" },
            { Ctl::Button, "fltSc", "FILT > S/C", 274, 340, 84, "", {}, {}, {}, amber },

            { Ctl::Knob, "hfG", "HF", 385, 115, 52, "red", {}, "-18", "+18" },
            { Ctl::Knob, "hfF", "FREQ", 385, 215, 42, "red", {}, "1.5k", "16k" },
            { Ctl::Button, "hfBell", "BELL", 385, 300, 50, "", {}, {}, {}, white },
            { Ctl::Knob, "hmG", "HMF", 475, 115, 52, "teal", {}, "-18", "+18" },
            { Ctl::Knob, "hmF", "FREQ", 475, 215, 42, "teal", {}, "600", "7k" },
            { Ctl::Knob, "hmQ", "Q", 475, 310, 38, "teal", {}, ".4", "4" },
            { Ctl::Knob, "lmG", "LMF", 565, 115, 52, "blue", {}, "-18", "+18" },
            { Ctl::Knob, "lmF", "FREQ", 565, 215, 42, "blue", {}, "200", "2.5k" },
            { Ctl::Knob, "lmQ", "Q", 565, 310, 38, "blue", {}, ".4", "4" },
            { Ctl::Knob, "lfG", "LF", 655, 115, 52, "black", {}, "-18", "+18" },
            { Ctl::Knob, "lfF", "FREQ", 655, 215, 42, "black", {}, "30", "450" },
            { Ctl::Button, "lfBell", "BELL", 655, 300, 50, "", {}, {}, {}, white },
            { Ctl::Button, "modern", "MODERNE", 470, 410, 80, "", {}, {}, {}, amber },
            { Ctl::Button, "eqOn", "EQ IN", 590, 410, 70, "", {}, {}, {}, green },

            { Ctl::Button, "dynOn", "DYN IN", 750, 68, 56, "", {}, {}, {}, green },
            { Ctl::Button, "dynPre", "DYN>EQ", 825, 68, 62, "", {}, {}, {}, amber },
            { Ctl::Button, "scListen", "ECOUTE S/C", 925, 68, 88, "", {}, {}, {}, amber },
            { Ctl::Selector, "cType", "", 760, 160, 40, "black", { "VCA", "FET", "OPTO", "TUBE", "CLEAN" } },
            { Ctl::Knob, "ratio", "RATIO", 880, 150, 48, "grey", {}, "1", "INF" },
            { Ctl::Knob, "thr", "SEUIL", 965, 150, 48, "grey", {}, "-50", "0" },
            { Ctl::Knob, "rel", "RELEASE", 760, 265, 42, "grey", {}, "50", "2s" },
            { Ctl::Button, "fast", "FAST", 850, 265, 50, "", {}, {}, {}, red },
            { Ctl::Knob, "cMix", "MIX", 965, 265, 42, "grey", {}, "DRY", "WET" },
            { Ctl::Knob, "gThr", "SEUIL", 750, 380, 40, "teal", {}, "OFF", "0" },
            { Ctl::Knob, "gRange", "RANGE", 820, 380, 40, "teal", {}, "0", "80" },
            { Ctl::Knob, "gRel", "REL", 890, 380, 40, "teal", {}, "20", "2s" },
            { Ctl::Button, "gFast", "FAST", 975, 360, 48, "", {}, {}, {}, red },
            { Ctl::Button, "gExp", "EXP", 975, 410, 48, "", {}, {}, {}, amber },

            { Ctl::Selector, "grain", "GRAIN", 1075, 128, 40, "black", { "CONSOLE", "CHAUD", "BANDE" } },
            { Ctl::Knob, "grainAmt", "DOSAGE", 1180, 128, 46, "chrome", {}, "0", "100" },
            { Ctl::Button, "clipOn", "ON", 1062, 225, 48, "", {}, {}, {}, red },
            { Ctl::Knob, "clipGain", "GAIN", 1128, 232, 40, "black", {}, "0", "+18" },
            { Ctl::Knob, "clipCeil", "PLAFOND", 1192, 232, 40, "black", {}, "-12", "0" },
            { Ctl::Knob, "clipCurve", "COURBE", 1100, 340, 40, "black", {}, "NET", "ROND" },
            { Ctl::Button, "clipListen", "ECOUTE", 1185, 335, 56, "", {}, {}, {}, amber },

            { Ctl::Knob, "width", "LARGEUR", 1285, 205, 44, "grey", {}, "0", "200" },
            { Ctl::Knob, "pan", "PAN", 1285, 300, 44, "grey", {}, "L", "R" },
            { Ctl::Knob, "outTrim", "OUT TRIM", 1285, 395, 42, "grey", {}, "-12", "+12" },
            { Ctl::Button, "mute", "MUTE", 1360, 410, 52, "", {}, {}, {}, red },
            { Ctl::Fader, "fader", "FADER", 1420, 300, 170, "", {}, "-INF", "+12" },
        };
        u.meters = {
            { 1246, 54, 106, 86, Src::InMax, "IN", "cream" },
            { 1356, 54, 106, 86, Src::OutMax, "OUT", "cream" },
            { 730, 296, 270, 12, Src::GrMax, "", "bar" },
            { 1062, 436, 154, 12, Src::Extra0, "", "bar" },
        };
        return new Editor (*this, u);
    }

private:
    struct Ch { Biquad hp, lp, hf, hm, lm, lf, scHp, scLp; OnePole hp1, scHp1, dc; Transformer xfCon, xfWarm, xfTape; };
    std::array<Ch, 2> st;
    Env env, gEnv;
    OptoCell opto;
    Rms rms;
    int numCh = 2;
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Channel(); }
