#include "PluginProcessor.h"
#include "PluginEditor.h"

// Order here == order of fields read in readPad(). Index 0 (chip) is a choice parameter.
const std::vector<ParamSpec>& Hex1Processor::specs()
{
    static const std::vector<ParamSpec> s = {
        { "chip",       "Chip",         0, 7, 3, "" },
        { "chipLvl",    "Chip Level",   0, 1, 0.5f, "" },
        { "chipPitch",  "Chip Pitch",   -24, 24, 0, "st" },
        { "chipBits",   "Chip Bits",    2, 8, 5, "bit" },
        { "tonePitch",  "Pitch",        25, 1200, 150, "Hz" },
        { "bend",       "Bend",         -1, 1, 0, "" },
        { "bendTime",   "Bend Time",    5, 3000, 200, "ms" },
        { "toneDecay",  "Tone Decay",   10, 3000, 300, "ms" },
        { "toneLvl",    "Tone Level",   0, 1, 0.5f, "" },
        { "noiseLvl",   "Noise Level",  0, 1, 0.5f, "" },
        { "noiseDecay", "Noise Decay",  5, 2000, 150, "ms" },
        { "noiseColor", "Noise Color",  0, 1, 0.5f, "" },
        { "cutoff",     "Cutoff",       40, 18000, 1500, "Hz" },
        { "reso",       "Resonance",    0, 1, 0.5f, "" },
        { "filtEnv",    "Filter Env",   0, 1, 0.5f, "" },
        { "filtDecay",  "Filter Decay", 5, 2000, 150, "ms" },
        { "drive",      "Drive",        0, 1, 0.5f, "" },
        { "velSens",    "Dynamics",     0, 1, 0.5f, "" },
        { "level",      "Level",        -40, 6, -6, "dB" },
        { "pan",        "Pan",          -1, 1, 0, "" },
        { "click",      "Click",        0, 1, 0.5f, "" },
        { "shape",      "Shape",        0, 1, 0.5f, "" },
        { "toneFilt",   "Tone Thru Filter", 0, 1, 0.5f, "" },
        { "lfoRate",    "LFO Rate",     0.1f, 40, 5, "LHz" },
        { "lfoPitch",   "LFO Pitch",    0, 1, 0.5f, "" },
        { "lfoCut",     "LFO Cutoff",   0, 1, 0.5f, "" },
        { "noiseSweep", "Noise Sweep",  -1, 1, 0, "" },
        { "velDecay",   "Vel Decay",    0, 1, 0.5f, "" },
        { "analogBend", "Bend Curve",   0, 1, 0.5f, "" },
        { "toneLP",     "Tone LP",      150, 20000, 2500, "Hz" },
        { "noiseHP",    "Noise HP",     20, 3000, 250, "Hz" },
        { "verb",       "Reverb",       0, 1, 0.5f, "" },
        { "gate",       "Reverb Gate",  0, 1, 0.5f, "gate" },
        { "out",        "Output",       0, 1, 0.5f, "" },
    };
    return s;
}

