# LB-303 0.99.6 — AHDSR Hold and REAPER Automation

**Release date:** 16 November 2026  
**Previous public release:** 0.99.5 (14 November 2026)  
**Based on:** TEST2.4.74 and TEST2.4.75

## AHDSR Hold — Bassline 1 and Bassline 2

- Added one independent **Hold** to the envelope of each bassline between **Attack** and **Decay**, making the contour Attack → Hold → Decay → Sustain → Release.
- Hold duration is **0.000–5.000 s**, in **0.001 s (1 ms)** steps, default **0.000 s** to retain the old envelope response in existing presets.
- Each bassline's two oscillators share the same envelope. Hold affects the envelope used for its amplitude and filter behavior; the two basslines have independent settings and sample counters.
- **Note Off** moves into Release even during Hold. Legato, retrigger and the original Decay/Release behavior remain supported; Hold timing follows the physical sample rate at **1×, 2× and 4× oversampling**.
- The new parameters **b1_env_hold** and **b2_env_hold** are appended after previously published VST3 parameters, without shifting existing automation IDs. Each is visible immediately after Attack on its Bassline page.
- Each Hold can be automated through REAPER and selected in the Alt+Q sequencer's **Parameter Locks** (new lock slot **56** per bassline; prior lock slots are unchanged).
- Alt+C/Alt+V Bassline clipboard now includes Hold in the new **44-parameter** format; old **43-parameter** clipboard text remains supported and leaves Hold unchanged.

## REAPER automation and backward page navigation

- **Ctrl+Shift+A** from Grid, Value or numeric Edit toggles the selected VST3 parameter envelope's **ACTIVE** status in REAPER, without deleting its points. NVDA adds **", Auto"** only when automation is active.
- **Ctrl+Tab** advances and **Ctrl+Shift+Tab** reverses through Bassline 1, Bassline 2, Sequencer and FX. The last selection on each page is retained.
- These shortcuts do not override modal operations in Alt+Q or the preset browser. Outside REAPER, there is no false claim of successful envelope deactivation when the host does not support it.

## Accessibility, compatibility and documentation

- **About (Alt+A):** version **0.99.6**, release date **16 November 2026**.
- Updated seven-language built-in HTML Help, README, VST3/CMake metadata and GitHub Actions artifact name.
- Audio synthesis and sequencing outside the already tested Hold feature are unchanged. Existing parameter IDs, presets, saved patterns and Song data remain compatible.
- Existing TEST2.4.74/TEST2.4.75 tests remain in CI; a new 0.99.6 metadata/documentation check is included.

**Validation:** source-level regressions run locally; the full Windows VST3 build and REAPER/NVDA behavior still require verification with GitHub Actions and the compiled binary.

Technical details: `Doc/TEST2.4.74_HOST_AUTOMATION_REVERSE_PAGE.md`, `Doc/TEST2.4.75_AHDSR_HOLD.md`.
