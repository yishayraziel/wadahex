"""Builds docs/WADAHEX_Manual.pdf. Run: python3 docs/make_manual.py"""
import os
from reportlab.lib.pagesizes import A4
from reportlab.lib import colors
from reportlab.lib.units import mm
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import (BaseDocTemplate, Frame, PageTemplate, Paragraph, Spacer, Image,
                                Table, TableStyle, PageBreak, KeepTogether)

HERE = os.path.dirname(os.path.abspath(__file__))
RED = colors.HexColor("#e8202a"); INK = colors.HexColor("#1b1b1f"); GREY = colors.HexColor("#6d6b73")

H1 = ParagraphStyle("h1", fontName="Helvetica-Bold", fontSize=22, leading=26, textColor=INK, spaceBefore=6, spaceAfter=8)
H2 = ParagraphStyle("h2", fontName="Helvetica-Bold", fontSize=13.5, leading=17, textColor=RED, spaceBefore=12, spaceAfter=5)
BODY = ParagraphStyle("b", fontName="Helvetica", fontSize=10, leading=14.5, textColor=INK, spaceAfter=6)
SMALL = ParagraphStyle("s", parent=BODY, fontSize=8.5, leading=12, textColor=GREY)
CELL = ParagraphStyle("c", parent=BODY, fontSize=9, leading=12, spaceAfter=0)
CELLB = ParagraphStyle("cb", parent=CELL, fontName="Helvetica-Bold")


def on_page(c, doc):
    w, h = A4
    c.setFillColor(INK); c.rect(0, h - 14 * mm, w, 14 * mm, fill=1, stroke=0)
    c.setFillColor(RED); c.rect(0, h - 15 * mm, w, 1 * mm, fill=1, stroke=0)
    c.setFillColor(colors.HexColor("#f1e9d8")); c.setFont("Helvetica-Bold", 12)
    c.drawString(18 * mm, h - 9.3 * mm, "WADAHEX")
    c.setFont("Helvetica", 8.5); c.setFillColor(colors.HexColor("#8a8890"))
    c.drawString(43 * mm, h - 9.3 * mm, "Hybrid Chip + Analog Drum Synth  |  User Manual v1.0")
    c.drawRightString(w - 18 * mm, 10 * mm, f"{doc.page}")
    c.drawString(18 * mm, 10 * mm, "by Yishay Raziel  -  yraziel.com")


def table(rows, widths, header=True):
    data = [[Paragraph(str(x), CELLB if (header and r == 0) or col == 0 else CELL) for col, x in enumerate(row)] for r, row in enumerate(rows)]
    t = Table(data, colWidths=widths, repeatRows=1 if header else 0)
    st = [("VALIGN", (0, 0), (-1, -1), "TOP"), ("LINEBELOW", (0, 0), (-1, -1), 0.4, colors.HexColor("#dddddd")),
          ("TOPPADDING", (0, 0), (-1, -1), 4), ("BOTTOMPADDING", (0, 0), (-1, -1), 4)]
    if header:
        st += [("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#f3efe6")), ("LINEBELOW", (0, 0), (-1, 0), 1, RED)]
    t.setStyle(TableStyle(st))
    return t


