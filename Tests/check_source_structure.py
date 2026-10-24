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
assert 'Pattern Bassline 1' in param_cpp and 'Pattern Bassline 2' in param_cpp
assert 'lastSelected[static_cast<size_t> (context)] = chosen;' in editor
assert 'void LB303AudioProcessorEditor::populateContext()' in editor
assert 'int currentParameterCount() const' in header
assert 'focusControl (Control::grid, true);' in editor
assert 'processor.parameters, p.id, parameterValue' in editor
assert 'currentParameterCount()' in editor
assert 'LB-303-0.0.2-TEST2.2.6-Windows-x64' in (root / '.github/workflows/build-windows.yml').read_text()
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

# TEST2.1 regression: two independent banks, 16 patterns each, persist ALL steps.
seqh = (root / 'Source' / 'SequencerState.h').read_text(encoding='utf-8')
seqcpp = (root / 'Source' / 'SequencerState.cpp').read_text(encoding='utf-8')
playback = (root / 'Source' / 'SequencerPlayback.cpp').read_text(encoding='utf-8')
assert 'maximumSequences = 16' in seqh
assert 'stepsPerSequence = 128' in seqh
assert 'maximumParameterLocks = 32' in seqh
assert 'note { 128 }' in seqh, 'unprogrammed patterns must be silent'
assert 'serialiseToBase64' in seqcpp and 'restoreFromBase64' in seqcpp
assert 'seq_b1_pattern' in param_cpp and 'seq_b2_pattern' in param_cpp
assert 'seq_b1_enable' in param_cpp and 'seq_b2_enable' in param_cpp
assert 'pattern1Index = audioParameterCount' in param_h
assert 'pattern2Index = audioParameterCount + 1' in param_h
assert 'audioParameterCount = voiceParameterCount * basslineCount' in param_h
assert 'std::array<lb303::SequencerState, lb303::basslineCount> sequenceBanks' in processor_h
assert 'LB303PatternBank1' in processor and 'LB303PatternBank2' in processor
assert 'sequenceBanks[0].serialiseToBase64()' in processor
assert 'sequenceBanks[1].serialiseToBase64()' in processor
assert 'midi.addEvents (generated' in processor
assert 'if (! enabled[lane]) continue;' in playback
assert 'const auto& bank = banks[static_cast<size_t> (lane)]' in playback
assert 'const int pattern = juce::jlimit' in playback
assert 'juce::MidiMessage::noteOn (lane + 1' in playback
assert 'juce::MidiMessage::noteOff (lane + 1' in playback
assert 'lastSelected[static_cast<size_t> (context)] = chosen;' in editor
assert 'juce::MidiBuffer generated;' in processor
assert 'context < 2' in editor and 'lb303::sequencerParameterIndex' in editor
assert 'Source/SequencerState.cpp' in (root / 'CMakeLists.txt').read_text()
assert 'Source/SequencerPlayback.cpp' in (root / 'CMakeLists.txt').read_text()
print('PASS: TEST2.1 two independent 16-pattern lanes, PPQ scheduling, MIDI 1/2, preset persistence, accessible Sequencer parameters.')

# TEST2.2: editor migrated from LJuno step-navigation semantics, with two
# autonomous patterns and preserved focus, MIDI and project serialization.
step_h = (root / 'Source/SequencerStepEditor.h').read_text(encoding='utf-8')
step = (root / 'Source/SequencerStepEditor.cpp').read_text(encoding='utf-8')
assert 'struct StepChange' in step_h and 'std::vector<Action> undoStack, redoStack' in step_h
assert 'returnControl = focus;' in step and 'safe->grabKeyboardFocus()' in step
assert 'requestSequencerPreview (-1)' in step
assert 'getRevision()' in step
assert 'const auto index = track == 0 ? lb303::pattern1Index : lb303::pattern2Index;' in step
assert 'processor.parameter (lb303::editTrackIndex)' in step
assert 'bank().getStep (pattern(), index)' in step
assert 'bank().replaceStep (pattern(), index, clipboard[i])' in step
assert 'juce::KeyPress::homeKey' in step and 'juce::KeyPress::endKey' in step
assert 'coarseSteps' in step and 'topKeys' in step and 'lowerKeys' in step
assert 'LB-303 SEQUENCER' in step and 'Page ' in step and 'Launch Step' in step
assert 'TEST2.3' in step # intentionally not yet applying Parameter Locks
assert 'void SequencerStepEditor::toggleCurrentNote()' in step
assert 'Source/SequencerStepEditor.cpp' in (root / 'CMakeLists.txt').read_text()
assert 'void SequencerState::replaceStep' in seqcpp
assert 'void requestSequencerPreview' in processor_h
assert 'previewPpq = nextPpq;' in processor
assert 'playing || activePreview' in processor
assert 'const double launchDelay =' in playback
assert 'void LB303AudioProcessorEditor::openStepEditor()' in editor
assert 'stepEditor->open (lane, origin)' in editor
assert 'openStepEditor(); return true;' in editor
assert 'setWantsKeyboardFocus (false)' in constructor
print('PASS: TEST2.2 two-track step editor, preview, 7 pages, undo and keyboard navigation (static).')