static float fieldOf (const PadParams& p, size_t i)
{
    switch (i)
    {
        case 0: return (float) p.chip;     case 1: return p.chipLvl;    case 2: return p.chipPitch;  case 3: return p.chipBits;
        case 4: return p.tonePitch;        case 5: return p.bend;       case 6: return p.bendTime;   case 7: return p.toneDecay;
        case 8: return p.toneLvl;          case 9: return p.noiseLvl;   case 10: return p.noiseDecay;case 11: return p.noiseColor;
        case 12: return p.cutoff;          case 13: return p.reso;      case 14: return p.filtEnv;   case 15: return p.filtDecay;
        case 16: return p.drive;           case 17: return p.velSens;
        case 18: return juce::Decibels::gainToDecibels (p.level, -40.0f);
        case 19: return p.pan;             case 20: return p.click;     case 21: return p.shape;
        case 22: return p.toneFilt ? 1.0f : 0.0f;                       case 23: return p.lfoRate;
        case 24: return p.lfoPitch;        case 25: return p.lfoCut;    case 26: return p.noiseSweep;
        case 27: return p.velDecay;
        case 28: return p.analogBend ? 1.0f : 0.0f;
        case 29: return p.toneLP;
        case 30: return p.noiseHP;
        case 31: return p.verb;
        case 32: return p.gate;
        default: return p.ownOut ? 1.0f : 0.0f;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout Hex1Processor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const auto kit = factoryKits()[0].make();
    juce::StringArray chipNames;
    for (auto* n : chips::names) chipNames.add (n);

    for (int pad = 0; pad < kNumPads; ++pad)
    {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup> ("pad" + juce::String (pad), padInfo[pad].name, " | ");
        const auto& sp = specs();
        for (size_t i = 0; i < sp.size(); ++i)
        {
            const auto& s = sp[i];
            const juce::String id = pid (pad, s.id);
            const juce::String name = juce::String (padInfo[pad].name) + " " + s.name;
            const float def = fieldOf (kit[(size_t) pad], i);
            if (i == 33)
            {
                group->addChild (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name,
                                                                               juce::StringArray { "Main", "Own" }, def > 0.5f ? 1 : 0));
                continue;
            }
            if (i == 28)
            {
                group->addChild (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name,
                                                                               juce::StringArray { "Fast", "Analog" }, def > 0.5f ? 1 : 0));
                continue;
            }
            if (i == 22)
            {
                // a 2-step choice rather than AudioParameterBool: it snaps host values, so state restores exactly
                group->addChild (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name,
                                                                               juce::StringArray { "Dry", "Filtered" }, def > 0.5f ? 1 : 0));
                continue;
            }
            if (i == 0)
            {
                group->addChild (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, chipNames, (int) def));
                continue;
            }
            juce::NormalisableRange<float> r (s.min, s.max);
            if (s.centre > s.min && s.centre < s.max && (juce::String (s.unit) == "Hz" || juce::String (s.unit) == "ms" || juce::String (s.unit) == "LHz"))
                r.setSkewForCentre (s.centre);
            if (i == 3) r.interval = 1.0f;
            const juce::String unit = s.unit;
            auto toText = [unit] (float v, int) {
                if (unit == "gate") return v <= 0.005f ? juce::String ("OFF")
                                                       : juce::String ((int) std::round ((0.75f - 0.55f * std::sqrt (v)) * 1000.0f)) + " ms";
                if (unit == "LHz") return juce::String (v, v < 10 ? 2 : 1) + " Hz";
                if (unit == "Hz") return v >= 1000 ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String ((int) std::round (v)) + " Hz";
                if (unit == "ms") return v >= 1000 ? juce::String (v / 1000.0f, 2) + " s" : juce::String ((int) std::round (v)) + " ms";
                if (unit == "dB") return v <= -39.9f ? juce::String ("-inf") : juce::String (v, 1) + " dB";
                if (unit == "st") return (v > 0 ? "+" : "") + juce::String ((int) std::round (v)) + " st";
                if (unit == "bit") return juce::String ((int) std::round (v)) + " bit";
                return juce::String ((int) std::round (v * 100.0f)) + "%";
            };
            group->addChild (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction (toText)));
        }
        layout.add (std::move (group));
    }

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "master", 1 }, "Master",
        juce::NormalisableRange<float> (-24.0f, 6.0f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    return layout;
}

Hex1Processor::Hex1Processor()
    : AudioProcessor (makeBuses()),
      apvts (*this, nullptr, "HEX1", createLayout())
{
    for (int pad = 0; pad < kNumPads; ++pad)
        for (size_t i = 0; i < specs().size(); ++i)
            ptrs[(size_t) pad].v[i] = apvts.getRawParameterValue (pid (pad, specs()[i].id));
    masterPtr = apvts.getRawParameterValue ("master");

    for (int c = 0; c < chips::kNumChips; ++c) chipBank.push_back (chips::make (c));
    for (auto& h : uiHit) h.store (-1.0f);
}

juce::AudioProcessor::BusesProperties Hex1Processor::makeBuses()
{
    // main stereo out + one optional stereo out per pad (off until the host enables it)
    auto b = BusesProperties().withOutput ("Main", juce::AudioChannelSet::stereo(), true);
    for (int i = 0; i < kNumPads; ++i)
        b = b.withOutput (juce::String (i + 2) + " " + juce::String (padInfo[i].name), juce::AudioChannelSet::stereo(), false);
    return b;
}

