// BAKR REPAIR - moteurs DSP
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>

namespace bakr
{
static inline float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }
static inline float gainToDb (float g)  { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }

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

    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }

    struct W { double c, s, a; };
    static W w (double f, double fs, double q)
    {
        f = std::clamp (f, 5.0, fs * 0.49);
        const double w0 = 2.0 * juce::MathConstants<double>::pi * f / fs;
        return { std::cos (w0), std::sin (w0), std::sin (w0) / (2.0 * q) };
    }

    void lowPass  (double f, double fs, double q = 0.7071) { auto k = w (f, fs, q); set ((1 - k.c) / 2, 1 - k.c, (1 - k.c) / 2, 1 + k.a, -2 * k.c, 1 - k.a); }
    void highPass (double f, double fs, double q = 0.7071) { auto k = w (f, fs, q); set ((1 + k.c) / 2, -(1 + k.c), (1 + k.c) / 2, 1 + k.a, -2 * k.c, 1 - k.a); }
    void notch    (double f, double fs, double q)          { auto k = w (f, fs, q); set (1, -2 * k.c, 1, 1 + k.a, -2 * k.c, 1 - k.a); }
    void bandPass (double f, double fs, double q)          { auto k = w (f, fs, q); set (k.a, 0, -k.a, 1 + k.a, -2 * k.c, 1 - k.a); }

    void peak (double f, double fs, double q, double gainDb)
    {
        auto k = w (f, fs, q); const double A = std::pow (10.0, gainDb / 40.0);
        set (1 + k.a * A, -2 * k.c, 1 - k.a * A, 1 + k.a / A, -2 * k.c, 1 - k.a / A);
    }
    void lowShelf (double f, double fs, double gainDb)
    {
        auto k = w (f, fs, 0.7071); const double A = std::pow (10.0, gainDb / 40.0), sA = 2 * std::sqrt (A) * k.a;
        set (A * ((A + 1) - (A - 1) * k.c + sA), 2 * A * ((A - 1) - (A + 1) * k.c), A * ((A + 1) - (A - 1) * k.c - sA),
             (A + 1) + (A - 1) * k.c + sA, -2 * ((A - 1) + (A + 1) * k.c), (A + 1) + (A - 1) * k.c - sA);
    }
    void highShelf (double f, double fs, double gainDb)
    {
        auto k = w (f, fs, 0.7071); const double A = std::pow (10.0, gainDb / 40.0), sA = 2 * std::sqrt (A) * k.a;
        set (A * ((A + 1) + (A - 1) * k.c + sA), -2 * A * ((A - 1) + (A + 1) * k.c), A * ((A + 1) + (A - 1) * k.c - sA),
             (A + 1) - (A - 1) * k.c + sA, 2 * ((A - 1) - (A + 1) * k.c), (A + 1) - (A - 1) * k.c - sA);
    }
};

//==============================================================================
struct DelayLine
{
    std::vector<float> b; int len = 0, p = 0;
    void prepare (int L) { len = std::max (0, L); b.assign ((size_t) std::max (1, len), 0.0f); p = 0; }
    void reset() { std::fill (b.begin(), b.end(), 0.0f); p = 0; }
    inline float process (float x) noexcept
    {
        if (len == 0) return x;
        const float y = b[(size_t) p]; b[(size_t) p] = x;
        if (++p >= len) p = 0;
        return y;
    }
};

//==============================================================================
// De-clip (reconstruction des plateaux écrêtés) + De-click, avec look-ahead.
class LookaheadRepair
{
public:
    static constexpr int size = 512, mask = size - 1, latency = 256;

    void reset()
    {
        buf.fill (0); clipAdd.fill (0); clickRem.fill (0);
        w = 0; clipGuard = 0; clickGuard = 0; env = 1.0e-4f;
    }

