# LB-303 TEST2.4.75 — AHDSR Hold, 16 November 2026

**Base:** 0.99.5 TEST2.4.74. Test build only; public release/version unchanged.

- Added a separate **Hold** parameter immediately after Attack in the accessible Bassline 1 and Bassline 2 pages. AHDSR order is Attack → Hold → Decay → Sustain → Release.
- Range: **0.000–5.000 s**, in steps of **0.001 s**; default **0.000 s**, matching old preset audio and gate/filter behavior.
- The two oscillators of each bassline share its envelope. The two basslines have **independent** Hold values and counters.
- Host parameters `b1_env_hold` and `b2_env_hold` are append-only after all existing parameters. Existing REAPER automation IDs, legacy slots and preset data are unchanged.
- Accessible sequencer Parameter Locks can select Hold; its new lock slot is **56** per bassline, after oscillator-2 slots 54–55. Old pattern formats and locks are preserved.
- At the end of Attack, Hold keeps the envelope exactly at 1.0, then hands over to original Decay. Note Off or releaseNote() interrupts any stage including Hold. Legato preserves the existing stage/timer; re-trigger after a note-off restarts Attack and Hold.
- Host automation of Hold is applied in real time; changing it during the stage changes the remaining time. Mid-note 1×/2×/4× oversampling retains elapsed physical time.
- Alt+C/Alt+V current Bassline clipboard includes Hold. Older **43-parameter V1** clipboards are accepted, with their old parameter positions preserved; new Hold is left unchanged when pasting an old clipboard.
- Baseline C++ DSP tests, source regressions and an additional actual-engine AHDSR test. CI runs `LB303HoldEnvelopeTest.exe` before uploading the Windows VST3.

**Testing:** full Windows JUCE VST3 build and audio/NVDA check are performed via GitHub Actions and REAPER; not claimed as locally built.
