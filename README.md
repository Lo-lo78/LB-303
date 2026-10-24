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
  No page combo box. Native focusable JUCE buttons with screen reader names,
  Windows UI Automation announcements following the LJuno approach.
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
| Alt+Shift+Up / Alt+Shift+Down | Coarse increase / decrease |
| Enter or Space | Increase parameter (native button activation) |
| Home / End | Minimum / maximum value |
| PageUp / PageDown | Coarse value increase / decrease |
| Ctrl+Home / Ctrl+End | First / last parameter |
| Alt+L | Return focus to parameter grid |
| Alt+Q | TEST1 announces that sequencer editor is scheduled for TEST2 |
| Ctrl+Tab | TEST1 announces that sequencer grid is scheduled for TEST2 |

The focused parameter name and value are also set as native accessibility text,
so screen-reader line reading has a concise value. Host interception of global
shortcuts may vary; please test Alt+L and arrow focus with NVDA/OSARA in REAPER.

## Build on GitHub Actions

1. Create a public (or private) repository named `LB-303`.
2. Upload the **contents of this archive** to the repository root. Keep the
   `.github/workflows/build-windows.yml` hierarchy.
3. Open **Actions → Build LB-303 Windows x64**. The `push` starts the build,
   or use `Run workflow` manually.
4. Download the `LB-303-0.0.1-TEST1-Windows-x64` artifact after it succeeds.
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
