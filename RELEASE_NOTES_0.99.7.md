# LB-303 0.99.7 — Sequencer Stability and Bug Fixes

**Release date:** 17 November 2026  
**Previous release:** 0.99.6

This maintenance release promotes the fixes validated in TEST2.4.76 and the GitHub Actions test corrections from TEST2.4.77.

- Fixed Accent/Velocity updates across tied steps without sending an extra Note On or retriggering the envelope.
- Explicit Velocity 127 accents remain intact when Velocity Random Depth is enabled.
- Fixed physical step-key micro-preview so it plays the programmed note regardless of external keyboard transposition.
- Improved Ctrl+K MIDI Step Entry recovery after an interrupted capture session.
- Improved Repeat Random performance and long-running Song playback by avoiding redundant recalculation on every audio buffer.
- Fixed Release-build regression testing and Windows CI text encoding.

No new audio parameters, VST3 parameter IDs, preset formats, or sequencer pages. The existing multi-language help is unchanged. About, build metadata and GitHub Actions artifact naming are updated for 0.99.7.

See `Doc/TEST2.4.76_SEQUENCER_STABILITY.md` and `Doc/TEST2.4.77_CI_RELEASE_FIX.md` for technical details.
