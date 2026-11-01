#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

// Chain7st - bloques de DSP (todo header-only, sin asignaciones en el hilo de audio)
namespace c7
{
constexpr double kPi = 3.14159265358979323846;

//==============================================================================
// Biquad RBJ (estado en double, hasta 2 canales)
struct Biquad
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double z1[2] = { 0.0, 0.0 }, z2[2] = { 0.0, 0.0 };

    void reset() noexcept { z1[0] = z1[1] = z2[0] = z2[1] = 0.0; }

    inline float process (float in, int ch) noexcept
    {
        const double x = (double) in;
        const double y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return (float) y;
    }

    void highPass (double fs, double f, double q) noexcept
    {
        const double w0 = 2.0 * kPi * clampF (fs, f) / fs, c = std::cos (w0), al = std::sin (w0) / (2.0 * q);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }

    void bandPass (double fs, double f, double q) noexcept
    {
        const double w0 = 2.0 * kPi * clampF (fs, f) / fs, c = std::cos (w0), al = std::sin (w0) / (2.0 * q);
        set (al, 0.0, -al, 1 + al, -2 * c, 1 - al);
    }

    void peak (double fs, double f, double q, double gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * clampF (fs, f) / fs, c = std::cos (w0), al = std::sin (w0) / (2.0 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }

    void lowShelf (double fs, double f, double gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * clampF (fs, f) / fs, c = std::cos (w0), s = std::sin (w0);
        const double al = s / 2.0 * std::sqrt (2.0);              // slope S = 1
        const double t = 2.0 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + t),
             2 * A * ((A - 1) - (A + 1) * c),
             A * ((A + 1) - (A - 1) * c - t),
             (A + 1) + (A - 1) * c + t,
             -2 * ((A - 1) + (A + 1) * c),
             (A + 1) + (A - 1) * c - t);
    }

    void highShelf (double fs, double f, double gainDb) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * kPi * clampF (fs, f) / fs, c = std::cos (w0), s = std::sin (w0);
        const double al = s / 2.0 * std::sqrt (2.0);
        const double t = 2.0 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + t),
             -2 * A * ((A - 1) + (A + 1) * c),
             A * ((A + 1) + (A - 1) * c - t),
             (A + 1) - (A - 1) * c + t,
             2 * ((A - 1) - (A + 1) * c),
             (A + 1) - (A - 1) * c - t);
    }

private:
    static double clampF (double fs, double f) noexcept { return std::min (std::max (f, 10.0), fs * 0.45); }

    void set (double B0, double B1, double B2, double A0, double A1, double A2) noexcept
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
};

//==============================================================================
// Auto-Pitch: detector YIN (a ~12 kHz) + pitch shifter de doble tap sincronizado con el periodo.
class PitchCorrector
{
public:
    struct Params
    {
        int   key      = 0;     // 0 = C ... 11 = B
        int   scale    = 0;     // 0 = Mayor, 1 = Menor, 2 = Cromatica
        float speedMs  = 0.f;   // tiempo de retencion (0 = instantaneo)
        float amount   = 0.f;   // 0..1
    };

    void prepare (double fs)
    {
        sr     = fs;
        decim  = std::max (1, (int) std::lround (fs / 12000.0));
        fsD    = fs / decim;
        tauMin = std::max (2, (int) std::floor (fsD / 900.0));
        tauMax = (int) std::ceil (fsD / 70.0) + 2;

        yin.assign ((size_t) (kWin + tauMax + 2), 0.f);
        dn.assign ((size_t) (tauMax + 2), 1.f);
        ring.fill (0.f);
        ringWp = 0; decAcc = 0.f; decCount = 0; hopCount = 0;

        int bufSize = 1;
        while (bufSize < (int) (fs * 0.12)) bufSize <<= 1;
        for (auto& b : delayBuf) b.assign ((size_t) bufSize, 0.f);
        mask = bufSize - 1;
        wp   = 0;

        gSamples = gTarget = (float) (0.020 * fs);
        d = gSamples * 0.5f;
        targetRatio = ratioSm = 1.f;
        corr = 0.f;
        unvoicedHops = 1000;
        hist = { 0.f, 0.f, 0.f };
        histIdx = 0;

        ratioCoef  = (float) (1.0 - std::exp (-1.0 / (0.004 * fs)));
        gCoef      = (float) (1.0 - std::exp (-1.0 / (0.060 * fs)));
        hopSeconds = (double) (kHop * decim) / fs;
    }

