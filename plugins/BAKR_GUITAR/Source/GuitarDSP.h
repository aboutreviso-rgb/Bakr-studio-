#pragma once
// BAKR GUITAR - moteur de guitare par modélisation physique (corde de Karplus-Strong étendue)
// Aucun échantillon : tout est synthétisé, donc léger et sans "Banque" à charger.
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <array>
#include <vector>

namespace bg
{
constexpr float kPi = 3.14159265358979f;

enum Guitar { Acier = 0, Nylon, DoubleBobinage, SimpleBobinage };
enum Artic  { Sustain = 0, PalmMute, Harmoniques, HammerOn, PullOff, Slide, Vibrato, SlideUp, SlideDown };

inline float midiToHz (float note) { return 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f); }

// Générateur de bruit rapide
struct Rng
{
    uint32_t s = 0x12345678u;
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (int32_t) s * (1.0f / 2147483648.0f); }
};

// Biquad simple (forme directe transposée II)
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    inline float process (float x)
    {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void set (double nb0, double nb1, double nb2, double na0, double na1, double na2)
    {
        b0 = (float) (nb0 / na0); b1 = (float) (nb1 / na0); b2 = (float) (nb2 / na0);
        a1 = (float) (na1 / na0); a2 = (float) (na2 / na0);
    }
    void peak (double sr, double f, double q, double db)
    {
        double A = std::pow (10.0, db / 40.0), w = 2 * juce::MathConstants<double>::pi * f / sr;
        double al = std::sin (w) / (2 * q), c = std::cos (w);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowShelf (double sr, double f, double db)
    {
        double A = std::pow (10.0, db / 40.0), w = 2 * juce::MathConstants<double>::pi * f / sr;
        double c = std::cos (w), al = std::sin (w) / 2 * std::sqrt (2.0), sA = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + sA), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sA),
             (A + 1) + (A - 1) * c + sA, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sA);
    }
    void highShelf (double sr, double f, double db)
    {
        double A = std::pow (10.0, db / 40.0), w = 2 * juce::MathConstants<double>::pi * f / sr;
        double c = std::cos (w), al = std::sin (w) / 2 * std::sqrt (2.0), sA = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sA), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sA),
             (A + 1) - (A - 1) * c + sA, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sA);
    }
    void lowPass (double sr, double f, double q)
    {
        double w = 2 * juce::MathConstants<double>::pi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highPass (double sr, double f, double q)
    {
        double w = 2 * juce::MathConstants<double>::pi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void bandPass (double sr, double f, double q)
    {
        double w = 2 * juce::MathConstants<double>::pi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        set (al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
};

// Caractère de chaque guitare de la bibliothèque
struct GuitarModel
{
    float t60Low, t60High;     // durée de résonance (s) en grave / aigu
    float loopBright;          // brillance de la boucle de corde (0..1)
    float pickPos;             // position du médiator (fraction de corde)
    float pickupPos;           // position du micro (électrique) ou 0
    float exciteBright;        // dureté du médiator
    float level;
};

inline GuitarModel getModel (int g)
{
    switch (g)
    {
        case Nylon:          return { 3.2f, 1.2f, 0.35f, 0.18f, 0.0f,  0.35f, 1.15f };
        case DoubleBobinage: return { 7.0f, 2.8f, 0.70f, 0.12f, 0.22f, 0.70f, 0.95f };
        case SimpleBobinage: return { 6.0f, 2.4f, 0.82f, 0.10f, 0.10f, 0.80f, 0.95f };
        default:             return { 5.0f, 1.8f, 0.78f, 0.14f, 0.0f,  0.75f, 1.0f };
    }
}

//==============================================================================
// Une corde de guitare
class StringVoice
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        int p = 1; while (p < (int) (sr / 20.0f) + 8) p <<= 1;
        buf.assign ((size_t) p, 0.0f); mask = p - 1;
        reset();
    }

    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        lpState = 0; dcX = dcY = 0; env = 0; active = false; exciteLeft = 0; held = false; note = -1;
    }

    // Lance une note. velocity 0..1
    void pluck (int midiNote, float velocity, int guitar, int artic, float bright, float attackMs,
                float startOffsetSemis, float glideMs, bool keepEnergy, float exciteGain)
    {
        model = getModel (guitar);
        note = midiNote; held = true; articulation = artic; guitarType = guitar;
        targetPitch = (float) midiNote + (artic == Harmoniques ? 12.0f : 0.0f);
        curPitch = targetPitch + startOffsetSemis;
        setGlide (glideMs);
        fallTimer = (artic == SlideDown) ? (int) (0.14f * sr) : -1;

        brightness = bright;
        float b = juce::jlimit (0.0f, 1.0f, model.loopBright * (0.55f + 0.9f * bright));
        if (artic == PalmMute)     b *= 0.35f;
        if (artic == Harmoniques)  b = juce::jmin (1.0f, b + 0.2f);
        loopA = 0.62f - 0.58f * b;                       // coefficient du filtre de boucle

        float hz = midiToHz (targetPitch);
        float t60 = juce::jmap (juce::jlimit (0.0f, 1.0f, (targetPitch - 40.0f) / 48.0f), model.t60Low, model.t60High);
        if (artic == PalmMute)    t60 = 0.22f + 0.0015f * (90.0f - targetPitch);
        if (artic == Harmoniques) t60 *= 1.4f;
        t60Base = t60;
        fbGain = std::pow (10.0f, -3.0f / (hz * t60));

        if (! keepEnergy)
            for (auto& s : buf) s *= 0.25f;              // la corde est re-frappée : l'ancienne vibration s'étouffe

        // Excitation : bruit filtré + profil du médiator
        float period = sr / hz;
        exciteLen = juce::jlimit (8, 4000, (int) period);
        exciteLeft = exciteLen;
        float hard = juce::jlimit (0.02f, 0.6f, 0.6f * model.exciteBright * (0.4f + 0.9f * bright) * (0.55f + 0.6f * velocity));
        if (artic == Harmoniques) hard *= 0.35f;
        if (artic == PalmMute)    hard *= 0.6f;
        hard *= 1.0f / (1.0f + attackMs * 0.04f);       // attaque lente = médiator plus doux
        exLpA = 1.0f - hard;
        exLp = exLp0 = 0;
        pickComb.assign ((size_t) juce::jmax (1, (int) (period * model.pickPos)), 0.0f);
        pickIdx = 0;
        exAmp = (0.35f + 0.65f * velocity) * exciteGain;

        attackInc = attackMs <= 0.05f ? 1.0f : 1.0f / (attackMs * 0.001f * sr);
        if (! active || ! keepEnergy) env = (attackMs <= 0.05f ? 1.0f : 0.0f);
        releaseCoef = 1.0f; relEnv = 1.0f;
        active = true;
        silentCount = 0;
        gain = model.level;
    }

    // Legato (hammer-on, pull-off, slide) : on change la hauteur sans re-frapper fort
    void legato (int midiNote, float glideMs, float extraExcite)
    {
        note = midiNote; held = true;
        targetPitch = (float) midiNote;
        setGlide (glideMs);
        float hz = midiToHz (targetPitch);
        fbGain = std::pow (10.0f, -3.0f / (hz * t60Base));
        if (extraExcite > 0)
        {
            exciteLen = juce::jlimit (8, 4000, (int) (sr / hz));
            exciteLeft = exciteLen;
            exAmp = extraExcite;
            exLpA = 0.85f;
            pickComb.assign ((size_t) juce::jmax (1, (int) (sr / hz * model.pickPos)), 0.0f);
            pickIdx = 0;
        }
        releaseCoef = 1.0f;
    }

    void release (float releaseMs)
    {
        held = false;
        // la main étouffe la corde : décroissance réglée par "Relâchement"
        releaseCoef = std::exp (-6.9f / (juce::jmax (5.0f, releaseMs) * 0.001f * sr));
    }

    void kill (float ms = 6.0f) { held = false; releaseCoef = std::exp (-6.9f / (ms * 0.001f * sr)); }

    // Rend un échantillon mono
    inline float render (float pitchMod, Rng& rng)
    {
        // glissé de hauteur
        if (glideInc != 0.0f)
        {
            float d = targetPitch - curPitch;
            if (std::abs (d) <= std::abs (glideInc)) { curPitch = targetPitch; glideInc = 0; }
            else curPitch += glideInc;
        }
        if (fallTimer > 0 && --fallTimer == 0)  // "Slide down" : la note retombe
        {
            targetPitch -= 12.0f; setGlide (420.0f);
            releaseCoef = std::exp (-6.9f / (0.5f * sr));
        }

        float hz = midiToHz (curPitch + pitchMod);
        float w = 2.0f * kPi * hz / sr;
        // retard de phase du filtre passe-bas de boucle à la fréquence jouée -> accord juste
        float lpDelay = std::atan2 (loopA * std::sin (w), 1.0f - loopA * std::cos (w)) / juce::jmax (1.0e-4f, w);
        float delay = juce::jlimit (2.0f, (float) mask - 2.0f, sr / hz - lpDelay);

        float rp = (float) writePos - delay;
        while (rp < 0) rp += (float) (mask + 1);
        int i0 = (int) rp; float fr = rp - (float) i0;
        float y = buf[(size_t) (i0 & mask)] * (1.0f - fr) + buf[(size_t) ((i0 + 1) & mask)] * fr;

        lpState = (1.0f - loopA) * y + loopA * lpState;
        float fb = lpState * fbGain;

        float ex = 0.0f;
        if (exciteLeft > 0)
        {
            float n = rng.next();
            exLp0 = (1.0f - exLpA) * n + exLpA * exLp0;
            exLp = (1.0f - exLpA) * exLp0 + exLpA * exLp;
            // profil du médiator : un creux en peigne selon l'endroit où l'on gratte
            float delayed = pickComb[(size_t) pickIdx];
            pickComb[(size_t) pickIdx] = exLp;
            pickIdx = (pickIdx + 1) % (int) pickComb.size();
            float win = 0.5f - 0.5f * std::cos (2.0f * kPi * (float) (exciteLen - exciteLeft) / (float) exciteLen);
            ex = (exLp - 0.9f * delayed) * win * exAmp;
            --exciteLeft;
        }

        buf[(size_t) writePos] = fb + ex;
        writePos = (writePos + 1) & mask;

        // micro (électrique) : filtrage en peigne selon sa position
        float out = y;
        if (model.pickupPos > 0.0f)
        {
            float pd = juce::jmax (1.0f, delay * model.pickupPos);
            float rp2 = (float) writePos - 1.0f - pd;
            while (rp2 < 0) rp2 += (float) (mask + 1);
            int j0 = (int) rp2; float f2 = rp2 - (float) j0;
            float y2 = buf[(size_t) (j0 & mask)] * (1.0f - f2) + buf[(size_t) ((j0 + 1) & mask)] * f2;
            out = (y - 0.7f * y2) * 0.65f;
        }

        // enveloppe
        if (env < 1.0f && held) env = juce::jmin (1.0f, env + attackInc);
        if (! held || releaseCoef < 1.0f) relEnv *= releaseCoef; else relEnv = 1.0f;

        // filtre anti-continu
        float o = out * env * relEnv * gain;
        dcY = o - dcX + 0.995f * dcY; dcX = o;

        if (std::abs (dcY) < 1.0e-5f) { if (++silentCount > (int) (sr * 0.05f)) active = false; }
        else silentCount = 0;
        if (relEnv < 1.0e-4f) active = false;
        return dcY;
    }

    void startFresh() { relEnv = 1.0f; }

    bool active = false, held = false;
    int note = -1, articulation = 0, guitarType = 0;
    float pan = 0.0f;
    float brightness = 0.5f;
    uint64_t age = 0;

private:
    void setGlide (float ms)
    {
        if (ms <= 0.0f) { curPitch = targetPitch; glideInc = 0.0f; return; }
        glideInc = (targetPitch - curPitch) / (ms * 0.001f * sr);
    }

    float sr = 48000.0f;
    std::vector<float> buf;
    int mask = 4095, writePos = 0;
    GuitarModel model = getModel (0);
    float targetPitch = 60, curPitch = 60, glideInc = 0;
    int fallTimer = -1;
    float loopA = 0.3f, lpState = 0, fbGain = 0.99f, t60Base = 3.0f;
    int exciteLeft = 0, exciteLen = 100;
    float exLpA = 0.5f, exLp = 0, exLp0 = 0, exAmp = 1;
    std::vector<float> pickComb { 0.0f };
    int pickIdx = 0;
    float env = 1, attackInc = 1, releaseCoef = 1, relEnv = 1, gain = 1;
    float dcX = 0, dcY = 0;
    int silentCount = 0;
};

