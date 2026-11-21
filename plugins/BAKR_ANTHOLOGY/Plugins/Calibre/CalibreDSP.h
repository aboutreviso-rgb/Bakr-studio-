#pragma once
// BAKR CALIBRE — calculs de mesure : balayage sinus, déconvolution, lissage, cibles, ajustement d'EQ
#include <JuceHeader.h>
#include <complex>
#include <vector>
#include <cmath>

namespace calib
{
    using cplx = std::complex<float>;
    constexpr double kPiD = 3.14159265358979323846;

    // grille logarithmique commune (affichage + calculs)
    constexpr int    kGrid  = 480;
    constexpr double kGridLo = 15.0, kGridHi = 22000.0;
    inline double gridFreq (int i) { return kGridLo * std::pow (kGridHi / kGridLo, (double) i / (kGrid - 1)); }
    inline double gridPos (double f) { return (kGrid - 1) * std::log (f / kGridLo) / std::log (kGridHi / kGridLo); }
    inline double pointsPerOctave() { return (kGrid - 1) / std::log2 (kGridHi / kGridLo); }

    using Curve = std::vector<float>;   // kGrid valeurs en dB

    //==========================================================================
    // Balayage sinus exponentiel (Farina) 20 Hz -> 20 kHz
    inline std::vector<float> makeSweep (double fs, double seconds, double f1 = 20.0, double f2 = 20000.0)
    {
        f2 = std::min (f2, fs * 0.45);
        const int n = (int) (seconds * fs);
        std::vector<float> s ((size_t) n);
        const double L = seconds / std::log (f2 / f1), K = 2.0 * kPiD * f1 * L;
        const int fadeIn = (int) (0.05 * fs), fadeOut = (int) (0.01 * fs);
        for (int i = 0; i < n; ++i)
        {
            const double t = i / fs;
            double v = std::sin (K * (std::exp (t / L) - 1.0));
            if (i < fadeIn)          v *= 0.5 - 0.5 * std::cos (kPiD * i / fadeIn);
            if (i > n - fadeOut)     v *= 0.5 - 0.5 * std::cos (kPiD * (n - i) / fadeOut);
            s[(size_t) i] = (float) v;
        }
        return s;
    }

    inline int nextPow2Order (size_t n) { int o = 1; while (((size_t) 1 << o) < n) ++o; return o; }

    //==========================================================================
    struct ChannelMeasure
    {
        Curve  magDb;          // réponse brute (non compensée micro), lissée sur la grille
        double arrivalSec = 0; // arrivée du son direct (relative, sert à comparer G/D)
        float  snrDb = 0;
    };

    struct Measure
    {
        bool ok = false;
        juce::String error, warning;
        float recPeakDb = -120.0f;
        ChannelMeasure ch[2];
    };

    // Lissage en fraction d'octave d'un spectre de puissance -> grille log (dB)
    inline Curve powerToGrid (const std::vector<double>& pw, double fs, int fftSize)
    {
        std::vector<double> pre (pw.size() + 1, 0.0);
        for (size_t k = 0; k < pw.size(); ++k) pre[k + 1] = pre[k] + pw[k];
        const double binHz = fs / fftSize;
        Curve out ((size_t) kGrid);
        for (int i = 0; i < kGrid; ++i)
        {
            const double f = gridFreq (i);
            double frac;                         // 1/6 oct en bas, 1/3 oct dans les aigus
            if (f <= 200.0) frac = 1.0 / 6.0;
            else if (f >= 2000.0) frac = 1.0 / 3.0;
            else frac = 1.0 / 6.0 + (1.0 / 6.0) * std::log10 (f / 200.0);
            const double lo = f * std::pow (2.0, -frac / 2), hi = f * std::pow (2.0, frac / 2);
            int k1 = (int) std::floor (lo / binHz), k2 = (int) std::ceil (hi / binHz);
            k1 = juce::jlimit (1, (int) pw.size() - 1, k1);
            k2 = juce::jlimit (k1 + 1, (int) pw.size(), k2);
            const double avg = (pre[(size_t) k2] - pre[(size_t) k1]) / (k2 - k1);
            out[(size_t) i] = (float) (10.0 * std::log10 (avg + 1e-30));
        }
        return out;
    }