    void process (float* const* data, int numCh, int numSamples, const Params& p) noexcept
    {
        numCh = std::min (numCh, 2);
        cur = p;

        for (int i = 0; i < numSamples; ++i)
        {
            float mono = 0.f;
            for (int ch = 0; ch < numCh; ++ch)
            {
                delayBuf[(size_t) ch][(size_t) wp] = data[ch][i];
                mono += data[ch][i];
            }
            mono /= (float) numCh;

            // diezmado + analisis periodico
            decAcc += mono;
            if (++decCount >= decim)
            {
                ring[(size_t) ringWp] = decAcc / (float) decim;
                ringWp = (ringWp + 1) & (kRing - 1);
                decAcc = 0.f; decCount = 0;
                if (++hopCount >= kHop) { hopCount = 0; analyse(); }
            }

            ratioSm  += (targetRatio - ratioSm) * ratioCoef;
            gSamples += (gTarget - gSamples) * gCoef;

            float inc = 1.f - ratioSm;
            if (std::abs (inc) < 1.0e-4f)        // sin correccion: volver a la posicion "limpia" (solo tap 1)
                inc = std::min (0.02f, std::max (-0.02f, gSamples * 0.5f - d));
            d += inc;
            while (d >= gSamples) d -= gSamples;
            while (d < 0.f)       d += gSamples;

            const float w1 = 0.5f - 0.5f * std::cos ((float) (2.0 * kPi) * d / gSamples);
            float d2 = d + 0.5f * gSamples;
            if (d2 >= gSamples) d2 -= gSamples;

            for (int ch = 0; ch < numCh; ++ch)
                data[ch][i] = w1 * readTap (ch, d + 1.f) + (1.f - w1) * readTap (ch, d2 + 1.f);

            wp = (wp + 1) & mask;
        }
    }

private:
    static constexpr int kWin  = 512;    // ventana de analisis (muestras diezmadas)
    static constexpr int kHop  = 64;     // salto entre analisis (muestras diezmadas)
    static constexpr int kRing = 4096;

    inline float readTap (int ch, float delay) const noexcept
    {
        const float pos = (float) wp - delay;
        const int   i0  = (int) std::floor (pos);
        const float fr  = pos - (float) i0;
        const auto& b   = delayBuf[(size_t) ch];
        return b[(size_t) (i0 & mask)] * (1.f - fr) + b[(size_t) ((i0 + 1) & mask)] * fr;
    }

    static float snapToScale (float midi, int key, int scale) noexcept
    {
        static const bool major[12] = { 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1 };
        static const bool minor[12] = { 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0 };
        if (scale >= 2) return std::round (midi);

        float best = midi, bestD = 1.0e9f;
        const int base = (int) std::floor (midi);
        for (int n = base - 2; n <= base + 3; ++n)
        {
            const int pc = (((n - key) % 12) + 12) % 12;
            const bool ok = (scale == 0) ? major[pc] : minor[pc];
            if (ok)
            {
                const float dist = std::abs ((float) n - midi);
                if (dist < bestD) { bestD = dist; best = (float) n; }
            }
        }
        return best;
    }

    void analyse() noexcept
    {
        const int N    = kWin + tauMax;
        const int base = tauMax;
        for (int k = 0; k < N; ++k)
            yin[(size_t) k] = ring[(size_t) ((ringWp - N + k) & (kRing - 1))];

        double energy = 0.0;
        for (int j = 0; j < kWin; ++j) { const double v = yin[(size_t) (base + j)]; energy += v * v; }
        const bool loudEnough = std::sqrt (energy / kWin) > 0.003;

        bool  voiced = false;
        float f0 = 0.f;

        if (loudEnough)
        {
            double running = 0.0;
            dn[0] = 1.f;
            for (int tau = 1; tau <= tauMax; ++tau)
            {
                double sum = 0.0;
                for (int j = 0; j < kWin; ++j)
                {
                    const double df = (double) yin[(size_t) (base + j)] - (double) yin[(size_t) (base + j - tau)];
                    sum += df * df;
                }
                running += sum;
                dn[(size_t) tau] = running > 1.0e-12 ? (float) (sum * tau / running) : 1.f;
            }

            int found = -1, gminTau = -1;
            float gmin = 1.0e9f;
            for (int tau = tauMin; tau <= tauMax - 1; ++tau)
            {
                if (dn[(size_t) tau] < 0.15f)
                {
                    while (tau + 1 <= tauMax - 1 && dn[(size_t) tau + 1] < dn[(size_t) tau]) ++tau;
                    found = tau;
                    break;
                }
                if (dn[(size_t) tau] < gmin) { gmin = dn[(size_t) tau]; gminTau = tau; }
            }
            if (found < 0 && gmin < 0.30f) found = gminTau;

            if (found > 1 && found < tauMax)
            {
                const float s0 = dn[(size_t) found - 1], s1 = dn[(size_t) found], s2 = dn[(size_t) found + 1];
                const float den = 2.f * (2.f * s1 - s0 - s2);
                const float shift = std::abs (den) > 1.0e-9f ? (s2 - s0) / den : 0.f;
                const float tauF = (float) found + std::min (1.f, std::max (-1.f, shift));
                f0 = (float) (fsD / tauF);
                voiced = (f0 > 70.f && f0 < 900.f);
            }
        }

        if (! voiced)
        {
            ++unvoicedHops;
            if ((double) unvoicedHops * hopSeconds > 0.06)      // mantener un instante y luego soltar
                corr += (0.f - corr) * 0.2f;
        }
        else
        {
            const float midi = 69.f + 12.f * std::log2 (f0 / 440.f);
            if (unvoicedHops > 0) hist = { midi, midi, midi };
            unvoicedHops = 0;
            hist[(size_t) histIdx] = midi;
            histIdx = (histIdx + 1) % 3;
            const float m = std::max (std::min (hist[0], hist[1]), std::min (std::max (hist[0], hist[1]), hist[2])); // mediana

            const float snapped = snapToScale (m, cur.key, cur.scale);
            const float raw     = (snapped - m) * cur.amount;
            const float alpha   = cur.speedMs <= 0.5f ? 1.f
                                : (float) (1.0 - std::exp (-hopSeconds / ((double) cur.speedMs * 0.001)));
            corr += (raw - corr) * alpha;

            gTarget = (float) std::min (0.060 * sr, std::max (0.006 * sr, 4.0 * sr / (double) f0));
        }

        targetRatio = std::pow (2.f, corr / 12.f);
    }

