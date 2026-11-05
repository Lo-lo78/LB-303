# LB-303 0.99.2 (TEST2.4.68)

**5 November 2026**

- Free LFOs in Bassline 1 and 2 restart at the host's Stop-to-Play transition, including Sample & Hold and One Shot.
- Triggered LFOs still follow MIDI Note On. Synthesis voices, sequencer, shared effects and preview remain untouched.
- No new parameters or shortcuts; native C++ regression test in GitHub Actions.
- Full VST3 compilation and audible DAW verification remain required.

---

# LB-303 0.99.2 (TEST2.4.67)

**4 November 2026**

- Incremental keyboard search in Grid and Value for both Bassline pages, Sequencer and FX.
- Same search in the Alt+Q Parameter Locks selection list, excluding the spoken `selected` prefix.
- Progressive text prefixes, beginnings of internal words, repeated-letter cycling, circular navigation, Unicode names and a one-second timeout.
- Shift reverses the search direction. Shift+digits match digits, not symbols on the keyboard layout.
- Accessibility guide updated in seven languages and About updated to 0.99.2.
- Win32 `NOMINMAX` protection and a standalone C++ regression test running during the GitHub Actions build.
- No changes to sound synthesis, parameter IDs, presets, MIDI routing, pattern/song data or existing shortcuts.

Windows VST3 compilation and final NVDA/REAPER verification are required before publishing this release.
