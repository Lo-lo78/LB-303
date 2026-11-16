# LB-303 — TEST2.4.76 — Sequencer playback resilience (16 November 2026)

Test source based on LB-303 v0.99.6. This is **not** a new public release and does not change VST parameter IDs, preset format, editor pages, song layout, or AHDSR defaults.

## Confirmed source-level failure paths fixed

1. **Alt+Q page 3 / Accent and ties:** One Note On is intentional for a sequence of equal-pitch Length 101 tied steps. The previous code consequently never told BassEngine about any subsequent step's Velocity. Internal poly-aftertouch now refreshes the **active** monophonic note's velocity and accent at the exact step timestamp, without creating a second Note On or restarting its AHDSR stage. This is **not** sent to REAPER's MIDI output. It also handles equal-pitch ordinary steps following a tie.
2. **Explicit maximum velocity:** A page-3 Enter toggle to Velocity 127 is a deliberate Accent. Velocity Random Depth previously could reduce it below the synth's Accent Threshold. The step now stays at 127; nonmaximum velocities still follow existing randomization.
3. **Micro-preview transpose:** Alt+Q physical step-key audition uses the programmed step note, unaffected by the last MIDI keyboard note. The separate global preview retains its previous transpose behavior.
4. **MIDI Step Entry Ctrl+K:** An incomplete prior session could retain an internally held note and suppress newly captured steps. Each new session publishes an epoch; the audio thread clears its own chord bookkeeping on the first input event of that session. All MIDI data remain owned by the realtime thread.
5. **Repeat Random performance:** Non-Song playback previously recalculated all elapsed random-repeat cycles from zero *every audio buffer*. A per-lane audio-thread cursor now resumes from the last musically relevant cycle, invalidated by edits, pattern changes, trigger origins, BPM and seek changes. Exact event scheduling remains based on absolute musical hashes.
6. **Song mode with Repeat Random:** The Song locator previously recomputed all elapsed row lengths for every audio buffer. It now caches the currently playing row and advances using an exact, validated preceding-row hint, with song/bank revision and tempo/seed invalidation. No approximate Song lengths are substituted.

## Tests

`LB303SequencerResilienceTest`: large block versus 500 smaller callbacks, tied Accent timing, and variable-duration Song position with and without hints. `LB303TiedAccentTest`: actual BassEngine gate/accent/AHDSR state. `LB303MidiStepRecoveryTest`: new recording session with the prior note still held. `LB303VelocityStateTest`: Note/Length/Velocity memories, state copy, v10 saves, and compatible legacy v6 loading.

Native tests were executed with the lightweight JUCE testing shims on Linux. A full Windows VST3 compile and interactive REAPER/NVDA test were **not** performed here; GitHub Actions build script is configured to compile and run the new regression tests.

## Listening/interaction checks after compiling

- On Alt+Q page 3, set a note to Velocity 127 with Enter, turn up Velocity Random Depth, and verify that each repeat is still accented.
- Make successive equal-pitch Length 101 steps alternate Velocity 80/127/70/127 and check that accents change while the note remains legato (no extra envelope attack).
- Record MIDI steps with Ctrl+K, exit before releasing a MIDI key, start a new Ctrl+K session, and confirm the next note is recorded.
- Play a long Pattern/Song with Repeat Random and verify that CPU and timing do not progressively deteriorate.
- With REAPER stopped, press step keys after transposing via an external MIDI keyboard; the micro-preview should play the programmed notes.
