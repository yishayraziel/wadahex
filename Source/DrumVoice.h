#pragma once
#include <cmath>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <array>
#include "Chips.h"

// Per-pad parameter snapshot (read once per block from the APVTS).
struct PadParams
{
    int   chip = 7;        // which 8-bit chip (7 = off)
    float chipLvl = 0.5f;  // 0..1
    float chipPitch = 0;   // semitones
    float chipBits = 8;    // 2..8 bits of resolution
    float tonePitch = 100; // Hz
    float bend = 0.5f;     // -1..1  (positive = sweep down to pitch, negative = sweep up)
    float bendTime = 80;   // ms
    float toneDecay = 300; // ms
    float toneLvl = 0.8f;
    float noiseLvl = 0.2f;
    float noiseDecay = 120;// ms
    float noiseColor = 0.5f;// 0 dark .. 1 bright
    float cutoff = 8000;   // Hz
    float reso = 0.2f;     // 0..1
    float filtEnv = 0.3f;  // 0..1 (up to +6 oct at the hit)
    float filtDecay = 150; // ms
    float drive = 0.1f;    // 0..1
    float velSens = 0.6f;  // 0..1
    float level = 0.8f;    // linear gain
    float pan = 0;         // -1..1
    // --- SDS-V voice additions
    float click = 0.0f;    // impact click amount
    float shape = 0.2f;    // 0 sine .. 1 triangle (the SDS-V VCO is a triangle)
    bool  toneFilt = true; // tone through the filter (false = SDS-V style: filter acts on noise + click only)
    float lfoRate = 6;     // Hz
    float lfoPitch = 0;    // 0..1 -> up to +/-1 octave
    float lfoCut = 0;      // 0..1 -> up to +/-2 octaves
    float noiseSweep = 0;  // -1..1 noise filter sweep (+/-4 oct, follows filter envelope)
    float velDecay = 0;    // 0..1 soft hits get shorter
    bool  analogBend = false; // true: SDS-V curve - PITCH is the strike pitch and it decays in Hz toward pitch / 2^(4*bend)
    float toneLP = 20000;  // Hz: the SDS-V "filtered triangle" output (20 kHz = open)
    float noiseHP = 20;    // Hz: high-pass on the noise after the noise-tone low-pass (20 Hz = off)
    float verb = 0;        // 80s reverb amount (0 = off)
    float gate = 0;        // 0 = natural tail, >0 = gated reverb (higher = shorter)
    bool  ownOut = false;  // route this pad to its own stereo output
    float verbTone = 0.6f; // reverb colour: 0 dark .. 1 bright
    float drift = 0.0f;    // analog drift: random per-hit variation of pitch, decay, level, filter
    bool  metal = false;   // hi-hat voice: six inharmonic square oscillators instead of the VCO
    bool  keys = false;    // pad plays chromatically from the keyboard
};

// Topology-preserving state-variable filter (Simper/Cytomic).
struct Svf
{
    float ic1 = 0, ic2 = 0, g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0;
    void reset() { ic1 = ic2 = 0; }
    void set (float fc, float q01, float sr)
    {
        fc = std::clamp (fc, 20.0f, sr * 0.45f);
        g = std::tan (3.14159265f * fc / sr);
        k = 2.0f - 1.95f * q01;               // q01 = 1 -> close to self-oscillation
        a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    // returns low-pass, writes band-pass
    inline float process (float v0, float& bp)
    {
        float v3 = v0 - ic2;
        float v1 = a1 * ic1 + a2 * v3;
        float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
        bp = v1; return v2;
    }
};

class DrumVoice
{
public:
    void prepare (double sampleRate, const std::vector<std::vector<int8_t>>* chipBank)
    {
        sr = (float) sampleRate; bank = chipBank; active = false;
        filt.reset(); nFilt.reset(); tFilt.reset(); hpState = 0.0f;
    }

