#include "PluginEditor.h"
#include "BinaryData.h"

using namespace hexui;

juce::Font hexui::font (float h, bool heavy)
{
    static auto tf700 = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensed700_ttf, BinaryData::BarlowCondensed700_ttfSize);
    static auto tf600 = juce::Typeface::createSystemTypefaceFor (BinaryData::BarlowCondensed600_ttf, BinaryData::BarlowCondensed600_ttfSize);
    return juce::Font (juce::FontOptions (heavy ? tf700 : tf600).withHeight (h));
}

static juce::Path makeHex (juce::Point<float> c, float r)
{
    juce::Path p;
    for (int i = 0; i < 6; ++i)
    {
        const float a = juce::MathConstants<float>::pi / 180.0f * (60.0f * (float) i - 90.0f); // pointy top
        const juce::Point<float> v (c.x + r * std::cos (a), c.y + r * std::sin (a));
        if (i == 0) p.startNewSubPath (v); else p.lineTo (v);
    }
    p.closeSubPath();
    return p;
}

// ============================================================ LookAndFeel
HexLookAndFeel::HexLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, valBlue);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, cyan.withAlpha (0.35f));
    setColour (juce::ComboBox::textColourId, cream);
    setColour (juce::ComboBox::arrowColourId, red);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff101418));
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, cyan.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::Label::textColourId, cream);
    setColour (juce::TextEditor::textColourId, valBlue);
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0b1015));
    setColour (juce::TextEditor::highlightColourId, cyan.withAlpha (0.3f));
    setColour (juce::CaretComponent::caretColourId, cyan);
}

void HexLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto c = juce::Rectangle<int> (x, y, w, h).toFloat().getCentre();
    const float r = s.getProperties().contains ("r") ? (float) s.getProperties()["r"] : juce::jmin (w, h) * 0.5f - 6.0f;
    const float ang = start + pos * (end - start);
    const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0;
    const auto rounded = [] (float wdt) { return juce::PathStrokeType (wdt, juce::PathStrokeType::curved, juce::PathStrokeType::rounded); };

    // dim track over the whole travel
    juce::Path track; track.addCentredArc (c.x, c.y, r, r, 0, start, end, true);
    g.setColour (juce::Colour (0xff5a3a26).withAlpha (0.75f)); g.strokePath (track, rounded (3.2f));

    // glowing value arc
    const float from = bipolar ? (start + end) * 0.5f : start;
    juce::Path arc; arc.addCentredArc (c.x, c.y, r, r, 0, juce::jmin (from, ang), juce::jmax (from, ang), true);
    g.setColour (red.withAlpha (0.10f)); g.strokePath (arc, rounded (13.0f));
    g.setColour (red.withAlpha (0.22f)); g.strokePath (arc, rounded (8.0f));
    g.setColour (red);                    g.strokePath (arc, rounded (3.4f));
    g.setColour (juce::Colour (0xffffb4a2)); g.strokePath (arc, rounded (1.2f));

    // cap
    const float cr = r - 8.0f;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (c.x - cr - 1, c.y - cr + 3, cr * 2 + 2, cr * 2 + 2);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c2f), c.x - cr * 0.4f, c.y - cr * 0.6f,
                                             juce::Colour (0xff040404), c.x + cr * 0.5f, c.y + cr, true));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    // knurled rim
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    for (int i = 0; i < 40; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 40.0f;
        g.drawLine (c.x + std::cos (a) * cr * 0.84f, c.y + std::sin (a) * cr * 0.84f, c.x + std::cos (a) * cr * 0.98f, c.y + std::sin (a) * cr * 0.98f, 1.0f);
    }
    // top face + soft highlight
    const float fr = cr * 0.80f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff232326), c.x, c.y - fr, juce::Colour (0xff0a0a0b), c.x, c.y + fr, false));
    g.fillEllipse (c.x - fr, c.y - fr, fr * 2, fr * 2);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.16f), c.x, c.y - fr, juce::Colours::transparentWhite, c.x, c.y, false));
    g.fillEllipse (c.x - fr * 0.8f, c.y - fr * 0.95f, fr * 1.6f, fr * 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f)); g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, 1.0f);

    // pointer
    const auto dir = juce::Point<float> (std::sin (ang), -std::cos (ang));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawLine (juce::Line<float> (c + dir * (cr * 0.30f) + juce::Point<float> (0, 1), c + dir * (cr * 0.86f) + juce::Point<float> (0, 1)), 4.0f);
    g.setColour (juce::Colour (0xfff4efe6));
    g.drawLine (juce::Line<float> (c + dir * (cr * 0.30f), c + dir * (cr * 0.86f)), 3.0f);
}

void HexLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    if (box.getName() == "lcd") return; // the LCD window + triangle are part of the skin
    auto b = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.0f);
    g.setColour (juce::Colour (0xff0d0d0f)); g.fillRoundedRectangle (b, 5.0f);
    g.setColour (juce::Colour (0xff4a403a)); g.drawRoundedRectangle (b, 5.0f, 1.2f);
    juce::Path arrow; const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 6, ay - 3, ax + 6, ay - 3, ax, ay + 5);
    g.setColour (red); g.fillPath (arrow);
}

juce::Font HexLookAndFeel::getComboBoxFont (juce::ComboBox& box) { return box.getName() == "lcd" ? font (34.0f) : font (22.0f, false); }

void HexLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    if (box.getName() == "lcd") label.setBounds (14, 0, box.getWidth() - 60, box.getHeight());
    else                        label.setBounds (8, 0, box.getWidth() - 30, box.getHeight());
    label.setFont (getComboBoxFont (box));
}

juce::Label* HexLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (font (21.0f, false));
    l->setColour (juce::Label::textColourId, valBlue);
    l->setColour (juce::Label::outlineWhenEditingColourId, cyan);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundWhenEditingColourId, juce::Colour (0xff0b1015));
    l->setColour (juce::Label::textWhenEditingColourId, valBlue);
    return l;
}

juce::Font HexLookAndFeel::getLabelFont (juce::Label& l)
{
    if (dynamic_cast<juce::Slider*> (l.getParentComponent()) != nullptr) return font (22.0f, false);
    return l.getFont();
}

void HexLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    if (b.getName() == "invisible")
    {
        if (over || down) { g.setColour (cyan.withAlpha (down ? 0.25f : 0.12f)); g.fillRoundedRectangle (b.getLocalBounds().toFloat().reduced (3), 4.0f); }
        return;
    }
    // LCD-style cyan switch, like the DRY button in the design
    auto r = b.getLocalBounds().toFloat().reduced (3.0f);
    const bool on = b.getToggleState();
    g.setColour (cyan.withAlpha (0.12f)); g.drawRoundedRectangle (r.expanded (2.0f), 6.0f, 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (on ? 0xff123345 : 0xff0b1a24), r.getX(), r.getY(),
                                             juce::Colour (on ? 0xff0a2230 : 0xff07121a), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (cyan.withAlpha (over ? 1.0f : 0.85f)); g.drawRoundedRectangle (r, 5.0f, 1.8f);
}

void HexLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    if (b.getName() == "invisible") return;
    g.setFont (font (22.0f));
    g.setColour (cyan.withAlpha (0.35f));
    g.drawText (b.getButtonText(), b.getLocalBounds().translated (0, 1), juce::Justification::centred);
    g.setColour (juce::Colour (0xffb8eeff));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

void HexLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
    g.setColour (cyan.withAlpha (0.5f)); g.drawRect (0, 0, w, h, 1);
}

// ============================================================ Pads
PadGrid::PadGrid (Hex1Processor& p) : proc (p) { startTimerHz (40); }

void PadGrid::resized()
{
    // centres from the design, in skin pixels (this component sits at the skin origin)
    const juce::Point<float> c[kNumPads] = {
        { 380, 712 }, { 158, 712 }, { 158, 340 }, { 380, 340 }, { 602, 340 }, { 269, 526 }, { 491, 526 }, { 602, 712 } };
    for (int i = 0; i < kNumPads; ++i) { centres[(size_t) i] = c[i]; hexes[(size_t) i] = makeHex (c[i], radius); }
}