//==============================================================================
// Bruits de main / bruits de guitare (frottements, coups sur la caisse, médiator)
class NoiseFx
{
public:
    enum Type { FretSqueak = 0, BodyKnock, PickScrape, ReleaseNoise, PickClick };

    void prepare (double sampleRate) { sr = (float) sampleRate; for (auto& v : v_) v.on = false; }

    void trigger (int type, float level, float pan)
    {
        auto* best = &v_[0];
        for (auto& v : v_) if (! v.on) { best = &v; break; }
        auto& v = *best;
        v.on = true; v.type = type; v.t = 0; v.level = level; v.pan = pan; v.state = 0;
        switch (type)
        {
            case FretSqueak:   v.len = (int) (0.09f * sr); v.f0 = 900.0f;  v.f1 = 2600.0f; break;
            case BodyKnock:    v.len = (int) (0.25f * sr); v.f0 = 140.0f;  v.f1 = 95.0f;   break;
            case PickScrape:   v.len = (int) (0.9f * sr);  v.f0 = 5200.0f; v.f1 = 700.0f;  break;
            case ReleaseNoise: v.len = (int) (0.05f * sr); v.f0 = 1800.0f; v.f1 = 1200.0f; break;
            default:           v.len = (int) (0.006f * sr);v.f0 = 6000.0f; v.f1 = 6000.0f; break;
        }
        v.bp.reset();
    }

