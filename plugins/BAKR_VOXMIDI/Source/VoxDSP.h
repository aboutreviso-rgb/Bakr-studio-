// BAKR VOXMIDI - moteurs DSP : détection de hauteur (YIN), suivi de notes, synthé intégré
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace vox
{
static inline float mtof (float m) { return 440.0f * std::pow (2.0f, (m - 69.0f) / 12.0f); }
static inline float dbToGain (float db) { return db <= -60.0f ? 0.0f : std::pow (10.0f, db / 20.0f); }
static inline float gainToDb (float g) { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }

//==============================================================================
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    inline float process (float x) noexcept
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float) y;
    }
    void lowPass (double f, double fs, double q = 0.7071)
    {
        f = std::clamp (f, 10.0, fs * 0.45);
        const double w0 = 2.0 * juce::MathConstants<double>::pi * f / fs, c = std::cos (w0), a = std::sin (w0) / (2.0 * q), a0 = 1 + a;
        b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0; b2 = (1 - c) / 2 / a0; a1 = -2 * c / a0; a2 = (1 - a) / a0;
    }
    void highPass (double f, double fs, double q = 0.7071)
    {
        f = std::clamp (f, 10.0, fs * 0.45);
        const double w0 = 2.0 * juce::MathConstants<double>::pi * f / fs, c = std::cos (w0), a = std::sin (w0) / (2.0 * q), a0 = 1 + a;
        b0 = (1 + c) / 2 / a0; b1 = -(1 + c) / a0; b2 = (1 + c) / 2 / a0; a1 = -2 * c / a0; a2 = (1 - a) / a0;
    }
};

//==============================================================================
// Détecteur de hauteur YIN (signal décimé à ~24 kHz)
class PitchTracker
{
public:
    void prepare (double hostSr)
    {
        factor = std::clamp ((int) std::round (hostSr / 24000.0), 1, 4);
        sr = hostSr / factor;
        W = (int) (sr * 0.017);
        tauMax = (int) (sr / 65.0);
        tauMin = std::max (2, (int) (sr / 1300.0));
        N = W + tauMax + 2;
        hop = std::max (32, (int) (sr * 0.0053));
        ring.assign (2048, 0.0f);
        frame.assign ((size_t) N, 0.0f);
        df.assign ((size_t) tauMax + 2, 0.0f);
        cm.assign ((size_t) tauMax + 2, 1.0f);
        aa1.lowPass (sr * 0.42, hostSr, 0.54); aa2.lowPass (sr * 0.42, hostSr, 1.31);
        hpf.highPass (55.0, hostSr);
        reset();
    }
    void reset() { std::fill (ring.begin(), ring.end(), 0.0f); w = 0; dc = 0; hc = 0; sumSq = 0; f0 = 0; conf = 0; rmsDb = -100; aa1.reset(); aa2.reset(); hpf.reset(); }

    // renvoie true quand une nouvelle analyse est disponible
    inline bool push (float x) noexcept
    {
        const float y = aa2.process (aa1.process (hpf.process (x)));
        if (++dc < factor) return false;
        dc = 0;
        ring[(size_t) (w & 2047)] = y; ++w;
        sumSq += y * y;
        if (++hc >= hop) { hc = 0; analyse(); return true; }
        return false;
    }

    double getHopSeconds() const { return hop / sr; }
    float f0 = 0, conf = 0, rmsDb = -100;

private:
    void analyse() noexcept
    {
        rmsDb = 10.0f * std::log10 (sumSq / hop + 1.0e-12f);
        sumSq = 0;
        for (int j = 0; j < N; ++j) frame[(size_t) j] = ring[(size_t) ((w - N + j) & 2047)];

        if (rmsDb < -75.0f) { f0 = 0; conf = 0; return; }

        // fonction de différence
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            float d = 0;
            const float* a = frame.data();
            const float* b = frame.data() + tau;
            for (int j = 0; j < W; ++j) { const float diff = a[j] - b[j]; d += diff * diff; }
            df[(size_t) tau] = d;
        }
        // différence normalisée cumulée
        cm[0] = 1.0f;
        double running = 0;
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            running += df[(size_t) tau];
            cm[(size_t) tau] = running > 0 ? (float) (df[(size_t) tau] * tau / running) : 1.0f;
        }
        int t = -1;
        for (int tau = tauMin; tau < tauMax; ++tau)
        {
            if (cm[(size_t) tau] < 0.15f)
            {
                while (tau + 1 < tauMax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau;
                t = tau; break;
            }
        }
        if (t < 0)
        {
            int best = tauMin; for (int tau = tauMin; tau < tauMax; ++tau) if (cm[(size_t) tau] < cm[(size_t) best]) best = tau;
            if (cm[(size_t) best] < 0.3f) t = best;
        }
        if (t < 0) { f0 = 0; conf = 0; return; }
        float better = (float) t;
        if (t > 1 && t < tauMax)
        {
            const float a = cm[(size_t) t - 1], b = cm[(size_t) t], c = cm[(size_t) t + 1], den = a - 2 * b + c;
            if (std::abs (den) > 1.0e-9f) better = t + 0.5f * (a - c) / den;
        }
        f0 = (float) (sr / better);
        conf = std::clamp (1.0f - cm[(size_t) t], 0.0f, 1.0f);
    }

    double sr = 24000;
    int factor = 2, W = 400, tauMax = 370, tauMin = 18, N = 800, hop = 128;
    std::vector<float> ring, frame, df, cm;
    int64_t w = 0;
    int dc = 0, hc = 0;
    float sumSq = 0;
    Biquad aa1, aa2, hpf;
};