    double sr = 48000.0, fsD = 12000.0, hopSeconds = 0.005;
    int decim = 4, tauMin = 10, tauMax = 180;

    std::vector<float> yin, dn;
    std::array<float, kRing> ring {};
    int ringWp = 0, decCount = 0, hopCount = 0;
    float decAcc = 0.f;

    std::array<std::vector<float>, 2> delayBuf;
    int mask = 0, wp = 0;
    float gSamples = 1000.f, gTarget = 1000.f, d = 500.f;
    float targetRatio = 1.f, ratioSm = 1.f, ratioCoef = 0.01f, gCoef = 0.001f;
    float corr = 0.f;
    int unvoicedHops = 1000;
    std::array<float, 3> hist {};
    int histIdx = 0;
    Params cur;
};

//==============================================================================
// De-Esser de banda dividida a 6.5 kHz
class DeEsser
{
public:
    void prepare (double fs)
    {
        bp.reset();
        bp.bandPass (fs, 6500.0, 1.2);
        atk = (float) std::exp (-1.0 / (0.0005 * fs));
        rel = (float) std::exp (-1.0 / (0.050  * fs));
        env = 0.f;
    }

    void process (float* const* d, int nCh, int n, float thresholdDb) noexcept
    {
        if (thresholdDb > -0.5f) { env = 0.f; return; }   // OFF

        for (int i = 0; i < n; ++i)
        {
            float band[2] = { 0.f, 0.f };
            float lvl = 0.f;
            for (int ch = 0; ch < nCh; ++ch)
            {
                band[ch] = bp.process (d[ch][i], ch);
                lvl = std::max (lvl, std::abs (band[ch]));
            }
            env = lvl > env ? lvl + atk * (env - lvl) : lvl + rel * (env - lvl);

            const float envDb = 20.f * std::log10 (env + 1.0e-9f);
            const float over  = envDb - thresholdDb;
            const float red   = over > 0.f ? std::min (over * 0.8f, 12.f) : 0.f;   // ratio ~5:1, max -12 dB
            const float g     = std::pow (10.f, -red / 20.f);

            for (int ch = 0; ch < nCh; ++ch)
                d[ch][i] += band[ch] * (g - 1.f);
        }
    }

private:
    Biquad bp;
    float atk = 0.f, rel = 0.f, env = 0.f;
};

//==============================================================================
// Compresor "optico": rodilla suave + release dependiente del programa
class Compressor
{
public:
    void prepare (double fs) { sr = fs; gr = 0.f; }

    void setParams (float thrDb, float ratio_, float atkMs, float relMs, float makeupDb)
    {
        threshold = thrDb;
        ratio     = std::max (1.f, ratio_);
        makeup    = makeupDb;
        atkCoef   = (float) std::exp (-1.0 / (std::max (0.05f, atkMs) * 0.001 * sr));
        const double r = std::max (5.f, relMs) * 0.001;
        relFast   = (float) std::exp (-1.0 / (r * sr));
        relSlow   = (float) std::exp (-1.0 / (r * 3.0 * sr));
    }

