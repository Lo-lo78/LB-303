"""Confirm that Alt+Q 8/9 reuse real sequencer VST3 switches, not fake pages."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
cpp = (root/'Source/SequencerStepEditor.cpp').read_text(encoding='utf-8')
h = (root/'Source/SequencerStepEditor.h').read_text(encoding='utf-8')
cat = (root/'Source/ParameterCatalog.cpp').read_text(encoding='utf-8')
proc = (root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
keys = cpp[cpp.index("if (ch >= '7' && ch <= '9'"):cpp.index("if (ch >= '1' && ch <= '6'", cpp.index("if (ch >= '7' && ch <= '9'"))]
assert "focusedGlobalParameter = static_cast<int> (ch - '7');" in keys
assert 'page =' not in keys, 'global focus must not change Note/Length/etc page'
assert 'announce (globalParameterText());' in keys
assert 'focusedGlobalParameter' in h
assert 'sequencer1EnabledIndex' in cpp and 'sequencer2EnabledIndex' in cpp
assert 'parameter (index)' in cpp and 'setValueNotifyingHost' in cpp
assert 'beginChangeGesture();' in cpp and 'endChangeGesture();' in cpp
assert 'changeSequencerEnabled (direction)' in cpp
assert 'if (focusedGlobalParameter > 0) { resetSequencerEnabled(); return; }' in cpp
assert 'if (focusedGlobalParameter >= 0)' in cpp.split('void SequencerStepEditor::selectStep')[1]
assert 'focusedGlobalParameter = -1;' in cpp.split('void SequencerStepEditor::selectStep')[1]
assert 'if (focusedGlobalParameter >= 0) return true;' in cpp, 'Enter must not alter hidden steps'
assert 'result[sequencer1EnabledIndex] = { "seq_b1_enable"' in cat
assert 'result[sequencer2EnabledIndex] = { "seq_b2_enable"' in cat
assert 'value (lb303::sequencer1EnabledIndex) > 0.5f' in proc
assert 'value (lb303::sequencer2EnabledIndex) > 0.5f' in proc
assert 'TEST2.4.73-Windows-x64' in (root/'.github/workflows/build-windows.yml').read_text(encoding='utf-8')
print('PASS: Alt+Q 8/9 use existing host-automatable Sequencer switches; step page persists')
