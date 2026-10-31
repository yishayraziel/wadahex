// Renders "seconds note velocity" lines (stdin) through a factory kit to a stereo WAV.
// Build: g++ -O2 -std=c++17 -I../Source render_events.cpp -o render_events
// Use:   ./render_events out.wav [kitIndex] < events.txt
#include "Presets.h"
#include "Reverb80s.h"
#include <cstdio>
#include <iostream>
#include <string>

static void writeWav (const std::string& path, const std::vector<float>& L, const std::vector<float>& R, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    const uint32_t n = (uint32_t) L.size(), bytes = n * 4;
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (2); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (bytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        int16_t a = (int16_t) std::lround (std::tanh (L[i]) * 32000), b = (int16_t) std::lround (std::tanh (R[i]) * 32000);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    const int sr = 48000, block = 64;
    const int kitIdx = argc > 2 ? std::atoi (argv[2]) : 0;
    Kit kit = factoryKits()[(size_t) kitIdx].make();
    for (int p = 0; p < kNumPads; ++p) kit[(size_t) p].metal = p >= kHatClosed;
    std::vector<std::vector<int8_t>> bank;
    for (int c = 0; c < chips::kNumChips; ++c) bank.push_back (chips::make (c));
    std::array<DrumVoice, kNumPads> v; for (auto& x : v) x.prepare (sr, &bank);
    std::array<Reverb80s, kNumPads> rv; for (auto& r : rv) r.prepare (sr);
    if (argc > 4) for (int p = 1; p < 6; ++p) { kit[(size_t) p].verb = (float) std::atof (argv[3]); kit[(size_t) p].gate = (float) std::atof (argv[4]); }
    std::vector<float> pl (block), pr (block);
    std::vector<std::tuple<double, int, int>> ev; double t; int n, vel;
    while (std::cin >> t >> n >> vel) ev.emplace_back (t, n, vel);
    const size_t total = (size_t) ((std::get<0> (ev.back()) + 2.5) * sr);
    std::vector<float> L (total + block), R (total + block);
    uint32_t rng = 99u; size_t pos = 0, e = 0;
    while (pos < total)
    {
        while (e < ev.size() && (size_t) (std::get<0> (ev[e]) * sr) <= pos)
        {
            for (int p = 0; p < kNumPads; ++p)
                if (padInfo[p].note == std::get<1> (ev[e]) || padInfo[p].altNote == std::get<1> (ev[e]))
                { v[(size_t) p].trigger (std::get<2> (ev[e]) / 127.0f, kit[(size_t) p]); rv[(size_t) p].trigger (kit[(size_t) p].gate);
                  if (p == kHatClosed) v[(size_t) kHatOpen].choke(); }
            ++e;
        }
        for (int p = 0; p < kNumPads; ++p)
        {
            std::fill (pl.begin(), pl.end(), 0.0f); std::fill (pr.begin(), pr.end(), 0.0f);
            v[(size_t) p].render (pl.data(), pr.data(), block, kit[(size_t) p], rng);
            rv[(size_t) p].process (pl.data(), pr.data(), pl.data(), pr.data(), block, kit[(size_t) p].verb, kit[(size_t) p].gate);
            for (int i = 0; i < block; ++i) { L[pos + (size_t) i] += pl[(size_t) i]; R[pos + (size_t) i] += pr[(size_t) i]; }
        }
        pos += block;
    }
    L.resize (total); R.resize (total);
    float pk = 1e-6f; for (size_t i = 0; i < total; ++i) pk = std::max (pk, std::max (std::abs (L[i]), std::abs (R[i])));
    const float g = 0.9f / pk; for (size_t i = 0; i < total; ++i) { L[i] *= g; R[i] *= g; }
    writeWav (argv[1], L, R, sr);
    return 0;
}
