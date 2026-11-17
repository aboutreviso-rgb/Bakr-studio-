#pragma once
#include <JuceHeader.h>
#include <cmath>

// Briques DSP communes aux plugins BAKR ANTHOLOGY
namespace bakr
{
    constexpr float kPi = 3.14159265358979f;

    inline float coefMs (float ms, double fs) noexcept
    {
        return 1.0f - std::exp (-1.0f / (juce::jmax (0.01f, ms) * 0.001f * (float) fs));
    }
    inline float dB (float g) noexcept     { return juce::Decibels::gainToDecibels (g, -120.0f); }
    inline float gain (float d) noexcept   { return juce::Decibels::decibelsToGain (d, -120.0f); }

    //==========================================================================
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

        inline float process (float x) noexcept
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
        void reset() noexcept { z1 = z2 = 0; }
        void identity() noexcept { b0 = 1; b1 = b2 = a1 = a2 = 0; }

        void set (double nb0, double nb1, double nb2, double a0, double na1, double na2) noexcept
        {
            b0 = (float) (nb0 / a0); b1 = (float) (nb1 / a0); b2 = (float) (nb2 / a0);
            a1 = (float) (na1 / a0); a2 = (float) (na2 / a0);
        }
        static double clampF (double fs, double f) { return juce::jlimit (10.0, fs * 0.45, f); }

