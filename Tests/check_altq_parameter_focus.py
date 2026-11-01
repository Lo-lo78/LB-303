"""Alt+Q page 6 Parameters, keys 7/8/9 only focus global parameters."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
cpp = (root / 'Source/SequencerStepEditor.cpp').read_text(encoding='utf-8')
h = (root / 'Source/SequencerStepEditor.h').read_text(encoding='utf-8')
assert 'int focusedGlobalParameter = -1;' in h
assert "if (ch >= '1' && ch <= '6'" in cpp
assert "if (ch >= '7' && ch <= '9'" in cpp
assert "focusedGlobalParameter = static_cast<int> (ch - '7');" in cpp
assert "const int target = static_cast<int> (ch - '1');" in cpp
assert 'else if (page == 5 && focusedGlobalParameter < 0) openParameterPicker();' in cpp
assert 'if (focusedGlobalParameter == 0) { changeLaunch (direction, amount); return; }' in cpp
assert 'if (focusedGlobalParameter > 0) { changeSequencerEnabled (direction); return; }' in cpp
assert 'if (focusedGlobalParameter >= 0) return true;' in cpp
assert 'focusedGlobalParameter = -1;' in cpp.split('void SequencerStepEditor::selectStep')[1]
assert 'if (focusedGlobalParameter == 0)' in cpp.split('void SequencerStepEditor::resetCurrent()')[1]
assert 'announce (globalParameterText());' in cpp
assert 'announce ("Page Launch Step' not in cpp
assert 'const juce::String pageNames[] { "Note", "Length", "Velocity", "Repeat", "Shift", "Parameters"' in cpp
assert 'page = target;' in cpp
assert 'TEST2.4.66-Windows-x64' in (root/'.github/workflows/build-windows.yml').read_text(encoding='utf-8')
print('PASS: Alt+Q 6 Parameters; 7 Launch Step; 8/9 Sequencer On/Off parameters')