    void render (float* L, float* R, int n)
    {
        for (auto& v : v_)
        {
            if (! v.on) continue;
            for (int i = 0; i < n && v.on; ++i)
            {
                float ph = (float) v.t / (float) v.len;
                float f = v.f0 + (v.f1 - v.f0) * ph;
                if ((v.t & 31) == 0) v.bp.bandPass (sr, f, v.type == BodyKnock ? 2.0 : 4.0);
                float src;
                if (v.type == BodyKnock)
                {
                    v.state += 2.0f * kPi * f / sr;
                    src = std::sin (v.state) * 0.8f + rng.next() * 0.25f;
                }
                else src = rng.next();
                float e = v.type == PickScrape ? std::sin (kPi * ph) : std::exp (-5.0f * ph) * juce::jmin (1.0f, (float) v.t / (0.002f * sr));
                float s = v.bp.process (src) * e * v.level;
                L[i] += s * (1.0f - juce::jmax (0.0f, v.pan));
                R[i] += s * (1.0f + juce::jmin (0.0f, v.pan));
                if (++v.t >= v.len) v.on = false;
            }
        }
    }

private:
    struct V { bool on = false; int type = 0, t = 0, len = 1; float level = 0, pan = 0, f0 = 1000, f1 = 1000, state = 0; Biquad bp; };
    std::array<V, 8> v_;
    float sr = 48000;
    Rng rng;
};

