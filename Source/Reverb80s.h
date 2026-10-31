#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>

// Bright, dense 80s room/plate built as an 8-line feedback delay network, with an optional
// hit-keyed gate that turns it into the classic gated reverb (long, non-decaying tail chopped short).
//   amount: 0..1 wet level          gate: 0 = natural tail, >0 = gated (higher = shorter gate)
class Reverb80s
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        static const float ms[8] = { 31.7f, 37.9f, 41.3f, 45.1f, 53.9f, 59.3f, 67.1f, 73.7f };
        for (int i = 0; i < 8; ++i)
        {
            len[(size_t) i] = std::max (8, (int) (ms[i] * 0.001f * sr));
            line[(size_t) i].assign ((size_t) len[(size_t) i], 0.0f);
        }
        static const float apMs[4] = { 4.77f, 3.59f, 12.73f, 9.31f };
        for (int i = 0; i < 4; ++i) { apLen[(size_t) i] = std::max (4, (int) (apMs[i] * 0.001f * sr)); ap[(size_t) i].assign ((size_t) apLen[(size_t) i], 0.0f); }
        pre.assign ((size_t) (0.012f * sr) + 1, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& l : line) std::fill (l.begin(), l.end(), 0.0f);
        for (auto& a : ap) std::fill (a.begin(), a.end(), 0.0f);
        std::fill (pre.begin(), pre.end(), 0.0f);
        pos.fill (0); apPos.fill (0); damp.fill (0.0f);
        prePos = 0; hp = 0.0f; inLp = 0.0f; gateEnv = 0.0f; holdLeft = 0; energy = 0.0f;
    }

    // a hit on the pad: (re)opens the gate for the hold time set by `gate`
    void trigger (float gate) { holdSamples = holdFor (gate); holdLeft = holdSamples; gateOpen = true; }
    int holdFor (float gate) const { return (int) ((0.75f - 0.55f * std::sqrt (std::clamp (gate, 0.0f, 1.0f))) * sr); } // 750 .. 200 ms

    bool isSilent() const { return energy < 1.0e-7f && amountSm < 1.0e-4f; }

    // adds the wet signal of (inL, inR) into (outL, outR)
    void process (const float* inL, const float* inR, float* outL, float* outR, int n, float amount, float gate, float tone = 0.6f)
    {
        const bool gated = gate > 0.005f;
        const float rt = gated ? 3.5f : 1.5f;                          // gated verbs sustain until the gate cuts them
        holdSamples = holdFor (gate);
        for (int i = 0; i < 8; ++i) g[(size_t) i] = std::pow (10.0f, -3.0f * (float) len[(size_t) i] / (rt * sr));
        // TONE: 0 = dark (soft, damped room) .. 1 = bright (shiny plate)
        const float t = std::clamp (tone, 0.0f, 1.0f);
        const float dampC = 1.0f - std::exp (-6.2831853f * std::min (0.45f * sr, 1800.0f * std::pow (2.0f, t * 3.2f)) / sr);
        const float inLpC = 1.0f - std::exp (-6.2831853f * std::min (0.45f * sr, 2200.0f * std::pow (2.0f, t * 3.4f)) / sr);
        const float hpC   = 1.0f - std::exp (-6.2831853f * 180.0f / sr);    // keep the low end out of the verb
        const float atk = 1.0f / (0.002f * sr), rel = 1.0f / (0.012f * sr);
        const float target = amount * amount * 1.4f;
        float en = energy;

        for (int s = 0; s < n; ++s)
        {
            amountSm += 0.0015f * (target - amountSm);
            // input: mono, predelay, high-passed
            float x = 0.5f * (inL[s] + inR[s]);
            const float d = pre[(size_t) prePos]; pre[(size_t) prePos] = x; prePos = (prePos + 1) % (int) pre.size(); x = d;
            hp += hpC * (x - hp); x -= hp;
            inLp += inLpC * (x - inLp); x = inLp;
            // two-stage diffusion
            for (int a = 0; a < 4; ++a)
            {
                auto& buf = ap[(size_t) a]; int& p = apPos[(size_t) a];
                const float z = buf[(size_t) p]; const float y = -0.6f * x + z;
                buf[(size_t) p] = x + 0.6f * y; p = (p + 1) % apLen[(size_t) a]; x = y;
            }
            // FDN with Householder feedback
            std::array<float, 8> o;
            float sum = 0.0f;
            for (int i = 0; i < 8; ++i) { o[(size_t) i] = line[(size_t) i][(size_t) pos[(size_t) i]]; sum += o[(size_t) i]; }
            sum *= 0.25f;
            float wl = 0.0f, wr = 0.0f;
            for (int i = 0; i < 8; ++i)
            {
                float fb = (o[(size_t) i] - sum) * g[(size_t) i];
                damp[(size_t) i] += dampC * (fb - damp[(size_t) i]); fb = damp[(size_t) i];
                line[(size_t) i][(size_t) pos[(size_t) i]] = fb + x * 0.35f;
                pos[(size_t) i] = (pos[(size_t) i] + 1) % len[(size_t) i];
                if (i & 1) wr += o[(size_t) i]; else wl += o[(size_t) i];
            }
            // gate: open on hits, hold, then fast close
            float gainG = 1.0f;
            if (gated)
            {
                if (holdLeft > 0) { --holdLeft; gateEnv = std::min (1.0f, gateEnv + atk); }
                else gateEnv = std::max (0.0f, gateEnv - rel);
                gainG = gateEnv;
            }
            const float k = (gated ? 0.55f : 0.35f) * amountSm * gainG;
            outL[s] += wl * k; outR[s] += wr * k;
            en = 0.999f * en + 0.001f * (wl * wl + wr * wr);
        }
        energy = en;
    }

private:
    float sr = 48000.0f;
    std::array<std::vector<float>, 8> line;
    std::array<int, 8> len {}, pos {};
    std::array<float, 8> g {}, damp {};
    std::array<std::vector<float>, 4> ap;
    std::array<int, 4> apLen {}, apPos {};
    std::vector<float> pre;
    int prePos = 0, holdSamples = 0, holdLeft = 0;
    float inLp = 0.0f, hp = 0.0f, gateEnv = 0.0f, amountSm = 0.0f, energy = 0.0f;
    bool gateOpen = false;
};
