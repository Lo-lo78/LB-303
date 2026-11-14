#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[1]
e=(r/'Source/SequencerStepEditor.cpp').read_text(encoding='utf8')
p=(r/'Source/PluginProcessor.cpp').read_text(encoding='utf8')
h=(r/'Source/SequencerStepAudition.h').read_text(encoding='utf8')
s=(r/'Source/SequencerPlayback.cpp').read_text(encoding='utf8')
t=(r/'Source/SequencerTiming.h').read_text(encoding='utf8')
assert 'cycleStepAuditionMode(); return true;' in e and 'auditionStep (true); return true;' in e
assert "pressedLower == 'p'" in e and 'code == juce::KeyPress::spaceKey' in e
assert 'if (previewCurrentStep && ! suppressMicroPreview) auditionStep (false);' in e
assert 'announceStepPosition(); return true;' in e
assert 'changeBlock (mods.isShiftDown() ? -1 : 1)' in e
assert 'stepAuditionCommand.exchange (0' in p
assert 'const bool activePreview = fullPreview || stepAuditionActive' in p
assert 'entries[static_cast<size_t> (lane)] = stepAuditionActive ? 0 : -2;' in p
assert 'stepAuditionActive ? &stepAuditionRange : nullptr' in p
assert 'sequencerPlayback.generate (stopNotes' in p
assert 'stepAuditionDuration' in p and 'stopSample' in p
assert 'localAuditionRange != nullptr ? 0.0' in s
assert 'if (songEntry && cycle > 0) break;' in s
assert 'inline double auditionDuration' in t
assert 'packStepAudition' in h and 'unpackStepAuditionPattern' in h
assert (r/'Tests/test270_step_preview.cpp').exists()
print('PASS TEST2.4.70 step preview entry, scope, PPQ, timing, one-shot and keyboard')