    // Analyse d'un enregistrement : [balayage G][silence][balayage D][silence]
    // segLen = distance (échantillons) entre le début du balayage G et celui du balayage D
    inline Measure analyse (const std::vector<float>& rec, const std::vector<float>& sweep, int segLen, double fs, int numSpk)
    {
        Measure m;
        float pk = 0;
        for (float v : rec) pk = std::max (pk, std::abs (v));
        m.recPeakDb = juce::Decibels::gainToDecibels (pk, -120.0f);
        if (rec.size() < sweep.size()) { m.error = juce::String (juce::CharPointer_UTF8 ("Enregistrement trop court : le micro n'a pas re\xc3\xa7u le son.")); return m; }
        if (m.recPeakDb < -70.0f)  { m.error = juce::String (juce::CharPointer_UTF8 ("Le micro n'entend rien : v\xc3\xa9rifie l'entr\xc3\xa9" "e de la piste du micro et le volume des HS8.")); return m; }
        if (m.recPeakDb > -0.3f)   m.warning = "Le micro sature : baisse le gain du micro ou le niveau du test.";

        // --- déconvolution par division spectrale régularisée
        const int order = nextPow2Order (rec.size() + sweep.size());
        const int N = 1 << order;
        juce::dsp::FFT fft (order);
        std::vector<cplx> a ((size_t) N), A ((size_t) N), b ((size_t) N), B ((size_t) N);
        for (size_t i = 0; i < rec.size(); ++i)   a[i] = rec[i];
        for (size_t i = 0; i < sweep.size(); ++i) b[i] = sweep[i];
        fft.perform (a.data(), A.data(), false);
        fft.perform (b.data(), B.data(), false);
        double maxS = 0;
        for (int k = 0; k <= N / 2; ++k) maxS = std::max (maxS, (double) std::norm (B[(size_t) k]));
        const double eps = maxS * 1e-5;
        const double binHz = fs / N, fLo = 14.0, fHi = std::min (21000.0, fs * 0.46);
        for (int k = 0; k < N; ++k)
        {
            const int kk = k <= N / 2 ? k : N - k;
            const double f = kk * binHz;
            double w = 1.0;
            if (f < fLo)      w = f < fLo * 0.5 ? 0.0 : 0.5 - 0.5 * std::cos (kPiD * (f - fLo * 0.5) / (fLo * 0.5));
            else if (f > fHi) w = f > fHi * 1.05 ? 0.0 : 0.5 + 0.5 * std::cos (kPiD * (f - fHi) / (fHi * 0.05));
            const cplx S = B[(size_t) k];
            const cplx H = A[(size_t) k] * std::conj (S) / (float) (std::norm (S) + eps);
            a[(size_t) k] = H * (float) w;
        }
        fft.perform (a.data(), A.data(), true);   // A = réponse impulsionnelle (circulaire)

        auto ir = [&] (long idx) { idx %= N; if (idx < 0) idx += N; return A[(size_t) idx].real(); };

        const int pre = (int) (0.003 * fs), win = (int) (0.5 * fs), taper = (int) (0.15 * fs);
        const int fftM = std::max (65536, 1 << nextPow2Order ((size_t) (win + pre) * 2));
        const int orderM = nextPow2Order ((size_t) fftM);
        juce::dsp::FFT fftW (orderM);

        for (int c = 0; c < numSpk; ++c)
        {
            const long base = (long) c * segLen;
            // pic du son direct : entre -20 ms et +1 s après le début théorique
            long best = base; float bestV = 0;
            for (long i = base - (long) (0.02 * fs); i < base + (long) (1.0 * fs); ++i)
            {
                const float v = std::abs (ir (i));
                if (v > bestV) { bestV = v; best = i; }
            }
            // début (onset) : premier échantillon à 30 % du pic dans les 10 ms qui précèdent
            long onset = best;
            for (long i = best - (long) (0.01 * fs); i <= best; ++i)
                if (std::abs (ir (i)) >= 0.3f * bestV) { onset = i; break; }
            m.ch[c].arrivalSec = (double) (onset - base) / fs;

            // bruit de fond (zone où la réverbération est éteinte)
            double nz = 0; int nn = 0;
            for (long i = best + (long) (1.6 * fs); i < best + (long) (3.0 * fs); ++i) { const double v = ir (i); nz += v * v; ++nn; }
            const double nrms = std::sqrt (nz / std::max (1, nn));
            m.ch[c].snrDb = (float) (20.0 * std::log10 ((bestV + 1e-20) / (nrms + 1e-20)));

            // fenêtre 0.5 s autour du son direct
            std::vector<cplx> x ((size_t) fftM), X ((size_t) fftM);
            for (int i = 0; i < pre + win; ++i)
            {
                double w = 1.0;
                if (i < pre) w = 0.5 - 0.5 * std::cos (kPiD * i / pre);
                const int fromEnd = pre + win - i;
                if (fromEnd < taper) w *= 0.5 - 0.5 * std::cos (kPiD * fromEnd / taper);
                x[(size_t) i] = (float) (ir (onset - pre + i) * w);
            }
            fftW.perform (x.data(), X.data(), false);
            std::vector<double> pw ((size_t) fftM / 2);
            for (size_t k = 0; k < pw.size(); ++k) pw[k] = std::norm (X[k]);
            m.ch[c].magDb = powerToGrid (pw, fs, fftM);
        }
        if (numSpk == 1) m.ch[1] = m.ch[0];

        const float snr = std::min (m.ch[0].snrDb, m.ch[1].snrDb);
        if (snr < 25.0f && m.warning.isEmpty())
            m.warning = juce::String (juce::CharPointer_UTF8 ("Mesure un peu bruit\xc3\xa9" "e (")) + juce::String (juce::roundToInt (snr)) + " dB) : monte le niveau du test ou fais moins de bruit.";
        m.ok = true;
        return m;
    }

