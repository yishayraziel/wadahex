#pragma once
#include "DrumVoice.h"
#include <array>
#include <functional>
#include <string>

static constexpr int kNumPads = 10;
static constexpr int kHatClosed = 8, kHatOpen = 9;

struct PadInfo { const char* name; int note; int altNote; };   // GM drum map (altNote -1 = none)
inline const PadInfo padInfo[kNumPads] = {
    { "KICK",  36, 35 }, { "SNARE", 38, 40 }, { "TOM 1", 48, 50 }, { "TOM 2", 45, 47 },
    { "TOM 3", 43, -1 }, { "TOM 4", 41, -1 }, { "CLAP",  39, -1 }, { "PERC",  37, 56 },
    { "HH CL", 42, 44 }, { "HH OP", 46, 49 }
};

// Hi-hats: six detuned square oscillators + bright noise through a steep high-pass.
inline PadParams hatClosed()
{
    PadParams q; q.metal = true; q.chip = 7; q.tonePitch = 420; q.bend = 0; q.analogBend = false; q.bendTime = 20;
    q.toneDecay = 75; q.toneLvl = 0.75f; q.noiseLvl = 0.65f; q.noiseDecay = 65; q.noiseColor = 1.0f; q.noiseHP = 6500;
    q.toneFilt = true; q.cutoff = 15000; q.reso = 0.25f; q.filtEnv = 0; q.click = 0.15f; q.drive = 0.15f;
    q.velSens = 0.65f; q.velDecay = 0.3f; q.level = 0.45f; q.pan = 0.25f; q.drift = 0.3f;
    return q;
}
inline PadParams hatOpen() { auto q = hatClosed(); q.toneDecay = 650; q.noiseDecay = 520; q.level = 0.4f; return q; }
inline void addHats (std::array<PadParams, kNumPads>& k)
{
    if (! k[kHatClosed].metal) k[kHatClosed] = hatClosed();
    if (! k[kHatOpen].metal)   k[kHatOpen]   = hatOpen();
}

using Kit = std::array<PadParams, kNumPads>;
#include "VintageLibrary.h"

// The reference kit: big bendy toms, crack snare, chip-assisted kick.
inline Kit classicKit()
{
    Kit k;
    for (auto& p : k) p.noiseHP = 400; // keep the hybrid kits' noise crisp
    auto& kick = k[0];
    kick.chip = 0; kick.chipLvl = 0.45f; kick.tonePitch = 48; kick.bend = 0.55f; kick.bendTime = 60;
    kick.toneDecay = 450; kick.toneLvl = 1.0f; kick.noiseLvl = 0.05f; kick.noiseDecay = 25; kick.noiseColor = 0.7f;
    kick.cutoff = 2500; kick.reso = 0.15f; kick.filtEnv = 0.25f; kick.filtDecay = 40; kick.drive = 0.35f; kick.level = 0.9f;

    auto& sn = k[1];
    sn.chip = 1; sn.chipLvl = 0.55f; sn.tonePitch = 190; sn.bend = 0.25f; sn.bendTime = 35; sn.toneDecay = 160;
    sn.toneLvl = 0.6f; sn.noiseLvl = 0.85f; sn.noiseDecay = 260; sn.noiseColor = 0.72f;
    sn.cutoff = 6000; sn.reso = 0.25f; sn.filtEnv = 0.35f; sn.filtDecay = 120; sn.drive = 0.25f; sn.level = 0.75f;

    const float tomPitch[4] = { 210, 160, 120, 88 };
    const float tomPan[4]   = { 0.35f, 0.12f, -0.12f, -0.35f };
    for (int t = 0; t < 4; ++t)
    {
        auto& tm = k[(size_t) (2 + t)];
        tm.chip = 2; tm.chipLvl = 0.3f; tm.chipPitch = (float) (6 - 4 * t);
        tm.tonePitch = tomPitch[t]; tm.bend = 0.6f; tm.bendTime = 380; tm.toneDecay = 700;
        tm.toneLvl = 0.9f; tm.noiseLvl = 0.12f; tm.noiseDecay = 90; tm.noiseColor = 0.4f;
        tm.cutoff = 3200; tm.reso = 0.35f; tm.filtEnv = 0.4f; tm.filtDecay = 250; tm.drive = 0.2f;
        tm.level = 0.75f; tm.pan = tomPan[t];
    }

    auto& cl = k[6];
    cl.chip = 3; cl.chipLvl = 0.8f; cl.tonePitch = 900; cl.bend = 0; cl.toneLvl = 0; cl.noiseLvl = 0.5f;
    cl.noiseDecay = 180; cl.noiseColor = 0.65f; cl.cutoff = 9000; cl.reso = 0.3f; cl.filtEnv = 0.1f;
    cl.drive = 0.15f; cl.level = 0.65f; cl.pan = -0.15f;

    auto& pc = k[7];
    pc.chip = 4; pc.chipLvl = 0.8f; pc.tonePitch = 620; pc.bend = 0.15f; pc.bendTime = 15; pc.toneDecay = 60;
    pc.toneLvl = 0.4f; pc.noiseLvl = 0.1f; pc.noiseDecay = 20; pc.noiseColor = 0.9f;
    pc.cutoff = 12000; pc.reso = 0.2f; pc.filtEnv = 0; pc.level = 0.6f; pc.pan = 0.2f;
    return k;
}

