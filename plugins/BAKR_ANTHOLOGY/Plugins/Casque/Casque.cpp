// BAKR CASQUE — calibration de casque + émulation d'enceintes dans une pièce (pensé pour le beyerdynamic DT 880)
//   CALIBRATION : corrige la courbe du casque (pic « beyer » vers 6 kHz, grave qui manque...).
//   ÉMULATION   : on entend comme devant deux enceintes (angle réglable) dans une pièce (ROOM).
//   SAFE MODE   : coupe tout pendant l'export + limiteur doux pour protéger les oreilles.
// Zéro latence (filtres IIR, le son direct n'est jamais retardé).
#include "../../Common/BakrUI.h"
#include "../Calibre/CalibreDSP.h"

#define T(x) juce::String::fromUTF8 (x)
using namespace calib;

namespace
{
    struct Phone { const char* name; std::vector<std::pair<float, float>> dev; };   // écart à une réponse neutre (dB), estimations

    const std::vector<Phone>& phones()
    {
        // DT 880 : grave qui manque, un peu trop de haut-grave / bas-médium, pic « beyer » vers 6 kHz
        static const std::vector<std::pair<float, float>> dt880 { { 20, -5.0f }, { 35, -3.5f }, { 60, -1.5f }, { 120, 0.5f }, { 250, 1.5f }, { 500, 0.5f }, { 1000, 0 }, { 2000, -0.5f }, { 3200, -1.5f }, { 4800, 2.0f }, { 5900, 7.0f }, { 7000, 2.5f }, { 8500, 0.5f }, { 10500, 2.5f }, { 13000, -1.0f }, { 16000, -3.0f }, { 20000, -6.0f } };
        static const std::vector<Phone> list {
            { "DT 880 (32 ohms)",          dt880 },
            { "DT 880 (250 ohms)",         dt880 },
            { "DT 880 (600 ohms)",         dt880 },
            { "DT 990 PRO (250 ohms)",     { { 20, -2.0f }, { 50, 1.0f }, { 100, 2.0f }, { 250, 0.5f }, { 1000, 0 }, { 3000, -1.5f }, { 5000, 2.0f }, { 6500, 5.0f }, { 8500, 8.0f }, { 10500, 4.0f }, { 13000, 1.0f }, { 16000, -2.0f }, { 20000, -6.0f } } },
            { "DT 770 PRO (80 / 250 ohms)", { { 20, 1.0f }, { 50, 3.0f }, { 100, 3.0f }, { 250, 1.0f }, { 600, -0.5f }, { 1000, 0 }, { 3000, -2.0f }, { 5000, 1.0f }, { 6000, 3.0f }, { 8500, 7.0f }, { 10500, 3.0f }, { 14000, 0.0f }, { 20000, -5.0f } } },
            { "Aucune correction (plat)",  { } },
        };
        return list;
    }
    juce::StringArray phoneNames() { juce::StringArray s; for (auto& p : phones()) s.add (p.name); return s; }

    constexpr int kCalBands = 9;
}

//==============================================================================
class Casque : public juce::AudioProcessor, private juce::AsyncUpdater, private juce::AudioProcessorValueTreeState::Listener
{
public:
    Casque()
        : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
          apvts (*this, nullptr, "BAKR_CASQUE", layout())
    {
        for (auto* id : { "model", "calAmt" }) apvts.addParameterListener (id, this);
        triggerAsyncUpdate();
    }
    ~Casque() override { for (auto* id : { "model", "calAmt" }) apvts.removeParameterListener (id, this); cancelPendingUpdate(); }