bool Hex1Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (! (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono())) return false;
    if (! layouts.inputBuses.isEmpty()) for (auto& in : layouts.inputBuses) if (! in.isDisabled()) return false;
    for (int i = 1; i < layouts.outputBuses.size(); ++i)
    {
        const auto& set = layouts.outputBuses.getReference (i);
        if (! (set.isDisabled() || set == juce::AudioChannelSet::stereo())) return false;
    }
    return true;
}

void Hex1Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for (auto& v : voices) v.prepare (sampleRate, &chipBank);
    for (auto& r : reverbs) r.prepare (sampleRate);
    scratch.setSize (2 * kNumPads, juce::jmax (512, samplesPerBlock), false, true, false);
}

void Hex1Processor::readPad (int pad, PadParams& p) const
{
    const auto& v = ptrs[(size_t) pad].v;
    p.chip = (int) v[0]->load();  p.chipLvl = v[1]->load();  p.chipPitch = v[2]->load(); p.chipBits = v[3]->load();
    p.tonePitch = v[4]->load();   p.bend = v[5]->load();     p.bendTime = v[6]->load();  p.toneDecay = v[7]->load();
    p.toneLvl = v[8]->load();     p.noiseLvl = v[9]->load(); p.noiseDecay = v[10]->load(); p.noiseColor = v[11]->load();
    p.cutoff = v[12]->load();     p.reso = v[13]->load();    p.filtEnv = v[14]->load();  p.filtDecay = v[15]->load();
    p.drive = v[16]->load();      p.velSens = v[17]->load();
    p.level = v[18]->load() <= -39.9f ? 0.0f : juce::Decibels::decibelsToGain (v[18]->load());
    p.pan = v[19]->load();        p.click = v[20]->load();   p.shape = v[21]->load();
    p.toneFilt = v[22]->load() > 0.5f; p.lfoRate = v[23]->load(); p.lfoPitch = v[24]->load(); p.lfoCut = v[25]->load();
    p.noiseSweep = v[26]->load(); p.velDecay = v[27]->load(); p.analogBend = v[28]->load() > 0.5f;
    p.toneLP = v[29]->load(); p.noiseHP = v[30]->load();
    p.verb = v[31]->load(); p.gate = v[32]->load(); p.ownOut = v[33]->load() > 0.5f;
}

int Hex1Processor::padForNote (int note) const
{
    for (int i = 0; i < kNumPads; ++i)
        if (padInfo[i].note == note || padInfo[i].altNote == note) return i;
    // Chromatic fallback: C2..G2 octave above the GM kit also plays pads 0..7
    if (note >= 60 && note < 60 + kNumPads) return note - 60;
    return -1;
}

