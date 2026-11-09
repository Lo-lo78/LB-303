# LB-303 v0.99.3 (TEST2.4.68)

**Release date: 9 November 2026**

## What's included

- Deterministic restart for Free-mode LFOs on both basslines when the VST3 host transitions from Stop to Play, preserving MIDI Note On behavior in Triggered mode.
- Incremental keyboard parameter search in Grid and Value across Bassline 1, Bassline 2, Sequencer and FX, as well as the Alt+Q Parameter Lock picker (TEST2.4.67).
- About version and release date updated to 0.99.3 / 9 November 2026.
- Seven-language built-in Help updated; README and release documentation refreshed.
- CMake project version and GitHub Actions Windows x64 VST3 artifact updated to 0.99.3.

## Compatibility and validation

No additional sound-engine, MIDI, preset, sequencer, parameter-ID or shortcut changes were made for this release bump. Source-level regression tests should pass; full Windows VST3 compilation and a functional REAPER/NVDA test are still required before publishing the binary.
