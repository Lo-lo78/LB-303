# LB-303 TEST2.2.9 — Keyboard transpose while Sequencer is On

- **C4 (MIDI note 60) = zero transpose**, as in LJuno: C5 raises the pattern by
  12 semitones; B3 lowers it by one. Pattern steps retain their saved pitches.
- MIDI channel **1 transposes only Bassline 1**; channel **2 only Bassline 2**.
  The two 16-pattern banks and independent Sequencer Off/On switches remain.
- When On, physical keyboard notes trigger and **transpose** the sequence,
  without producing doubled direct notes. With multiple held keys, the last
  pressed key determines pitch. On releasing it, pitch returns to the most
  recently pressed key still held. Changing keys does **not restart** the
  pattern; releasing the last key stops it.
- Note Off uses the actual pitch previously sent at Note On even if a keyboard
  note changes between them, avoiding hanging notes when changing transpose.
- Step-editor Space preview follows the **last physical keyboard note** on the
  corresponding channel (defaults to C4).
- No new parameters or altered IDs. Pattern save format, Grid/Value/Edit,
  screen-reader announcements and Parameter Lock data are unchanged. Lock
  **audio playback** is still deferred to TEST2.3.
- Static tests only; compile on GitHub Actions and verify behavior in REAPER.

---

# LB-303 TEST2.2.8 — MIDI-triggered sequencer mode (per bassline)

The Sequencer parameter grid contains two independent automatable Off/On
switches named **Sequencer Bassline 1** and **Sequencer Bassline 2**. They
reuse the old persistent IDs `seq_b1_enable` and `seq_b2_enable`, respectively,
so existing projects and automation retain their values. Both default Off.

- **Off:** keyboard MIDI notes play the selected bassline directly on MIDI
  channel 1 (Bassline 1) or channel 2 (Bassline 2). The sequencer does not
  generate notes for that lane (except manual Alt+Q preview).
- **On:** pressing a keyboard note on that bassline's MIDI channel **triggers
  its selected pattern** at step 1; the physical note is **not** played or
  forwarded alongside the generated notes. Releasing the final held key stops
  the sequence and releases its generated note. Holding multiple keys keeps
  the sequencer running until the last release. Each lane triggers and stops
  independently. Keyboard notes select timing, not pitch transposition in this
  version.
- The pattern clock follows REAPER's BPM; while stopped, it uses the last
  available host BPM (120 if none is supplied). A note retriggers step 1 when
  all keys had been released. Preview via the step editor still works.
- Other channel messages (such as CC or pitch bend) still reach the relevant
  bassline. MIDI channels 3–16 remain ignored by the sound engines.
- Level of Bassline 2 still defaults to 0; raise Level to hear it.

Only static regression checks are run locally. Build on GitHub Actions, then
try both modes and MIDI channels independently in an empty REAPER project.

---

# LB-303 0.0.2 — TEST2.2.7 (fix compilazione GitHub Actions)

- Risolve errore MSVC `C2248` in `Source/SequencerStepEditor.cpp` alla riga 21:
  `juce::FontOptions` ora riceve esplicitamente `juce::Font::plain` come terzo argomento.
- Non cambia DSP, sequencer, pattern, scorciatoie, identificatori dei parametri o accessibilità.
- Aggiunto un controllo statico per questa regressione; l'artefatto Actions porta TEST2.2.7.
- Il log precedente contiene anche warning non bloccanti di conversione e signedness.
- Non è stata eseguita una compilazione locale; verificare su GitHub Actions.

---

# LB-303 0.0.2 — TEST2.2.5 (parameter picker can change bassline)

- **Page 7 — Parameters** (zero-based code index `6`): on a selected step, **Enter** opens a separate accessible selection view listing **37 parameters of the current bassline only** in columns of eight rows. Page 6 remains Launch Step.
- In the selector use Up/Down (rows), Left/Right (columns), Home/End (column edges), Ctrl+Home/End (first/last), PageUp/PageDown (five rows), or initial letters to find parameters. **Enter** assigns/selects one of up to 32 locks for the step and returns to Parameters; **Alt+Enter** adds/removes the highlighted lock without closing the selector; **Esc** returns without changing selections. Boundary keys are silent.
- On the Parameters page, Up/Down edits the highlighted lock value using the parameter's documented range/step, Alt+Home/End changes current lock, Delete removes one, Alt+Delete clears all, Backspace resets one to its initial value. Changes participate in the existing step Undo/Redo and are stored in the pattern bank; they never modify the APVTS base parameter values. Lock playback in the audio engine is **not yet enabled** (TEST2.3).
- All 32 patterns are independent and pattern selectors remain automatable. No synth DSP modifications, parameter ID changes, or new requirements for local compilation. Only static checks have run: GitHub Actions and NVDA/REAPER test pending.

---

