# LB-303 (VST3) — version 0.0.1 TEST1

Accessible monophonic bassline synthesizer for REAPER, ported from **LB-303.jsfx**
by Lo-lo. This TEST1 source is prepared for Windows x64 VST3 builds using **GitHub Actions**.

Repository: https://github.com/Lo-lo78/LB-303

## TEST1 scope

- All **35 LB-303 JSFX parameters** retain their original order, values, defaults
  and meaningful ranges. Persistent VST parameter IDs are `slider001` … `slider035`.
- PolyBLEP saw/pulse, PWM, portamento, velocity/accent, filter ADSR,
  Lowpass Gate, resonant two-stage filter, fixed high-air peak, post-filter,
  saturation and input passthrough have been translated to C++.
- Delay includes tempo divisions, ping-pong, spread/Haas, RBJ LP/HP,
  modulation, dry/wet, feedback and transpose.
- REAPER project state uses JUCE APVTS XML. Sample-accurate MIDI note scheduling.
- Accessible **single Bassline grid**, with **five columns, eight rows per column**.
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
| Ctrl+Tab | TEST1 announces that sequencer grid is scheduled for TEST2 |

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
4. Download the `LB-303-0.0.1-TEST1.2-Windows-x64` artifact after it succeeds.
5. Extract the VST3 directory under the usual VST3 folder, e.g.
   `C:\Program Files\Common Files\VST3\`, and scan in REAPER.

No local compilation was performed during source preparation. The first
GitHub Actions run is the actual compiler check.

## Upcoming roadmap (not implemented in TEST1)

- **TEST2:** one-track LJuno-style sequencer, 16 patterns/128 steps, second grid
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