    inline float process (float x, bool clipOn, float thrLin, bool clickOn,
                          float& clipAdded, float& clickRemoved, int& clicks) noexcept
    {
        const int wi = (int) (w & mask);
        buf[(size_t) wi] = x; clipAdd[(size_t) wi] = 0; clickRem[(size_t) wi] = 0;

        // ---- De-clip : point d'analyse w-140, peut regarder jusqu'à w-2 (plateaux jusqu'à 130 échantillons)
        const int64_t c = w - 140;
        if (clipOn && c > clipGuard && c > 4)
        {
            const float t = thrLin * 0.985f;
            const float v = at (c), prev = at (c - 1);
            if (std::abs (v) >= t && std::abs (prev) < t)
            {
                const float sgn = v > 0 ? 1.0f : -1.0f;
                int64_t e = c;
                while (e < w - 2 && std::abs (at (e)) >= t && at (e) * sgn > 0) ++e;
                const int len = (int) (e - c);
                if (e < w - 2 && len >= 2 && len <= 130)
                {
                    const float p0 = at (c - 1), p1 = at (e);
                    const float L  = (float) (len + 1);
                    const float m0 = (p0 - at (c - 2)) * L, m1 = (at (e + 1) - p1) * L;
                    for (int i = 1; i <= len; ++i)
                    {
                        const float s = (float) i / L, s2 = s * s, s3 = s2 * s;
                        float y = (2 * s3 - 3 * s2 + 1) * p0 + (s3 - 2 * s2 + s) * m0
                                + (-2 * s3 + 3 * s2) * p1 + (s3 - s2) * m1;
                        const int64_t idx = c - 1 + i;
                        const float o = at (idx);
                        if (y * sgn < o * sgn) y = o;
                        if (std::abs (y) > 2.5f * t) y = sgn * 2.5f * t;
                        clipAdd[(size_t) (idx & mask)] += y - o;
                        buf[(size_t) (idx & mask)] = y;
                    }
                    clipGuard = e;
                }
            }
        }

        // ---- De-click : point d'analyse w-160 (déjà passé par le de-clip)
        const int64_t k = w - 160;
        if (k > 8)
        {
            const float e2 = std::abs (at (k) - 2.0f * at (k - 1) + at (k - 2));
            if (clickOn && k > clickGuard && e2 > 12.0f * env + 0.003f)
            {
                const int64_t a = k - 4, b = k + 8;
                const float ya = at (a - 1), yb = at (b + 1);
                const float span = (float) (b - a + 2);
                for (int64_t i = a; i <= b; ++i)
                {
                    const float tt = (float) (i - (a - 1)) / span;
                    const float y = ya + (yb - ya) * tt;
                    clickRem[(size_t) (i & mask)] += at (i) - y;
                    buf[(size_t) (i & mask)] = y;
                }
                clickGuard = b + 1;
                ++clicks;
            }
            else
            {
                env = 0.995f * env + 0.005f * e2;
            }
        }

        const int o = (int) ((w - latency) & mask);
        clipAdded = clipAdd[(size_t) o];
        clickRemoved = clickRem[(size_t) o];
        const float out = buf[(size_t) o];
        ++w;
        return out;
    }

private:
    inline float at (int64_t i) const noexcept { return buf[(size_t) (i & mask)]; }
    std::array<float, size> buf {}, clipAdd {}, clickRem {};
    int64_t w = 0, clipGuard = 0, clickGuard = 0;
    float env = 1.0e-4f;
};