//==============================================================================
// Synthé monophonique intégré
class VoxSynth
{
public:
    enum Inst { Guitare = 0, Basse, Cuivres, Lead, Kora, Balafon, B808, Flute, NumInst };

    void prepare (double s)
    {
        sr = s;
        ks.assign (8192, 0.0f);
        reset();
    }
    void reset()
    {
        std::fill (ks.begin(), ks.end(), 0.0f);
        ksW = 0; exciteLeft = 0; env = 0; gate = false; phase1 = phase2 = phase3 = 0; s1 = s2 = 0; ksLp = 0;
        curMidi = targetMidi = 60; expr = exprT = 0; pluck = 0; click = 0; bp1 = bp2 = 0; dropEnv = 0;
    }
    void setInstrument (int i) { inst = std::clamp (i, 0, (int) NumInst - 1); }
    void setParams (float glideMs, float brightness) { glide = glideMs <= 0.5f ? 1.0f : 1.0f - std::exp (-1.0f / (float) (glideMs * 0.001 * sr)); tone = brightness; }
    void setTarget (float midi) { targetMidi = midi; }
    void setExpression (float e) { exprT = std::clamp (e, 0.0f, 1.0f); }

    void noteOn (float midi, float vel)
    {
        const bool legato = gate;
        targetMidi = midi; if (! legato || inst == Guitare || inst == Kora || inst == Balafon || inst == B808) curMidi = midi;
        velocity = vel; gate = true;
        if (inst == Guitare || inst == Kora) { exciteLeft = (int) (sr / mtof (curMidi)); exciteLp = 0; }
        if (inst == Balafon || inst == B808 || inst == Guitare || inst == Kora) pluck = 1.0f;
        if (inst == Balafon) { click = 1.0f; phase1 = phase2 = 0; }
        if (inst == B808) { dropEnv = 1.0f; phase1 = 0; }
    }
    void noteOff() { gate = false; }