    static juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        using namespace bakr;
        Layout l;
        addChoice (l, "model", "Casque", phoneNames(), 1);
        addBool (l, "calOn", "Calibration", true);
        addFloat (l, "calAmt", "Force de la calibration", 0, 100, 100, "%", 0, 0);
        addBool (l, "emuOn", "Emulation enceintes", true);
        addFloat (l, "angle", "Angle des enceintes", 30, 120, 60, "deg", 0, 0);
        addFloat (l, "room", "Piece", 0, 100, 25, "%", 0, 0);
        addBool (l, "safe", "Safe mode", true);
        addFloat (l, "out", "Volume", -24, 6, 0, "dB");
        addBool (l, "bypass", "Bypass", false);
        return l;
    }
    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    const juce::String getName() const override { return "BAKR CASQUE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
    }

    void prepareToPlay (double sampleRate, int) override
    {
        fs = sampleRate;
        for (auto& c : ch) c.reset();
        for (auto& d : itd) { d.assign (512, 0.0f); }
        itdW = 0;
        erBuf.assign (8192, 0.0f); erW = 0;
        for (auto& f : fdn) { f.buf.assign (8192, 0.0f); f.w = 0; f.lp = 0; }
        calMix.reset (sampleRate, 0.05); emuMix.reset (sampleRate, 0.05); dry.reset (sampleRate, 0.05);
        calMix.setCurrentAndTargetValue (0); emuMix.setCurrentAndTargetValue (0); dry.setCurrentAndTargetValue (0);
        lastAngle = -1;
        triggerAsyncUpdate();
    }
    void releaseResources() override {}

    //--------------------------------------------------------------------------
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals nd;
        const int n = buffer.getNumSamples();
        if (n <= 0 || buffer.getNumChannels() < 2) return;

        if (calDirty.load())
        {
            const juce::SpinLock::ScopedTryLockType tl (calSpin);
            if (tl.isLocked()) { calAudio = calPending; calDirty.store (false); for (auto& c : ch) for (int b = 0; b < kCalBands; ++b) c.cal[(size_t) b].k = calAudio.k[b]; }
        }
        const float ang = p ("angle");
        if (std::abs (ang - lastAngle) > 0.01f) { lastAngle = ang; setupEmulation (ang); }

        const bool exporting = isNonRealtime() && p ("safe") > 0.5f;
        const bool off = p ("bypass") > 0.5f || exporting;
        dry.setTargetValue (off ? 1.0f : 0.0f);
        calMix.setTargetValue (p ("calOn") > 0.5f ? p ("calAmt") > 0 ? 1.0f : 0.0f : 0.0f);
        emuMix.setTargetValue (p ("emuOn") > 0.5f ? 1.0f : 0.0f);
        const float room = p ("room") * 0.01f;
        const float outG = juce::Decibels::decibelsToGain (p ("out")) * calAudio.preamp;
        const bool safe = p ("safe") > 0.5f;

        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);
        float pk = 0, emuLvl = 0;
        for (int i = 0; i < n; ++i)
        {
            const float xl = L[i], xr = R[i];
            const float cm = calMix.getNextValue(), em = emuMix.getNextValue(), dm = dry.getNextValue();

            // 1. émulation d'enceintes (crossfeed type tête + pinna)
            double l = xl, r = xr;
            if (em > 1.0e-4f)
            {
                itd[0][(size_t) itdW] = xl; itd[1][(size_t) itdW] = xr;
                const float dl = readFrac (itd[0], itdSamples), dr = readFrac (itd[1], itdSamples);
                itdW = (itdW + 1) & 511;
                // côté opposé : retardé (ITD), plus faible et assombri par l'ombre de la tête
                const double cL = ch[0].shadow2.process (ch[0].shadow.process (dr)) * contraGain;
                const double cR = ch[1].shadow2.process (ch[1].shadow.process (dl)) * contraGain;
                double el = ch[0].pinna.process (xl) + cL, er = ch[1].pinna.process (xr) + cR;
                el *= normGain; er *= normGain;

                // pièce : premières réflexions + petite réverbération
                if (room > 0.001f)
                {
                    const float mono = (float) (el + er) * 0.5f;
                    erBuf[(size_t) erW] = mono;
                    double rl = 0, rr = 0;
                    static const int tapsMs10[6] = { 73, 113, 157, 199, 251, 311 };
                    static const float tapG[6] = { 0.55f, 0.48f, 0.40f, 0.34f, 0.28f, 0.22f };
                    for (int t = 0; t < 6; ++t)
                    {
                        const int d = (int) (tapsMs10[t] * 0.0001 * fs);
                        const float v = erBuf[(size_t) ((erW - d) & 8191)] * tapG[t];
                        if (t & 1) rl += v; else rr += v;
                    }
                    erW = (erW + 1) & 8191;
                    // FDN 4 lignes, amortie (pièce de mixage traitée, RT ~0.3 s)
                    float o[4];
                    for (int k = 0; k < 4; ++k) o[k] = fdn[(size_t) k].buf[(size_t) ((fdn[(size_t) k].w - fdn[(size_t) k].len) & 8191)];
                    const float h = 0.5f;
                    const float m0 = h * ( o[0] + o[1] + o[2] + o[3]), m1 = h * ( o[0] - o[1] + o[2] - o[3]),
                                m2 = h * ( o[0] + o[1] - o[2] - o[3]), m3 = h * ( o[0] - o[1] - o[2] + o[3]);
                    const float mm[4] = { m0, m1, m2, m3 };
                    for (int k = 0; k < 4; ++k)
                    {
                        auto& f = fdn[(size_t) k];
                        f.lp += 0.45f * (mm[k] * f.g - f.lp);
                        f.buf[(size_t) f.w] = f.lp + (float) (k < 2 ? rl : rr) * 0.35f;
                        f.w = (f.w + 1) & 8191;
                    }
                    const double wl = ch[0].roomLp.process (rl * 0.6 + o[0] + o[2] * 0.5);
                    const double wr = ch[1].roomLp.process (rr * 0.6 + o[1] + o[3] * 0.5);
                    el = el * (1.0 - room * 0.25) + wl * room * 0.55;
                    er = er * (1.0 - room * 0.25) + wr * room * 0.55;
                }
                l = xl + (el - xl) * em; r = xr + (er - xr) * em;
                emuLvl = juce::jmax (emuLvl, (float) std::abs (cL));
            }

            // 2. calibration du casque
            if (cm > 1.0e-4f)
            {
                double cl = l, cr = r;
                for (int b = 0; b < calAudio.nb; ++b) { cl = ch[0].cal[(size_t) b].process (cl); cr = ch[1].cal[(size_t) b].process (cr); }
                l += (cl - l) * cm; r += (cr - r) * cm;
            }

            float ol = (float) l * outG, orr = (float) r * outG;
            if (safe) { ol = soft (ol); orr = soft (orr); }
            L[i] = ol + (xl - ol) * dm;
            R[i] = orr + (xr - orr) * dm;
            pk = juce::jmax (pk, std::abs (L[i]), std::abs (R[i]));
        }
        outPeak.store (juce::jmax (pk, outPeak.load()));
        crossLevel.store (juce::jmax (emuLvl, crossLevel.load()));
        isExporting.store (exporting);
    }

    // courbes pour l'affichage
    struct View { Curve original, corr, result; };
    View getView() const { const juce::ScopedLock sl (viewLock); return view; }
    int viewVersion() const { return viewVer.load(); }

    std::atomic<float> outPeak { 0 }, crossLevel { 0 };
    std::atomic<bool> isExporting { false };
    double fs = 48000.0;
    juce::AudioProcessorValueTreeState apvts;

    void getStateInformation (juce::MemoryBlock& d) override { if (auto x = apvts.copyState().createXml()) copyXmlToBinary (*x, d); }
    void setStateInformation (const void* data, int size) override
    {
        if (auto x = getXmlFromBinary (data, size)) apvts.replaceState (juce::ValueTree::fromXml (*x));
        triggerAsyncUpdate();
    }