// Classic hex-pad voicing: triangle VCO left dry, filter on noise + click only, click on every
// channel, no chips on the drum channels.
inline Kit handTunedKit()
{
    // Voiced against measurements of vintage hardware one-shots: strike pitch decaying in Hz
    // (tau ~0.19 s kick, ~0.17 s snare, ~0.3 s toms), ~2 octaves audible fall, noise on
    // toms and snare, bright impact click. bendTime = tau * 6.9 (it is a 60 dB time).
    Kit k;
    for (auto& p : k)
    {
        p.chip = 7; p.shape = 0.9f; p.toneFilt = false; p.analogBend = true; p.velSens = 0.7f; p.velDecay = 0.35f;
        p.drive = 0.25f; p.lfoRate = 6; p.lfoPitch = 0; p.lfoCut = 0; p.noiseSweep = 0; p.noiseHP = 300;
    }

    auto& bd = k[0];
    bd.tonePitch = 150; bd.bend = 1.0f; bd.bendTime = 780; bd.toneDecay = 390; bd.toneLvl = 1.0f;
    bd.noiseLvl = 0.3f; bd.noiseDecay = 70; bd.noiseColor = 0.55f; bd.click = 1.0f; bd.shape = 1.0f;
    bd.cutoff = 2500; bd.reso = 0.0f; bd.filtEnv = 0.3f; bd.filtDecay = 30; bd.drive = 0.3f; bd.level = 0.9f;

    auto& sn = k[1];
    sn.tonePitch = 320; sn.bend = 1.0f; sn.bendTime = 1030; sn.toneDecay = 460; sn.toneLvl = 0.75f;
    sn.noiseLvl = 0.8f; sn.noiseDecay = 360; sn.noiseColor = 0.68f; sn.noiseSweep = 0.2f; sn.click = 0.5f;
    sn.cutoff = 3500; sn.reso = 0.1f; sn.filtEnv = 0.4f; sn.filtDecay = 120; sn.level = 0.75f;

    const float tomPitch[4] = { 330, 260, 200, 150 };
    const float tomPan[4]   = { 0.35f, 0.12f, -0.12f, -0.35f };
    for (int t = 0; t < 4; ++t)
    {
        auto& tm = k[(size_t) (2 + t)];
        tm.tonePitch = tomPitch[t]; tm.bend = 0.68f; tm.bendTime = 1600; tm.toneDecay = 800; tm.toneLvl = 0.9f;
        tm.noiseLvl = 0.6f; tm.noiseDecay = 370; tm.noiseColor = 0.7f; tm.click = 1.0f;
        tm.cutoff = 4500; tm.reso = 0.2f; tm.filtEnv = 0.4f; tm.filtDecay = 200;
        tm.level = 0.75f; tm.pan = tomPan[t];
    }

    auto& cl = k[6];   // noise clap with the clap chip for the bursts
    cl.chip = 3; cl.chipLvl = 0.7f; cl.chipBits = 8; cl.toneLvl = 0; cl.noiseLvl = 0.55f; cl.noiseDecay = 200; cl.analogBend = false;
    cl.noiseColor = 0.65f; cl.click = 0.2f; cl.cutoff = 8000; cl.reso = 0.25f; cl.filtEnv = 0.1f; cl.level = 0.65f; cl.pan = -0.15f;

    auto& rim = k[7];  // rimshot: high strike, very fast fall, lots of click, a touch of "rimshot modulation"
    rim.tonePitch = 1300; rim.bend = 0.5f; rim.bendTime = 140; rim.toneDecay = 65; rim.toneLvl = 0.7f;
    rim.lfoRate = 37; rim.lfoPitch = 0.3f; rim.noiseLvl = 0.25f; rim.noiseDecay = 40; rim.noiseColor = 0.9f;
    rim.click = 0.9f; rim.cutoff = 9000; rim.reso = 0.2f; rim.filtEnv = 0; rim.level = 0.6f; rim.pan = 0.2f;
    return k;
}