    //==========================================================================
    // Courbes de micros (réponse approximative, en dB) : la mesure est corrigée de cette courbe
    inline float interpPoints (const std::vector<std::pair<float, float>>& pts, double f)
    {
        if (pts.empty()) return 0.0f;
        if (f <= pts.front().first) return pts.front().second;
        if (f >= pts.back().first)  return pts.back().second;
        for (size_t i = 1; i < pts.size(); ++i)
            if (f < pts[i].first)
            {
                const double t = std::log (f / pts[i - 1].first) / std::log (pts[i].first / pts[i - 1].first);
                return (float) (pts[i - 1].second + t * (pts[i].second - pts[i - 1].second));
            }
        return pts.back().second;
    }

    inline Curve micCurve (int type, const Curve& custom)
    {
        static const std::vector<std::pair<float, float>> condenser { { 20, -2.5f }, { 50, -0.8f }, { 100, 0 }, { 1000, 0 }, { 3000, 1.0f }, { 5000, 2.2f }, { 8000, 3.0f }, { 12000, 2.0f }, { 16000, 0.0f }, { 20000, -3.0f } };
        static const std::vector<std::pair<float, float>> sm58      { { 20, -14 }, { 50, -7 }, { 100, -2.5f }, { 200, 0 }, { 1000, 0 }, { 3000, 3.0f }, { 5000, 5.0f }, { 8000, 3.0f }, { 10000, 1.0f }, { 15000, -4.0f }, { 20000, -14.0f } };
        static const std::vector<std::pair<float, float>> sm7b      { { 20, -7 }, { 50, -2.5f }, { 100, 0 }, { 1000, 0 }, { 3000, 1.5f }, { 5000, 3.0f }, { 8000, 2.0f }, { 12000, 0.0f }, { 16000, -4.0f }, { 20000, -10.0f } };
        Curve c ((size_t) kGrid, 0.0f);
        if (type == 4 && (int) custom.size() == kGrid) return custom;
        const std::vector<std::pair<float, float>>* p = nullptr;
        if (type == 1) p = &condenser; else if (type == 2) p = &sm58; else if (type == 3) p = &sm7b;
        if (p != nullptr)
            for (int i = 0; i < kGrid; ++i) c[(size_t) i] = interpPoints (*p, gridFreq (i));
        return c;
    }