void PadGrid::paint (juce::Graphics& g)
{
    // 1) red halo behind lit / selected pads
    for (int i = 0; i < kNumPads; ++i)
    {
        const float gl = juce::jmax (glow[(size_t) i], i == selected ? 0.25f : 0.0f);
        if (gl < 0.01f) continue;
        for (int k = 0; k < 4; ++k)
        {
            g.setColour (red.withAlpha (gl * (0.10f - 0.02f * (float) k)));
            g.strokePath (hexes[(size_t) i], juce::PathStrokeType (10.0f + 12.0f * (float) k, juce::PathStrokeType::curved));
        }
    }

    for (int i = 0; i < kNumPads; ++i)
    {
        const auto c = centres[(size_t) i];
        const float R = radius;
        const float gl = glow[(size_t) i];
        const bool sel = i == selected;

        // drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillPath (hexes[(size_t) i], juce::AffineTransform::translation (0, 6));

        // metallic beige bevel
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff6ead0), c.x - R * 0.3f, c.y - R,
                                                 juce::Colour (0xff806f57), c.x + R * 0.3f, c.y + R, false));
        g.fillPath (hexes[(size_t) i]);
        auto bevelIn = makeHex (c, R - 6.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffcdbd9c), c.x, c.y + R, juce::Colour (0xffb3a283), c.x, c.y - R, false));
        g.fillPath (bevelIn);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.strokePath (hexes[(size_t) i], juce::PathStrokeType (1.4f));

        // groove + rubber face
        auto groove = makeHex (c, R - 13.0f);
        g.setColour (juce::Colour (0xff1a1210)); g.fillPath (groove);
        auto face = makeHex (c, R - 16.0f);
        const auto mid = juce::Colour (0xff2b2220).interpolatedWith (juce::Colour (0xffff3a22), gl);
        const auto edge = juce::Colour (0xff120d0c).interpolatedWith (redDeep, gl);
        g.setGradientFill (juce::ColourGradient (mid, c.x, c.y - R * 0.15f, edge, c.x + R * 0.9f, c.y + R * 0.6f, true));
        g.fillPath (face);
        // fine rubber grain
        g.saveState(); g.reduceClipRegion (face);
        g.setColour (juce::Colours::white.withAlpha (0.018f));
        for (float yy = c.y - R; yy < c.y + R; yy += 3.0f) g.drawHorizontalLine ((int) yy, c.x - R, c.x + R);
        g.restoreState();

        // red rim light inside the face, strongest on the top edges (like the design)
        auto rim = makeHex (c, R - 19.0f);
        g.setColour (red.withAlpha (0.28f + 0.6f * gl + (sel ? 0.25f : 0.0f)));
        g.strokePath (rim, juce::PathStrokeType (2.2f));
        {
            juce::Path top; const float rr = R - 19.0f;
            top.startNewSubPath (c.x - rr * 0.866f, c.y - rr * 0.5f); top.lineTo (c.x, c.y - rr); top.lineTo (c.x + rr * 0.866f, c.y - rr * 0.5f);
            g.setColour (red.withAlpha (0.18f)); g.strokePath (top, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (juce::Colour (0xffff6a50).withAlpha (0.75f)); g.strokePath (top, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        if (gl > 0.02f)
        {
            g.setColour (juce::Colour (0xffffd0c4).withAlpha (gl * 0.9f));
            g.strokePath (face, juce::PathStrokeType (2.5f));
        }
        if (sel)
        {
            g.setColour (juce::Colour (0xffffe9dd).withAlpha (0.85f));
            g.strokePath (makeHex (c, R - 1.0f), juce::PathStrokeType (2.0f));
        }

        // mute: dim the whole face
        if (proc.isMuted (i))
        {
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillPath (makeHex (c, R - 14.0f));
        }
        else if (proc.isSoloed (i))
        {
            g.setColour (cyan.withAlpha (0.8f));
            g.strokePath (makeHex (c, R - 15.0f), juce::PathStrokeType (2.5f));
        }
        // M / S pills
        for (int k = 0; k < 2; ++k)
        {
            const auto r = pillRect (i, k);
            const bool on = k == 0 ? proc.isMuted (i) : proc.isSoloed (i);
            const auto col = k == 0 ? juce::Colour (0xffffb020) : cyan;
            g.setColour (on ? col : juce::Colour (0xff0d0b0a).withAlpha (0.85f));
            g.fillRoundedRectangle (r, 5.0f);
            g.setColour (on ? col.brighter (0.4f) : col.withAlpha (0.55f));
            g.drawRoundedRectangle (r, 5.0f, 1.4f);
            g.setFont (font (18.0f));
            g.setColour (on ? juce::Colour (0xff111111) : col.withAlpha (0.85f));
            g.drawText (k == 0 ? "M" : "S", r, juce::Justification::centred);
        }

        // labels
        const auto nameBox = juce::Rectangle<float> (c.x - R, c.y - 36.0f, R * 2, 40.0f);
        g.setFont (font (36.0f));
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawText (padInfo[i].name, nameBox.translated (0, 2), juce::Justification::centred);
        g.setColour (juce::Colour (0xfff3e9da)); g.drawText (padInfo[i].name, nameBox, juce::Justification::centred);
        g.setFont (font (23.0f, false));
        g.setColour (gl > 0.5f ? juce::Colour (0xffffe0d6) : juce::Colour (0xffff3b2a));
        g.drawText (juce::MidiMessage::getMidiNoteName (padInfo[i].note, true, true, 3),
                    juce::Rectangle<float> (c.x - R, c.y + 8.0f, R * 2, 28.0f), juce::Justification::centred);
    }
}

juce::Rectangle<float> PadGrid::pillRect (int pad, int which) const
{
    const auto c = centres[(size_t) pad];
    return { c.x + (which == 0 ? -40.0f : 6.0f), c.y + 48.0f, 34.0f, 24.0f };
}

void PadGrid::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < kNumPads; ++i)
        for (int k = 0; k < 2; ++k)
            if (pillRect (i, k).expanded (2.0f).contains (e.position))
            {
                proc.toggle (i, k == 0 ? "mute" : "solo");
                repaint();
                return;
            }
    for (int i = 0; i < kNumPads; ++i)
        if (hexes[(size_t) i].contains (e.position))
        {
            // hit the centre for a hard hit, the edge for a soft one
            const float d = e.position.getDistanceFrom (centres[(size_t) i]) / radius;
            proc.triggerFromUI (i, juce::jlimit (0.25f, 1.0f, 1.05f - d * 0.8f));
            selected = i;
            if (onSelect) onSelect (i);
            repaint();
            return;
        }
}

