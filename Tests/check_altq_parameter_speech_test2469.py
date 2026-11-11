"""Regression test: Alt+Q page 6 speech must use internal page 5."""
from pathlib import Path
source = (Path(__file__).resolve().parents[1] / 'Source/SequencerStepEditor.cpp').read_text(encoding='utf-8')
assert 'case 5: return index == currentStep ? currentLockText()' in source
assert 'case 6: return index == currentStep ? currentLockText()' not in source
assert 'if (page == 5)\n            announceCurrent();' in source
assert 'void SequencerStepEditor::changeSelectedLock (int direction)' in source
assert 'if (code == juce::KeyPress::homeKey) { changeSelectedLock (-1); return true; }' in source
assert 'if (code == juce::KeyPress::endKey) { changeSelectedLock (1); return true; }' in source
assert '+ ", Parameters, " + lb303::catalog()[catalogIndex].name + " "' in source
assert 'announceCurrent();\n}\nvoid SequencerStepEditor::changeSelectedLock' not in source
print('PASS TEST2.4.69 Parameters screen reader announcements')