//==============================================================================
// De-noise + De-reverb spectral (STFT 1024, recouvrement 75 %)
class SpectralCleaner
{
public:
    static constexpr int order = 10, N = 1 << order, hop = N / 4, bins = N / 2 + 1, maxD = 32;
    static constexpr int latency = N;

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        window.resize (N);
        for (int i = 0; i < N; ++i)
            window[(size_t) i] = std::sqrt (0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) N));
        inFifo.assign (N, 0); outAcc.assign (N, 0); remAcc.assign (N, 0);
        fftBuf.assign (2 * N, 0); remBuf.assign (2 * N, 0);
        sP.assign (bins, 0); minP.assign (bins, 0); gRaw.assign (bins, 1); gs.assign (bins, 1);
        hist.assign ((size_t) maxD * bins, 0);
        D = std::clamp ((int) std::round (0.05 * fs / hop), 2, maxD - 1);
        reset();
    }

    void reset()
    {
        std::fill (inFifo.begin(), inFifo.end(), 0.0f);
        std::fill (outAcc.begin(), outAcc.end(), 0.0f);
        std::fill (remAcc.begin(), remAcc.end(), 0.0f);
        std::fill (sP.begin(), sP.end(), 0.0f);
        std::fill (minP.begin(), minP.end(), 0.0f);
        std::fill (gs.begin(), gs.end(), 1.0f);
        std::fill (hist.begin(), hist.end(), 0.0f);
        pos = cnt = histPos = 0; frames = 0;
    }

    void setParams (float denoise, float dereverb, bool enabled, bool instrument, bool wantRemoved)
    {
        dn = enabled ? denoise : 0.0f;
        dr = enabled ? dereverb : 0.0f;
        needRemoved = wantRemoved;
        const double framesPerSec = fs / hop;
        rise = (float) std::pow (10.0, (instrument ? 1.5 : 4.0) / 10.0 / framesPerSec);
        overN  = 1.0f + 2.5f * dn;
        floorN = dbToGain (-30.0f * dn);
        const double t60 = 0.3 + 1.0 * dr;
        revDecay = (float) std::pow (10.0, -6.0 * D * hop / (t60 * fs));
        overR  = 0.8f + 1.2f * dr;
        floorR = dbToGain (-20.0f * dr);
    }

    inline float process (float x, float& removed) noexcept
    {
        inFifo[(size_t) pos] = x;
        const float y = outAcc[(size_t) pos];
        removed = remAcc[(size_t) pos];
        outAcc[(size_t) pos] = 0; remAcc[(size_t) pos] = 0;
        pos = (pos + 1) & (N - 1);
        if (++cnt >= hop) { cnt = 0; processFrame(); }
        return y;
    }