void PadGrid::timerCallback()
{
    bool dirty = false;
    int ms = 0;
    for (int i = 0; i < kNumPads; ++i) ms |= (proc.isMuted (i) ? 1 : 0) << i | (proc.isSoloed (i) ? 1 : 0) << (i + 8);
    if (ms != lastMuteSolo) { lastMuteSolo = ms; dirty = true; }
    for (int i = 0; i < kNumPads; ++i)
    {
        const float target = juce::jlimit (0.0f, 1.0f, proc.getPadMeter (i) * 1.4f);
        auto& gl = glow[(size_t) i];
        const float next = target > gl ? target : gl * 0.84f;
        if (std::abs (next - gl) > 0.003f) { gl = next; dirty = true; }
    }
    if (dirty) repaint();
}

// ============================================================ Layout table (skin pixels)
namespace
{
struct Cell { int spec; const char* label; };   // spec >= 0 knob, -1 chip box, -2 tone-in switch, -3 bend curve switch
struct Sec { int x0, x1, labelY, knobY, valueY; float knobR; std::vector<Cell> cells; };
const std::vector<Sec>& sections()
{
    static const std::vector<Sec> s = {
        { 757, 1250, 253, 313, 376, 30.0f, { { 4, "PITCH" }, { 5, "BEND" }, { 6, "BEND TIME" }, { -3, "CURVE" }, { 7, "DECAY" }, { 21, "SHAPE" }, { 8, "LEVEL" } } },
        { 1277, 1641, 253, 313, 376, 31.0f, { { 9, "LEVEL" }, { 10, "DECAY" }, { 11, "COLOR" }, { 26, "SWEEP" }, { 30, "HP" } } },
        { 757, 1121, 508, 566, 626, 34.0f, { { -1, "SOUND" }, { 1, "LEVEL" }, { 2, "PITCH" }, { 3, "BITS" } } },
        { 1149, 1641, 508, 566, 626, 32.0f, { { 12, "CUTOFF" }, { 13, "RESO" }, { 14, "ENV" }, { 15, "DECAY" }, { 29, "TONE LP" }, { -2, "TONE IN" } } },
        { 757, 1031, 745, 803, 864, 34.0f, { { 23, "RATE" }, { 24, "PITCH" }, { 25, "CUTOFF" } } },
        { 1060, 1641, 745, 803, 864, 26.0f, { { 20, "CLICK" }, { 16, "DRIVE" }, { 17, "DYNAMICS" }, { 27, "VEL DCY" }, { 31, "VERB" }, { 34, "V.TONE" }, { 32, "GATE" }, { 18, "LEVEL" }, { 19, "PAN" } } },
    };
    return s;
}
}

