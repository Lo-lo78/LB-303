# TEST2.4.64 — Escape cancels MIDI Step Entry

During Ctrl+K, Escape discards all notes, rests, and overwritten step data
from the current session and restores their state at Ctrl+K activation.
No ghost Undo entries are created. Ctrl+K or automatic End Step commits the
recording as one undoable action. Escape outside the mode closes Alt+Q as before.