//==============================================================================
// Caisse de résonance (acoustique) / couleur du micro (électrique)
class BodyFilter
{
public:
    void prepare (double s) { sr = s; setGuitar (-1); }
    void setGuitar (int g)
    {
        if (g == current) return;
        current = g;
        for (int c = 0; c < 2; ++c)
        {
            auto& f = flt[c];
            switch (juce::jmax (0, g))
            {
                case Nylon:
                    f[0].peak (sr, 98, 1.6, 6.0);  f[1].peak (sr, 205, 1.4, 4.0);
                    f[2].peak (sr, 2600, 0.8, -3.0); f[3].lowPass (sr, 6500, 0.6); break;
                case DoubleBobinage:
                    f[0].highPass (sr, 70, 0.7); f[1].peak (sr, 450, 0.8, 2.5);
                    f[2].peak (sr, 2300, 1.3, 5.0); f[3].lowPass (sr, 4200, 0.9); break;
                case SimpleBobinage:
                    f[0].highPass (sr, 90, 0.7); f[1].peak (sr, 250, 0.9, -2.0);
                    f[2].peak (sr, 4300, 1.6, 5.0); f[3].lowPass (sr, 7000, 0.9); break;
                default: // acier acoustique
                    f[0].peak (sr, 105, 1.6, 6.0); f[1].peak (sr, 230, 1.4, 3.5);
                    f[2].peak (sr, 3300, 0.9, 3.0); f[3].highShelf (sr, 9000, 2.0); break;
            }
            for (auto& b : f) b.reset();
        }
    }
    inline float process (int ch, float x) { for (auto& b : flt[ch]) x = b.process (x); return x; }
private:
    double sr = 48000; int current = -2;
    std::array<std::array<Biquad, 4>, 2> flt;
};

