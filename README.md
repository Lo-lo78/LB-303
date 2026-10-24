# LB-303 (VST3) — version 0.0.1 TEST1.7

Accessible dual-engine bassline synthesizer for REAPER, ported from **LB-303.jsfx**
by Lo-lo. This TEST1 source is prepared for Windows x64 VST3 builds using **GitHub Actions**.

Repository: https://github.com/Lo-lo78/LB-303

## TEST1 scope

- All **35 LB-303 JSFX parameters** retain their original order, values, defaults
  and meaningful ranges. Persistent VST parameter IDs are `slider001` … `slider035` for Bassline 1, with independent `b2_slider001` … `b2_slider035` for Bassline 2. Each has an additional output Level and Pan.
- PolyBLEP saw/pulse, PWM, portamento, velocity/accent, filter ADSR,
  Lowpass Gate, resonant two-stage filter, fixed high-air peak, post-filter,
  saturation and input passthrough have been translated to C++.
- Delay includes tempo divisions, ping-pong, spread/Haas, RBJ LP/HP,
  modulation, dry/wet, feedback and transpose.
- REAPER project state uses JUCE APVTS XML. Sample-accurate MIDI note scheduling.
- Two independent accessible Bassline grids, each **five columns with eight rows per column**.
  No page combo box. Starting with TEST1.3, a single LJuno-style accessible
  ComboBox selects parameters; a dedicated Value slider and numeric Edit field
  replace the 35 individual buttons.
- GitHub Actions will build and upload an installable VST3 bundle.

### Important TEST1 qualification

**Audio equivalence has not been validated yet.** Do an A/B listening test in REAPER
against the included original JSFX, with matched parameter values and velocity.
The delay transposition is initially implemented with a real-time dual-grain
crossfading algorithm. The original JSFX uses **Saike's WSOLA+autocorrelation
pitch shifter**; this effect is therefore not guaranteed to sound identical,
especially on transients. The original JSFX and included libraries are kept in
`Reference/` so the exact algorithm can be transplanted in a later fidelity pass.
The default `Delay Transpose = 0` bypasses the difference.

Other behaviors requiring close listening: the original JSFX has some surprising
implementation details, including half-frequency oscillator phase increments,
accent envelope renewal and a 1.25x free delay time scale. TEST1 intentionally
keeps them, rather than silently modifying the character.

### Keyboard navigation

| Keys | Action |
| --- | --- |
| Up / Down | Previous / next parameter in the same column |
| Left / Right | Previous / next column (8 parameter positions) |
| Alt+Up / Alt+Down | Increase / decrease parameter value |
| Alt+Shift+Up / Alt+Shift+Down | Change by 10 selected steps |
| Alt+Left / Alt+Right | Change the step width: 1, 5, 10, 15, 20 (announces `Step 5`, etc.) |
| Alt+PageUp / Alt+PageDown | Increase / decrease by 40 selected steps (choices change by 1) |
| Alt+Home / Alt+End | Set selected parameter to maximum / minimum, as in LJuno |
| Enter (Grid) | Focus Value for the selected parameter |
| Enter (Value) | Return to the same parameter in Grid |
| Tab / Shift+Tab | Cycle Grid, Value and Edit (both directions) |
| Enter (Edit) | Commit typed value and return to Grid |
| Home / End (Grid) | First / last parameter in the current column |
| Home / End (Value) | Minimum / maximum value |
| PageUp / PageDown (Grid) | Up / down 5 rows in the same column |
| PageUp / PageDown (Value) | Change by 40 selected steps |
| Left / Right (Value) | Change the step width, as in LJuno |
| Ctrl+Home / Ctrl+End | First / last parameter |
| Alt+L / Alt+V / Alt+E | Focus Grid / Value / Edit |
| Alt+Q | TEST1 announces that sequencer editor is scheduled for TEST2 |
| Ctrl+Tab | Cycle Bassline 1 → Bassline 2 → Sequencer placeholder → Bassline 1 |

The focused parameter name and value are also set as native accessibility text,
so screen-reader line reading has a concise value. Host interception of global
shortcuts may vary; please test Alt+L and arrow focus with NVDA/OSARA in REAPER.

## TEST1.1 — GitHub Actions build compatibility fix

- Fixes JUCE 8.0.15 / MSVC error C2039 in `Source/PluginProcessor.cpp`:
  `juce::Optional<double>` has no `value_or()` method.
- Host BPM now uses the same `if (const auto bpm = position->getBpm())`
  and `*bpm` idiom already present in LJuno-116.
