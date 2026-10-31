# WADAHEX — Hybrid Chip + Analog Drum Synth (by Yishay Raziel)

8-pad VST3 drum synth modelled on the SDS-V analog voice: triangle VCO with bend, swept filtered noise, impact click, resonant filter on noise + click, LFO — plus an optional 8-bit chip layer. All sounds are synthesised — no vintage samples.

## Features
- 8 pads (Kick, Snare, Tom 1–4, Clap, Perc), GM drum map + alt notes + chromatic C3–G3
- Per pad: Chip (8 chips, pitch, 2–8 bit), Tone (pitch, bend ±3 oct, bend time, decay), Noise (level, decay, color), Filter (cutoff, reso, env, decay), Output (drive, dynamics, level, pan)
- 80s reverb per pad (VERB + GATE for gated reverb), separate stereo output per pad
- 15 factory kits incl. SDS-V matched to real SDS5 recordings, full automation, state saved with the project
- Resizable UI (70–200%), velocity-sensitive clickable hex pads

## Build locally
```
git clone https://github.com/juce-framework/JUCE.git --branch 8.0.4 --depth 1   # optional; CMake fetches it otherwise
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Outputs land in `build/HEX1_artefacts/Release/` (VST3 + Standalone).

## Mac + Windows installers
Push to GitHub: `.github/workflows/build.yml` builds a universal macOS DMG (pkg installer + manual) and a Windows Inno Setup installer. Push a tag like `v1.0.0` to publish both as a GitHub Release.

## Manual
`docs/WADAHEX_Manual.pdf` (regenerate with `python3 docs/make_manual.py`).

## Tests
`tests/render_test.cpp` renders every pad of every kit plus a demo groove to WAV:
```
g++ -O2 -std=c++17 -ISource tests/render_test.cpp -o render_test && ./render_test .
```

Built with JUCE 8 (free Personal licence covers non-commercial / under-revenue-cap use; check JUCE terms before selling).