    // Fichier de calibration texte (format miniDSP/UMIK, REW) : "fréquence  dB [phase]"
    inline Curve loadCalFile (const juce::File& f, juce::String& err)
    {
        std::vector<std::pair<float, float>> pts;
        juce::StringArray lines;
        f.readLines (lines);
        for (auto& l : lines)
        {
            auto t = l.trim();
            if (t.isEmpty() || ! (juce::CharacterFunctions::isDigit (t[0]) || t[0] == '.')) continue;
            auto tok = juce::StringArray::fromTokens (t.replaceCharacter (',', ' ').replaceCharacter (';', ' '), " \t", "");
            tok.removeEmptyStrings();
            if (tok.size() < 2) continue;
            const float fr = tok[0].getFloatValue(), db = tok[1].getFloatValue();
            if (fr > 0 && (pts.empty() || fr > pts.back().first)) pts.push_back ({ fr, db });
        }
        if (pts.size() < 5) { err = juce::String (juce::CharPointer_UTF8 ("Fichier non reconnu (il faut des lignes \xc2\xab fr\xc3\xa9quence dB \xc2\xbb).")); return {}; }
        Curve c ((size_t) kGrid);
        for (int i = 0; i < kGrid; ++i) c[(size_t) i] = interpPoints (pts, gridFreq (i));
        return c;
    }

    //==========================================================================
    // Courbes cibles (dB, 0 = médiums)
    inline float targetAt (int type, double f)
    {
        auto lowShelf = [f] (double fc, double g) { const double x = std::log2 (f / fc); return g / (1.0 + std::exp (x * 3.0)); };
        auto tilt = [f] (double from, double dbPerOct) { return f > from ? dbPerOct * std::log2 (f / from) : 0.0; };
        switch (type)
        {
            case 1:  return (float) (lowShelf (110.0, 2.5) + tilt (1000.0, -0.8));   // Studio (pente douce)
            case 2:  return (float) (lowShelf (90.0, 4.5) + tilt (1500.0, -0.6));    // Basses+ (Afro / Rap)
            default: return 0.0f;                                                  // Plat
        }
    }

    //==========================================================================
    // Filtre en cloche (RBJ) : coefficients + module
    struct Band { float f = 1000, q = 1, g = 0; };
    struct Coefs { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

    inline Coefs peakCoefs (double fs, double f, double q, double g)
    {
        Coefs c;
        if (std::abs (g) < 0.01) return c;
        f = juce::jlimit (10.0, fs * 0.45, f);
        const double A = std::pow (10.0, g / 40.0), w = 2.0 * kPiD * f / fs, cs = std::cos (w), al = std::sin (w) / (2.0 * q);
        const double a0 = 1 + al / A;
        c.b0 = (1 + al * A) / a0; c.b1 = -2 * cs / a0; c.b2 = (1 - al * A) / a0;
        c.a1 = -2 * cs / a0;      c.a2 = (1 - al / A) / a0;
        return c;
    }

    inline double magDb (const Coefs& c, double f, double fs)
    {
        const double w = 2.0 * kPiD * f / fs;
        const std::complex<double> z1 = std::polar (1.0, -w), z2 = z1 * z1;
        const auto num = c.b0 + c.b1 * z1 + c.b2 * z2, den = 1.0 + c.a1 * z1 + c.a2 * z2;
        return 20.0 * std::log10 (std::abs (num) / std::abs (den) + 1e-30);
    }

    inline Curve bandsCurve (const std::vector<Band>& bands, double fs)
    {
        Curve c ((size_t) kGrid, 0.0f);
        for (auto& b : bands)
        {
            const auto k = peakCoefs (fs, b.f, b.q, b.g);
            for (int i = 0; i < kGrid; ++i)
            {
                const double f = gridFreq (i);
                if (f < fs * 0.49) c[(size_t) i] += (float) magDb (k, f, fs);
            }
        }
        return c;
    }

    inline Curve smoothGrid (const Curve& in, double octaves)
    {
        const int h = std::max (1, (int) std::round (pointsPerOctave() * octaves * 0.5));
        Curve out (in.size());
        for (int i = 0; i < (int) in.size(); ++i)
        {
            double s = 0; int n = 0;
            for (int j = std::max (0, i - h); j <= std::min ((int) in.size() - 1, i + h); ++j) { s += in[(size_t) j]; ++n; }
            out[(size_t) i] = (float) (s / n);
        }
        return out;
    }

    struct FitParams
    {
        double lo = 40, hi = 500, maxBoost = 3, maxCut = 12, amount = 1.0;
        int maxBands = 10;
    };