static void styleLabel (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setJustificationType (juce::Justification::centred);
    l.setFont (font (18.0f, false));
    l.setColour (juce::Label::textColourId, cream);
    l.setInterceptsMouseClicks (false, false);
}

static void styleKnob (juce::Slider& s, float r)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 96, 24);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.setMouseDragSensitivity (260);
    s.getProperties().set ("r", r);
}

// ============================================================ Editor
Hex1Editor::Hex1Editor (Hex1Processor& p) : AudioProcessorEditor (p), proc (p), pads (p)
{
    setLookAndFeel (&lnf);
    plate = juce::ImageCache::getFromMemory (BinaryData::plate_png, BinaryData::plate_pngSize);
    logo  = juce::ImageCache::getFromMemory (BinaryData::logo_lockup_png, BinaryData::logo_lockup_pngSize);

    addAndMakeVisible (content);
    content.addAndMakeVisible (pads);
    pads.onSelect = [this] (int i) { selectPad (i); };

    for (auto& sec : sections())
        for (auto& c : sec.cells)
        {
            if (c.spec < 0) continue;
            auto k = std::make_unique<Knob>();
            k->spec = c.spec;
            styleKnob (k->slider, sec.knobR);
            styleLabel (k->label, c.label);
            content.addAndMakeVisible (k->slider); content.addAndMakeVisible (k->label);
            knobs.push_back (std::move (k));
        }
    styleLabel (chipLabel, "SOUND"); styleLabel (toneFiltLabel, "TONE IN"); styleLabel (curveLabel, "CURVE");
    for (auto* l : { &chipLabel, &toneFiltLabel, &curveLabel }) content.addAndMakeVisible (*l);

    curveBtn.setClickingTogglesState (true);
    curveBtn.onStateChange = [this] { curveBtn.setButtonText (curveBtn.getToggleState() ? "ANLG" : "FAST"); };
    curveBtn.setTooltip ("ANLG = SDS-V analog curve: PITCH is the strike pitch, falling in Hz over BEND TIME. FAST = quick sweep down to PITCH.");
    toneFiltBtn.setClickingTogglesState (true);
    toneFiltBtn.onStateChange = [this] { toneFiltBtn.setButtonText (toneFiltBtn.getToggleState() ? "FILTERED" : "DRY"); };
    toneFiltBtn.setTooltip ("DRY = SDS-V routing: filter acts on noise + click only. FILTERED = whole voice through the filter.");
    content.addAndMakeVisible (curveBtn); content.addAndMakeVisible (toneFiltBtn);
    outBtn.setClickingTogglesState (true);
    outBtn.onStateChange = [this] { outBtn.setButtonText (outBtn.getToggleState() ? "OUT: OWN" : "OUT: MAIN"); };
    outBtn.setTooltip ("MAIN = the pad plays on the main stereo output. OWN = the pad goes to its own stereo output "
                       "(enable WADAHEX's extra outputs in your DAW: 2 KICK, 3 SNARE ... 9 PERC).");
    content.addAndMakeVisible (outBtn);
    followBtn.setClickingTogglesState (true);
    followBtn.setToggleState (proc.followMidi.load(), juce::dontSendNotification);
    followBtn.setButtonText (proc.followMidi.load() ? "SELECT: MIDI" : "SELECT: CLICK");
    followBtn.onClick = [this] {
        proc.followMidi.store (followBtn.getToggleState());
        followBtn.setButtonText (followBtn.getToggleState() ? "SELECT: MIDI" : "SELECT: CLICK");
    };
    followBtn.setTooltip ("CLICK = choose the pad to edit by clicking it. MIDI = the editor jumps to whichever pad your MIDI plays.");
    content.addAndMakeVisible (followBtn);
    lastMidiCount = proc.midiHitCount.load();

    for (int i = 0; i < chips::kNumChips; ++i) chipBox.addItem (chips::names[i], i + 1);
    content.addAndMakeVisible (chipBox);

    kitBox.setName ("lcd");
    kitBox.setColour (juce::ComboBox::textColourId, cyan);
    for (int i = 0; i < proc.getNumPrograms(); ++i) kitBox.addItem (proc.getProgramName (i), i + 1);
    kitBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    kitBox.onChange = [this] { proc.setCurrentProgram (kitBox.getSelectedId() - 1); };
    prevKit.setName ("invisible"); nextKit.setName ("invisible");
    prevKit.onClick = [this] { const int n = kitBox.getNumItems(); kitBox.setSelectedId ((kitBox.getSelectedId() - 2 + n) % n + 1); };
    nextKit.onClick = [this] { const int n = kitBox.getNumItems(); kitBox.setSelectedId (kitBox.getSelectedId() % n + 1); };
    content.addAndMakeVisible (kitBox); content.addAndMakeVisible (prevKit); content.addAndMakeVisible (nextKit);

    styleKnob (master.slider, 40.0f);
    master.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    master.slider.setTooltip ("Master level");
    master.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "master", master.slider);
    content.addAndMakeVisible (master.slider);

    selectPad (0);
    layoutContent();
    sendLookAndFeelChange();   // children were created before they had a parent: give them the skin's look

    setResizable (true, true);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) skinW / skinH);
    setResizeLimits (skinW * 55 / 100, skinH * 55 / 100, skinW * 120 / 100, skinH * 120 / 100);
    setSize (skinW * 72 / 100, skinH * 72 / 100);
    startTimerHz (30);
}

