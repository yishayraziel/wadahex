// Headless tests of the real processor: choke, KEYS, mute/solo, separate outs, user kits, export.
#include "../Source/PluginProcessor.h"
#include <cstdio>

static int fails = 0;
#define CHECK(c, msg) do { if (c) std::printf ("  ok   %s\n", msg); else { std::printf ("  FAIL %s\n", msg); ++fails; } } while (0)

static float rms (const juce::AudioBuffer<float>& b, int ch, int s0, int n)
{ double a = 0; for (int i = s0; i < s0 + n; ++i) a += (double) b.getSample (ch, i) * b.getSample (ch, i); return (float) std::sqrt (a / n); }

struct Rig
{
    Hex1Processor p; double sr = 48000; int bs = 256;
    Rig() { p.setRateAndBufferSizeDetails (sr, bs); p.prepareToPlay (sr, bs); }
    void set (int pad, const char* id, float v) { auto* q = p.apvts.getParameter (Hex1Processor::pid (pad, id)); q->setValueNotifyingHost (q->convertTo0to1 (v)); }
    // render `seconds`, sending note events (time sec, note, vel)
    juce::AudioBuffer<float> run (double seconds, std::vector<std::tuple<double,int,int>> ev)
    {
        const int total = (int) (seconds * sr);
        juce::AudioBuffer<float> out (p.getTotalNumOutputChannels(), total); out.clear();
        juce::AudioBuffer<float> blk (p.getTotalNumOutputChannels(), bs);
        for (int pos = 0; pos < total; pos += bs)
        {
            juce::MidiBuffer m;
            for (auto& [t, n, v] : ev) { const int s = (int) (t * sr); if (s >= pos && s < pos + bs) m.addEvent (juce::MidiMessage::noteOn (10, n, (juce::uint8) v), s - pos); }
            blk.clear(); p.processBlock (blk, m);
            const int n = std::min (bs, total - pos);
            for (int ch = 0; ch < out.getNumChannels(); ++ch) out.copyFrom (ch, pos, blk, ch, 0, n);
        }
        return out;
    }
};

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    {
        std::printf ("hi-hats + choke\n");
        Rig r;
        auto open = r.run (1.0, { { 0.0, 46, 120 } });
        auto choked = r.run (1.0, { { 0.0, 46, 120 }, { 0.2, 42, 1 } });
        const float a = rms (open, 0, 12000, 9600), b = rms (choked, 0, 12000, 9600);
        std::printf ("    open hat 250-450 ms rms %.4f, after closed-hat choke %.5f\n", a, b);
        CHECK (a > 0.005f, "open hat rings");
        CHECK (b < a * 0.05f, "closed hat chokes the open hat");
    }
    {
        std::printf ("KEYS\n");
        auto pure = [] (Rig& x) { x.set (2, "keys", 1.0f); x.set (2, "noiseLvl", 0.0f); x.set (2, "click", 0.0f); x.set (2, "bend", 0.0f); x.set (2, "drift", 0.0f); };
        Rig r; pure (r);  // TOM 1 on keys, tone only
        auto lo = r.run (0.5, { { 0.0, 60, 110 } });
        Rig r2; pure (r2);
        auto hi = r2.run (0.5, { { 0.0, 84, 110 } });
        auto zc = [] (const juce::AudioBuffer<float>& b) { int z = 0; for (int i = 4800; i < 9600; ++i) z += (b.getSample (0, i - 1) < 0) != (b.getSample (0, i) < 0); return z; };
        std::printf ("    zero crossings 100-200 ms: C3 %d, C5 %d\n", zc (lo), zc (hi));
        CHECK (std::abs ((float) zc (hi) / (float) zc (lo) - 4.0f) < 0.3f, "C5 plays two octaves above C3");
        CHECK (rms (lo, 0, 0, 4800) > 0.001f, "keys pad sounds on note 60");
    }
    {
        std::printf ("mute / solo\n");
        Rig r; r.set (0, "mute", 1.0f);
        auto b = r.run (0.4, { { 0.0, 36, 120 } });
        CHECK (rms (b, 0, 2400, 9600) < 1e-5f, "muted kick is silent");
        Rig r2; r2.set (1, "solo", 1.0f);
        auto c = r2.run (0.4, { { 0.0, 36, 120 } });
        auto d = r2.run (0.4, { { 0.0, 38, 120 } });
        CHECK (rms (c, 0, 2400, 9600) < 1e-5f && rms (d, 0, 2400, 9600) > 1e-3f, "solo snare silences the kick, snare plays");
    }
    {
        std::printf ("separate outputs\n");
        Rig r;
        auto layout = r.p.getBusesLayout();
        for (int i = 1; i < layout.outputBuses.size(); ++i) layout.outputBuses.getReference (i) = juce::AudioChannelSet::stereo();
        CHECK (r.p.setBusesLayout (layout), "all 10 extra stereo outputs can be enabled");
        r.p.prepareToPlay (r.sr, r.bs);
        r.set (1, "out", 1.0f);  // snare -> own
        auto b = r.run (0.4, { { 0.0, 38, 120 } });
        std::printf ("    channels %d\n", b.getNumChannels());
        const float mainR = rms (b, 0, 0, 9600), snareOut = rms (b, 4, 0, 9600);   // bus 2 = SNARE -> channels 4,5
        CHECK (mainR < 1e-5f && snareOut > 1e-3f, "snare plays only on its own output (3 SNARE)");
    }
    {
        std::printf ("user kits\n");
        Rig r; r.set (0, "tonePitch", 77.0f);
        CHECK (r.p.saveUserKit ("zz test kit"), "save kit");
        CHECK (Hex1Processor::userKitNames().contains ("zz test kit"), "kit listed");
        r.set (0, "tonePitch", 200.0f);
        CHECK (r.p.loadUserKit ("zz test kit"), "load kit");
        const float v = r.p.apvts.getParameter (Hex1Processor::pid (0, "tonePitch"))->convertFrom0to1 (r.p.apvts.getParameter (Hex1Processor::pid (0, "tonePitch"))->getValue());
        std::printf ("    kick pitch after load %.2f\n", v);
        CHECK (std::abs (v - 77.0f) < 0.5f, "kit restores the saved value");
        Hex1Processor::userKitDir().getChildFile ("zz test kit.wadakit").deleteFile();
    }
    {
        std::printf ("export one-shots\n");
        Rig r;
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("wadahex_export_test");
        dir.deleteRecursively();
        const int n = r.p.exportOneShots (dir);
        auto files = dir.findChildFiles (juce::File::findFiles, false, "*.wav");
        std::printf ("    %d files, e.g. %s (%lld bytes)\n", n, files.isEmpty() ? "-" : files[0].getFileName().toRawUTF8(), files.isEmpty() ? 0LL : (long long) files[0].getSize());
        CHECK (n == kNumPads && files.size() == kNumPads, "one WAV per pad");
    }
    {
        std::printf ("sound browser\n");
        Rig r;
        r.p.applyPad (3, vintageSounds()[0].params);
        const float v = r.p.apvts.getParameter (Hex1Processor::pid (3, "tonePitch"))->convertFrom0to1 (r.p.apvts.getParameter (Hex1Processor::pid (3, "tonePitch"))->getValue());
        CHECK (std::abs (v - vintageSounds()[0].params.tonePitch) < 1.0f, "vintage sound loads onto a pad");
    }
    {
        std::printf ("pad names\n");
        Rig r;
        CHECK (Hex1Processor::nameForSound (0, "CLAP") == "CLAP", "CLAP onto KICK renames the pad");
        CHECK (Hex1Processor::nameForSound (3, "TOM") == "TOM 2", "a tom onto TOM 2 keeps its number");
        CHECK (Hex1Processor::nameForSound (0, "TOM 3") == "TOM", "a factory tom onto KICK becomes TOM");
        r.p.setPadName (0, "clap");
        CHECK (r.p.getPadName (0) == "CLAP", "name is stored upper-case");
        juce::MemoryBlock mb; r.p.getStateInformation (mb);
        Rig r2; r2.p.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (r2.p.getPadName (0) == "CLAP", "name survives a session save / load");
        CHECK (r2.p.saveUserKit ("zz names kit"), "save kit with names");
        r2.p.setCurrentProgram (0);
        CHECK (r2.p.getPadName (0) == "KICK", "factory kit resets names");
        r2.p.loadUserKit ("zz names kit");
        CHECK (r2.p.getPadName (0) == "CLAP", "user kit restores names");
        Hex1Processor::userKitDir().getChildFile ("zz names kit.wadakit").deleteFile();
        r2.p.setPadName (0, "toolongname1");
        CHECK (r2.p.getPadName (0) == "TOOLONGN", "names are capped at 8 characters");
    }
    std::printf (fails ? "\n%d FAILED\n" : "\nALL PASSED\n", fails);
    return fails ? 1 : 0;
}
