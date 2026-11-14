#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[1]
e=(r/'Source/SequencerStepEditor.cpp').read_text(encoding='utf-8')
h=(r/'Source/SequencerStepEditor.h').read_text(encoding='utf-8')
a=(r/'Source/SequencerStepAudition.h').read_text(encoding='utf-8')
w=(r/'.github/workflows/build-windows.yml').read_text(encoding='utf-8')
select=e[e.index('void SequencerStepEditor::selectStep (int inBlock)'):e.index('void SequencerStepEditor::selectVisibleSteps()')]
assert select.count('auditionStep (false);') == 2
assert 'currentStep = index;\n    // Physical' in select
assert 'currentStep = index;\n        auditionStep (false);' in select
assert 'if (midiEntryActive)' in select and select.index('if (midiEntryActive)') < select.index('auditionStep (false);')
assert 'if (doublePress)' in select and 'item = ! item' in select
assert 'stepAuditionMode = (stepAuditionMode + 1) % 2' in e
assert 'stepAuditionMode + 1' in e
assert 'const char* names[] { "Start to current step", "Current step to end" }' in e
assert 'int stepAuditionMode = 0' in h
assert 'StepAuditionMode::current' in a
assert 'processor.isPreviewPlaying() || processor.isHostTransportPlaying()' in e
assert 'LB-303-0.99.5-TEST2.4.73-Windows-x64' in w
assert 'python Tests/check_test2471_press_step_preview.py' in w
print('PASS TEST2.4.71: immediate step key audition, two Ctrl+P ranges and no MIDI-entry overlap')