## TEST2.2.3 — Tab follows the active Start/End Step range

In Alt+Q, the current pattern on the current bassline has a separate playback
range: **Z** decreases Start Step, **X** increases Start Step, **C** decreases
End Step, and **V** increases End Step (1–128), matching LJuno. This range is
saved with that pattern. If Start=1 and End=32, **Tab** traverses only the
1–16 and 17–32 blocks; at the final block Tab is silent, and Shift+Tab is
silent at the first block. Different patterns on Bassline 1 and Bassline 2
retain independent limits. The editor also does not select or modify steps
outside the active range, and automatically moves selection into range when
Start/End, track, or pattern changes. Editing the range never erases step data
outside it; lengthening the pattern exposes those steps again.

All existing shortcuts remain: Home/End choose track, Ctrl+Up/Down choose the
current track's pattern, and Ctrl+Tab remains unassigned inside Alt+Q.
No synthesizer DSP or VST parameter IDs changed. Static checks only: compile
using GitHub Actions and test behavior with NVDA/REAPER.

---

# LB-303 — VST3 0.0.2 TEST2.2.3 (Active step range)

**Two independent pattern selectors:** Pattern Bassline 1 (1–16) and Pattern Bassline 2 (1–16). Selecting a new pattern in either lane never changes the other. There are **32 patterns total** (16 per bassline), each with 128 steps and up to 32 parameter-lock slots per step. Both use REAPER's transport clock, but have independent pattern lengths, playback positions and note events. Bassline 1 outputs MIDI channel 1; Bassline 2 outputs MIDI channel 2.

Use **Ctrl+Tab** to open the Sequencer parameter grid (Bassline 1 → Bassline 2 → Sequencer). This grid has **Pattern Bassline 1, Pattern Bassline 2, Sequencer Bassline 1, Sequencer Bassline 2, Editor Track**. Navigation remains Grid/Value/Edit with silent bounds and NVDA announcements. The two Play controls default to Off, so existing projects remain unaffected. Bassline 2 Level still defaults to 0; raise it to hear track 2.

**TEST2.2:** Alt+Q opens the accessible step editor and can now program both
independent 16-pattern banks: Note, Length, Velocity, Repeat, Shift and Launch Step.
The seventh page shows Parameter Lock counts only; adding/applying locks is
planned for TEST2.3. Pattern content is retained when a project is reopened.
All 32 new patterns start as rests until you program them. GitHub Actions
compilation and actual testing with NVDA/REAPER have not yet been performed.

---

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
| Alt+Q | Open the accessible two-track step editor (TEST2.2) |
| Ctrl+Tab | Cycle Bassline 1 → Bassline 2 → Sequencer → Bassline 1 |

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
4. Download the `LB-303-0.0.2-TEST2.2.5-Windows-x64` artifact after it succeeds.
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
  did not contain a functional sequencer engine, clocks or patterns (now added in TEST2).
- `Ctrl+Tab` rotates **Bassline 1 → Bassline 2 → Sequencer → Bassline 1**.
  The Sequencer context displays a descriptive placeholder without bogus
  editable controls. Earlier TEST2.1 still reported that step editing was not yet
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


## TEST2.2 — accessible two-track step editor

Press **Alt+Q** while focused in Grid, Value or Edit. The current Bassline
context chooses the track; from Sequencer context the independent **Editor
Track** parameter chooses it. **Esc** returns to the exact control used to
open the editor. Loading the plug-in still does not steal focus from REAPER.

Within the editor:

- **1–7**: Note, Length, Velocity, Repeat, Shift, Launch Step, Parameters.
- **Q W E R T Y U I / A S D F G H J K**: select one of 16 visible steps.
- Double-press a step key to select/deselect it for multi-step edits.
- **Tab / Shift+Tab**: move between the eight blocks of 16 steps (128 total).
- **Home / End**: previous/next bassline editing track (Home selects Bassline 1; End selects Bassline 2). Silent at boundaries. Ctrl+Tab has no action inside the step editor; outside it still switches the three parameter grids.
- **Ctrl+Up / Ctrl+Down**: next/previous pattern (1–16) for the **currently edited bassline only**; the other pattern stays unchanged. The new selection is announced to NVDA/OSARA and updates the corresponding host-automatable VST3 parameter.
- **Up/Down**: edit step; **Left/Right**: increment width 1,5,10,...40;
  **PageUp/PageDown**: move by chosen increment.
- **Enter on Note page**: toggle Pause / C4 (note 60); useful to program an initially empty pattern.
- **Backspace**: reset musical properties of selected steps (rest = MIDI note 128).
- **Ctrl+C, Ctrl+X, Ctrl+V**: copy/cut/paste one or selected steps;
  **Ctrl+Z / Ctrl+Shift+Z**: undo/redo (50 actions).