private:
    static float soft (float x)
    {
        const float lim = 0.891f;   // -1 dBFS
        const float a = std::abs (x);
        return a <= lim ? x : std::copysign (lim + (1.0f - lim) * std::tanh ((a - lim) / (1.0f - lim)), x);
    }
    float readFrac (const std::vector<float>& b, float d) const
    {
        const int di = (int) d; const float fr = d - di;
        const float a = b[(size_t) ((itdW - di) & 511)], c = b[(size_t) ((itdW - di - 1) & 511)];
        return a + (c - a) * fr;
    }

    struct BQ
    {
        Coefs k; double z1 = 0, z2 = 0;
        inline double process (double x) noexcept { const double y = k.b0 * x + z1; z1 = k.b1 * x - k.a1 * y + z2; z2 = k.b2 * x - k.a2 * y; return y; }
    };
    struct Chan
    {
        std::array<BQ, kCalBands> cal;
        BQ shadow, shadow2, pinna, roomLp;
        void reset() { for (auto& b : cal) b.z1 = b.z2 = 0; for (auto* b : { &shadow, &shadow2, &pinna, &roomLp }) b->z1 = b->z2 = 0; }
    };
    struct CalSet { Coefs k[kCalBands]; int nb = 0; float preamp = 1.0f; };
    struct Line { std::vector<float> buf; int w = 0, len = 1000; float g = 0.7f, lp = 0; };

    static Coefs shelfCoefs (double fs, double f, double g, bool high)
    {
        Coefs c; const double A = std::pow (10.0, g / 40.0), w = 2 * kPiD * f / fs, cs = std::cos (w), sn = std::sin (w), al = sn / 2 * std::sqrt (2.0);
        const double sA = 2 * std::sqrt (A) * al;
        if (high)
        {
            const double a0 = (A + 1) - (A - 1) * cs + sA;
            c.b0 = A * ((A + 1) + (A - 1) * cs + sA) / a0; c.b1 = -2 * A * ((A - 1) + (A + 1) * cs) / a0; c.b2 = A * ((A + 1) + (A - 1) * cs - sA) / a0;
            c.a1 = 2 * ((A - 1) - (A + 1) * cs) / a0; c.a2 = ((A + 1) - (A - 1) * cs - sA) / a0;
        }
        else
        {
            const double a0 = (A + 1) + (A - 1) * cs + sA;
            c.b0 = A * ((A + 1) - (A - 1) * cs + sA) / a0; c.b1 = 2 * A * ((A - 1) - (A + 1) * cs) / a0; c.b2 = A * ((A + 1) - (A - 1) * cs - sA) / a0;
            c.a1 = -2 * ((A - 1) + (A + 1) * cs) / a0; c.a2 = ((A + 1) + (A - 1) * cs - sA) / a0;
        }
        return c;
    }
    static Coefs lpCoefs (double fs, double f, double q)
    {
        Coefs c; const double w = 2 * kPiD * f / fs, cs = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
        c.b0 = (1 - cs) / 2 / a0; c.b1 = (1 - cs) / a0; c.b2 = (1 - cs) / 2 / a0; c.a1 = -2 * cs / a0; c.a2 = (1 - al) / a0; return c;
    }

    // modèle de tête : enceintes à ±angle/2
    void setupEmulation (float angleDeg)
    {
        const double az = juce::degreesToRadians (angleDeg * 0.5), a = 0.0875, c = 343.0;
        itdSamples = (float) juce::jlimit (0.0, 500.0, a / c * (az + std::sin (az)) * fs);    // Woodworth
        const double s = std::sin (az);
        contraGain = (float) juce::Decibels::decibelsToGain (-2.5 - 3.5 * s);                     // niveau du côté opposé
        const double shadowDb = -(4.0 + 10.0 * s);                                                // ombre de la tête dans les aigus
        for (auto& cc : ch)
        {
            cc.shadow.k = shelfCoefs (fs, 900.0 + 700.0 * (1.0 - s), shadowDb, true);
            cc.shadow2.k = lpCoefs (fs, juce::jmin (fs * 0.45, 9000.0 - 3000.0 * s), 0.6);
            cc.pinna.k = peakCoefs (fs, 3000.0, 1.2, 1.0 + 1.5 * s);                               // petite présence d'oreille
            cc.roomLp.k = lpCoefs (fs, juce::jmin (fs * 0.45, 6500.0), 0.6);
        }
        normGain = (float) (1.0 / (1.0 + contraGain * 0.75));                                     // garde le même volume
        const int lens[4] = { 1031, 1327, 1523, 1871 };
        for (int k = 0; k < 4; ++k)
        {
            fdn[(size_t) k].len = (int) (lens[k] * fs / 48000.0);
            fdn[(size_t) k].g = (float) std::pow (10.0, -3.0 * fdn[(size_t) k].len / (0.32 * fs));   // RT60 ≈ 0.32 s
        }
    }

    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override
    {
        const int mi = juce::jlimit (0, (int) phones().size() - 1, (int) p ("model"));
        const auto& ph = phones()[(size_t) mi];
        const float amt = p ("calAmt") * 0.01f;
        Curve dev ((size_t) kGrid, 0.0f), zero ((size_t) kGrid, 0.0f);
        for (int i = 0; i < kGrid; ++i) dev[(size_t) i] = ph.dev.empty() ? 0.0f : interpPoints (ph.dev, gridFreq (i));
        FitParams fp; fp.lo = 20; fp.hi = 18000; fp.maxBoost = 6; fp.maxCut = 10; fp.amount = amt; fp.maxBands = kCalBands;
        const auto bands = ph.dev.empty() ? std::vector<Band>() : fitBands (dev, zero, fp, fs);
        CalSet cs;
        cs.nb = (int) bands.size();
        for (int b = 0; b < cs.nb; ++b) cs.k[b] = peakCoefs (fs, bands[(size_t) b].f, bands[(size_t) b].q, bands[(size_t) b].g);
        View v; v.original = dev; v.corr = bandsCurve (bands, fs); v.result.resize ((size_t) kGrid);
        float mx = 0;
        for (int i = 0; i < kGrid; ++i) { v.result[(size_t) i] = dev[(size_t) i] + v.corr[(size_t) i]; if (gridFreq (i) >= 20 && gridFreq (i) <= 20000) mx = juce::jmax (mx, v.corr[(size_t) i]); }
        cs.preamp = juce::Decibels::decibelsToGain (-mx * 0.5f);   // le reste est tenu par le limiteur doux (SAFE MODE)
        { const juce::SpinLock::ScopedLockType sl (calSpin); calPending = cs; calDirty.store (true); }
        { const juce::ScopedLock sl (viewLock); view = v; }
        viewVer.fetch_add (1);
    }

    std::array<Chan, 2> ch;
    std::array<std::vector<float>, 2> itd;
    int itdW = 0;
    float itdSamples = 10, contraGain = 0.4f, normGain = 0.8f, lastAngle = -1;
    std::vector<float> erBuf; int erW = 0;
    std::array<Line, 4> fdn;
    juce::LinearSmoothedValue<float> calMix, emuMix, dry;
    juce::SpinLock calSpin;
    CalSet calPending, calAudio;
    std::atomic<bool> calDirty { false };
    mutable juce::CriticalSection viewLock;
    View view;
    std::atomic<int> viewVer { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Casque)
};