def build(path):
    doc = BaseDocTemplate(path, pagesize=A4, leftMargin=18 * mm, rightMargin=18 * mm, topMargin=24 * mm, bottomMargin=18 * mm,
                          title="WADAHEX User Manual", author="YRAZIEL")
    doc.addPageTemplates([PageTemplate(frames=[Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="f")], onPage=on_page)])
    W = doc.width
    s = []
    s += [Paragraph("WADAHEX User Manual", H1),
          Paragraph("WADAHEX is an eight-pad drum synth modelled on the voice design of the classic mid-80s analog hexagon-pad kits: "
                    "a <b>triangle VCO</b> with a pitch <b>bend</b> sweep, <b>filtered noise</b> with its own sweep, an <b>impact click</b> "
                    "and a resonant low-pass filter working on the noise and click, plus an <b>LFO</b> for wobble and rimshot-style tones. "
                    "On top of that sits an optional <b>8-bit chip</b> layer for hybrid, sample-assisted hits.", BODY),
          Paragraph("Load the <b>SDS-V</b> kit for the authentic analog voicing; the other kits use the hybrid chip + analog sound.", BODY),
          Image(os.path.join(HERE, "wadahex_ui.png"), width=W, height=W * 678 / 1204), Spacer(1, 4)]

    s += [Paragraph("Installation", H2),
          table([["Platform", "Steps"],
                 ["macOS", "Open <b>WADAHEX_mac.dmg</b> and run <b>WADAHEX Installer.pkg</b>. The VST3 goes to /Library/Audio/Plug-Ins/VST3 and the "
                           "standalone app to /Applications. The build is not notarised: if macOS blocks it, right-click the installer and choose "
                           "Open, or allow it in System Settings > Privacy &amp; Security."],
                 ["Windows", "Run <b>WADAHEX_win_setup.exe</b>. The VST3 goes to C:\\Program Files\\Common Files\\VST3, the standalone app and "
                             "this manual to Program Files\\YRAZIEL\\WADAHEX. If SmartScreen warns, choose More info > Run anyway."],
                 ["Your DAW", "Rescan plugins, then load WADAHEX as an instrument on a MIDI track. Output is one stereo bus."]],
                [26 * mm, W - 26 * mm])]

    s += [PageBreak(), Paragraph("Pads &amp; MIDI map", H2),
          Paragraph("Pads follow the General MIDI drum map, so most drum clips and e-kits play it straight away. "
                    "Each pad also answers an alternate note, and C3-G3 (60-67) play pads 1-8 chromatically. "
                    "Click a pad to select and audition it: clicking the <b>centre</b> gives a hard hit, the <b>edge</b> a soft one.", BODY),
          table([["Pad", "Note", "Alt", "Chromatic"],
                 ["KICK", "C1 (36)", "35", "60"], ["SNARE", "D1 (38)", "40", "61"], ["TOM 1", "C2 (48)", "50", "62"],
                 ["TOM 2", "A1 (45)", "47", "63"], ["TOM 3", "G1 (43)", "44", "64"], ["TOM 4", "F1 (41)", "42", "65"],
                 ["CLAP", "D#1 (39)", "46", "66"], ["PERC", "C#1 (37)", "49", "67"]],
                [30 * mm, 35 * mm, 25 * mm, 30 * mm]),
          Paragraph("Note names use the C3 = 60 convention (Cubase, Logic, Ableton). In DAWs that use C4 = 60 the kick shows as C2.", SMALL)]

    s += [Paragraph("Signal flow", H2),
          Paragraph("TONE IN = DRY (SDS-V):  TONE  +  FILTER( NOISE + CLICK + CHIP )  ->  DRIVE  ->  LEVEL / PAN  ->  MASTER", CELLB),
          Paragraph("TONE IN = FILTERED:  FILTER( TONE + NOISE + CLICK + CHIP )  ->  DRIVE  ->  LEVEL / PAN  ->  MASTER", CELLB),
          Spacer(1, 4),
          Paragraph("Every pad is its own voice with its own settings. A new hit on a pad restarts it (no click), just like hitting the real pad again.", BODY)]

    sections = [
        ("TONE", [["PITCH", "Resting pitch of the oscillator, 25 Hz - 1.2 kHz."],
                  ["BEND", "The signature sweep. Positive starts above the pitch and falls to it (classic tom \"pew\"); negative rises up to it. Up to 3 octaves."],
                  ["BEND TIME", "How long the sweep takes (up to 3 s). Short = punch, long = the big 80s fall."],
                  ["CURVE", "ANLG = the SDS-V analog curve: PITCH is the strike pitch and it falls in Hz, almost all the way down, over BEND TIME. "
                            "Measured against real SDS5 recordings. FAST = a quick sweep from above that settles on PITCH."],
                  ["DECAY", "Length of the tone."],
                  ["SHAPE", "Oscillator waveform, sine to triangle. The SDS-V VCO is a triangle: turn up for the buzzy, hollow body, down for a rounder tone."],
                  ["LEVEL", "Tone amount in the mix."]]),
        ("NOISE", [["LEVEL", "Noise amount - the snare wires, the air on toms."], ["DECAY", "Noise length. Long noise on the snare = gated arena."],
                   ["COLOR", "Noise tone: the low-pass cutoff on the white noise, 200 Hz to 18 kHz, like the SDS-V's NOISE TONE knob."],
                   ["SWEEP", "Sweeps the noise filter with the filter envelope, up to 4 octaves either way. Positive = bright crack that darkens."],
                   ["HP", "High-pass on the noise (20 Hz = off). Thins the noise out: use it for crisp snares and airy toms."]]),
        ("CHIP", [["SOUND", "Which 8-bit chip plays: KICK, SNARE, TOM, CLAP, RIM, METAL, ZAP or OFF."],
                  ["LEVEL", "Chip amount."], ["PITCH", "Playback speed of the chip, +/-24 semitones. Lower = longer and grittier."],
                  ["BITS", "Bit depth, 8 down to 2. Fewer bits = more crunch."]]),
        ("FILTER", [["CUTOFF", "Low-pass cutoff for the noise, click and chip (and the tone when TONE IN is FILTERED)."], ["RESO", "Resonance. High values ring and almost whistle."],
                    ["ENV", "How far the filter opens on the hit (up to 6 octaves above cutoff)."], ["DECAY", "How fast it closes again."],
                    ["TONE LP", "A gentle low-pass on the oscillator only: the SDS-V's 'filtered triangle' output. Lower = rounder, more drum-like tone."],
                    ["TONE IN", "DRY = SDS-V routing, the tone skips the filter. FILTERED = the whole voice goes through it."]]),
        ("LFO", [["RATE", "LFO speed, 0.1-40 Hz. Fast rates on a short tone give rimshot / metallic tones; slow rates a wobble."],
                 ["PITCH", "LFO to oscillator pitch, up to +/-1 octave."], ["CUTOFF", "LFO to filter cutoff, up to +/-2 octaves."]]),
        ("OUTPUT", [["CLICK", "The impact transient that gives the analog pads their attack. Gets louder with harder hits."],
                    ["DRIVE", "Soft saturation. Thickens and glues the layers."],
                    ["DYNAMICS", "How much velocity matters. High = soft hits get quieter, darker, with less bend; hard hits get slightly sharper in pitch."],
                    ["VEL DECAY", "Soft hits ring shorter, like a real pad played lightly."],
                    ["VERB", "Amount of the pad's own 80s reverb: a bright, dense room/plate. 0 = off."],
                    ["GATE", "OFF = natural reverb tail. Turn it up for the classic gated reverb: a big sustained tail chopped short "
                             "after each hit (750 ms down to 200 ms). Each pad's gate opens on its own hits."],
                    ["LEVEL", "Pad volume."], ["PAN", "Stereo position."]]),
    ]
    s += [PageBreak(), Paragraph("Controls", H2)]
    for name, rows in sections:
        s += [KeepTogether([Paragraph(name, CELLB), Spacer(1, 2), table(rows, [28 * mm, W - 28 * mm], header=False), Spacer(1, 8)])]
    s += [Paragraph("<b>MASTER</b> (top right) sets the overall output with a gentle safety limiter. Click a value to type it in; drag knobs up/down to change them.", BODY)]

    s += [Paragraph("Separate outputs", H2),
          Paragraph("Every pad can play on the main stereo output or on its own stereo output. Select a pad and click "
                    "<b>OUT: MAIN</b> (bottom of the pad panel) to switch it to <b>OUT: OWN</b>. Then enable WADAHEX's extra outputs in your DAW: "
                    "they are named <b>2 KICK, 3 SNARE, 4 TOM 1, 5 TOM 2, 6 TOM 3, 7 TOM 4, 8 CLAP, 9 PERC</b>. The pad's reverb travels with it. "
                    "If an output isn't enabled in the DAW, the pad stays on the main output so nothing goes silent. Changing kits keeps your routing.", BODY),
          Paragraph("Logic: load WADAHEX as Multi Output and add the extra channels with + on the mixer. Ableton: create MIDI tracks with "
                    "Audio From = WADAHEX and pick the output. Cubase: activate the outputs in the instrument's output menu. "
                    "FL Studio: route the plugin's outputs to mixer tracks in its Processing tab.", SMALL)]
    s += [KeepTogether([Paragraph("Factory kits", H2),
          table([["Kit", "Character"],
                 ["SDS-V", "Every drum matched by computer to a real SDS5 kit recording (kick, snare, four toms, rimshot). Pure synthesis, fully editable."],
                 ["SDS-V Wobble", "SDS-V with LFO on the toms and snare filter for that vibrating, slightly unstable analog character."],
                 ["SDS5 Library 1-6", "Six more kits matched to individual SDS5 one-shots: different kicks, snares and tom sets (high to low)."],
                 ["SDS-V Hand-Tuned", "The earlier by-ear SDS-V voicing, kept for comparison."],
                 ["Classic Hex", "Hybrid chip + analog reference sound: bendy toms, cracking snare, chip-assisted kick."],
                 ["Pew Pew Toms", "Toms pushed to the extreme: full bend, long sweeps, more resonance."],
                 ["Electro '84", "Long boomy kick, tight toms with a ZAP chip, metallic perc."],
                 ["Chip Crush", "Chips up front at 4 bits, analog layers pulled back, more drive."],
                 ["Gated Arena", "Long noisy snare and toms with the gated reverb on: the big-room 80s drum sound."]],
                [34 * mm, W - 34 * mm]),
          Paragraph("Changing kit overwrites the current pad settings. Your tweaks are saved with the DAW project.", SMALL)])]

    s += [Paragraph("Quick recipes", H2),
          table([["Sound", "Try"],
                 ["SDS-V tom", "Chip OFF, CURVE ANLG, SHAPE 90%, TONE IN DRY, PITCH 200-330 Hz, BEND 70%, BEND TIME 1.6 s, DECAY 800 ms, NOISE 60%, CLICK 100%."],
                 ["Hybrid 80s tom", "Chip TOM low, BEND 60-80%, BEND TIME 300-500 ms, DECAY 700 ms+, RESO 30-50%."],
                 ["Rimshot", "PITCH 480 Hz, DECAY 70 ms, LFO RATE 35-40 Hz, LFO PITCH 45%, CLICK 85%."],
                 ["Laser / syndrum", "TONE PITCH 300 Hz, BEND 100%, BEND TIME 150 ms, noise and chip off."],
                 ["Rising 'boing'", "BEND negative, BEND TIME 200 ms, DECAY long."],
                 ["Lo-fi electro snare", "Chip SNARE, BITS 3-4, NOISE COLOR 80%, DRIVE 50%."],
                 ["Sub kick", "PITCH 40 Hz, BEND 30%, BEND TIME 60 ms, DECAY 1 s, CUTOFF 800 Hz."]],
                [36 * mm, W - 36 * mm])]

    s += [Paragraph("How the SDS-V kits were made", H2),
          Paragraph("Each sound was matched by computer: WADAHEX rendered a hit, compared it to a real SDS5 one-shot "
                    "(spectrum at three time resolutions, pitch sweep and envelope), and adjusted its own knobs thousands of times "
                    "until the difference was as small as possible. No audio from the recordings is inside the plugin, only the settings it found.", BODY)]
    s += [Spacer(1, 10), Paragraph("WADAHEX is an original instrument inspired by 1980s analog and hybrid electronic drum kits. "
                                   "All sounds are synthesised; it contains no samples from any vintage hardware. "
                                   "VST is a registered trademark of Steinberg Media Technologies GmbH.", SMALL)]
    doc.build(s)


if __name__ == "__main__":
    build(os.path.join(HERE, "WADAHEX_Manual.pdf"))