    // Ajustement automatique : place des cloches là où l'écart à la cible est le plus grand.
    // Les creux étroits (annulations de la pièce) ne sont pas remontés : on ne boost que les creux larges.
    inline std::vector<Band> fitBands (const Curve& meas, const Curve& target, const FitParams& fp, double fs)
    {
        std::vector<Band> bands;
        const Curve wide = smoothGrid (meas, 2.0 / 3.0);
        const double hi = std::min (fp.hi, fs * 0.42);
        Curve want ((size_t) kGrid, 0.0f);
        std::vector<char> use ((size_t) kGrid, 0);
        for (int i = 0; i < kGrid; ++i)
        {
            const double f = gridFreq (i);
            if (f < fp.lo || f > hi) continue;
            use[(size_t) i] = 1;
            double d = target[(size_t) i] - meas[(size_t) i];
            if (d > 0) d = std::max (0.0, std::min (d, (double) target[(size_t) i] - wide[(size_t) i]));
            want[(size_t) i] = (float) juce::jlimit (-fp.maxCut, fp.maxBoost, d * fp.amount);
        }

        Curve corr ((size_t) kGrid, 0.0f);
        for (int b = 0; b < fp.maxBands; ++b)
        {
            int at = -1; double best = 0;
            for (int i = 0; i < kGrid; ++i)
            {
                if (! use[(size_t) i]) continue;
                const double r = want[(size_t) i] - corr[(size_t) i];
                const double s = std::abs (r) * (r < 0 ? 1.0 : 0.7);
                if (s > best) { best = s; at = i; }
            }
            if (at < 0) break;
            const double r = want[(size_t) at] - corr[(size_t) at];
            if (std::abs (r) < 0.8) break;
            int il = at, ir = at;
            auto sameHalf = [&] (int i) { const double v = want[(size_t) i] - corr[(size_t) i]; return use[(size_t) i] && v * r > 0 && std::abs (v) > std::abs (r) * 0.5; };
            while (il > 0 && sameHalf (il - 1)) --il;
            while (ir < kGrid - 1 && sameHalf (ir + 1)) ++ir;
            const double bw = std::max (0.08, std::log2 (gridFreq (ir + 1) / gridFreq (std::max (0, il - 1))));
            const double p2 = std::pow (2.0, bw);
            double q = std::sqrt (p2) / (p2 - 1.0);
            q = juce::jlimit (0.6, r < 0 ? 10.0 : 3.0, q);
            bands.push_back ({ (float) gridFreq (at), (float) q, (float) r });

            // affinage des gains de toutes les cloches
            for (int it = 0; it < 12; ++it)
            {
                corr = bandsCurve (bands, fs);
                for (auto& bd : bands)
                {
                    const int gi = juce::jlimit (0, kGrid - 1, (int) std::round (gridPos (bd.f)));
                    bd.g = (float) juce::jlimit (-fp.maxCut - 3.0, fp.maxBoost, bd.g + 0.6 * (want[(size_t) gi] - corr[(size_t) gi]));
                }
            }
            corr = bandsCurve (bands, fs);
        }
        bands.erase (std::remove_if (bands.begin(), bands.end(), [] (const Band& b) { return std::abs (b.g) < 0.3f; }), bands.end());
        return bands;
    }

    inline double meanRange (const Curve& c, double f1, double f2)
    {
        double s = 0; int n = 0;
        for (int i = 0; i < kGrid; ++i) { const double f = gridFreq (i); if (f >= f1 && f <= f2) { s += c[(size_t) i]; ++n; } }
        return n ? s / n : 0.0;
    }

    //==========================================================================
    // Bruit rose (Paul Kellet)
    struct Pink
    {
        float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
        juce::Random rng { 7 };
        float next() noexcept
        {
            const float w = rng.nextFloat() * 2.0f - 1.0f;
            b0 = 0.99886f * b0 + w * 0.0555179f; b1 = 0.99332f * b1 + w * 0.0750759f;
            b2 = 0.96900f * b2 + w * 0.1538520f; b3 = 0.86650f * b3 + w * 0.3104856f;
            b4 = 0.55000f * b4 + w * 0.5329522f; b5 = -0.7616f * b5 - w * 0.0168980f;
            const float o = b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362f;
            b6 = w * 0.115926f;
            return o * 0.11f;
        }
    };
}