Hex1Editor::~Hex1Editor() { setLookAndFeel (nullptr); }

void Hex1Editor::selectPad (int pad)
{
    selectedPad = pad;
    pads.setSelected (pad);
    for (auto& k : knobs)
    {
        k->att.reset();
        k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            proc.apvts, Hex1Processor::pid (pad, Hex1Processor::specs()[(size_t) k->spec].id), k->slider);
    }
    chipAtt.reset();
    chipAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, Hex1Processor::pid (pad, "chip"), chipBox);
    toneFiltAtt.reset();
    toneFiltAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, Hex1Processor::pid (pad, "toneFilt"), toneFiltBtn);
    toneFiltBtn.setButtonText (toneFiltBtn.getToggleState() ? "FILTERED" : "DRY");
    curveAtt.reset();
    curveAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, Hex1Processor::pid (pad, "analogBend"), curveBtn);
    curveBtn.setButtonText (curveBtn.getToggleState() ? "ANLG" : "FAST");
    outAtt.reset();
    outAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, Hex1Processor::pid (pad, "out"), outBtn);
    outBtn.setButtonText (outBtn.getToggleState() ? "OUT: OWN" : "OUT: MAIN");
    content.repaint (500, 840, 220, 50);
}

void Hex1Editor::resized()
{
    content.setBounds (0, 0, skinW, skinH);
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) skinW));
}

