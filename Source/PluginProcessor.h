#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "Presets.h"
#include "Reverb80s.h"

// One entry per per-pad parameter: id suffix, display name, range, default comes from the Classic kit.
struct ParamSpec { const char* id; const char* name; float min, max, centre; const char* unit; };

class Hex1Processor : public juce::AudioProcessor
{
public:
    Hex1Processor();
    ~Hex1Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "WADAHEX"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return (int) factoryKits().size(); }
    int getCurrentProgram() override { return currentKit; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // UI helpers
    void triggerFromUI (int pad, float velocity) { uiHit[(size_t) pad].store (velocity); }
    float getPadMeter (int pad) const { return padMeter[(size_t) pad].load(); }
    float getOutputPeak() { return outPeak.exchange (0.0f); }
    static juce::String pid (int pad, const char* suffix) { return "p" + juce::String (pad) + "_" + suffix; }
    static const std::vector<ParamSpec>& specs();

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static BusesProperties makeBuses();
    void readPad (int pad, PadParams& p) const;
    void applyKit (const Kit& kit);
    int padForNote (int note) const;

    struct PadPtrs { std::array<std::atomic<float>*, 34> v {}; };
    std::array<PadPtrs, kNumPads> ptrs;
    std::atomic<float>* masterPtr = nullptr;

    std::vector<std::vector<int8_t>> chipBank;
    std::array<DrumVoice, kNumPads> voices;
    std::array<Reverb80s, kNumPads> reverbs;
    juce::AudioBuffer<float> scratch;
    std::array<PadParams, kNumPads> padParams;
    std::array<std::atomic<float>, kNumPads> uiHit {};
    std::array<std::atomic<float>, kNumPads> padMeter {};
    std::atomic<float> outPeak { 0.0f };
    uint32_t rng = 0x1234567u;
    int currentKit = 0;
    float masterSmoothed = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Hex1Processor)
};
