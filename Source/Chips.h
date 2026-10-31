#pragma once
#include <vector>
#include <cmath>
#include <cstdint>

// "Chips": short 8-bit transient recordings, in the spirit of the swappable
// EPROM sound chips of mid-80s hybrid drum modules. All are synthesised here
// from scratch (no third-party samples) at a native rate of 20 kHz, then
// quantised to 8 bits so the grit is baked in like a real chip.
namespace chips
{
static constexpr double kChipRate = 20000.0;
static constexpr int kNumChips = 8;

inline const char* names[kNumChips] = { "KICK", "SNARE", "TOM", "CLAP", "RIM", "METAL", "ZAP", "OFF" };

struct Rng
{
    uint32_t s = 0x9E3779B9u;
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) ((double) s / 4294967295.0 * 2.0 - 1.0); }
};

inline std::vector<int8_t> quantise (const std::vector<float>& x)
{
    float peak = 1.0e-6f;
    for (auto v : x) peak = std::fmax (peak, std::fabs (v));
    std::vector<int8_t> out (x.size());
    for (size_t i = 0; i < x.size(); ++i)
        out[i] = (int8_t) std::lround (std::fmax (-127.0f, std::fmin (127.0f, x[i] / peak * 127.0f)));
    return out;
}

inline std::vector<int8_t> make (int which)
{
    const double sr = kChipRate;
    const double twoPi = 6.283185307179586;
    Rng rng; rng.s += (uint32_t) which * 7919u;
    std::vector<float> x;

    auto len = [&] (double sec) { x.assign ((size_t) (sec * sr), 0.0f); };

    switch (which)
    {
        case 0: // KICK – fast downward sine sweep with a beater click
        {
            len (0.30); double ph = 0;
            for (size_t i = 0; i < x.size(); ++i)
            {
                double t = i / sr;
                double f = 48.0 + 260.0 * std::exp (-t / 0.018);
                ph += twoPi * f / sr;
                double click = t < 0.004 ? rng.next() * (1.0 - t / 0.004) : 0.0;
                x[i] = (float) (std::sin (ph) * std::exp (-t / 0.09) + 0.6 * click);
            }
            break;
        }
        case 1: // SNARE – two drum-head modes + bright noise crack
        {
            len (0.25); double p1 = 0, p2 = 0; float lp = 0;
            for (size_t i = 0; i < x.size(); ++i)
            {
                double t = i / sr;
                p1 += twoPi * (185.0 + 60.0 * std::exp (-t / 0.01)) / sr;
                p2 += twoPi * 330.0 / sr;
                float n = rng.next(); lp += 0.55f * (n - lp);
                float hp = n - lp;
                x[i] = (float) ((0.6 * std::sin (p1) + 0.3 * std::sin (p2)) * std::exp (-t / 0.05)
                                + 0.9 * hp * std::exp (-t / 0.07));
            }
            break;
        }
        case 2: // TOM – round sine body with a soft stick transient
        {
            len (0.40); double ph = 0;
            for (size_t i = 0; i < x.size(); ++i)
            {
                double t = i / sr;
                ph += twoPi * (110.0 + 70.0 * std::exp (-t / 0.05)) / sr;
                double stick = t < 0.006 ? rng.next() * 0.5 * (1.0 - t / 0.006) : 0.0;
                x[i] = (float) (std::sin (ph + 0.4 * std::sin (2 * ph)) * std::exp (-t / 0.14) + stick);
            }
            break;
        }
        case 3: // CLAP – three quick bursts then a diffuse tail, band-passed
        {
            len (0.30); float b1 = 0, b2 = 0;
            for (size_t i = 0; i < x.size(); ++i)
            {
                double t = i / sr;
                double env = 0;
                for (int k = 0; k < 3; ++k) { double tk = t - k * 0.011; if (tk >= 0 && tk < 0.011) env = std::fmax (env, std::exp (-tk / 0.003)); }
                double tt = t - 0.033; if (tt >= 0) env = std::fmax (env, 0.8 * std::exp (-tt / 0.08));
                float n = rng.next();
                b1 += 0.35f * (n - b1); b2 += 0.35f * (b1 - b2); // crude band-pass around 1 kHz
                x[i] = (float) ((b1 - b2) * 3.0 * env);
            }
            break;
        }
        case 4: // RIM – tight woody knock
        {
            len (0.06); double p1 = 0, p2 = 0;
            for (size_t i = 0; i < x.size(); ++i)
            {
                double t = i / sr;
                p1 += twoPi * 1700.0 / sr; p2 += twoPi * 520.0 / sr;
                x[i] = (float) ((std::sin (p1) * 0.7 + std::sin (p2)) * std::exp (-t / 0.008) + rng.next() * 0.3 * std::exp (-t / 0.002));
            }
            break;
        }
        case 5: // METAL – six detuned squares, inharmonic like a cheap cymbal circuit
        {
            len (0.45); const double fr[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
            double ph[6] = {}; float lp = 0;
            for (size_t i = 0; i < x.size(); ++i)
            {
                double t = i / sr, s = 0;
                for (int k = 0; k < 6; ++k) { ph[k] += fr[k] * 2.0 / sr; s += std::fmod (ph[k], 2.0) < 1.0 ? 1.0 : -1.0; }
                lp += 0.6f * ((float) s - lp);
                x[i] = (float) (((float) s - lp) * std::exp (-t / 0.12));
            }
            break;
        }
        case 6: // ZAP – laser-ish fast sweep
        {
            len (0.08); double ph = 0;
            for (size_t i = 0; i < x.size(); ++i)
            {
                double t = i / sr;
                ph += twoPi * (90.0 + 3500.0 * std::exp (-t / 0.008)) / sr;
                x[i] = (float) ((std::sin (ph) > 0 ? 1.0 : -1.0) * std::exp (-t / 0.025));
            }
            break;
        }
        default: break; // OFF
    }
    // fade the last 20% so the chip never ends with a step (8-bit tail grit stays, the click doesn't)
    const size_t fadeLen = x.size() / 5;
    for (size_t i = 0; i < fadeLen; ++i)
    {
        const float g = (float) i / (float) fadeLen;
        x[x.size() - 1 - i] *= g * g;
    }
    return quantise (x);
}
} // namespace chips