- No DSP, parameter IDs, presets, UI or keyboard controls have been modified.
- Static source checks only; compilation is reserved for GitHub Actions.

## TEST1.2 — REAPER editor-opening crash fix

- **Fixed a reproducible invalid pointer dereference in the UI constructor.**
  The previous editor called `setResizeLimits(720, 390, ...)` before constructing
  the 35 `ParameterCell` components. JUCE may resize the window synchronously,
  invoke `resized()`, and trigger a null-pointer access while opening the editor.
- All 35 cells are now created before `setResizable`, `setResizeLimits`, and `setSize`.
  `resized()` also checks for null children as a defensive measure.
- Added static regression checks for the constructor ordering and null guard.
- No changes to DSP, parameter IDs, stored state, navigation, or screen-reader
  announcements; GitHub Actions must recompile, and REAPER needs a fresh runtime test.
- To verify: test the fixed VST3 on a blank REAPER project first. Remove the
  old `LB-303.vst3` directory before installing the updated VST3 directory.

## TEST1.4 — LJuno step-width keyboard shortcuts

- Alt+Left/Right cycle **1, 5, 10, 15, 20** and announce only when the setting changes.
- Alt+PageUp/PageDown adjust by **40 selected steps**; enumerated choice parameters move by one choice.
- Alt+Home takes the selected value to its **maximum**; Alt+End takes it to its **minimum** (LJuno convention).
- Value Left/Right and Value PageUp/PageDown mirror LJuno, also without Alt.
- The step width is stored in the project state and restored when reopening.
- Silent borders remain unchanged; all shortcuts work in Grid, Value and Edit without stealing focus on plug-in insertion.
- Only the editor, tests and documentation are changed; DSP and the 35 persistent parameter IDs are untouched.
- **Static checks only; no local build.** Compile on GitHub Actions and test in REAPER.

## Build on GitHub Actions

1. Create a public (or private) repository named `LB-303`.
2. Upload the **contents of this archive** to the repository root. Keep the
   `.github/workflows/build-windows.yml` hierarchy.
3. Open **Actions → Build LB-303 Windows x64**. The `push` starts the build,
   or use `Run workflow` manually.