//==============================================================================
namespace cu
{
    const juce::Colour bg (0xff1b1d21), panel (0xff24272c), dark (0xff111214), txt (0xffe9ebee), dim (0xff8c929b),
                       orange (0xffff9a2e), pink (0xffff6fa3), cyan (0xff5fd2ff), line (0x22ffffff);
    inline void text (juce::Graphics& g, const juce::String& s, juce::Rectangle<float> r, float size, juce::Colour c, bool bold = false, juce::Justification j = juce::Justification::centred)
    {
        g.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
        g.setColour (c); g.drawFittedText (s, r.toNearestInt(), j, 2, 0.9f);
    }
    inline void disc (juce::Graphics& g, juce::Point<float> c, float r)
    {
        g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillEllipse (juce::Rectangle<float> (r * 2 + 16, r * 2 + 16).withCentre (c.translated (0, 4)));
        juce::ColourGradient gr (juce::Colour (0xff3a3e45), c.x, c.y - r, juce::Colour (0xff15171a), c.x, c.y + r, false);
        g.setGradientFill (gr); g.fillEllipse (juce::Rectangle<float> (r * 2 + 12, r * 2 + 12).withCentre (c));
        g.setColour (dark); g.fillEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c));
        g.setColour (line); g.drawEllipse (juce::Rectangle<float> (r * 2, r * 2).withCentre (c), 1.0f);
    }
}