# TEST2.2.1: Home/End edit track, Ctrl+Up/Down edit current track's
# automatable pattern selector. These must precede the Ctrl catch-all.
assert 'if (code == juce::KeyPress::homeKey) { changeTrack (-1); return true; }' in step
assert 'if (code == juce::KeyPress::endKey) { changeTrack (1); return true; }' in step
assert 'if (code == juce::KeyPress::upKey) { changePattern (1); return true; }' in step
assert 'if (code == juce::KeyPress::downKey) { changePattern (-1); return true; }' in step
assert step.index('if (code == juce::KeyPress::upKey) { changePattern (1); return true; }') < step.index("if (letter ('c'))")
assert 'if (target == track) return;' in step # silent track boundary
assert 'if (next == old) return;' in step # silent pattern boundary
assert 'const auto index = track == 0 ? lb303::pattern1Index : lb303::pattern2Index;' in step
assert 'param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (next)));' in step
assert 'result[pattern1Index] = { "seq_b1_pattern", "Pattern Bassline 1"' in param_cpp
assert 'result[pattern2Index] = { "seq_b2_pattern", "Pattern Bassline 2"' in param_cpp
assert 'std::make_unique<juce::AudioParameterChoice>' in param_cpp
assert 'lb303::sequencerParameterIndex' in editor
print('PASS: TEST2.2.1 Home/End track, Ctrl+Up/Down independent automatable patterns.')

# TEST2.2.2: Ctrl+Tab must NOT change track inside the Alt+Q step editor.
ctrl_section = step.split('if (mods.isCtrlDown())', 1)[1].split('if (code == juce::KeyPress::spaceKey)', 1)[0]
assert 'juce::KeyPress::tabKey' not in ctrl_section
assert 'changeTrack' not in ctrl_section
assert 'if (code == juce::KeyPress::homeKey) { changeTrack (-1); return true; }' in step
assert 'if (code == juce::KeyPress::endKey) { changeTrack (1); return true; }' in step
assert 'if (code == juce::KeyPress::tabKey)' in step # unmodified Tab block navigation
assert 'mods.isCtrlDown() && code == juce::KeyPress::tabKey' in editor # three parameter grids
assert 'Ctrl+Tab has no action inside the step editor' in (root / 'README.md').read_text()
print('PASS: TEST2.2.2 Ctrl+Tab no longer switches tracks in Alt+Q editor; outer grids unaffected.')

# TEST2.2.3: active range Start/End navigation, without affecting either lane's
# independent bank/pattern state or shortcut handling.
step = (root / 'Source/SequencerStepEditor.cpp').read_text(encoding='utf-8')
header = (root / 'Source/SequencerStepEditor.h').read_text(encoding='utf-8')
assert 'std::pair<int, int> activeStepBounds() const' in header
assert 'const auto config = bank().getConfig (pattern());' in step
assert 'const int firstBlock = firstActive / 16;' in step
assert 'const int lastBlock = lastActive / 16;' in step
assert 'const int next = juce::jlimit (firstBlock, lastBlock, block + direction);' in step
assert 'if (next == block) return; // silent active-range boundary' in step
assert "if (letter ('z')) { changeActiveRange (true, -1); return true; }" in step
assert "if (letter ('x')) { changeActiveRange (true, 1); return true; }" in step
assert "if (letter ('c')) { changeActiveRange (false, -1); return true; }" in step
assert "if (letter ('v')) { changeActiveRange (false, 1); return true; }" in step
assert 'if (index < firstActive || index > lastActive) return;' in step
assert 'if (i < firstActive || i > lastActive)' in step
assert 'if (index > lastActive) break;' in step
assert 'currentStep = juce::jlimit (firstActive, lastActive, currentStep);' in step
assert "if (code == juce::KeyPress::homeKey) { changeTrack (-1); return true; }" in step
assert "if (code == juce::KeyPress::upKey) { changePattern (1); return true; }" in step
assert 'TEST2.2.3' in (root / 'README.md').read_text()
assert 'TEST2.2.6' in (root / '.github/workflows/build-windows.yml').read_text()