void Hex1Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    const int numSamples = buffer.getNumSamples();
    if (numSamples == 0) return;
    for (int pad = 0; pad < kNumPads; ++pad) readPad (pad, padParams[(size_t) pad]);

    // per-pad stereo scratch (pad p uses channels 2p, 2p+1)
    if (scratch.getNumSamples() < numSamples) scratch.setSize (2 * kNumPads, numSamples, false, false, true);
    scratch.clear (0, numSamples);

    // where each pad was hit inside this block (keys the reverb gate)
    std::array<std::array<int, 16>, kNumPads> hits {};
    std::array<int, kNumPads> numHits {};
    auto hitPad = [&] (int pad, float vel, int at)
    {
        voices[(size_t) pad].trigger (vel, padParams[(size_t) pad]);
        if (numHits[(size_t) pad] < 16) hits[(size_t) pad][(size_t) numHits[(size_t) pad]++] = at;
    };

    for (int pad = 0; pad < kNumPads; ++pad)
    {
        const float v = uiHit[(size_t) pad].exchange (-1.0f);
        if (v >= 0.0f) hitPad (pad, v, 0);
    }

    int pos = 0;
    auto renderTo = [&] (int end)
    {
        if (end <= pos) return;
        for (int pad = 0; pad < kNumPads; ++pad)
            voices[(size_t) pad].render (scratch.getWritePointer (2 * pad) + pos, scratch.getWritePointer (2 * pad + 1) + pos,
                                         end - pos, padParams[(size_t) pad], rng);
        pos = end;
    };
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (! msg.isNoteOn()) continue;
        const int at = juce::jlimit (0, numSamples, meta.samplePosition);
        renderTo (at);
        const int pad = padForNote (msg.getNoteNumber());
        if (pad >= 0) hitPad (pad, msg.getFloatVelocity(), at);
    }
    renderTo (numSamples);

    // reverb per pad, then route the pad to the main bus or its own bus
    auto mainBus = getBusBuffer (buffer, false, 0);
    float* mL = mainBus.getWritePointer (0);
    float* mR = mainBus.getNumChannels() > 1 ? mainBus.getWritePointer (1) : nullptr;
    for (int pad = 0; pad < kNumPads; ++pad)
    {
        const auto& pp = padParams[(size_t) pad];
        float* pL = scratch.getWritePointer (2 * pad);
        float* pR = scratch.getWritePointer (2 * pad + 1);
        auto& rv = reverbs[(size_t) pad];
        if (pp.verb > 0.001f || ! rv.isSilent())
        {
            int start = 0;
            for (int k = 0; k <= numHits[(size_t) pad]; ++k)
            {
                const int end = k < numHits[(size_t) pad] ? hits[(size_t) pad][(size_t) k] : numSamples;
                if (end > start) rv.process (pL + start, pR + start, pL + start, pR + start, end - start, pp.verb, pp.gate);
                if (k < numHits[(size_t) pad]) rv.trigger (pp.gate);
                start = juce::jmax (start, end);
            }
        }

        float* dL = mL; float* dR = mR;
        if (pp.ownOut)
            if (auto* bus = getBus (false, pad + 1); bus != nullptr && bus->isEnabled() && bus->getNumberOfChannels() == 2)
            {
                auto own = getBusBuffer (buffer, false, pad + 1);
                dL = own.getWritePointer (0); dR = own.getWritePointer (1);
            }
        if (dR != nullptr) { juce::FloatVectorOperations::add (dL, pL, numSamples); juce::FloatVectorOperations::add (dR, pR, numSamples); }
        else for (int i = 0; i < numSamples; ++i) dL[i] += 0.5f * (pL[i] + pR[i]);
    }

    // master gain (smoothed) + soft safety clip on every active output
    const float target = juce::Decibels::decibelsToGain (masterPtr->load());
    const float startGain = masterSmoothed;
    for (int i = 0; i < numSamples; ++i) masterSmoothed += 0.001f * (target - masterSmoothed);
    float pk = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        float gsm = startGain;
        for (int i = 0; i < numSamples; ++i)
        {
            gsm += 0.001f * (target - gsm);
            d[i] = std::tanh (d[i] * gsm);
            pk = std::max (pk, std::abs (d[i]));
        }
    }

    for (int pad = 0; pad < kNumPads; ++pad) padMeter[(size_t) pad].store (voices[(size_t) pad].meter());
    if (pk > outPeak.load()) outPeak.store (pk);
}

void Hex1Processor::applyKit (const Kit& kit)
{
    for (int pad = 0; pad < kNumPads; ++pad)
        for (size_t i = 0; i < specs().size(); ++i)
            if (i == 33) continue;   // a kit change never touches the pad's output routing
            else if (auto* p = apvts.getParameter (pid (pad, specs()[i].id)))
                p->setValueNotifyingHost (p->convertTo0to1 (fieldOf (kit[(size_t) pad], i)));
}

void Hex1Processor::setCurrentProgram (int index)
{
    const auto& kits = factoryKits();
    if (index < 0 || index >= (int) kits.size()) return;
    currentKit = index;
    applyKit (kits[(size_t) index].make());
}

const juce::String Hex1Processor::getProgramName (int index)
{
    const auto& kits = factoryKits();
    return (index >= 0 && index < (int) kits.size()) ? juce::String (kits[(size_t) index].name) : juce::String();
}

void Hex1Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("kit", currentKit, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void Hex1Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentKit = tree.getProperty ("kit", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* Hex1Processor::createEditor() { return new Hex1Editor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Hex1Processor(); }