    void process (float* const* d, int nCh, int n) noexcept
    {
        const float knee = 8.f;
        for (int i = 0; i < n; ++i)
        {
            float lv = 0.f;
            for (int ch = 0; ch < nCh; ++ch) lv = std::max (lv, std::abs (d[ch][i]));
            const float lvDb = lv > 1.0e-6f ? 20.f * std::log10 (lv) : -120.f;

            const float over = lvDb - threshold;
            float outDb;
            if (2.f * over < -knee)                  outDb = lvDb;
            else if (2.f * std::abs (over) <= knee) { const float t = over + knee * 0.5f; outDb = lvDb + (1.f / ratio - 1.f) * t * t / (2.f * knee); }
            else                                     outDb = threshold + over / ratio;

            const float target = std::max (0.f, lvDb - outDb);
            if (target > gr)
                gr = target + atkCoef * (gr - target);
            else
            {
                const float k = std::min (gr / 12.f, 1.f);                 // mas reduccion -> release mas lento
                gr = target + (relFast + k * (relSlow - relFast)) * (gr - target);
            }

            const float gain = std::exp ((makeup - gr) * 0.11512925f);
            for (int ch = 0; ch < nCh; ++ch) d[ch][i] *= gain;
        }
    }

private:
    double sr = 48000.0;
    float threshold = 0.f, ratio = 1.f, makeup = 0.f;
    float atkCoef = 0.f, relFast = 0.f, relSlow = 0.f, gr = 0.f;
};

//==============================================================================
// Espacio: reverb + delay en paralelo, cada uno con su propio nivel wet
class SpaceFx
{
public:
    void prepare (double fs, int maxBlock)
    {
        sr = fs;
        reverb.setSampleRate (fs);
        reverb.reset();

        dlSize = (int) (fs * 1.4) + 8;
        for (auto& b : dl) b.assign ((size_t) dlSize, 0.f);
        wp = 0;
        lp[0] = lp[1] = 0.f;

        maxB = std::max (64, maxBlock);
        tmp[0].assign ((size_t) maxB, 0.f);
        tmp[1].assign ((size_t) maxB, 0.f);

        delaySm.reset (fs, 0.08);
        delaySm.setCurrentAndTargetValue ((float) (0.25 * fs));
    }

    // size, rvMix, dlFb, dlMix en 0..1
    void process (float* const* d, int nCh, int n, float size, float rvMix, float dlMs, float dlFb, float dlMix) noexcept
    {
        // 1) reverb (solo senal humeda) a partir de la entrada seca
        juce::Reverb::Parameters rp;
        rp.roomSize = size; rp.damping = 0.45f; rp.wetLevel = rvMix * 0.33f; rp.dryLevel = 0.f;
        rp.width = 1.f; rp.freezeMode = 0.f;
        reverb.setParameters (rp);

        // 2) delay sobre la entrada seca (en sitio), guardando antes la copia para la reverb
        delaySm.setTargetValue (std::min ((float) sr, std::max (1.f, (float) (dlMs * 0.001 * sr))));

        int pos = 0;
        while (pos < n)
        {
            const int c = std::min (n - pos, maxB);
            for (int ch = 0; ch < nCh; ++ch)
                std::copy (d[ch] + pos, d[ch] + pos + c, tmp[(size_t) ch].begin());

            if (nCh >= 2) reverb.processStereo (tmp[0].data(), tmp[1].data(), c);
            else          reverb.processMono   (tmp[0].data(), c);

            for (int i = 0; i < c; ++i)
            {
                const float dS = delaySm.getNextValue();
                for (int ch = 0; ch < nCh; ++ch)
                {
                    const float dsCh = ch == 0 ? dS : dS * 1.12f;      // canal R algo mas largo = anchura
                    float rpos = (float) wp - dsCh;
                    if (rpos < 0.f) rpos += (float) dlSize;
                    const int i0 = (int) rpos;
                    const float fr = rpos - (float) i0;
                    const int i1 = (i0 + 1 >= dlSize) ? 0 : i0 + 1;
                    const auto& line = dl[(size_t) ch];
                    const float wet = line[(size_t) i0] * (1.f - fr) + line[(size_t) i1] * fr;

                    lp[ch] += 0.35f * (wet - lp[ch]);                  // oscurece las repeticiones
                    const float in = d[ch][pos + i];
                    dl[(size_t) ch][(size_t) wp] = in + lp[ch] * dlFb;
                    d[ch][pos + i] = in + wet * dlMix + tmp[(size_t) ch][(size_t) i];
                }
                if (++wp >= dlSize) wp = 0;
            }
            pos += c;
        }
    }

private:
    double sr = 48000.0;
    juce::Reverb reverb;
    std::array<std::vector<float>, 2> dl, tmp;
    int dlSize = 1, wp = 0, maxB = 512;
    float lp[2] = { 0.f, 0.f };
    juce::SmoothedValue<float> delaySm;
};

} // namespace c7