//==============================================================================
// Ampli guitare (préampli à saturation + simulation de baffle)
class Amp
{
public:
    void prepare (double s)
    {
        sr = s;
        for (int c = 0; c < 2; ++c)
        {
            hp[c].highPass (sr, 85, 0.7); mid[c].peak (sr, 750, 0.9, 3.0);
            cab1[c].lowPass (sr, 5200, 0.8); cab2[c].peak (sr, 120, 1.0, 3.0); cab3[c].peak (sr, 2600, 2.0, -3.0);
            hp[c].reset(); mid[c].reset(); cab1[c].reset(); cab2[c].reset(); cab3[c].reset();
        }
    }
    inline float process (int c, float x, float drive) // drive 0..1
    {
        float g = 1.0f + drive * drive * 40.0f;
        float v = mid[c].process (hp[c].process (x)) * g;
        v = std::tanh (v + 0.12f * drive) - std::tanh (0.12f * drive);   // un peu d'asymétrie (lampe)
        v /= std::pow (g, 0.6f);                                          // niveau stable quel que soit le gain
        v = cab3[c].process (cab2[c].process (cab1[c].process (v)));
        return v;
    }
private:
    double sr = 48000;
    Biquad hp[2], mid[2], cab1[2], cab2[2], cab3[2];
};

//==============================================================================
// Compresseur simple
class Comp
{
public:
    void prepare (double s) { sr = (float) s; env = 0; }
    inline void process (float& l, float& r, float amount)
    {
        float lvl = juce::jmax (std::abs (l), std::abs (r));
        float coef = lvl > env ? std::exp (-1.0f / (0.004f * sr)) : std::exp (-1.0f / (0.13f * sr));
        env = lvl + coef * (env - lvl);
        float db = juce::Decibels::gainToDecibels (env, -100.0f);
        float thr = -6.0f - amount * 24.0f, ratio = 2.0f + amount * 4.0f;
        float gr = db > thr ? (thr + (db - thr) / ratio) - db : 0.0f;
        float g = juce::Decibels::decibelsToGain (gr + amount * 9.0f);
        l *= g; r *= g;
    }
private:
    float sr = 48000, env = 0;
};

//==============================================================================
// Delay stéréo (avec retour filtré)
class StereoDelay
{
public:
    void prepare (double s)
    {
        sr = (float) s;
        int n = 1; while (n < (int) (sr * 2.1f)) n <<= 1;
        bl.assign ((size_t) n, 0.0f); br.assign ((size_t) n, 0.0f); mask = n - 1; pos = 0;
        lpL = lpR = 0;
    }
    inline void process (float& l, float& r, float timeSec, float mix, float feedback)
    {
        smTime += (timeSec - smTime) * 0.0005f;
        float d = juce::jlimit (1.0f, (float) mask - 2.0f, smTime * sr);
        float dl = read (bl, d), dr = read (br, d * 1.5f > (float) mask - 2.0f ? d : d * 1.5f);
        lpL += 0.35f * (dl - lpL); lpR += 0.35f * (dr - lpR);
        bl[(size_t) pos] = l + lpR * feedback;     // ping-pong
        br[(size_t) pos] = r + lpL * feedback;
        pos = (pos + 1) & mask;
        l += lpL * mix; r += lpR * mix;
    }
private:
    inline float read (const std::vector<float>& b, float d) const
    {
        float rp = (float) pos - d; while (rp < 0) rp += (float) (mask + 1);
        int i = (int) rp; float f = rp - (float) i;
        return b[(size_t) (i & mask)] * (1 - f) + b[(size_t) ((i + 1) & mask)] * f;
    }
    float sr = 48000, smTime = 0.375f, lpL = 0, lpR = 0;
    std::vector<float> bl, br; int mask = 0, pos = 0;
};
} // namespace bg
