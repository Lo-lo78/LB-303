"""TEST2.4.58: Page 6 Parameters; 7 selects Launch Step as a parameter.
Source-only guards in the GitHub workflow, no local VST3 build.
"""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
cpp=(root/'Source/SequencerStepEditor.cpp').read_text()
h=(root/'Source/SequencerStepEditor.h').read_text()
assert 'bool launchControlSelected = false;' in h
assert "if (ch >= '1' && ch <= '6'" in cpp
assert "if (ch == '7'" in cpp
assert 'launchControlSelected = true; // a parameter, NOT a page switch' in cpp
assert "const int target = static_cast<int> (ch - '1');" in cpp
assert 'else if (page == 5 && ! launchControlSelected) openParameterPicker();' in cpp
assert 'if (launchControlSelected) { changeLaunch (direction, amount); return; }' in cpp
assert 'if (launchControlSelected) return true;' in cpp
assert 'if (launchControlSelected)' in cpp.split('void SequencerStepEditor::resetCurrent()')[1]
assert 'launchControlSelected = false;' in cpp.split('void SequencerStepEditor::selectStep')[1]
assert 'announce ("Launch Step " + juce::String (bank().getConfig (pattern()).launchStep));' in cpp
assert 'announce ("Page Launch Step' not in cpp
assert 'const juce::String pageNames[] { "Note", "Length", "Velocity", "Repeat", "Shift", "Parameters"' in cpp
assert 'page = target;' in cpp
assert 'TEST2.4.58-Windows-x64' in (root/'.github/workflows/build-windows.yml').read_text()
print('PASS: LB TEST2.4.58 Launch Step parameter and page 6 Parameters')
