# LB-303 0.99.5 TEST2.4.72 — Classic delay reset during step audition

Date: 14 November 2026. Built on TEST2.4.71.

**Fix:** Ctrl+Space, step-key auditions and automatic step previews previously
called `SharedEffects::clearTails()`, which reset the classic delay's internal
`delayTargetSamples` to zero. The `BassEngine` parameter cache remained marked
as prepared, so reapplying unchanged user Delay Time / BPM values was skipped.
The delay could sound instantaneous, excessively feedback-heavy or silent until
the user moved a delay parameter. A subsequent step audition could repeat the
problem. `BassEngine::reset()` now invalidates that parameter cache; the next
audio-thread parameter application restores the original delay time and filter
coefficients even if all APVTS values are unchanged.

**Preserved:** shared Delay and Reverb settings, unchanged parameter IDs, legacy
and dual tape delay routing, preset state, one-lane audition, instant step-key
preview, automatic preview on edits, Ctrl+P (two spans), Ctrl+Space, Pattern/Song,
Length 101 and MIDI Step Entry. No saved preset migration.

**Regression:** `Tests/DelayResetAuditionTests.cpp` runs the real BassEngine and
SharedEffects through the existing JUCE shim and compares a clean FX rack with
one reset repeatedly without changing parameters. It failed before the fix
and passes after. GitHub Actions builds and runs `LB303DelayResetTest.exe`.

Full Windows JUCE/MSVC build and NVDA/REAPER listening still require confirmation.
