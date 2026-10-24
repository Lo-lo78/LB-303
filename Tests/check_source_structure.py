#!/usr/bin/env python3
"""Static regressions for LB-303 TEST1.7. No local compilation."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
original = (root / 'Reference' / 'LB-303.jsfx').read_text(encoding='utf-8')
catalog = (root / 'Source' / 'ParameterCatalog.cpp').read_text(encoding='utf-8')
ids = re.findall(r'"slider(\d{3})"', catalog)
assert ids == [f'{n:03}' for n in range(1, 36)], (len(ids), ids) # stable JSFX source IDs
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
print('PASS: TEST1.5 UI/step-width/focus/static regressions; no compilation performed.')

# TEST1.5: actual value notifications must reach NVDA/OSARA independently of
# whether Grid, Value or Edit has keyboard focus. All shortcut value changes
# converge on setValue(), which must not speak on effective-value boundaries.
start = editor.index('void LB303AudioProcessorEditor::setValue (float value)')
end = editor.index('void LB303AudioProcessorEditor::changeValue (', start)
value_code = editor[start:end]
assert 'if (std::abs (legal - original) <= 1.0e-6f) return;' in value_code
assert 'param->setValueNotifyingHost (param->convertTo0to1 (legal));' in value_code
assert value_code.index('setValueNotifyingHost') < value_code.index('announceSelectedValueChanged();')
assert 'void LB303AudioProcessorEditor::announceSelectedValueChanged()' in value_code
assert 'numericEdit.hasKeyboardFocus (true)' in value_code
assert 'parameterValue.hasKeyboardFocus (true)' in value_code
assert 'parameterGrid.hasKeyboardFocus (true)' in value_code
assert 'lb303::announceToActiveScreenReader (*active, text);' in value_code
assert 'numericEdit.setText (text, juce::dontSendNotification);' in value_code
assert 'numericEdit.selectAll();' in value_code
assert 'void announceSelectedValueChanged();' in header
assert 'handler->notifyAccessibilityEvent (juce::AccessibilityEvent::valueChanged);' not in value_code
print('PASS: TEST1.5 accessible value announcements for Grid/Value/Edit and silent bounds.')

# TEST1.6: two DSP engines, separate per-bassline parameter groups and
# post-delay Level/Pan. The original JSFX has only 35 DSP parameters.
param_h = (root / 'Source' / 'ParameterCatalog.h').read_text(encoding='utf-8')
param_cpp = (root / 'Source' / 'ParameterCatalog.cpp').read_text(encoding='utf-8')
engine_h = (root / 'Source' / 'BassEngine.h').read_text(encoding='utf-8')
processor_h = (root / 'Source' / 'PluginProcessor.h').read_text(encoding='utf-8')
processor = (root / 'Source' / 'PluginProcessor.cpp').read_text(encoding='utf-8')
assert 'synthParameterCount = 35' in param_h
assert 'voiceParameterCount = synthParameterCount + 2' in param_h
assert 'basslineCount = 2' in param_h
assert 'std::array<float, synthParameterCount>' in engine_h
assert 'std::array<lb303::BassEngine, lb303::basslineCount> engines;' in processor_h
assert 'result[base + i].id = juce::String ("b2_") + originals[i].id;' in param_cpp
assert 'b1_level' in param_cpp and 'b2_level' in param_cpp
assert 'b1_pan' in param_cpp and 'b2_pan' in param_cpp
assert 'voice == 0 ? 1.0f : 0.0f' in param_cpp
assert '"Bassline " + juce::String (1 + index / voiceParameterCount)' in param_cpp
assert 'lb303::parameterIndex (voice, i)' in processor
assert 'for (auto& engine : engines) engine.midi (entry.getMessage());' not in processor
assert 'engines[voice].tick (0.0f, 0.0f)' in processor
assert 'outL = l[nextSample], outR = r[nextSample]' in processor
assert 'outL += generated.first * leftGain[voice]' in processor
assert 'outR += generated.second * rightGain[voice]' in processor
assert 'context = (context + 1) % static_cast<int> (lb303::basslineCount + 1)' in editor
assert 'Bassline 1' in editor and 'Bassline 2' in editor and 'Sequencer' in editor
assert 'Two-track sequencer' in editor
assert 'lastSelected[static_cast<size_t> (context)] = chosen;' in editor
assert 'void LB303AudioProcessorEditor::populateContext()' in editor
assert 'if (! hasBassline()) return;' in editor
assert 'focusControl (Control::grid, true);' in editor
assert 'processor.parameters, p.id, parameterValue' in editor
assert 'lb303::voiceParameterCount' in editor
assert 'LB-303-0.0.1-TEST1.7-Windows-x64' in (root / '.github/workflows/build-windows.yml').read_text()
print('PASS: TEST1.6 dual independent basslines, panning, persistent IDs and 3 contexts.')

# TEST1.7: separate MIDI channel routing for the two existing DSP engines.
# Channel numbers from JUCE are one-based, while the engines array is zero-based.
assert 'const int midiChannel = message.getChannel();' in processor
assert 'midiChannel >= 1 && midiChannel <= static_cast<int> (lb303::basslineCount)' in processor
assert 'engines[static_cast<size_t> (midiChannel - 1)].midi (message);' in processor
assert 'const auto offset = std::clamp (entry.samplePosition, 0, count);' in processor
assert processor.index('renderTo (offset);') < processor.index('const int midiChannel = message.getChannel();')
assert 'Bassline 1, channel 1' in (root / 'README.md').read_text()
assert 'Bassline 2, channel 2' in (root / 'README.md').read_text()
print('PASS: TEST1.7 MIDI ch1 -> Bassline 1, ch2 -> Bassline 2 only.')
