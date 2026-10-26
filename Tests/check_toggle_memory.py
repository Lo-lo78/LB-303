"""Static safety regression for persistent per-step Enter toggles.

Run alongside the existing static checks. Executable C++ SequencerState tests
also run independently with a JUCE test stand-in, where JUCE is unavailable.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
state = (root / 'Source/SequencerState.cpp').read_text(encoding='utf-8')
header = (root / 'Source/SequencerState.h').read_text(encoding='utf-8')
editor = (root / 'Source/SequencerStepEditor.cpp').read_text(encoding='utf-8')
assert 'int noteBeforePause = 60;' in header
assert 'int lengthBeforeLegato = 50;' in header
assert 'std::atomic<int> noteBeforePause { 60 };' in header
assert 'std::atomic<int> lengthBeforeLegato { 50 };' in header
assert 'const int target = change.before.note >= 128 ? change.before.noteBeforePause : 128;' in editor
assert 'if (after.length == 101) after.length = after.lengthBeforeLegato;' in editor
assert 'else { after.lengthBeforeLegato = after.length; after.length = 101; }' in editor
assert 'const int target = change.before.note >= 128 ? 60 : 128;' not in editor
assert 'after.length == 101 ? 100 : 101' not in editor
assert 'if (requestedNote < 128)' in state
assert 'step.noteBeforePause.store (requestedNote' in state
assert 'if (requestedLength != 101)' in state
assert 'step.lengthBeforeLegato.store (requestedLength' in state
assert 'result.noteBeforePause = loadRelaxed (step.noteBeforePause)' in state
assert 'result.lengthBeforeLegato = loadRelaxed (step.lengthBeforeLegato)' in state
assert 'source.noteBeforePause' in state and 'source.lengthBeforeLegato' in state
assert 'destinationStep.noteBeforePause' in state and 'destinationStep.lengthBeforeLegato' in state
assert 'stream.writeInt (5);' in state
assert 'formatVersion != 4 && formatVersion != 5' in state
assert 'stream.writeByte (static_cast<char> (juce::jlimit (0, 127, step.noteBeforePause)))' in state
assert 'stream.writeByte (static_cast<char> (juce::jlimit (0, 100, step.lengthBeforeLegato)))' in state
assert 'formatVersion >= 5' in state
assert '60), std::memory_order_relaxed)' in state
assert '100), std::memory_order_relaxed)' in state
assert 'reset.noteBeforePause = 60; reset.lengthBeforeLegato = 50;' in editor
assert 'if (page == 7) openParameterPicker();' in editor
assert 'else after.accent = 1 - after.accent;' in editor
print('PASS: Enter per-step Note/Length memory, serialization v5, old-format reading, Undo/copy/picker unchanged (static).')
