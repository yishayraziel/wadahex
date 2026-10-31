#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace hexui
{
const juce::Colour cream   { 0xffece2cf };
const juce::Colour red     { 0xffff2a1a };
const juce::Colour redDeep { 0xff8a0d06 };
const juce::Colour cyan    { 0xff4fd6ff };
const juce::Colour valBlue { 0xff8fd3ff };
const juce::Colour grey    { 0xff9a948a };

juce::Font font (float h, bool heavy = true);
}

class HexLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HexLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getPopupMenuFont() override { return hexui::font (19.0f, false); }
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
};

// Honeycomb of 8 clickable hexagon pads (coordinates are in skin pixels).
class PadGrid : public juce::Component, private juce::Timer
{
public:
    explicit PadGrid (Hex1Processor& p);
    std::function<void (int)> onSelect;
    void setSelected (int pad) { selected = pad; repaint(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<float> pillRect (int pad, int which) const;   // which: 0 = mute, 1 = solo
    int lastMuteSolo = -1;
    Hex1Processor& proc;
    std::array<juce::Path, kNumPads> hexes;
    std::array<juce::Point<float>, kNumPads> centres;
    std::array<float, kNumPads> glow {};
    float radius = 124;
    int selected = 0;
};

class Hex1Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Hex1Editor (Hex1Processor&);
    ~Hex1Editor() override;
    void paint (juce::Graphics&) override {}
    void resized() override;

    // the skin is authored at this size; the editor scales it
    static constexpr int skinW = 1672, skinH = 941;

private:
    struct Content : public juce::Component
    {
        explicit Content (Hex1Editor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        Hex1Editor& ed;
    };
    struct Knob { juce::Slider slider; juce::Label label; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att; int spec = 0; };

    void timerCallback() override;
    void selectPad (int pad);
    void layoutContent();

    Hex1Processor& proc;
    HexLookAndFeel lnf;
    Content content { *this };
    PadGrid pads;
    juce::Image plate, logo;
    juce::ComboBox kitBox, chipBox;
    juce::TextButton prevKit, nextKit;
    juce::TextButton toneFiltBtn { "DRY" }, curveBtn { "FAST" }, outBtn { "OUT: MAIN" }, followBtn { "SELECT: CLICK" };
    int lastMidiCount = 0;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> outAtt;
    juce::Label chipLabel, toneFiltLabel, curveLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> chipAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> toneFiltAtt, curveAtt;
    std::vector<std::unique_ptr<Knob>> knobs;
    Knob master;
    float meterL = 0.0f, meterHold = 0.0f;
    int holdFrames = 0;
    int selectedPad = 0;

    friend struct Content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Hex1Editor)
};
