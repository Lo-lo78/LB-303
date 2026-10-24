#!/usr/bin/env python3
"""Static regressions for LB-303 TEST1.4. No local compilation."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
original = (root / 'Reference' / 'LB-303.jsfx').read_text(encoding='utf-8')
catalog = (root / 'Source' / 'ParameterCatalog.cpp').read_text(encoding='utf-8')
ids = re.findall(r'"slider(\d{3})"', catalog)
assert ids == [f'{n:03}' for n in range(1, 36)], (len(ids), ids)
assert len(re.findall(r'^slider\d+:', original, re.M)) == 35
assert '.value_or(' not in (root / 'Source' / 'PluginProcessor.cpp').read_text(encoding='utf-8')
for src in ['ParameterCatalog.cpp', 'BassEngine.cpp', 'PluginEditor.cpp',
            'PluginProcessor.cpp', 'ScreenReaderAnnouncer.cpp']:
    assert (root / 'Source' / src).is_file(), src
assert 'LB303_artefacts' in (root / '.github/workflows/build-windows.yml').read_text()

header = (root / 'Source' / 'PluginEditor.h').read_text(encoding='utf-8')
editor = (root / 'Source' / 'PluginEditor.cpp').read_text(encoding='utf-8')
assert 'struct ParameterGrid final : juce::ComboBox' in header
assert 'struct ParameterValue final : juce::Slider' in header
assert 'struct NumericEdit final : juce::TextEditor' in header
assert 'struct ParameterCell' not in header and 'juce::TextButton' not in header
assert 'std::array<std::unique_ptr<ParameterCell>' not in header
assert 'rowsPerColumn = 8' in editor
assert 'valueAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>' in editor
assert 'focusControl (current == Control::grid ? Control::value : Control::grid)' in editor
assert 'commitEdit()' in editor and 'Control::grid, true' in editor
assert 'if (index == selected) return;' in editor  # silent edges
assert 'setLineReadingMode()' in editor and 'setEntryAccessibility()' in editor
assert 'return createIgnoredAccessibilityHandler (*this)' in editor
assert 'changeSelection (index)' in editor # mouse support
assert 'startTimerHz (8)' in editor

constructor = editor.split('LB303AudioProcessorEditor::LB303AudioProcessorEditor (', 1)[1].split('LB303AudioProcessorEditor::~', 1)[0]
assert constructor.index('addAndMakeVisible (numericEdit)') < constructor.index('setResizable (true, true)')
assert constructor.index('setResizable (true, true)') < constructor.index('setResizeLimits (')
# Loading the FX must NOT steal focus from REAPER or enter the first parameter.
assert 'SafePointer<LB303AudioProcessorEditor> (this)' not in constructor
assert 'grabKeyboardFocus()' not in constructor
assert 'setWantsKeyboardFocus (false)' in constructor
assert 'Alt+Q' in (root / 'README.md').read_text(encoding='utf-8')
assert 'constexpr int stepWidths[] { 1, 5, 10, 15, 20 };' in editor
assert 'constexpr int valuePageStep = 40;' in editor
assert 'editorStepWidthIndex' in editor
assert 'void LB303AudioProcessorEditor::changeStepWidth (int direction)' in editor
assert 'if (next == stepWidthIndex) return;' in editor
assert 'p.choiceCount > 0 ? 1.0f' in editor
assert 'if (code == juce::KeyPress::leftKey) { changeStepWidth (-1); return true; }' in editor
assert 'if (code == juce::KeyPress::rightKey) { changeStepWidth (1); return true; }' in editor
assert 'if (code == juce::KeyPress::homeKey) { setBoundary (true); return true; }' in editor
assert 'if (code == juce::KeyPress::endKey) { setBoundary (false); return true; }' in editor
assert 'if (code == juce::KeyPress::pageUpKey) { changeValue (1, valuePageStep); return true; }' in editor
assert 'if (code == juce::KeyPress::pageDownKey) { changeValue (-1, valuePageStep); return true; }' in editor
assert 'void changeValue (int direction, int speed = 1);' in header
assert 'void changeStepWidth (int direction);' in header
assert 'int stepWidthIndex = 0;' in header
print('PASS: TEST1.4 UI/step-width/focus/static regressions; no compilation performed.')
