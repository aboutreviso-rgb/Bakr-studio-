#pragma once
#include <cmath>
#include <algorithm>

// BAKR 81 - DSP : filtres biquad, préampli classe A + transformateur
namespace b81
{
constexpr double pi = 3.14159265358979323846;

inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }

struct Coefs { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

struct Biquad
{
    Coefs c;
    double z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    inline float process (float xf)
    {
        const double x = xf;
        const double y = c.b0 * x + z1;
        z1 = c.b1 * x - c.a1 * y + z2;
        z2 = c.b2 * x - c.a2 * y;
        return (float) y;
    }
};

inline Coefs norm (double b0, double b1, double b2, double a0, double a1, double a2)
{
    Coefs c; c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0; c.a1 = a1 / a0; c.a2 = a2 / a0; return c;
}

inline double clampF (double f, double sr) { return std::clamp (f, 10.0, sr * 0.47); }

inline Coefs peak (double f, double sr, double q, double gdb)
{
    f = clampF (f, sr);
    const double A = std::pow (10.0, gdb / 40.0), w = 2 * pi * f / sr, al = std::sin (w) / (2 * q), cw = std::cos (w);
    return norm (1 + al * A, -2 * cw, 1 - al * A, 1 + al / A, -2 * cw, 1 - al / A);
}

inline Coefs lowShelf (double f, double sr, double q, double gdb)
{
    f = clampF (f, sr);
    const double A = std::pow (10.0, gdb / 40.0), w = 2 * pi * f / sr, cw = std::cos (w), al = std::sin (w) / (2 * q), s = 2 * std::sqrt (A) * al;
    return norm (A * ((A + 1) - (A - 1) * cw + s), 2 * A * ((A - 1) - (A + 1) * cw), A * ((A + 1) - (A - 1) * cw - s),
                 (A + 1) + (A - 1) * cw + s, -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - s);
}

inline Coefs highShelf (double f, double sr, double q, double gdb)
{
    f = clampF (f, sr);
    const double A = std::pow (10.0, gdb / 40.0), w = 2 * pi * f / sr, cw = std::cos (w), al = std::sin (w) / (2 * q), s = 2 * std::sqrt (A) * al;
    return norm (A * ((A + 1) + (A - 1) * cw + s), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - s),
                 (A + 1) - (A - 1) * cw + s, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - s);
}

inline Coefs highPass2 (double f, double sr, double q)
{
    f = clampF (f, sr);
    const double w = 2 * pi * f / sr, cw = std::cos (w), al = std::sin (w) / (2 * q);
    return norm ((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + al, -2 * cw, 1 - al);
}

inline Coefs lowPass2 (double f, double sr, double q)
{
    f = clampF (f, sr);
    const double w = 2 * pi * f / sr, cw = std::cos (w), al = std::sin (w) / (2 * q);
    return norm ((1 - cw) / 2, 1 - cw, (1 - cw) / 2, 1 + al, -2 * cw, 1 - al);
}

inline Coefs highPass1 (double f, double sr)
{
    f = clampF (f, sr);
    const double k = std::tan (pi * f / sr);
    return norm (1, -1, 0, 1 + k, k - 1, 0);
}

// ---------------------------------------------------------------------------
// Préampli : transformateur d'entrée (saturation des basses, légère compression)
// + étage classe A asymétrique (harmoniques paires) + transformateur de sortie.
// Tourne en suréchantillonnage x2.
struct Preamp
{
    float aLow = 0.01f, aHf = 0.9f, rDc = 0.999f;
    float low = 0, hf = 0, dcx = 0, dcy = 0;

    void prepare (double osRate)
    {
        aLow = (float) (1.0 - std::exp (-2 * pi * 110.0 / osRate));
        aHf  = (float) (1.0 - std::exp (-2 * pi * std::min (32000.0, osRate * 0.4) / osRate));
        rDc  = (float) std::exp (-2 * pi * 8.0 / osRate);
        reset();
    }
    void reset() { low = hf = dcx = dcy = 0; }

    static inline float softClip (float x) { return x / std::sqrt (1.0f + x * x); }

    // colour 0..1 : quantité de caractère (augmente avec le gain)
    inline float process (float x, float colour)
    {
        // Transformateur d'entrée : les basses saturent en premier (gros, rond)
        low += aLow * (x - low);
        const float kT = 1.2f + 1.6f * colour;
        const float lowSat = softClip (low * kT) / kT;
        float s = x + (lowSat - low) * 0.85f;

        // Étage classe A : courbe asymétrique -> 2e harmonique, normalisée à gain 1 en petit signal
        constexpr float bias = 0.22f;
        const float k = 0.42f + 0.5f * colour;
        static const float tb = std::tanh (bias);
        const float slope = 1.0f - tb * tb;
        float y = (std::tanh (k * s + bias) - tb) / (k * slope);

        // Transformateur de sortie : légère perte d'aigus ultra-hautes
        hf += aHf * (y - hf);
        y = hf;

        // Blocage du continu créé par l'asymétrie
        const float out = y - dcx + rDc * dcy;
        dcx = y; dcy = out;
        return out;
    }
};

} // namespace b81
