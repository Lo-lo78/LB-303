# LB-303 0.99.5 — TEST2.4.74 (16 November 2026)

- **Ctrl+Shift+A** on Grid, Value, and Edit: toggle the real REAPER FX envelope ACTIVE flag for the selected VST3 parameter. The existing points are preserved, and NVDA announces `, Auto` only when ACTIVE. Based on the verified LR-608 TEST36.3 / LSampler-24 host extension; includes the Windows Ctrl/Command alias fix.
- **Ctrl+Shift+Tab**: page backward in the same four-page cycle already used by Ctrl+Tab, preserving last selection on each page. Reverse order: FX → Sequencer → Bassline 2 → Bassline 1 → FX.
- This does **not** alter parameter IDs, layouts, audio DSP, presets, song banks, patterns, Alt+Q shortcuts, or the tab order (Grid, Value, Edit). Inside modal Alt+Q and preset browser, the shortcuts remain owned by the modal component.
- If the plugin is hosted somewhere other than REAPER and the host lacks a uniquely identifiable activate/deactivate envelope context-menu command, the fallback only marks the parameter last-touched; it never claims that hiding a lane deactivates its envelope.
- In REAPER the extension validates the focused FX's parent track and exact parameter name before changing any envelope. The new API never erases recorded envelope points.

TEST2.4.74 is for CI and keyboard/NVDA validation, **not** a new public version.