        void highPass (double fs, double f, double q = 0.707)
        {
            const double w = juce::MathConstants<double>::twoPi * clampF (fs, f) / fs, c = std::cos (w), a = std::sin (w) / (2 * q);
            set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + a, -2 * c, 1 - a);
        }
        void lowPass (double fs, double f, double q = 0.707)
        {
            const double w = juce::MathConstants<double>::twoPi * clampF (fs, f) / fs, c = std::cos (w), a = std::sin (w) / (2 * q);
            set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + a, -2 * c, 1 - a);
        }
        void peak (double fs, double f, double q, double g)
        {
            if (std::abs (g) < 0.01) { identity(); return; }
            const double A = std::pow (10.0, g / 40.0);
            const double w = juce::MathConstants<double>::twoPi * clampF (fs, f) / fs, c = std::cos (w), a = std::sin (w) / (2 * q);
            set (1 + a * A, -2 * c, 1 - a * A, 1 + a / A, -2 * c, 1 - a / A);
        }
        // shelf avec "pente" Q (Q > 0.707 = bosse/creux résonant à la Brit)
        void lowShelf (double fs, double f, double g, double q = 0.707)
        {
            if (std::abs (g) < 0.01) { identity(); return; }
            const double A = std::pow (10.0, g / 40.0);
            const double w = juce::MathConstants<double>::twoPi * clampF (fs, f) / fs, c = std::cos (w), s = std::sin (w);
            const double al = s / (2 * q), sq = 2 * std::sqrt (A) * al;
            set (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
                 (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
        }
        void highShelf (double fs, double f, double g, double q = 0.707)
        {
            if (std::abs (g) < 0.01) { identity(); return; }
            const double A = std::pow (10.0, g / 40.0);
            const double w = juce::MathConstants<double>::twoPi * clampF (fs, f) / fs, c = std::cos (w), s = std::sin (w);
            const double al = s / (2 * q), sq = 2 * std::sqrt (A) * al;
            set (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
                 (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
        }
    };

    struct OnePole
    {
        float s = 0, a = 1;
        void setLP (double fs, double f) { a = 1.0f - std::exp (-2.0f * kPi * (float) f / (float) fs); }
        inline float lp (float x) noexcept { s += a * (x - s); return s; }
        inline float hp (float x) noexcept { s += a * (x - s); return x - s; }
        void reset() { s = 0; }
    };

    //==========================================================================
    // Saturations analogiques
    //==========================================================================

    // Lampe triode : asymétrique (harmoniques paires), gain petit-signal = 1
    inline float triode (float x, float drive, float bias = 0.22f) noexcept
    {
        const float a = 1.0f + 3.0f * drive;
        const float tb = std::tanh (a * bias);
        return (std::tanh (a * (x + bias)) - tb) / (a * (1.0f - tb * tb));
    }

    // Etage symétrique doux (harmoniques impaires), gain petit-signal = 1
    inline float softClip (float x, float drive) noexcept
    {
        const float a = 1.0f + 4.0f * drive;
        return std::tanh (a * x) / a;
    }

    // Transfo : sature surtout le grave, arrondit les aigus, petite bosse de grave
    struct Transformer
    {
        OnePole split, hf, dc;
        Biquad bump;
        float amount = 0.5f;

        void prepare (double fs, float amt, float hfCut = 22000.0f, float bumpDb = 0.7f)
        {
            amount = amt;
            split.setLP (fs, 140.0);
            hf.setLP (fs, juce::jmin ((double) hfCut, fs * 0.45));
            dc.setLP (fs, 5.0);
            bump.lowShelf (fs, 80.0, bumpDb * amt, 0.8);
        }
        void reset() { split.reset(); hf.reset(); dc.reset(); bump.reset(); }

        inline float process (float x) noexcept
        {
            const float lf = split.lp (x);
            const float g = 1.0f + 2.5f * amount;
            float y = (x - lf) + std::tanh (g * lf) / g;
            y = y - 0.06f * amount * y * y * y / (1.0f + std::abs (y));
            y = dc.hp (y);
            return bump.process (hf.lp (y));
        }
    };

    //==========================================================================
    // Compression
    //==========================================================================

    // réduction de gain (dB, positive) pour un dépassement donné, genou doux
    inline float kneeGr (float overDb, float ratio, float knee) noexcept
    {
        const float slope = 1.0f - 1.0f / juce::jmax (1.0f, ratio);
        if (knee <= 0.01f) return overDb > 0.0f ? overDb * slope : 0.0f;
        if (overDb <= -knee * 0.5f) return 0.0f;
        if (overDb >= knee * 0.5f) return overDb * slope;
        const float x = overDb + knee * 0.5f;
        return slope * x * x / (2.0f * knee);
    }

    // courbe variable-mu : genou très large, ratio qui grimpe avec le niveau
    inline float variMuGr (float overDb) noexcept
    {
        constexpr float knee = 12.0f, r0 = 2.0f, rMax = 14.0f, tau = 16.0f;
        const float x = overDb + knee * 0.5f;
        if (x <= 0.0f) return 0.0f;
        const float s0 = 1.0f - 1.0f / r0;
        if (x < knee) return s0 * x * x / (2.0f * knee);
        const float u = x - knee;
        return s0 * (x - knee * 0.5f) + (1.0f / r0 - 1.0f / rMax) * (u - tau * (1.0f - std::exp (-u / tau)));
    }

    struct Rms
    {
        float ms = 0;
        inline float process (float x, float c) noexcept { ms += c * (x * x - ms); return std::sqrt (ms); }
        void reset() { ms = 0; }
    };

    struct Env
    {
        float v = 0;
        inline float process (float target, float atk, float rel) noexcept
        {
            v += (target > v ? atk : rel) * (target - v);
            return v;
        }
        void reset() { v = 0; }
    };

    // Cellule opto : attaque ~10 ms, relâchement en 2 temps + mémoire
    struct OptoCell
    {
        Env fast, slow;
        float charge = 0;

        inline float process (float targetGr, double fs, float atkMs, float relScale) noexcept
        {
            const float a = coefMs (atkMs, fs);
            const float on = targetGr > 1.0f ? 1.0f : 0.0f;
            charge += (on > charge ? coefMs (3000.0f, fs) : coefMs (10000.0f, fs)) * (on - charge);
            fast.process (targetGr, a, coefMs (60.0f * relScale, fs));
            slow.process (targetGr, a, coefMs ((500.0f + 4500.0f * charge) * relScale, fs));
            return 0.5f * (fast.v + slow.v);
        }
        void reset() { fast.reset(); slow.reset(); charge = 0; }
    };
}
