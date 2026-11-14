# LB-303 0.99.5 TEST2.4.73 — Micro-preview must not alter shared delay

Date: 14 November 2026. Built on TEST2.4.72.

**Fixed:** Local step-key, edit-triggered and Ctrl+Space previews no longer reset
`SharedEffects` at the beginning of a one-shot. The former `sharedFx.clearTails()`
call erased the Classic delay history and its slowly smoothed read position.
Even after the TEST2.4.72 cache correction, every reset retriggered the tape-like
~0.5 s time glide and erased pending echoes. Now only sequencer scheduling and
held synth notes restart. The shared delay and other FX remain continuous.

**Delay isolation:** A micro-preview reads the normal host-controlled Delay
Time/Speed, Feedback, Glide, filters and other FX settings without modifying
them. Historic per-step B1 delay-parameter locks remain valid during normal
pattern/song playback, but do not override the shared delay during local
one-shot previews, including the final audio block when an audition stops. Per-step *Delay Send* levels (including Normal/Accent/Mute)
continue to control the amount of newly generated step audio sent into the
existing FX bus; previews therefore can still be heard through delay and its
natural feedback tail. Reverb sends, chorus and ordinary audition note/lock
processing are preserved.

**Regression:** New `Tests/check_test2473_fx_isolation.py` checks that
micro-preview does not invoke `clearTails()` and that legacy delay controls are
not overridden during it. Native `LB303StepPreviewFxIsolationTest` drives the
real `SharedEffects`/`BassEngine` delay with repeated step boundaries and
verifies identical glide and audible feedback tails. The previous test for
explicit reset (`LB303DelayResetTest`) is still retained: Alt+I must continue
to reset FX safely. No new VST parameters or preset migration are required.
