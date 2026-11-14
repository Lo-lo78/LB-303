# LB-303 0.99.5 — TEST2.4.70 Step micro-preview

Date: 14 November 2026. Based on 0.99.4 TEST2.4.69 release sources.

In Alt+Q Steps, Ctrl+P selects Current / Start-to-current / Current-to-end; Ctrl+Space auditions that range regardless of Alt+P. A real edit of the focused step automatically auditions the current step, except during live DAW/preview/step recording. Tab reads Bassline/Pattern/Step; Ctrl+Tab navigates bounded 16-step blocks (Ctrl+Shift+Tab backwards).

DSP: one-lane, one-pass PPQ audition through SequencerPlayback, real per-step gates and Parameter Locks. Release sustained notes at the end boundary. Preview request uses one atomic command; no file/preset state changes. Silence other lane and preserve existing host/preview modes.

Requires MSVC GitHub Actions VST3 build and NVDA/REAPER audition to confirm final behaviour.