class PowerButton : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (2);
        const bool on = getToggleState();
        g.setColour (juce::Colour (0xff2b2e33)); g.fillEllipse (r);
        g.setColour (on ? cu::orange : juce::Colour (0xff4a4e55)); g.drawEllipse (r.reduced (1), 2.0f);
        auto c = r.getCentre(); const float rad = r.getWidth() * 0.22f;
        juce::Path pth; pth.addCentredArc (c.x, c.y, rad, rad, 0, 0.7f, juce::MathConstants<float>::twoPi - 0.7f, true);
        g.setColour (on ? cu::orange : (over ? cu::txt : cu::dim));
        g.strokePath (pth, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (c.x, c.y - rad - 2, c.x, c.y - 1, 2.0f);
    }
};

class SwitchButton : public juce::ToggleButton
{
public:
    void paintButton (juce::Graphics& g, bool, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (2);
        const bool on = getToggleState();
        g.setColour (on ? cu::orange.withAlpha (0.85f) : juce::Colour (0xff3a3d43)); g.fillRoundedRectangle (r, r.getHeight() / 2);
        const float d = r.getHeight() - 4;
        g.setColour (cu::txt); g.fillEllipse (on ? r.getRight() - d - 2 : r.getX() + 2, r.getY() + 2, d, d);
    }
};

