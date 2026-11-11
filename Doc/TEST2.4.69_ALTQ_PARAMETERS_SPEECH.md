# LB-303 TEST2.4.69 — Alt+Q Parameters speech fix

Based on LB-303 0.99.3 TEST2.4.68. No release-version change.

- Fixed the internal page-index mismatch: Parameters is page 5 (the sixth page), not page 6. Step announcements now include lock ordinal, name and value.
- Pressing 6 in Alt+Q announces the current step and its Parameters lock status.
- Up/Down and PageUp/PageDown announce the updated Parameter Lock name and value without its ordinal, like LJuno-116.
- Alt+Home/Alt+End retain their previous/next Parameter Lock behavior and announce its ordinal, name and value.
- No changes to DSP, sequencer playback, serialization, MIDI, or parameter identifiers.