    inline float render() noexcept
    {
        curMidi += (targetMidi - curMidi) * glide;
        const float f = mtof (curMidi), dt = (float) (f / sr);
        expr += (exprT - expr) * 0.002f;
        float out = 0;

        switch (inst)
        {
            case Guitare:
            case Kora:
            {
                const bool kora = inst == Kora;
                float exc = 0;
                if (exciteLeft > 0) { --exciteLeft; const float n = rnd() * velocity; exciteLp += (kora ? 0.85f : 0.45f + 0.3f * tone) * (n - exciteLp); exc = exciteLp; }
                const float D = (float) (sr / f);
                const float rp = (float) ksW - D;
                const int i0 = (int) std::floor (rp); const float fr = rp - (float) i0;
                const float y = ks[(size_t) (i0 & 8191)] * (1 - fr) + ks[(size_t) ((i0 + 1) & 8191)] * fr;
                const float a = kora ? 0.62f + 0.2f * tone : 0.48f + 0.25f * tone;
                ksLp = a * y + (1 - a) * ksLp;
                const float fb = gate ? (kora ? 0.993f : 0.9965f) : 0.94f;
                ks[(size_t) (ksW & 8191)] = exc + ksLp * fb;
                ++ksW;
                out = y * 0.9f;
                break;
            }
            case Basse:
            {
                env = gate ? env + (1.0f - env) * 0.004f : env * 0.9993f;
                const float saw = polySaw (phase1, dt);
                const float sub = std::sin (phase2 * juce::MathConstants<float>::twoPi); advance (phase2, dt * 0.5f);
                out = svfLp (saw * 0.7f + sub * 0.6f, 250.0f + tone * 1400.0f, 0.8f) * env * (0.6f + 0.4f * velocity);
                break;
            }
            case Cuivres:
            {
                env = gate ? env + (1.0f - env) * 0.0015f : env * 0.9985f;
                const float s = polySaw (phase1, dt * 1.003f) + polySaw (phase3, dt * 0.997f);
                const float cut = 400.0f + (700.0f + tone * 4000.0f) * env * (0.4f + 0.6f * expr);
                out = svfLp (s * 0.45f, cut, 0.9f) * env * (0.35f + 0.65f * expr);
                break;
            }
            case Lead:
            {
                env = gate ? env + (1.0f - env) * 0.01f : env * 0.999f;
                const float saw = polySaw (phase1, dt);
                const float sq = (phase3 < 0.5f ? 0.6f : -0.6f); advance (phase3, dt * 1.004f);
                out = svfLp (saw * 0.5f + sq * 0.35f, 1200.0f + tone * 6000.0f, 2.2f) * env * (0.3f + 0.7f * expr);
                break;
            }
            case Balafon:
            {
                pluck *= gate ? 0.99988f : 0.9996f;
                const float p1 = std::sin (phase1 * juce::MathConstants<float>::twoPi); advance (phase1, dt);
                const float p2 = std::sin (phase2 * juce::MathConstants<float>::twoPi); advance (phase2, dt * 3.93f);
                click *= 0.993f;
                out = (p1 * pluck + p2 * pluck * pluck * pluck * 0.35f + rnd() * click * click * 0.25f) * (0.5f + 0.5f * velocity);
                break;
            }
            case B808:
            {
                env = gate ? 1.0f : env * 0.9994f;
                pluck *= 0.99997f;
                dropEnv *= 0.9985f;
                const float s = std::sin (phase1 * juce::MathConstants<float>::twoPi); advance (phase1, dt * (1.0f + 0.6f * dropEnv));
                out = std::tanh (s * (1.5f + 2.5f * tone)) * 0.7f * pluck * env;
                break;
            }
            case Flute:
            {
                env = gate ? env + (1.0f - env) * 0.003f : env * 0.998f;
                const float s = std::sin (phase1 * juce::MathConstants<float>::twoPi) + 0.12f * std::sin (phase1 * 2.0f * juce::MathConstants<float>::twoPi);
                advance (phase1, dt);
                const float breath = svfBp (rnd(), f * 2.0f, 2.0f) * (0.1f + 0.2f * tone);
                out = (s * 0.5f + breath) * env * (0.15f + 0.85f * expr);
                break;
            }
            default: break;
        }
        return out;
    }

private:
    static inline void advance (float& p, float d) { p += d; if (p >= 1.0f) p -= 1.0f; }
    static inline float blep (float t, float dt)
    {
        if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }
    inline float polySaw (float& p, float dt) { const float v = 2.0f * p - 1.0f - blep (p, dt); advance (p, dt); return v; }
    inline float rnd() { seed = seed * 1664525u + 1013904223u; return (float) ((seed >> 9) & 0x7fffff) / 4194304.0f - 1.0f; }
    inline float svfLp (float x, float fc, float q)
    {
        const float g = std::tan (juce::MathConstants<float>::pi * std::min (fc, (float) sr * 0.45f) / (float) sr), k = 1.0f / q;
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2 * v1 - s1; s2 = 2 * v2 - s2; return v2;
    }
    inline float svfBp (float x, float fc, float q)
    {
        const float g = std::tan (juce::MathConstants<float>::pi * std::min (fc, (float) sr * 0.45f) / (float) sr), k = 1.0f / q;
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - bp2, v1 = a1 * bp1 + a2 * v3, v2 = bp2 + a2 * bp1 + a3 * v3;
        bp1 = 2 * v1 - bp1; bp2 = 2 * v2 - bp2; return v1;
    }

    double sr = 48000;
    int inst = Guitare;
    std::vector<float> ks;
    int64_t ksW = 0;
    int exciteLeft = 0;
    float exciteLp = 0, ksLp = 0, env = 0, velocity = 0.8f, glide = 1, tone = 0.5f;
    float phase1 = 0, phase2 = 0, phase3 = 0, s1 = 0, s2 = 0, bp1 = 0, bp2 = 0;
    float curMidi = 60, targetMidi = 60, expr = 0, exprT = 0, pluck = 0, click = 0, dropEnv = 0;
    bool gate = false;
    uint32_t seed = 22222u;
};
} // namespace vox
