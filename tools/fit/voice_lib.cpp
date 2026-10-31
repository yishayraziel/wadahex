// C entry point so Python can render a single WADAHEX hit directly (no plugin host).
// Build: g++ -O3 -std=c++17 -shared -fPIC -I../hex1/Source voice_lib.cpp -o libvoice.so
#include "DrumVoice.h"
#include <vector>

// Parameter order shared with fit.py (PARAMS list).
extern "C" int render (const float* p, int np, int sampleRate, int numSamples, float velocity, unsigned seed, float* out)
{
    PadParams q;
    q.chip = 7;
    auto get = [&] (int i, float def) { return i < np ? p[i] : def; };
    q.tonePitch  = get (0, 150);  q.bend       = get (1, 1);    q.bendTime  = get (2, 1000); q.toneDecay = get (3, 500);
    q.toneLvl    = get (4, 1);    q.shape      = get (5, 0.9f); q.noiseLvl  = get (6, 0.3f); q.noiseDecay = get (7, 200);
    q.noiseColor = get (8, 0.6f); q.noiseSweep = get (9, 0);    q.cutoff    = get (10, 3000); q.reso      = get (11, 0.2f);
    q.filtEnv    = get (12, 0.3f); q.filtDecay = get (13, 100); q.click     = get (14, 0.5f); q.drive     = get (15, 0.2f);
    q.toneFilt   = get (16, 0) > 0.5f;
    q.analogBend = get (17, 1) > 0.5f;
    q.toneLP     = get (18, 20000); q.noiseHP = get (19, 20);
    q.velSens = 0.7f; q.velDecay = 0.0f; q.level = 1.0f; q.pan = 0.0f;
    q.lfoRate = 6; q.lfoPitch = 0; q.lfoCut = 0;
    static std::vector<std::vector<int8_t>> bank (8);
    DrumVoice v; v.prepare (sampleRate, &bank);
    std::vector<float> R ((size_t) numSamples, 0.0f);
    for (int i = 0; i < numSamples; ++i) out[i] = 0.0f;
    uint32_t rng = seed ? seed : 1234567u;
    v.trigger (velocity, q);
    const int block = 256;
    for (int pos = 0; pos < numSamples; pos += block)
        v.render (out + pos, R.data() + pos, std::min (block, numSamples - pos), q, rng);
    return 0;
}
