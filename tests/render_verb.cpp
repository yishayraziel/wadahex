// Snare hits through Reverb80s: dry | natural tail | gated (two gate settings) -> WAV
#include "Presets.h"
#include "Reverb80s.h"
#include <cstdio>
#include <string>
static void writeWav (const char* path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path, "wb"); const uint32_t n = (uint32_t) L.size(), b = n * 4;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); }; auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + b); std::fwrite ("WAVEfmt ", 1, 8, f); w32 (16); w16 (1); w16 (2); w32 (sr); w32 (sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (b);
    for (uint32_t i = 0; i < n; ++i) { int16_t a = (int16_t) std::lround (std::tanh (L[i]) * 32000), c = (int16_t) std::lround (std::tanh (R[i]) * 32000); std::fwrite (&a, 2, 1, f); std::fwrite (&c, 2, 1, f); }
    std::fclose (f);
}
int main()
{
    const int sr = 48000, blk = 128, seg = sr * 2;
    Kit kit = factoryKits()[0].make();
    std::vector<std::vector<int8_t>> bank (8);
    const float setting[4][2] = { { 0, 0 }, { 0.6f, 0 }, { 0.6f, 0.3f }, { 0.6f, 0.8f } };
    std::vector<float> L (seg * 4 + blk), R (seg * 4 + blk);
    uint32_t rng = 5;
    for (int k = 0; k < 4; ++k)
    {
        DrumVoice v; v.prepare (sr, &bank); Reverb80s rv; rv.prepare (sr);
        PadParams p = kit[1]; p.verb = setting[k][0]; p.gate = setting[k][1];
        for (int pos = 0; pos < seg; pos += blk)
        {
            if (pos == 0 || pos == sr) { v.trigger (1.0f, p); rv.trigger (p.gate); }
            float* l = &L[(size_t) (k * seg + pos)]; float* r = &R[(size_t) (k * seg + pos)];
            v.render (l, r, blk, p, rng);
            rv.process (l, r, l, r, blk, p.verb, p.gate);
        }
        float tail = 0; for (int i = (int) (0.9 * sr); i < sr; ++i) tail = std::max (tail, std::abs (L[(size_t) (k * seg + i)]));
        float at300 = 0; for (int i = (int) (0.30 * sr); i < (int) (0.32 * sr); ++i) at300 = std::max (at300, std::abs (L[(size_t) (k * seg + i)]));
        std::printf ("verb %.1f gate %.1f : level @300ms %.4f  @0.9-1.0s %.5f\n", p.verb, p.gate, at300, tail);
    }
    writeWav ("out/verb_test.wav", L, R, sr);
}