- **Space**: preview selected bassline even with REAPER stopped; pressing
  again or leaving the editor stops preview. Existing MIDI 1/2 routing is kept.
- **Alt+I**: read track, pattern, page, step and current playback range.

All 32 patterns are independent and are stored in the REAPER project via the
existing TEST2.1 state format. **Page 7 Parameters** displays lock counts,
but Parameter Lock assignment and playback are intentionally reserved for
TEST2.3. The synth audio path and original parameter IDs are unchanged.

Compiled only through GitHub Actions; no local build or REAPER audio test performed.


## TEST2.2.1 — separate track and pattern shortcuts

This release changes **only** the keyboard behavior in the sequencer step editor:

- **Home** moves to the previous bassline (only two: 1 and 2); **End** to the next.
  At Bassline 1 with Home and Bassline 2 with End the navigation is silent.
- **Ctrl+Up** selects the next pattern 1–16 of the edited bassline; **Ctrl+Down**
  the preceding pattern, with silent limits. The other bassline's pattern
  selection is untouched.
- Both **Pattern Bassline 1** (`seq_b1_pattern`) and **Pattern Bassline 2**
  (`seq_b2_pattern`) are already present in the **Sequencer** Grid/Value/Edit
  parameter context (reach it with Ctrl+Tab from Bassline 1, then Ctrl+Tab
  from Bassline 2) as two separate JUCE AudioParameterChoice VST3 parameters.
  They can be automated independently in REAPER, and the editor shortcuts
  update the **same** automatable parameter, not a private editor value.
- The two pattern banks still hold 16 patterns each (32 total); MIDI routes to
  channels 1 and 2. Synth DSP, parameter IDs, project persistence, editor
  focus, Grid/Value/Edit, and Parameter Locks roadmap are unchanged.
- The matching **LJuno-116** shortcut change is *proposed for a future update*;
  it is not part of this LB-303 patch.

This source has static checks only: compile using GitHub Actions and check the
keyboard/screen-reader behavior in REAPER before considering the change verified.


## TEST2.2.5 — Assign locks across both basslines without exiting the picker

In Alt+Q step editor, press 7 for Parameters, then Enter for the picker:

- **Ctrl+Up / Ctrl+Down while the picker is OPEN:** switch to Bassline 2 / Bassline 1. This does **not** change either pattern; each lane retains its own pattern selector (1–16) and step bounds.
- **Alt+Enter:** toggle the highlighted parameter lock **in the current bassline** and keep the picker open. Repeat to assign multiple locks. If the same parameter is already locked, Alt+Enter removes it (as in LJuno).
- **Enter:** select/add the highlighted parameter lock in the current bassline, then close the picker, returning to page 7. If that lock is already present, Enter simply selects it and closes; it does **not** add a duplicate.
- **Esc:** close the picker without deleting locks already assigned.
- **Ctrl+Up / Ctrl+Down when picker is CLOSED:** unchanged; change pattern 1–16 of the current bassline. **Home/End** in the step editor still switch tracks. All boundary attempts remain silent.

For example, add three locks on Bassline 1 using Alt+Enter, Ctrl+Up to Bassline 2, add two locks with Alt+Enter, then Enter while an already-assigned parameter is highlighted. The result is **3 locks on Bassline 1 and 2 on Bassline 2**, both on their respective currently selected patterns and step positions. The picker remembers the selected step separately for each lane, clamped to that lane's Start/End range, and announces bassline, pattern, step and parameter when switching. Undo/Redo and project-state storage remain independent per lane/pattern/step.

**Parameter Lock playback is still deferred to TEST2.3.** Static tests only; GitHub Actions and real NVDA/REAPER tests still required. LJuno itself is not modified.

## TEST2.2.6 — Simplified parameter picker and selected announcements

This release supersedes the **picker-only** Ctrl+Up/Down track switching added
in TEST2.2.5. In the open picker, Ctrl+Up/Down now has **no action** (silently
consumed); it cannot switch basslines or accidentally adjust a pattern. Choose
Bassline 1 or 2 with **Home/End in the step editor before opening the picker**.
With the picker closed, Ctrl+Up/Down still changes the independent 1–16 pattern
of the current bassline, exactly as before.

On page 7 (Parameters), Enter opens the picker. Any parameter **already assigned
as a Parameter Lock to the current step** now has the literal prefix **`selected `**
BEFORE its name, both in the grid and in the NVDA/OSARA announcement, matching
LJuno's convention. Non-assigned parameters are read by name only. This status
updates immediately when Alt+Enter toggles the lock, without closing the picker.
Enter adds/selects and closes; Escape closes without discarding locks already
added. The separate 16-pattern banks and saved parameters are unchanged.

Parameter Lock **playback** is still planned for TEST2.3. Only static checks
have been run; build on GitHub Actions and test announcements in REAPER/NVDA.