private:
    void processFrame() noexcept
    {
        for (int i = 0; i < N; ++i) fftBuf[(size_t) i] = inFifo[(size_t) ((pos + i) & (N - 1))] * window[(size_t) i];
        std::fill (fftBuf.begin() + N, fftBuf.end(), 0.0f);
        fft.performRealOnlyForwardTransform (fftBuf.data());
        ++frames;

        const bool active = (dn > 0.0001f || dr > 0.0001f);
        const int pastRow = (histPos - D + maxD) % maxD;

        for (int k = 0; k < bins; ++k)
        {
            const float re = fftBuf[(size_t) (2 * k)], im = fftBuf[(size_t) (2 * k + 1)];
            const float P = re * re + im * im + 1.0e-20f;
            sP[(size_t) k] = 0.6f * sP[(size_t) k] + 0.4f * P;
            if (frames <= 2 || sP[(size_t) k] < minP[(size_t) k]) minP[(size_t) k] = sP[(size_t) k];
            else minP[(size_t) k] *= rise;

            float g = 1.0f;
            if (active)
            {
                if (dn > 0.0001f)
                {
                    const float noise = minP[(size_t) k] * 3.0f;
                    g *= std::max (1.0f - overN * noise / P, floorN);
                }
                if (dr > 0.0001f)
                {
                    const float late = revDecay * hist[(size_t) (pastRow * bins + k)];
                    g *= std::max (1.0f - overR * late / P, floorR);
                }
            }
            gRaw[(size_t) k] = g;
            hist[(size_t) (histPos * bins + k)] = sP[(size_t) k];
        }
        histPos = (histPos + 1) % maxD;

        if (needRemoved) std::copy (fftBuf.begin(), fftBuf.end(), remBuf.begin());

        for (int k = 0; k < bins; ++k)
        {
            float g = 1.0f;
            if (active)
            {
                const float gl = gRaw[(size_t) std::max (0, k - 1)], gr = gRaw[(size_t) std::min (bins - 1, k + 1)];
                const float gf = 0.25f * gl + 0.5f * gRaw[(size_t) k] + 0.25f * gr;
                float& s = gs[(size_t) k];
                s += (gf > s ? 0.7f : 0.4f) * (gf - s);
                g = s;
            }
            else gs[(size_t) k] = 1.0f;

            applyGain (fftBuf, k, g);
            if (needRemoved) applyGain (remBuf, k, 1.0f - g);
        }

        fft.performRealOnlyInverseTransform (fftBuf.data());
        for (int i = 0; i < N; ++i)
            outAcc[(size_t) ((pos + i) & (N - 1))] += fftBuf[(size_t) i] * window[(size_t) i] * olaScale;

        if (needRemoved)
        {
            fft.performRealOnlyInverseTransform (remBuf.data());
            for (int i = 0; i < N; ++i)
                remAcc[(size_t) ((pos + i) & (N - 1))] += remBuf[(size_t) i] * window[(size_t) i] * olaScale;
        }
    }

    static inline void applyGain (std::vector<float>& b, int k, float g) noexcept
    {
        b[(size_t) (2 * k)] *= g; b[(size_t) (2 * k + 1)] *= g;
        if (k > 0 && k < N / 2) { const int j = N - k; b[(size_t) (2 * j)] *= g; b[(size_t) (2 * j + 1)] *= g; }
    }

    juce::dsp::FFT fft { order };
    double fs = 48000;
    std::vector<float> window, inFifo, outAcc, remAcc, fftBuf, remBuf, sP, minP, gRaw, gs, hist;
    int pos = 0, cnt = 0, histPos = 0, D = 9;
    int64_t frames = 0;
    float dn = 0, dr = 0, rise = 1.002f, overN = 1, floorN = 1, revDecay = 0.5f, overR = 1, floorR = 1;
    bool needRemoved = false;
public:
    float olaScale = 0.5f;
};

//==============================================================================
// Learn : écoute ~4 s de signal et propose des réglages
struct LearnResult
{
    float denoise = 0.3f, dereverb = 0.3f, tone = 0.0f, dsThresh = -24.0f, dsShape = 0.5f, dcThresh = 0.0f;
    bool dehum = false, hum60 = false, declick = false, locut = false, hicut = false, dsOn = true, dcOn = true;
};

class Learner
{
public:
    static constexpr int order = 13, N = 1 << order, hop = 2048;

    void prepare (double sampleRate)
    {
        fs = sampleRate;
        ring.assign (N, 0); fftBuf.assign (2 * N, 0); avgSpec.assign (N / 2, 0);
        win.resize (N);
        for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) N);
        energies.reserve (1024); sibs.reserve (1024); scratch.reserve (1024); decays.reserve (512);
        sibBp.bandPass (6500.0, fs, 1.2);
        active = false; requested = false;
    }

    void request (bool instrumentMode) { instrument = instrumentMode; done = false; progress = 0; requested = true; }
    void cancel() { cancelReq = true; }

    void push (const float* l, const float* r, int n) noexcept
    {
        if (requested.exchange (false)) begin();
        if (cancelReq.exchange (false)) { active = false; progress = 0; }
        if (! active) return;

        for (int i = 0; i < n && active; ++i)
        {
            const float m = r != nullptr ? 0.5f * (l[i] + r[i]) : l[i];
            ring[(size_t) rp] = m; rp = (rp + 1) & (N - 1);
            sumSq += m * m;

            // plateaux d'écrêtage
            const float am = std::abs (m);
            if (std::abs (m - prevM) < 1.0e-4f && am > 0.25f) ++runLen;
            else { if (runLen >= 2) { ++clipRuns; clipLevel = std::max (clipLevel, std::abs (prevM)); } runLen = 0; }

            // clics
            const float e2 = std::abs (m - 2.0f * prevM + prevM2);
            if (e2 > 12.0f * clickEnv + 0.003f) { if (sinceClick > 64) ++clicks; sinceClick = 0; }
            else clickEnv = 0.995f * clickEnv + 0.005f * e2;
            ++sinceClick;
            prevM2 = prevM; prevM = m;

            sibMax = std::max (sibMax, std::abs (sibBp.process (m)));

            if (++cnt >= hop) { cnt = 0; analyse(); }
        }
    }

    std::atomic<bool> requested { false }, cancelReq { false }, active { false }, done { false };
    std::atomic<float> progress { 0 };
    LearnResult result;