void Hex1Editor::layoutContent()
{
    pads.setBounds (0, 0, 730, 900);   // pad centres are in skin pixels, so the grid sits at the origin

    kitBox.setBounds (888, 54, 334, 46);
    prevKit.setBounds (820, 53, 48, 48);
    nextKit.setBounds (1237, 53, 48, 48);
    master.slider.setBounds (1461, 42, 100, 100);
    outBtn.setBounds (548, 842, 150, 38);
    followBtn.setBounds (528, 178, 170, 34);

    size_t kIdx = 0;
    for (auto& sec : sections())
    {
        const float cellW = (float) (sec.x1 - sec.x0) / (float) sec.cells.size();
        for (size_t i = 0; i < sec.cells.size(); ++i)
        {
            const auto& c = sec.cells[i];
            const int cx = (int) std::round ((float) sec.x0 + cellW * ((float) i + 0.5f));
            const int cw = (int) cellW;
            const juce::Rectangle<int> labelR (cx - cw / 2 - 4, sec.labelY - 12, cw + 8, 24);
            if (c.spec == -1)      { chipLabel.setBounds (labelR);     chipBox.setBounds (cx - 40, sec.knobY - 21, 80, 42); continue; }
            if (c.spec == -2)      { toneFiltLabel.setBounds (labelR); toneFiltBtn.setBounds (cx - 48, sec.knobY - 23, 96, 46); continue; }
            if (c.spec == -3)      { curveLabel.setBounds (labelR);    curveBtn.setBounds (cx - 34, sec.knobY - 21, 68, 42); continue; }
            auto& k = *knobs[kIdx++];
            k.label.setBounds (labelR);
            // slider = square rotary area centred on knobY + a 24 px value box centred on valueY
            const int a = sec.valueY - 12 - sec.knobY;
            k.slider.setBounds (cx - cw / 2, sec.knobY - a, cw, 2 * a + 24);
        }
    }
}

void Hex1Editor::timerCallback()
{
    const int cnt = proc.midiHitCount.load();
    if (cnt != lastMidiCount)
    {
        lastMidiCount = cnt;
        const int p = proc.midiHitPad.load();
        if (proc.followMidi.load() && p >= 0 && p != selectedPad) selectPad (p);
    }
    const float pk = proc.getOutputPeak();
    meterL = pk > meterL ? pk : meterL * 0.86f;
    if (pk >= meterHold) { meterHold = pk; holdFrames = 30; }
    else if (--holdFrames < 0) meterHold *= 0.93f;
    content.repaint (1578, 26, 66, 110);
}

void Hex1Editor::Content::paint (juce::Graphics& g)
{
    g.drawImageAt (ed.plate, 0, 0);

    // brand lockup where the badge sits, MIDI info bottom-right of the pad panel
    if (ed.logo.isValid())
    {
        const float h = 40.0f, w = h * (float) ed.logo.getWidth() / (float) ed.logo.getHeight();
        g.setOpacity (0.92f);
        g.drawImage (ed.logo, juce::Rectangle<float> (60.0f, 842.0f, w, h), juce::RectanglePlacement::stretchToFit);
        g.setOpacity (1.0f);
    }
    const int pad = ed.selectedPad;
    g.setColour (cream.withAlpha (0.75f)); g.setFont (font (17.0f, false));
    g.drawText ("MIDI " + juce::MidiMessage::getMidiNoteName (padInfo[pad].note, true, true, 3) + " (" + juce::String (padInfo[pad].note)
                + ")   alt " + juce::String (padInfo[pad].altNote), 290, 850, 246, 22, juce::Justification::centredRight);

    // output LED meter: 2 columns x 12 segments, green-free retro red/amber
    const int segs = 12;
    const float db = juce::Decibels::gainToDecibels (ed.meterL, -60.0f);
    const float holdDb = juce::Decibels::gainToDecibels (ed.meterHold, -60.0f);
    for (int col = 0; col < 2; ++col)
        for (int s = 0; s < segs; ++s)
        {
            const float segDb = -48.0f + 50.0f * (float) s / (float) (segs - 1);   // -48 .. +2 dB
            const bool lit = db >= segDb;
            const bool hold = std::abs (holdDb - segDb) < 2.3f && ed.meterHold > 0.001f;
            const auto base = s >= segs - 2 ? juce::Colour (0xffff2a1a) : s >= segs - 5 ? juce::Colour (0xffff7a1a) : juce::Colour (0xffff4a22);
            const auto r = juce::Rectangle<float> (1587.0f + 19.0f * (float) col, 120.0f - 7.8f * (float) s, 15.0f, 5.4f);
            if (lit || hold)
            {
                g.setColour (base.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (2.0f), 2.0f);
                g.setColour (base); g.fillRoundedRectangle (r, 1.0f);
            }
            else
            {
                g.setColour (base.withAlpha (0.13f)); g.fillRoundedRectangle (r, 1.0f);
            }
        }
}
