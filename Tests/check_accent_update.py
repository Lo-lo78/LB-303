from pathlib import Path
p=Path(__file__).resolve().parents[1]/'Source'
h=(p/'SequencerState.h').read_text(); st=(p/'SequencerState.cpp').read_text(); e=(p/'SequencerStepEditor.cpp').read_text(); pb=(p/'SequencerPlayback.cpp').read_text(); b=(p/'BassEngine.cpp').read_text(); pr=(p/'PluginProcessor.cpp').read_text()
assert 'int accent = 0;' in h and 'std::atomic<int> accent' in h
assert 'stream.writeInt (5)' in st and 'formatVersion >= 4' in st
assert 'result.accent = loadRelaxed (step.accent)' in st and 'source.accent' in st
assert '"Note", "Length", "Accent", "Velocity", "Repeat", "Shift", "Launch Step", "Parameters"' in e
assert 'if (after.length == 101) after.length = after.lengthBeforeLegato;' in e
assert 'else after.accent = 1 - after.accent;' in e
assert 'page == 2 ? 3 : page == 3 ? 2 : page >= 4 ? page - 1 : page' in e
assert '119, step.accent ? 127 : 0' in pb
assert 'pendingSequencerAccent >= 0' in b
assert 'outputMessage.getControllerNumber() == 119) continue;' in pr
print('PASS: accent step storage, eight pages, Enter toggles, internal note-on override, version 4 backward compatibility (static).')
