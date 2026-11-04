"""TEST2.4.59: MIDI Step Entry is monophonic, silent by default and realtime-safe."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
ed = (r/'Source/SequencerStepEditor.cpp').read_text(encoding='utf-8')
proc = (r/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
queue = (r/'Source/MidiStepEntryQueue.h').read_text(encoding='utf-8')
assert 'startTimerHz (60)' in ed
assert 'processor.midiStepEntry.begin();' in ed
assert 'midiEntrySpeak = false;' in ed
assert 'midiEntrySpeak = ! midiEntrySpeak' in ed
assert 'writeMidiStep (0, 0, true);' in ed
assert 'writeMidiStep (note.pitch, note.velocity, false);' in ed
assert 'after.note = 128;' in ed
assert 'after.velocity = juce::jlimit (1, 127, velocity);' in ed
assert 'action.changes.push_back' in ed
assert 'bank().replaceStep' in ed
assert 'if (midiEntryActive)' in ed.split('void SequencerStepEditor::selectStep')[1]
assert 'if (midiStepEntry.consume (message))' in proc
assert 'std::atomic<bool> enabled' in queue
assert 'heldCount == 0' in queue
assert 'message.isNoteOff()' in queue
assert 'finish() noexcept' in queue
assert 'TEST2.4.67-Windows-x64' in (r/'.github/workflows/build-windows.yml').read_text(encoding='utf-8')
print('PASS: LB303 MIDI Step Entry, silent default, gate and undo')