private:
    void begin()
    {
        std::fill (ring.begin(), ring.end(), 0.0f); std::fill (avgSpec.begin(), avgSpec.end(), 0.0f);
        energies.clear(); sibs.clear(); decays.clear();
        rp = cnt = 0; sumSq = 0; prevM = prevM2 = 0; runLen = clipRuns = 0; clipLevel = 0;
        clicks = 0; clickEnv = 1.0e-4f; sinceClick = 1000; sibMax = 0; signalFrames = 0; totalFrames = 0;
        sibBp.reset(); active = true; progress = 0;
    }

    void analyse() noexcept
    {
        const float E = 10.0f * std::log10 (sumSq / hop + 1.0e-12f);
        sumSq = 0; ++totalFrames;
        if (energies.size() < energies.capacity()) energies.push_back (E);
        if (E > -60.0f)
        {
            ++signalFrames;
            if (sibs.size() < sibs.capacity()) sibs.push_back (gainToDb (sibMax));
            for (int i = 0; i < N; ++i) fftBuf[(size_t) i] = ring[(size_t) ((rp + i) & (N - 1))] * win[(size_t) i];
            std::fill (fftBuf.begin() + N, fftBuf.end(), 0.0f);
            fft.performFrequencyOnlyForwardTransform (fftBuf.data());
            for (int k = 0; k < N / 2; ++k) avgSpec[(size_t) k] += fftBuf[(size_t) k] * fftBuf[(size_t) k];
        }
        sibMax = 0;

        const int target = (int) (4.0 * fs / hop), maxFrames = (int) (15.0 * fs / hop);
        progress = std::min (1.0f, (float) signalFrames / (float) target);
        if (signalFrames >= target) finish();
        else if (totalFrames >= maxFrames) { if (signalFrames > target / 4) finish(); else { active = false; progress = 0; } }
    }

    float percentile (const std::vector<float>& v, float p)
    {
        if (v.empty()) return -100.0f;
        scratch.assign (v.begin(), v.end());
        auto idx = (size_t) std::clamp ((int) (p * (float) (scratch.size() - 1)), 0, (int) scratch.size() - 1);
        std::nth_element (scratch.begin(), scratch.begin() + (long) idx, scratch.end());
        return scratch[idx];
    }

    float bandDb (double f1, double f2) const
    {
        const double binHz = fs / N; double s = 0; int n = 0;
        for (int k = std::max (1, (int) (f1 / binHz)); k <= std::min (N / 2 - 1, (int) (f2 / binHz)); ++k) { s += avgSpec[(size_t) k]; ++n; }
        return 10.0f * (float) std::log10 (s / std::max (1, n) + 1.0e-20);
    }

    float humScore (double f0) const
    {
        const double binHz = fs / N; float sc[5] {};
        for (int h = 1; h <= 4; ++h)
        {
            const int b = (int) std::round (h * f0 / binHz);
            float pk = 0; for (int j = -1; j <= 1; ++j) pk = std::max (pk, avgSpec[(size_t) std::clamp (b + j, 1, N / 2 - 1)]);
            double nb = 0; int nn = 0;
            for (int j = (int) (12 / binHz) + 1; j <= (int) (24 / binHz) + 1; ++j)
            { nb += avgSpec[(size_t) std::clamp (b + j, 1, N / 2 - 1)] + avgSpec[(size_t) std::clamp (b - j, 1, N / 2 - 1)]; nn += 2; }
            sc[h] = 10.0f * (float) std::log10 ((pk + 1.0e-20) / (nb / std::max (1, nn) + 1.0e-20));
        }
        // le fondamental du secteur compte le plus
        return 0.6f * sc[1] + 0.4f * std::max ({ sc[2], sc[3], sc[4] });
    }

    void finish()
    {
        LearnResult r;
        const float noise = percentile (energies, 0.10f), sig = percentile (energies, 0.90f);
        const float snr = sig - noise;
        r.denoise = std::clamp ((50.0f - snr) / 40.0f, 0.05f, 0.85f) * (instrument ? 0.7f : 1.0f);

        // décroissance -> estimation du T60
        decays.clear();
        for (size_t t = 1; t + 3 < energies.size(); ++t)
        {
            // on saute la première chute (fin du son direct) et on mesure la queue sur 3 trames
            const float e0 = energies[t - 1];
            if (e0 >= sig - 12.0f && energies[t] < e0 - 1.0f && energies[t + 1] < energies[t] && energies[t + 2] < energies[t + 1]
                && energies[t + 3] < energies[t + 2] && energies[t + 3] > noise + 3.0f)
                decays.push_back ((energies[t + 1] - energies[t + 3]) / 2.0f);
        }
        const float fps = (float) (fs / hop);
        float t60 = 0.3f;
        if (! decays.empty()) t60 = 60.0f / (std::max (0.5f, percentile (decays, 0.5f)) * fps);
        r.dereverb = decays.size() >= 3 ? std::clamp ((t60 - 0.2f) / 0.9f, 0.05f, 0.9f) : 0.3f;

        const float h50 = humScore (50.0), h60 = humScore (60.0);
        r.dehum = std::max (h50, h60) > 8.0f;
        {
            // 50 ou 60 Hz : on compare l'énergie exacte du fondamental et du 2e harmonique
            const double binHz = fs / N;
            auto at = [&] (double f) { return avgSpec[(size_t) std::clamp ((int) std::round (f / binHz), 1, N / 2 - 1)]; };
            r.hum60 = (at (60) + at (120)) > (at (50) + at (100));
        }

        const float secs = (float) totalFrames * hop / (float) fs;
        r.declick = (float) clicks / std::max (1.0f, secs) > 0.5f;

        const float low = bandDb (150, 600), high = bandDb (2500, 8000), rumble = bandDb (30, 80);
        const float targetTilt = instrument ? -14.0f : -16.0f;
        r.tone = std::clamp ((targetTilt - (high - low)) / 12.0f, -1.0f, 1.0f) * 0.6f;
        r.locut = ! instrument || (rumble - low) > -10.0f;
        r.hicut = false;

        r.dsOn = ! instrument;
        r.dsThresh = std::clamp (percentile (sibs, 0.95f) - 7.0f, -50.0f, -6.0f);
        r.dsShape = 0.5f;

        r.dcOn = true;
        r.dcThresh = clipRuns > 3 ? std::clamp (gainToDb (clipLevel) - 0.3f, -24.0f, 0.0f) : 0.0f;

        result = r;
        active = false; progress = 1.0f; done = true;
    }

    juce::dsp::FFT fft { order };
    double fs = 48000;
    std::vector<float> ring, fftBuf, avgSpec, win, energies, sibs, scratch, decays;
    Biquad sibBp;
    bool instrument = false;
    int rp = 0, cnt = 0, runLen = 0, clipRuns = 0, clicks = 0, sinceClick = 1000, signalFrames = 0, totalFrames = 0;
    float sumSq = 0, prevM = 0, prevM2 = 0, clipLevel = 0, clickEnv = 1.0e-4f, sibMax = 0;
};

} // namespace bakr