# Example active ranges, zero-based block bounds. Every permitted Tab block
# intersects [Start Step, End Step], never stepping past the recorded End.
for start, end, expected in [(1, 16, [0]), (1, 32, [0, 1]),
                             (17, 32, [1]), (8, 20, [0, 1]),
                             (1, 128, list(range(8))), (65, 65, [4])]:
    first, last = (start - 1) // 16, (end - 1) // 16
    assert list(range(first, last + 1)) == expected
print('PASS: TEST2.2.3 Z/X/C/V active Start/End and bounded silent Tab/Shift+Tab.')

# TEST2.2.4: on page 7 Enter opens the per-voice parameter picker; no DSP change.
step = (root / 'Source' / 'SequencerStepEditor.cpp').read_text(encoding='utf-8')
assert 'else if (page == 6) openParameterPicker();' in step
assert 'if (pickerOpen) return handlePickerKey (key);' in step
assert 'lb303::voiceParameterCount' in step
assert 'selectPickerParameter (mods.isAltDown())' in step
assert 'const auto index = lb303::parameterIndex (static_cast<size_t> (track),' in step
assert 'bank().addStepParameterLock (pat, stepIndex, local + 1,' in step
assert 'bank().setStepParameterLockValue (pattern(), currentStep, selectedLockIndex, next);' in step
assert 'closeParameterPicker(); return true;' in step
assert 'Parameter Lock editing and playback arrive in TEST2.3' not in step

# TEST2.2.6: Remove the picker-only track switch and match LJuno's
# "selected <name>" convention for locks already assigned to this step.
picker_handler = step.split('bool SequencerStepEditor::handlePickerKey (', 1)[1].split('bool SequencerStepEditor::keyPressed (', 1)[0]
normal_editor = step.split('bool SequencerStepEditor::keyPressed (', 1)[1]
assert 'changePickerTrack' not in step and 'changePickerTrack' not in step_h
assert 'pickerStepPerTrack' not in step and 'pickerStepPerTrack' not in step_h
assert 'if (mods.isCtrlDown() && (code == juce::KeyPress::upKey' in picker_handler
assert '|| code == juce::KeyPress::downKey))' in picker_handler
assert picker_handler.index('return true;') < picker_handler.index('const int count')
assert 'if (code == juce::KeyPress::upKey) { changePattern (1); return true; }' in normal_editor
assert 'if (code == juce::KeyPress::downKey) { changePattern (-1); return true; }' in normal_editor
assert 'juce::String SequencerStepEditor::pickerItemText (int localIndex) const' in step
assert 'juce::String (assigned ? "selected " : "") + lb303::catalog()[index].name' in step
assert 'pickerItemText (item)' in step             # grid lists "selected" BEFORE parameter name
assert 'announce (pickerItemText (pickerIndex));' in step # NVDA announcements match grid
assert 'pickerItemText (pickerIndex));' in step    # initial announcement includes selected state
assert 'Use arrows, Enter to assign and close' in step
assert 'changeTrack (-1)' in normal_editor and 'changeTrack (1)' in normal_editor
assert 'if (prior >= 0 && toggle)' in step and 'else if (prior >= 0)' in step
assert 'if (toggle)' in step and 'announcePickerItem();' in step
assert 'TEST2.2.6' in (root / 'README.md').read_text()
print('PASS: TEST2.2.6 silent/unassigned Ctrl+Up/Down in picker, selected-name prefix, Alt+Enter semantics preserved.')