    // keyMul: pitch ratio when the pad is played chromatically (KEYS)
    void trigger (float velocity, const PadParams& p, float keyMul = 1.0f)
    {
        vel = std::clamp (velocity, 0.0f, 1.0f);
        // analog drift: every hit a little different, like the real hardware
        auto rnd = [this] { driftRng ^= driftRng << 13; driftRng ^= driftRng >> 17; driftRng ^= driftRng << 5;
                            return (float) ((double) driftRng / 4294967295.0 * 2.0 - 1.0); };
        const float d = std::clamp (p.drift, 0.0f, 1.0f);
        const float dPitch = d > 0 ? std::pow (2.0f, rnd() * d * 0.035f) : 1.0f;      // up to +/-42 cents
        const float dDecay = d > 0 ? 1.0f + rnd() * d * 0.14f : 1.0f;
        const float dLevel = d > 0 ? 1.0f + rnd() * d * 0.16f : 1.0f;
        const float dFilt  = d > 0 ? 1.0f + rnd() * d * 0.25f : 1.0f;
        const float dyn = 1.0f - p.velSens + p.velSens * vel;   // dynamics: level, bend, click & brightness
        hitGain = dyn * dLevel;
        bendDepth = p.bend * (p.analogBend ? 4.0f : 3.0f) * (1.0f - p.velSens * 0.5f + p.velSens * 0.5f * vel); // up to ±3 (exp) / ±4 (analog) oct
        analogBend = p.analogBend;
        bendFloor = std::pow (2.0f, -std::fabs (bendDepth));
        pitchOffset = std::pow (2.0f, p.velSens * vel * 2.0f / 12.0f) * dPitch * keyMul;  // harder hit = slightly higher
        chipMul = (double) keyMul;
        filtBoost = p.filtEnv * (0.4f + 0.6f * dyn) * dFilt;
        clickGain = p.click * (0.25f + 0.75f * dyn);
        decayScale = (1.0f - p.velDecay * 0.75f * (1.0f - vel)) * dDecay;              // soft hits ring shorter
        chokeGain = 1.0f; choking = false;
        tAmp = 1.0f; nAmp = 1.0f; bendEnv = 1.0f; fEnv = 1.0f; cEnv = 1.0f; lfoPhase = 0.0f;
        chipPos = 0.0; chipOn = (p.chip >= 0 && p.chip < chips::kNumChips - 1);
        if (! active) { phase = 0.0; mph.fill (0.0); filt.reset(); nFilt.reset(); tFilt.reset(); hpState = 0.0f; mhp1 = mhp2 = 0.0f; attack = 0.0f; }
        active = true; ageSamples = 0;
    }

    void seedDrift (uint32_t s) { driftRng = s ? s : 0x2545F491u; }

    // Hi-hat choke: fade out in ~5 ms (closed hat cuts the open hat).
    void choke() { if (active) choking = true; }