class CasqueContent : public juce::Component, private juce::Timer
{
public:
    static constexpr int W = 980, H = 640;
    explicit CasqueContent (Casque& p) : proc (p)
    {
        setLookAndFeel (&lnf);
        auto bAtt = [this] (juce::Button& b, const char* id) { addAndMakeVisible (b); batts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, b)); };
        bAtt (calOn, "calOn"); bAtt (emuOn, "emuOn"); bAtt (safe, "safe");
        bypass.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff2b2e33));
        bypass.setColour (juce::TextButton::buttonOnColourId, cu::orange);
        bypass.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        bypass.setClickingTogglesState (true);
        bAtt (bypass, "bypass");

        model.addItemList (phoneNames(), 1);
        addAndMakeVisible (model);
        modelAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "model", model);

        auto knob = [this] (juce::Slider& s, const char* id, const char* look)
        {
            s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            s.setRotaryParameters (-0.75f * juce::MathConstants<float>::pi, 0.75f * juce::MathConstants<float>::pi, true);
            s.getProperties().set ("look", look); s.getProperties().set ("tick", cu::txt.toString());
            s.setPopupDisplayEnabled (true, true, this);
            addAndMakeVisible (s);
            satts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, s));
        };
        knob (room, "room", "black"); knob (calAmt, "calAmt", "grey"); knob (vol, "out", "chrome");

        for (auto* b : { &angDown, &angUp })
        {
            b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            b->setColour (juce::TextButton::textColourOffId, cu::txt);
            addAndMakeVisible (*b);
        }
        angDown.onClick = [this] { stepAngle (-5); };
        angUp.onClick = [this] { stepAngle (5); };

        calOn.setBounds (318, 360, 44, 44);
        emuOn.setBounds (868, 360, 44, 44);
        safe.setBounds (464, 330, 52, 24);
        bypass.setBounds (820, 20, 130, 28);
        model.setBounds (90, 470, 260, 28);
        calAmt.setBounds (190, 540, 60, 60);
        room.setBounds (650, 478, 76, 76);
        vol.setBounds (455, 520, 70, 70);
        angDown.setBounds (680, 268, 30, 30); angUp.setBounds (790, 268, 30, 30);
        startTimerHz (30);
    }
    ~CasqueContent() override { setLookAndFeel (nullptr); }

    void paint (juce::Graphics& g) override
    {
        using namespace cu;
        g.fillAll (bg);
        { juce::ColourGradient gr (juce::Colour (0xff2a2d33), 0, 0, bg, 0, (float) H * 0.6f, false); g.setGradientFill (gr); g.fillRect (getLocalBounds()); }
        text (g, "BAKR", { 24, 18, 80, 30 }, 22, txt, true, juce::Justification::centredLeft);
        text (g, "CASQUE", { 96, 22, 140, 24 }, 16, orange, true, juce::Justification::centredLeft);
        text (g, T("calibration + émulation d'enceintes"), { 330, 20, 320, 26 }, 13, dim);

        // --- disque calibration
        const juce::Point<float> cL (220, 230), cR (760, 230);
        const float R = 160;
        disc (g, cL, R);
        auto plot = juce::Rectangle<float> (cL.x - 120, cL.y - 80, 240, 130);
        g.setColour (line);
        for (double f : { 100.0, 1000.0, 10000.0 }) { const float x = xOf (plot, f); g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom()); }
        g.drawHorizontalLine ((int) yOf (plot, 0), plot.getX(), plot.getRight());
        drawCurve (g, plot, v.original, juce::Colours::white.withAlpha (0.45f), 1.4f, true);
        if (proc.p ("calOn") > 0.5f)
        {
            drawCurve (g, plot, v.corr, pink, 1.8f, false);
            drawCurve (g, plot, v.result, cyan, 2.0f, false);
        }
        text (g, "Original", { cL.x - 60, cL.y + 56, 120, 14 }, 11, dim);
        g.setColour (pink); g.fillEllipse (cL.x - 62, cL.y + 78, 6, 6);
        text (g, "Correction", { cL.x - 52, cL.y + 72, 80, 16 }, 11, txt, false, juce::Justification::centredLeft);
        g.setColour (cyan); g.fillEllipse (cL.x - 62, cL.y + 96, 6, 6);
        text (g, T("Résultat"), { cL.x - 52, cL.y + 90, 80, 16 }, 11, txt, false, juce::Justification::centredLeft);
        text (g, "HEADPHONE CALIBRATION", { 60, 430, 320, 20 }, 15, txt, true);
        text (g, "FORCE", { 250, 562, 80, 16 }, 11, dim, true, juce::Justification::centredLeft);
        text (g, proc.apvts.getParameter ("calAmt")->getCurrentValueAsText(), { 250, 578, 80, 16 }, 11, txt, false, juce::Justification::centredLeft);

        // --- disque émulation
        disc (g, cR, R);
        const float ang = proc.p ("angle"), half = juce::degreesToRadians (ang * 0.5f);
        const bool emu = proc.p ("emuOn") > 0.5f;
        for (int s = -1; s <= 1; s += 2)
        {
            const float a = s * half;
            juce::Path wedge;
            const float r1 = 105, r2 = 140, w = 0.28f;
            wedge.addCentredArc (cR.x, cR.y, r2, r2, 0, a - w, a + w, true);
            wedge.addCentredArc (cR.x, cR.y, r1, r1, 0, a + w, a - w, false);
            wedge.closeSubPath();
            juce::ColourGradient gr (orange.withAlpha (emu ? 0.95f : 0.25f), cR.x, cR.y - r2, orange.withAlpha (emu ? 0.35f : 0.08f), cR.x, cR.y - r1, false);
            g.setGradientFill (gr); g.fillPath (wedge);
        }
        // tête
        g.setColour (txt.withAlpha (0.85f));
        g.drawEllipse (cR.x - 14, cR.y - 52, 28, 32, 2.0f);
        g.drawLine (cR.x - 20, cR.y - 12, cR.x - 24, cR.y + 2, 2.0f); g.drawLine (cR.x + 20, cR.y - 12, cR.x + 24, cR.y + 2, 2.0f);
        { juce::Path arc; arc.addCentredArc (cR.x, cR.y - 36, 24, 22, 0, -1.6f, 1.6f, true); g.strokePath (arc, juce::PathStrokeType (2.0f)); }
        text (g, juce::String (juce::roundToInt (ang)) + T("°"), { cR.x - 40, cR.y + 22, 80, 40 }, 24, txt, true);
        text (g, "LOUDSPEAKER ANGLE", { cR.x - 100, cR.y + 62, 200, 16 }, 11, dim);
        // vu crossfeed
        g.setColour (orange.withAlpha (0.25f + 0.75f * juce::jlimit (0.0f, 1.0f, xfeed * 3.0f)));
        g.fillEllipse (cR.x - 3, cR.y + 88, 6, 6);
        text (g, "LOUDSPEAKER EMULATION", { 600, 430, 320, 20 }, 15, txt, true);
        text (g, "ROOM", { 638, 458, 100, 16 }, 11, dim, true);
        text (g, "DRY", { 620, 556, 40, 14 }, 10, dim); text (g, "WET", { 716, 556, 40, 14 }, 10, dim);

        // --- centre
        text (g, "SAFE MODE", { 420, 306, 140, 16 }, 11, dim, true);
        text (g, T("coupe à l'export + limite à -1 dB"), { 400, 358, 180, 14 }, 9.5f, dim);
        text (g, "VOLUME", { 440, 498, 100, 16 }, 11, dim, true);
        text (g, proc.apvts.getParameter ("out")->getCurrentValueAsText(), { 440, 594, 100, 14 }, 10.5f, txt);
        if (proc.isExporting.load()) text (g, T("EXPORT : correction coupée"), { 380, 610, 220, 16 }, 11, orange, true);
        text (g, T("Mets BAKR CASQUE en DERNIER sur le Master, seulement pour écouter au casque."), { 20, 612, 360, 20 }, 10, dim, false, juce::Justification::centredLeft);
    }

