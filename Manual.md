# ESC Delay Trio — Manual

## Installation and common behavior

For the original OP-1, firmware 246, with Opus 2.6. Copy `bubbler.opmod`, `delayed.opmod` and `pitchdelay.opmod` to `mods`, safely eject and restart. Choose the effect on a synth or drum instrument. All three are instrument-only; selecting them as master effects shows **instrument only**.

All three have a maximum delay time of **one second**, with mono wet processing. Mix blends the dry signal with the effect. The red characters become brighter as Mix rises, but remain visible at minimum. Turning an effect off uses the native Opus/OP-1 FX OFF indication over the dimmed screen.

### Updating

Replace the existing files, eject and restart. Current native builds can be replaced directly. Very old standard-format builds can leave incompatible saved defaults in the FX list and may crash when opening the page. Back up your OP-1 before upgrading those builds. The previously tested temporary-name migration is required for that case; do not repeatedly open a crashing old entry. This release does not include the experimental migration or diagnostic builds.

## Bubbler

A granular delay: overlapping, smoothly enveloped pieces of recorded sound are recombined into the output and feedback. Delay time, grain starts, size, direction and octave can vary independently.

| Page 1 | Control |
| --- | --- |
| Blue | Time: 30 ms–1 second |
| Green | Feedback: 0–95% |
| White | Density: 0–100%, controlling grain activity |
| Orange | Mix: dry/wet balance |

Hold **Shift** to access the additional pages. While holding Shift, Right goes forward and Left goes backward between pages 2 and 3. Release Shift to return to page 1. The next Shift press recalls the last additional page. The page counter shows your position.

| Page 2 | Control |
| --- | --- |
| Blue | Time Var: random variation of each grain’s delay time |
| Green | Start Var: variation of grain start timing |
| White | Size: grain duration, 2–50% of Time |
| Orange | Size Var: random variation of grain duration |

| Page 3 | Control |
| --- | --- |
| Blue | Filter Freq: feedback filter cutoff |
| Green | Reso: filter resonance |
| White | Reverse: probability that a grain plays backward |
| Orange | Pitch Var: probability of adding a random octave shift to each grain |

At Pitch Var 0%, no random octave is added; at 100%, every grain receives a random -12 or +12 semitone shift. This is added to the selected base pitch.

On page 3, **Shift + orange push** cycles base pitch through Off, keyboard mode, -12, -7, +7 and +12 semitones. The selected mode/value appears below Pitch Var. Keyboard mode uses the latest OP-1 or incoming MIDI note; MIDI note 60 is unison, and note release retains the last pitch. Push mode starts Off after restart.

Green bubbles follow Feedback, blue elements follow Time and white accents follow Density. Additional-page edits do not animate the main artwork. This port does not expose the original plug-in’s pitch-button chords, just-intonation switch or time-lock function.

## Delayed

A delay with a resonant feedback filter and LFO modulation.

| Main page | Control |
| --- | --- |
| Blue | Time: 1 ms–1 second |
| Green | Feedback: 0–200% |
| White | Filter Freq: 20 Hz–20 kHz |
| Orange | Mix |

**White push** switches the white encoder between Filter Freq and Reso. Resonance is displayed directly below the filter value; the edit indicator identifies the active secondary control.

**Orange push** toggles Freeze, holding the current delay content. Press again to resume normal input. Ice and white artwork indicate Freeze.

| Hold Shift | Control |
| --- | --- |
| Blue | LFO Freq: 0.01–10 Hz |
| Green | LFO Depth: 0–100% |
| White | LFO Shape: Sine, Tri, Square, Up, Down, Rand |
| Orange | LFO Phase: -180° to +180° |

Phase offsets the modulation cycle; this mono wet engine does not provide stereo phase widening. Feedback above 100% can build up strongly; the original feedback range is retained with limiting in the feedback path. Raise it gradually.

The blue clock grows with Time, green stripes follow Feedback, white sparkles follow Filter Freq, and the red running girl follows Mix.

## Pitch Delay

A delay with a moving-read-head pitch shifter. Pitchshift and Octave combine to set playback pitch; Loop Depth sets the pitch-shifting window as a portion of the delay time.

| Encoder | Control |
| --- | --- |
| Blue | Time: 1 ms–1 second |
| Green | Feedback: 0–200% |
| White | Pitchshift: 0 to +12 semitones |
| Orange | Mix |

**White push** switches the white encoder to Octave (-3 to +3 octaves), shown below Pitchshift. Push again to return to Pitchshift.

**Orange push** switches the orange encoder to Loop Depth (10–100%), shown below Mix. Push again to return to Mix. Shorter windows give a different, more granular pitch-shifting texture.

Feedback above 100% can build up strongly. The red girl and boy remain in their approved static pose and brighten with Mix; blue and white sparkles and green side lines show the other main controls.

## Credits

Based on **SoundHack Delay Trio by Tom Erbe**, using the **Opus framework**. OP-1 ports by **ESC_CODE**. See [Credits](Credits.md) for links and licence notices.