    // Adds stereo output into L/R.
    void render (float* L, float* R, int n, const PadParams& p, uint32_t& rngState)
    {
        if (! active) return;

        const float tDec  = coef (p.toneDecay * decayScale);
        const float nDec  = coef (p.noiseDecay * decayScale);
        const float bDec  = coef (p.bendTime);
        const float fDec  = coef (p.filtDecay);
        const float cDec  = coef (4.0f);                      // click: 60 dB down in 4 ms
        const float atk   = 1.0f / std::max (1.0f, 0.001f * sr);
        const float twoPi = 6.2831853f;
        const float lfoInc = p.lfoRate / sr;
        const float lfoP = p.lfoPitch * p.lfoPitch;           // squared for finer control at low depths
        const float lfoC = p.lfoCut * 2.0f;

        // noise tone = low-pass cutoff on white noise (SDS-V style), 200 Hz .. 18 kHz
        const float nCutBase = 200.0f * std::pow (2.0f, p.noiseColor * 6.5f);
        const float hpCoef = 1.0f - std::exp (-6.2831853f * std::max (20.0f, p.noiseHP) / sr);
        const bool  toneLPOn = p.toneLP < 19000.0f;
        if (toneLPOn) tFilt.set (p.toneLP, 0.05f, sr);

        const double chipStep = chips::kChipRate / sr * std::pow (2.0, p.chipPitch / 12.0) * chipMul;
        // hi-hat: 808-style inharmonic ratios, base = PITCH
        static constexpr float ratio[6] = { 1.0f, 1.4827f, 1.8002f, 2.5459f, 2.6302f, 3.8967f };
        const float metalHP = 1.0f - std::exp (-6.2831853f * std::max (400.0f, p.noiseHP) / sr);
        const float chokeStep = 1.0f / (0.005f * sr);
        const std::vector<int8_t>* chip = chipOn ? &(*bank)[(size_t) p.chip] : nullptr;
        const float levels = std::pow (2.0f, std::round (p.chipBits) - 1.0f);

        const float drv = 1.0f + p.drive * 9.0f;
        const float drvComp = 1.0f / std::tanh (drv);
        const float panL = std::cos ((p.pan + 1.0f) * 0.25f * 3.14159265f);
        const float panR = std::sin ((p.pan + 1.0f) * 0.25f * 3.14159265f);
        const float outGain = p.level * hitGain;

        for (int i = 0; i < n; ++i)
        {
            attack = std::min (1.0f, attack + atk);
            const float lfo = std::sin (twoPi * lfoPhase);
            lfoPhase += lfoInc; if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

            // ---- VCO: sine..triangle with pitch bend sweep and LFO
            float bendMul;
            if (! analogBend)        bendMul = std::pow (2.0f, bendDepth * bendEnv);
            else if (bendDepth >= 0) bendMul = bendFloor + (1.0f - bendFloor) * bendEnv;          // strike pitch falling in Hz
            else                     bendMul = bendFloor + (1.0f - bendFloor) * (1.0f - bendEnv); // rising up to pitch
            const float f = p.tonePitch * pitchOffset * bendMul * std::pow (2.0f, lfoP * lfo);
            phase += (double) (f / sr);
            if (phase >= 1.0) phase -= 1.0;
            float tone;
            if (p.metal)
            {
                float m = 0.0f;
                for (int k = 0; k < 6; ++k)
                {
                    mph[(size_t) k] += (double) (f * ratio[k] / sr);
                    if (mph[(size_t) k] >= 1.0) mph[(size_t) k] -= 1.0;
                    m += mph[(size_t) k] < 0.5 ? 1.0f : -1.0f;
                }
                tone = m * (1.0f / 6.0f) * tAmp * p.toneLvl;
            }
            else
            {
                const float ph = (float) phase;
                const float sine = std::sin (twoPi * ph);
                const float tri  = 1.0f - 4.0f * std::fabs (ph - 0.5f);
                tone = ((1.0f - p.shape) * sine + p.shape * tri) * tAmp * p.toneLvl;
            }
            if (toneLPOn) { float tbp; tone = tFilt.process (tone, tbp); }

            // ---- noise through band-pass (colour), optionally swept by the filter envelope
            rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;
            const float white = (float) ((double) rngState / 4294967295.0 * 2.0 - 1.0);
            if ((ageSamples & 7) == 0)
                nFilt.set (nCutBase * std::pow (2.0f, 4.0f * p.noiseSweep * fEnv), 0.15f, sr);
            float nbp; float nz = nFilt.process (white, nbp);
            hpState += hpCoef * (nz - hpState);
            if (p.noiseHP > 21.0f) nz -= hpState;
            const float noise = nz * 1.4f * nAmp * p.noiseLvl;

            // ---- impact click: a very short pulse with a little noise, like the pad transient
            const float clickS = cEnv > 1.0e-4f ? clickGain * cEnv * (0.45f + 0.9f * white) * 1.4f : 0.0f; // bright impact transient

            // ---- 8-bit chip, zero-order hold playback (aliasing is the point)
            float chipS = 0.0f;
            if (chip != nullptr)
            {
                const size_t idx = (size_t) chipPos;
                if (idx < chip->size())
                {
                    float s = (*chip)[idx] / 127.0f;
                    s = std::round (s * levels) / levels;     // bit reduction
                    chipS = s * p.chipLvl;
                    chipPos += chipStep;
                }
                else chip = nullptr, chipOn = false;
            }

            // ---- low-pass filter with envelope sweep + LFO
            if ((ageSamples & 7) == 0)
                filt.set (p.cutoff * std::pow (2.0f, 6.0f * filtBoost * fEnv + lfoC * lfo), p.reso, sr);
            float bp;
            float x;
            if (p.toneFilt) x = filt.process (tone + noise + chipS, bp);
            else            x = tone + filt.process (noise + chipS, bp); // SDS-V routing
            x += clickS;                                                  // click sits on top, unfiltered
            if (p.metal)
            {   // hats: 12 dB/oct high-pass at the HP knob (min 400 Hz)
                mhp1 += metalHP * (x - mhp1); x -= mhp1;
                mhp2 += metalHP * (x - mhp2); x -= mhp2;
                x *= 4.0f;   // make-up gain after the steep high-pass
            }

            // ---- drive
            x = std::tanh (x * drv) * drvComp;

            if (choking) { chokeGain = std::max (0.0f, chokeGain - chokeStep); if (chokeGain <= 0.0f) { tAmp = nAmp = 0.0f; chipOn = false; chip = nullptr; } }
            const float out = x * outGain * attack * chokeGain;
            L[i] += out * panL; R[i] += out * panR;

            tAmp *= tDec; nAmp *= nDec; bendEnv *= bDec; fEnv *= fDec; cEnv *= cDec;
            ++ageSamples;
        }

        if (tAmp < 1.0e-4f && nAmp < 1.0e-4f && ! chipOn) active = false;
    }

    bool isActive() const { return active; }
    float meter() const { return active ? std::max (tAmp, nAmp) * hitGain : 0.0f; }

private:
    // per-sample multiplier so that the envelope falls by ~60 dB over "ms"
    float coef (float ms) const { return std::exp (-6.9f / std::max (1.0f, ms * 0.001f * sr)); }

    float sr = 44100.0f;
    const std::vector<std::vector<int8_t>>* bank = nullptr;
    bool active = false, chipOn = false, analogBend = false;
    float bendFloor = 1.0f;
    double phase = 0.0, chipPos = 0.0;
    float vel = 1, hitGain = 1, bendDepth = 0, pitchOffset = 1, filtBoost = 0, clickGain = 0, decayScale = 1;
    float tAmp = 0, nAmp = 0, bendEnv = 0, fEnv = 0, cEnv = 0, attack = 1, lfoPhase = 0;
    uint32_t ageSamples = 0;
    Svf filt, nFilt, tFilt;
    float hpState = 0.0f, mhp1 = 0.0f, mhp2 = 0.0f, chokeGain = 1.0f;
    bool choking = false;
    std::array<double, 6> mph {};
    double chipMul = 1.0;
    uint32_t driftRng = 0x2545F491u;
};
