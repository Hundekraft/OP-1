# Fluke 1.0.0

A native melody sequencer for the original OP-1 with firmware #246 and Opus
core 2.8 or newer. Fluke uses an integer adaptation of the Marbles locked
random loop and weighted note quantizer by Emilie Gillet / Mutable Instruments.
It plays the selected synth, sampler or Opus engine. It is not a synth or effect.

## Controls

| Encoder | Main | Hold Shift |
| --- | --- | --- |
| Blue | Length 1–16 | Range range −3…+3 |
| Green | Density 1–16 or Pulses 0–Length | Style: ten named melody profiles |
| White | Rotate 0…Length−1 | Dir: Fwd / Rev / F+R / Jump / RJump |
| Orange | Rate | Swing 0–100 |
| Blue press | New melody | No action |
| Green press | Toggle Density / Pulses | No action |
| White press | Next scale | No action |
| Orange press | Hold on/off | No action |

The keyboard sets the root and base register and starts playback. Release
the last key to stop unless Hold is enabled. With Hold on, the most recently
pressed key changes the root without restarting the sequence. This is root
selection, not chord-input capture. The arcade header shows Fluke at the upper left, with the root and scale
together at the upper right. The logo keeps the approved pixel alphabet; all labels and values use the
readable native OP-1 text renderer and the sizes from version 0.2.0.