4. Download the `LB-303-0.0.1-TEST1.7-Windows-x64` artifact after it succeeds.
5. Extract the VST3 directory under the usual VST3 folder, e.g.
   `C:\Program Files\Common Files\VST3\`, and scan in REAPER.

No local compilation was performed during source preparation. The first
GitHub Actions run is the actual compiler check.

## Upcoming roadmap (not implemented in TEST1)

- **TEST2:** two-track LJuno-style sequencer, 16 patterns/128 steps, second grid
  for sequencer-wide settings, Alt+Q step editor (Note, Length, Velocity,
  Repeat, Shift, Launch Step, Parameter Locks), copy/paste/undo.
- **TEST3:** precise MIDI note overlap, portamento/legato, accent,
  envelope retriggering and lock interpolation in sync with REAPER.
- **TEST4:** preset/pattern persistence, focus regression tests, MIDI and host
  automation compatibility.

## Attribution and licensing

This repository's new VST3 sources use AGPL-3.0-or-later, matching the LJuno-116
source license. The reference `qddelay` JSFX helper files contain components by
Olivier Belanger (CookDSP permission notice), Theo Niessink (WTFPL v2), and
Joep Vanlier (Saike Pitchlib, MIT); their original notices are preserved.
Reference files are not compiled directly into the VST3.

## TEST1.3 — LJuno-style parameter controls and REAPER focus

- Replaces the initial grid of 35 JUCE TextButtons with **one accessible
  ComboBox** (LJuno-style parameter grid), one APVTS-attached **Value** slider,
  and one separate native **Edit** control for typing numerical/choice values.
  The on-screen grid is still drawn in 5 columns of 8 rows for mouse users,
  but is not made of 35 separate focusable buttons.
- **Tab** cycles Grid → Value → Edit → Grid; Shift+Tab goes backwards.
  **Enter** on Grid focuses Value; Enter on Value returns to the currently
  selected Grid parameter. Enter on Edit commits and returns to Grid.
- **Alt+L / Alt+V / Alt+E** provide direct focus. Grid arrow keys navigate
  by row or 8-item column; Alt+Up/Down edits values. Home/End, page navigation,
  Ctrl+Home/End, type-to-jump, Backspace reset and silent navigation limits.
- Accessibility names follow LJuno's **entry context only** design: Grid and
  Value announce their role on entry but omit repetitive descriptions for
  reading and navigating. The root editor is ignored by the accessibility tree.
- **No automatic focus transfer** when REAPER creates/opens the FX editor.
  The plug-in cannot force REAPER to keep its window closed; that is a host
  preference. This change prevents LB-303 itself from moving the keyboard
  focus away from the host FX Chain during initialization.
- The 35 original parameter IDs, DSP, JUCE version and Actions workflow
  remain unchanged. Compile and runtime testing remain GitHub/REAPER tasks.

## TEST1.5 — Spoken value shortcuts from Grid, Value and Edit

- Fixes a missing accessibility announcement when Alt+Up/Down, Alt+PageUp/PageDown,
  Alt+Home/End or other parameter-changing shortcuts are used with **Grid** or
  **Edit** focused. The previous implementation only sent a valueChanged event
  when the **Value** slider itself had keyboard focus.
- Announces the **resulting parameter value only**, using the same Windows UI
  Automation notification mechanism as LJuno. Applies also in Value.
- The operation is silent when the resulting value is unchanged at a minimum,
  maximum or due to range/step snapping (silent boundaries remain intact).
- An Alt value shortcut while Edit is focused also updates the edit field to
  the actual new value and selects the text for further editing.
- Restores unobtrusive accessibility descriptions after speaking; does not
  move focus, change the audio engine, change keyboard mappings, or change any
  of the 35 parameter IDs.
- Static regression tests pass; compilation still takes place exclusively
  through GitHub Actions.

## TEST1.6 — Two independent basslines, Level/Pan, three contexts

- Two real `BassEngine` instances, each with its own oscillator, phase, envelopes,
  filter, accent, delay and internal state. No shared synthesis state.
- Each bassline retains the original 35 sound parameters, plus **Level** and
  **Pan** (37 controls per grid, five columns × eight rows). There are 74 VST3
  parameter IDs total, grouped under Bassline 1 / Bassline 2 in the host list.
- Bassline 1 preserves the existing 35 automation IDs exactly. Bassline 2 uses
  stable `b2_slider001` through `b2_slider035` IDs; output controls use
  `b1_level`, `b1_pan`, `b2_level`, `b2_pan`.
- `Volume` is the original pre-filter amplitude control, untouched; `Level` is
  an additional **final** post-delay level. Pan: -100% left, 0 centre, +100% right.
  Linear balance panning keeps the original Bassline 1 signal unchanged at centre.
- Default final Level: Bassline 1 = 1.00, Bassline 2 = 0.00. Raise Bassline 2's
  Level to hear it. This avoids suddenly doubling the output on old projects.
- In TEST1.6, incoming MIDI was layered to both basslines. Starting in TEST1.7,
  Bassline 1 receives MIDI channel 1 exclusively and Bassline 2 channel 2.
  The future two-track sequencer will address them independently. TEST1.7 still
  does not contain a functional sequencer engine, clocks or patterns.
- `Ctrl+Tab` rotates **Bassline 1 → Bassline 2 → Sequencer → Bassline 1**.
  The Sequencer context displays a descriptive placeholder without bogus
  editable controls. Alt+Q continues to report that step editing is not yet
  implemented. The two bassline grids remember their selections separately.
- Grid → Value → Edit, Enter, Alt+Left/Right, Alt+PageUp/PageDown, Alt+Home/End,
  silent borders, NVDA/OSARA speech and no-focus-steal-on-load are preserved.
- **No local compilation.** GitHub Actions is responsible for the VST3 build;
  please test both engine outputs and UI focus using NVDA in REAPER.

## TEST1.7 — Independent MIDI channels

- **Bassline 1, channel 1:** receives only MIDI on channel 1.
- **Bassline 2, channel 2:** receives only MIDI on channel 2.
- Note On, Note Off and any channel MIDI messages are routed to the matching
  bassline only; messages on channels 3–16 or channel-less system events
  are not routed into either bassline engine.
- REAPER can host both basslines as one VST3 instrument: send MIDI 1 for the
  first instrument and MIDI 2 for the second. MIDI output is passed through.
- The Bassline 2 Level parameter still defaults to **0.00**. Increase it to
  hear the second bassline, even after sending it notes on channel 2.
- No synthesis algorithms, parameter IDs, keyboard controls or UI navigation
  have been changed. Only MIDI event routing, tests and build artifact naming.
- Static checks only. GitHub Actions must compile; REAPER must verify
  channel isolation, including overlapping notes and Note Off messages.