struct KitDef { const char* name; std::function<Kit()> make; };

inline const std::vector<KitDef>& rawFactoryKits()
{
    static const std::vector<KitDef> kits = {
        { "Hex Classic", [] { return hexKits()[0].make(); } },
        { "Hex Wobble", [] {
            auto k = hexKits()[0].make();
            for (int t = 2; t < 6; ++t) { k[(size_t) t].lfoRate = 9; k[(size_t) t].lfoPitch = 0.3f; k[(size_t) t].toneDecay = 1300; }
            k[1].lfoRate = 4; k[1].lfoCut = 0.4f;
            return k; } },
        { "Vintage Kit 1", [] { return hexKits()[1].make(); } },
        { "Vintage Kit 2", [] { return hexKits()[2].make(); } },
        { "Vintage Kit 3", [] { return hexKits()[3].make(); } },
        { "Vintage Kit 4", [] { return hexKits()[4].make(); } },
        { "Vintage Kit 5", [] { return hexKits()[5].make(); } },
        { "Vintage Kit 6", [] { return hexKits()[6].make(); } },
        { "Hex Hand-Tuned", [] { return handTunedKit(); } },
        { "Hybrid Hex", [] { return classicKit(); } },
        { "Pew Pew Toms", [] {
            auto k = classicKit();
            for (int t = 2; t < 6; ++t) { k[(size_t) t].bend = 0.95f; k[(size_t) t].bendTime = 700; k[(size_t) t].toneDecay = 1300; k[(size_t) t].reso = 0.55f; }
            k[0].bend = 0.8f; k[0].bendTime = 140;
            return k; } },
        { "Electro '84", [] {
            auto k = classicKit();
            k[0].toneDecay = 1100; k[0].bend = 0.35f; k[0].drive = 0.55f; k[0].chipLvl = 0.6f;
            k[1].noiseDecay = 140; k[1].noiseColor = 0.85f; k[1].chip = 6; k[1].chipLvl = 0.35f;
            for (int t = 2; t < 6; ++t) { k[(size_t) t].toneDecay = 350; k[(size_t) t].bendTime = 120; k[(size_t) t].chip = 6; k[(size_t) t].chipLvl = 0.2f; }
            k[7].chip = 5; k[7].chipLvl = 0.6f; k[7].toneLvl = 0; k[7].cutoff = 14000;
            return k; } },
        { "Chip Crush", [] {
            auto k = classicKit();
            for (auto& p : k) { p.chipLvl = std::min (1.0f, p.chipLvl + 0.45f); p.chipBits = 4; p.toneLvl *= 0.45f; p.noiseLvl *= 0.4f; p.drive = 0.5f; }
            return k; } },
        { "Gated Arena", [] {
            auto k = classicKit();
            k[1].verb = 0.6f; k[1].gate = 0.55f; k[6].verb = 0.45f; k[6].gate = 0.55f;
            for (int t = 2; t < 6; ++t) { k[(size_t) t].verb = 0.45f; k[(size_t) t].gate = 0.4f; }
            k[1].noiseDecay = 520; k[1].noiseLvl = 1.0f; k[1].drive = 0.45f; k[1].reso = 0.15f;
            k[0].toneDecay = 650; k[0].drive = 0.5f;
            for (int t = 2; t < 6; ++t) { k[(size_t) t].toneDecay = 1100; k[(size_t) t].noiseLvl = 0.3f; k[(size_t) t].noiseDecay = 300; k[(size_t) t].drive = 0.4f; }
            return k; } },
    };
    return kits;
}

// Every factory kit gets hi-hats; the matched hex kits also get a touch of analog drift.
inline const std::vector<KitDef>& factoryKits()
{
    static const std::vector<KitDef> kits = [] {
        std::vector<KitDef> out;
        for (const auto& k : rawFactoryKits())
        {
            const std::string nm (k.name);
            const bool matched = nm.rfind ("Hex", 0) == 0 || nm.rfind ("Vintage", 0) == 0;
            auto make = k.make;
            out.push_back ({ k.name, [make, matched] {
                auto kit = make();
                addHats (kit);
                if (matched) for (int p = 0; p < 8; ++p) if (kit[(size_t) p].drift == 0.0f) kit[(size_t) p].drift = 0.2f;
                return kit; } });
        }
        return out;
    }();
    return kits;
}
