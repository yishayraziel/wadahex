// Offline render of every pad of every factory kit -> WAV, plus a demo groove.
// Build: g++ -O2 -std=c++17 -I../Source render_test.cpp -o render_test
#include "Presets.h"
#include <cstdio>
#include <string>

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    const uint32_t n = (uint32_t) L.size(), dataBytes = n * 4;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + dataBytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (dataBytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) std::lround (std::tanh (L[i]) * 32000), b = (int16_t) std::lround (std::tanh (R[i]) * 32000);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    const std::string out = argc > 1 ? argv[1] : ".";
    const int sr = 48000, block = 256;
    std::vector<std::vector<int8_t>> bank;
    for (int c = 0; c < chips::kNumChips; ++c) bank.push_back (chips::make (c));
    uint32_t rng = 1234567u;

    int k = 0;
    for (const auto& kd : factoryKits())
    {
        Kit kit = kd.make();
        // one-shots: every pad, hard hit then soft hit, 1.2 s apart
        std::vector<float> L ((size_t) (sr * 1.2 * kNumPads * 2)), R (L.size());
        std::array<DrumVoice, kNumPads> v;
        for (auto& x : v) x.prepare (sr, &bank);
        std::printf ("%-14s", kd.name);
        for (int pad = 0; pad < kNumPads; ++pad)
            for (int hit = 0; hit < 2; ++hit)
            {
                const size_t start = (size_t) (sr * 1.2 * (pad * 2 + hit));
                v[(size_t) pad].trigger (hit == 0 ? 1.0f : 0.45f, kit[(size_t) pad]);
                float peak = 0; size_t last = start;
                for (size_t s = start; s < start + (size_t) (sr * 1.2); s += block)
                {
                    v[(size_t) pad].render (&L[s], &R[s], block, kit[(size_t) pad], rng);
                    for (int i = 0; i < block; ++i) { float a = std::fabs (L[s + i]); peak = std::fmax (peak, a); if (a > 0.001f) last = s + i; }
                }
                if (hit == 0) std::printf (" %s:%.2f/%dms", padInfo[pad].name, peak, (int) ((last - start) * 1000 / sr));
            }
        std::printf ("\n");
        writeWav (out + "/kit" + std::to_string (k++) + "_oneshots.wav", L, R, sr);
    }

    // demo groove with the Classic kit, 104 BPM, 4 bars
    Kit kit = factoryKits()[0].make();
    std::array<DrumVoice, kNumPads> v; for (auto& x : v) x.prepare (sr, &bank);
    const double step = 60.0 / 104.0 / 4.0; // 16ths
    const int bars = 4, steps = bars * 16;
    std::vector<float> L ((size_t) (sr * (step * steps + 2.0))), R (L.size());
    //                     0123456789abcdef
    const char* kick  =   "x.....x.x.......";
    const char* snare =   "....x.......x...";
    const char* clap  =   "............x...";
    const char* perc  =   "..x...x...x...xx";
    size_t pos = 0;
    for (int st = 0; st <= steps; ++st)
    {
        const size_t at = (size_t) (st * step * sr);
        while (pos < at) { const int n = (int) std::min<size_t> (block, at - pos); for (int p = 0; p < kNumPads; ++p) v[(size_t) p].render (&L[pos], &R[pos], n, kit[(size_t) p], rng); pos += (size_t) n; }
        if (st == steps) break;
        const int s = st % 16, bar = st / 16;
        if (kick[s] == 'x') v[0].trigger (1.0f, kit[0]);
        if (snare[s] == 'x') v[1].trigger (0.95f, kit[1]);
        if (clap[s] == 'x' && bar % 2) v[6].trigger (0.8f, kit[6]);
        if (perc[s] == 'x') v[7].trigger (s % 4 == 2 ? 0.9f : 0.5f, kit[7]);
        if (bar == 3 && s >= 8 && s % 2 == 0) v[(size_t) (2 + (s - 8) / 2)].trigger (0.95f, kit[(size_t) (2 + (s - 8) / 2)]); // tom fill
    }
    while (pos < L.size()) { const int n = (int) std::min<size_t> (block, L.size() - pos); for (int p = 0; p < kNumPads; ++p) v[(size_t) p].render (&L[pos], &R[pos], n, kit[(size_t) p], rng); pos += (size_t) n; }
    writeWav (out + "/demo_groove.wav", L, R, sr);
    return 0;
}