Hold off is dim red (#661a19); Hold on is red (#ff423f). Green uses the design
guide token #00ed95. Notes are blue dots with radar-style corner brackets; rests are smaller
white dots. The green playback line has layered vector glow. The background
uses the approved dark neon grid, magenta horizon and amber Fluke logo.
The chart shows relative pitch, so changing root/register preserves its
melodic contour. Dots and brackets stay inside the chart at extreme registers. New uses light blue (#9baee0); Pulses/Density uses light green (#8acbb1);
Scale uses white (#ffffff).

Main and Shift are the only pages. Release Shift to return to Main. All encoder
press actions are on Main only. Shift has no New or Hold labels and does not
require shifted encoder presses. T1–T4
have no assigned Fluke actions. There is no generator-selection setting.

## Lift / Drop / Split

- **Lift ↑:** save the exact current pattern and settings. A monochrome screen
  with a white outlined **SAVED** popup confirms the write has completed.
- **Drop ↓:** recall the saved pattern on the next existing step boundary,
  then play from visible step one. The clock is not restarted. If the saved Rate
  differs, it takes over at that boundary. A **RECALLED** popup confirms recall.
  A stopped sequence remains stopped. Hold and held keys stay current.
- **Split / Cut ✂:** jump to visible step one on the next step boundary.
  It does not restart the clock, and holding/releasing it has no extra action.
  All Directions start at visible step one and continue in their chosen order.

Startup still generates a fresh pattern with default settings; use Drop for
explicit recall. New does not overwrite the saved snapshot. One snapshot is
stored across restart using alternating files in this plugin's own folder.
If **SAVE FAIL** appears, the newest snapshot remains available in this session
but may not survive restart. Lift retries the write; **NO SAVE** means no valid
snapshot is available. Files are accessed only at startup and on the main thread.

The header has a centered **1/2** or **2/2** page indicator. Root and scale have
fixed left aligned positions next to each other on the right.

## Scales and Style

White press on Main cycles:

Chromatic → Major → Minor → Pentatonic → Dorian → Mixolydian → Chromatic.

Scales are equal-tempered and relative to the keyboard root. Pentatonic is
major pentatonic (C, D, E, G, A at root C). The initial scale is Major.
Changing scale keeps the underlying 16 random values and density mask, and
requantizes their pitches. Blue press without Shift creates a new set of
random values and a new density ordering, while retaining your controls.

Style has ten descriptive values:

- Scale: all notes of the selected scale are available.
- Chord: retain higher-priority chord tones. For Major at C: C, E, G, B;
  for Minor at C: C, Eb, G, Bb. In Pentatonic: C, E, G.
- Fifth: root and perfect fifth, across octaves.
- Root: the root note, across octaves.
- Triad: root, scale-appropriate major/minor third, and fifth across octaves.
- Walk: favours repeated and neighbouring allowed notes.
- Leap: favours wider intervals, with occasional recovery in the opposite direction.
- Motif: favours a recurring short motif, with changes for contour and cadence.
- Reply: a related answering phrase with a tonic ending.
- Anchor: alternate root notes with other scale tones.

Motif and Reply are constructed over the complete 16-step pattern; shorter
Length and alternate directions expose different parts of those phrases.

Style reshapes the same generated melody. It does not generate a new seed.
Range controls the range around the keyboard-selected root:
- −3: three octaves below the root through the root's octave.
- −2: two octaves below the root through the root's octave.
- −1: one octave below the root through the root's octave.
- 0: root's octave only (0…11 semitones above root).
- +1: root's octave and one octave above it.
- +2: root's octave and two octaves above it.
- +3: root's octave and three octaves above it.

MIDI output remains bounded to 0…127. Scale/chord pitch classes are preserved.
Changing Range does not change the displayed root. Density selects a
nested mask: raising it adds notes without reshuffling the existing ones.
With a short Length and low Density, the notes can lie outside the active
loop; increase Density or press New if that produces silence.

## Melodic variation

The original Marbles-derived proposals dominate again. There is no fixed rise
and fall, compulsory tonic start/end, or forced midpoint question. New retains
the character of each Style instead of forcing every melody into the same arch.
Light seeded preferences may resolve a leading note, recover after a large leap,
or nudge a strong-beat note toward a tonic-triad tone. These are occasional
nudges, not strict classical rules. Motif, Reply and Anchor keep their original
proposal patterns; Leap can use larger intervals again.

Root randomly chooses root notes from the allowed octave range. With at least
two sounding notes and Range ±1/±2/±3, the first two sounding roots use different
octaves; later choices can repeat naturally. At Range 0 there is only one
available root pitch, so a single pitch is expected. Scale membership, octave
bounds, visible-note rotation and Euclidean pulses remain intact.

Fluke has no second-voice or chord-progression input. It cannot check strict
counterpoint or parallel fifths/octaves against a bass line.

## Density and Euclidean pulses

Press green on Main to toggle Density and Pulses. The green label and value
show the active rhythm method; the caption underneath names the method the
next press will select. Each method retains its own amount during the session.

Density keeps the random nested mask (1–16 notes over the full pattern).
Pulses places exactly 0…Length hits evenly on the active step grid. For 3 hits
in 8 steps, hit positions are 1, 4 and 7. Cyclic gaps differ by at most one
step; zero pulses is silence and Length pulses fills every step.

Green turn in Pulses adjusts the hit count, clamped to Length. Shortening Length
also clamps the remembered pulse count. Rotate moves only the currently visible notes in both rhythm modes.
Shortening Length hides the tail of the displayed sequence; rotating skips
those hidden notes. Increasing Length restores hidden notes in their original
positions around the rotated circular sequence. The rotation readout updates
to match the new length/order. Pulse hits remain evenly distributed on the
active grid.
Directions traverse the active grid in their selected order, so Jump can change
how the evenly spaced grid hits arrive in time. New generates new pitches while
keeping the selected rhythm mode and pulse count; pulse spacing stays deterministic.

## Timing and direction

Rates: 1/32, 1/24, 1/16, 1/12, 1/8, 1/6, 3/16, 1/4, 5/16, 1/3, 3/8,
1/2, 3/4, 1. The sequencer follows the OP-1 clock at 96 ticks per quarter.
Swing delays alternating playback positions by up to half a step.
Notes use fixed velocity 100 and half-step duration, with explicit note-offs.
There are no Gate, Accent, glide or slide controls.

- Fwd: 1, 2, 3, …, Length.
- Rev: Length, …, 3, 2, 1.
- F+R: forward/reverse without repeated endpoints. Length 4: 1, 2, 3, 4, 3, 2.
- Jump: odd steps then even. Length 16: 1, 3, …, 15, 2, 4, …, 16.
- RJump: the reverse Jump ordering. Length 16: 16, 14, …, 2, 15, 13, …, 1.

All directions respect Length, including odd lengths and length 1. Editing
Direction or Length starts the new traversal at the next step boundary.
Stop releases sequencer notes. MIDI Start restarts a running sequence;
physical synchronization behavior needs OP-1 testing.

## Reload behavior

A new pattern and default controls are established at startup and whenever
Fluke is selected again after another sequencer. Hold is off, playback stopped,
keyboard state and partial encoder turns cleared. Defaults: Density 14,
Length 16, Rotate 0, Rate 1/16, Range +1, Swing 0, Style Chord, Direction Fwd,
Scale Major, root C2. No pattern or performance settings are recalled automatically. Lift/Drop provide one explicit saved snapshot.
Default rhythm is Density; the remembered initial pulse amount is 14, clamped
to Length when Pulses is selected. The static screen artwork and the two internal snapshot files stay in Fluke's own folder.

## Installation and first test

Fluke has a new identity, esc.fluke. Remove the previous sequencer plugin
from mods when replacing it, then install fluke.opmod. It appears as Fluke
in the mods list and fluke in the synth sequencer list.

1. For initial hardware testing, enable developer mode in Opus: hold synth +
   drum + tape + mixer, then T1 for system settings.
2. Enter disk mode (Shift + COM, then T3), copy fluke.opmod into mods, eject
   and restart. Check the Opus menu's T2 mods list: Fluke should say loaded.
3. Select Synth and a sound. Shift + sequencer opens its list; choose fluke.
4. Press the sequencer key to view Fluke. Hold a note key and check playback.
5. Press blue for New. Press white on Main: the scale name should advance
   and the existing melody should follow its new note set. Press green to
   switch Density to Pulses and turn green to choose evenly spaced hit counts.
6. Check all ten Style labels, both pages, all directions, New and Hold.
   Select another sequencer and Fluke again to check the default reset.

The core also exposes the row on the drum side; Fluke displays synth only
there and leaves note keys to the OS. Use the Synth side for this build.
If a plugin prevents startup, hold Stop while powering on to start without
plugins; remove fluke.opmod in disk mode if needed.

## Source and validation

There is one melody generator. The MIDI player, controls, density masks,
directions and screen are native sequencer infrastructure. Marbles references
and MIT attribution are bundled in marbles-reference and LICENSE-Marbles.txt.
The algorithm adapts its locked Deja Vu branch and nearest-note quantizer
without hysteresis. Scales/masks and ten named profiles, including discrete phrase variations, are added
for this OP-1 interface. It is not a full Marbles module emulation: no analogue
beta-distribution shaping, live mutation, external CV input, microtonality,
CV slew, T/Y outputs, or chord capture. Integer quantization and random values
are not bit-identical to the original hardware.

Blackfin build and .opmod readback passed with warnings treated as errors.
SVG validation: zero errors/warnings. AddressSanitizer/UndefinedBehaviorSanitizer
host tests passed for 1,800 seed/scale combinations, all six scale masks,
nearest-note/tie behavior, all ten Styles including phrase/interval properties,
root transposition, scale-cycle preservation via green press, New, defaults/reset,
keyboard/Hold, all 14 rates at max swing, note cleanup, and all five directions
at lengths 1–16. Pulse counts and cyclic gap balance passed for every Length
1–16, every pulse count 0…Length and all 16 rotations. Main-only push routing,
Shift caption removal, pulse bounds and length clamps also passed.
Sixty-nine native callback states pass the host checks. Labels use native
constructor size 11, values 18, and captions 10, matching the earlier readable
release. Root and scale stay adjacent in the header. Only the static Fluke
logo uses pixel lettering. Host checks verify the text strings, positions,
sizes, colors and bounds; actual native font appearance still needs the OP-1.

Hardware loading, audible output, display/text legibility, physical encoder
sensitivity, tape recording, synchronization and plugin coexistence remain
pending. CPU is unmeasured; no device-check claim or cycle measurement is invented.

## Rebuild

From this delivery folder with the documented Blackfin toolchain configured:

    python3 "../../Opus Plugin Kit/plugin_tool.py" build Source/esc.fluke --out fluke.opmod
    python3 "../../Opus Plugin Kit/plugin_tool.py" show fluke.opmod
    python3 "../../Opus Plugin Kit/plugin_tool.py" svg Source/esc.fluke/screen.svg

Host checks:

    clang -std=c11 -Wall -Wextra -Werror -Wno-unused-function -fsanitize=address,undefined -g -I Source/sdk Source/tests/test_fluke.c -o /tmp/fluke-test
    FLUKE_TEST_OUTPUT=Source/tests /tmp/fluke-test

Source/sdk
and its provenance identify the exact declarations used for this build.

Version 0.3.2: root and scale use the same native text size (11); muted-note dots are white; the purple baseline is a single 0.5-unit stroke matching the grid weight.

Version 0.3.3 fixes Triad nearest-note selection: the distance bound now uses Q16 units, preventing unintended fallback to root. A regression over 1,800 seed/scale cases failed before the fix and passes afterward.

Version 0.3.4 renames Answer to Reply; Rotate wraps 0 backwards to 15 and 15 forwards to 0. Range now expands root-relative range (0…+4) instead of transposing the keyboard register.

Version 0.3.5 corrects the Range control to −3, −2, −1, 0, +1, +2, +3.

Version 0.3.6: visible-step rotation with ordered restoration. Example: start
A…P, shorten to A…M, rotate right once to M,A…L. Expanding to 16 restores
M,N,O,P,A…L. Rotate wraps at the active Length rather than always at 16.

Version 0.4.1 restores greater proposal variety and removes the fixed arch and
mandatory period. Host variation tests cover 9,000 seed/scale/range settings;
1,221,140 of 1,296,000 non-Root proposals are retained (about 94%). Root pitch
classes and audible octave variety are tested. Main controls are Length/New
on blue, Density/Pulses on green, Rotate/Scale on white, and Rate/Hold on orange.

Rate: turning orange right selects faster divisions; turning left selects slower divisions. Shift + white Direction keeps its normal order.

Version 0.6.2 replaces Cut stutter with a clock-aligned Split jump and aligns
recall to the existing step boundary. It adds a centered page indicator, fixed
root/scale anchors and monochrome save/recall popups. ASan/UBSan host tests cover
all Rates and Directions, swing, recall with Rate changes, snapshot integrity
and display state. Physical OP-1 timing, persistence and rendering remain pending.

Version 0.6.2 keeps one saved phrase: Lift saves, Drop recalls. Shift + Lift/Drop
have no action. The popup reads SAVED or RECALLED and lasts 30 display frames.
Previous phrase A remains the saved phrase; former phrase B files are ignored.
Range now spans −3…+3, with the default still +1. Root and scale move inward
with fixed left anchors. Short header names are CHROM, MAJOR, MINOR, PENT,
DORIAN and MIXO, leaving room before the purple line's right edge.

Version 1.0.0 matches the page indicator to Bubbler's soft dim gray (#6f7173).

Page 2: blue Range, green Style, white Direction, orange Swing.