private:
    static float xOf (juce::Rectangle<float> r, double f) { return r.getX() + r.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); }
    static float yOf (juce::Rectangle<float> r, double db) { return r.getCentreY() - (float) db / 12.0f * r.getHeight() * 0.5f; }
    void drawCurve (juce::Graphics& g, juce::Rectangle<float> r, const Curve& c, juce::Colour col, float w, bool dotted)
    {
        if ((int) c.size() != kGrid) return;
        juce::Path pth; bool st = false;
        for (int i = 0; i < kGrid; ++i)
        {
            const double f = gridFreq (i);
            if (f < 20 || f > 20000) continue;
            const float x = xOf (r, f), y = juce::jlimit (r.getY(), r.getBottom(), yOf (r, c[(size_t) i]));
            if (! st) { pth.startNewSubPath (x, y); st = true; } else pth.lineTo (x, y);
        }
        g.setColour (col);
        if (dotted) { juce::Path d; const float dl[] = { 2.0f, 3.0f }; juce::PathStrokeType (w).createDashedStroke (d, pth, dl, 2); g.fillPath (d); }
        else g.strokePath (pth, juce::PathStrokeType (w, juce::PathStrokeType::curved));
    }
    void stepAngle (int d)
    {
        if (auto* prm = proc.apvts.getParameter ("angle"))
        {
            const float v = juce::jlimit (30.0f, 120.0f, proc.p ("angle") + (float) d);
            prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->convertTo0to1 (v)); prm->endChangeGesture();
        }
    }
    void timerCallback() override
    {
        const int vv = proc.viewVersion();
        if (vv != lastV) { lastV = vv; v = proc.getView(); }
        xfeed = juce::jmax (proc.crossLevel.exchange (0.0f), xfeed * 0.85f);
        repaint();
    }

    Casque& proc;
    bakr::LookAndFeel lnf;
    PowerButton calOn, emuOn;
    SwitchButton safe;
    juce::TextButton bypass { "BYPASS" }, angDown { juce::String::fromUTF8 ("\xe2\x97\x80") }, angUp { juce::String::fromUTF8 ("\xe2\x96\xb6") };
    juce::ComboBox model;
    juce::Slider room, calAmt, vol;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAtt;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> batts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> satts;
    Casque::View v;
    int lastV = -1;
    float xfeed = 0;
};

class CasqueEditor : public juce::AudioProcessorEditor
{
public:
    explicit CasqueEditor (Casque& p) : AudioProcessorEditor (&p), content (p)
    {
        addAndMakeVisible (content);
        content.setBounds (0, 0, CasqueContent::W, CasqueContent::H);
        setResizable (true, true);
        setResizeLimits (CasqueContent::W / 2, CasqueContent::H / 2, CasqueContent::W * 2, CasqueContent::H * 2);
        if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) CasqueContent::W / CasqueContent::H);
        setSize (CasqueContent::W, CasqueContent::H);
    }
    void resized() override { content.setTransform (juce::AffineTransform::scale ((float) getWidth() / CasqueContent::W)); }
private:
    CasqueContent content;
};

juce::AudioProcessorEditor* Casque::createEditor() { return new CasqueEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Casque(); }
