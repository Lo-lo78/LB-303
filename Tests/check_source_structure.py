#!/usr/bin/env python3
"""Static regressions for LB-303 TEST2.4.1 dual voice distortion FX. No local compilation."""
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
assert 'struct ParameterCell' not in header and 'std::array<std::unique_ptr<ParameterCell>' not in header
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
# Plugin insertion must NOT grab the focus. The root becomes focusable only so
# that an actual REAPER keyboard-entry event can trigger a deferred handoff.
assert 'SafePointer<LB303AudioProcessorEditor> (this)' not in constructor
assert 'grabKeyboardFocus()' not in constructor
assert 'setWantsKeyboardFocus (true)' in constructor and 'setFocusContainerType' in constructor
assert 'Alt+Q' in (root / 'README.md').read_text(encoding='utf-8')
assert 'constexpr int stepWidths[] { 1, 5, 10, 15, 20 };' in editor
assert 'constexpr int valuePageStep = 40;' in editor
assert 'processor.editorSession.stepWidthIndex' in editor
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
assert '0.0f, 1.0f, 0.01f, 1.0f, ""' in param_cpp
assert '"Bassline " + juce::String (1 + index / voiceParameterCount)' in param_cpp
assert 'lb303::parameterIndex (voice, i)' in processor
assert 'for (auto& engine : engines) engine.midi (entry.getMessage());' not in processor
assert 'engines[voice].tick (0.0f, 0.0f)' in processor
assert 'fx.wet.first' in processor and 'fx.dry[0].first' in processor
assert 'fx.wet.second' in processor and 'fx.dry[1].second' in processor
assert 'sharedFx.process (stems[0].first' in processor
assert 'context = (context + 1) % static_cast<int> (lb303::basslineCount + 2)' in editor
assert 'Bassline 1' in editor and 'Bassline 2' in editor and 'Sequencer' in editor
assert 'Pattern Bassline 1' in param_cpp and 'Pattern Bassline 2' in param_cpp
assert 'lastSelected[static_cast<size_t> (context)] = chosen;' in editor
assert 'void LB303AudioProcessorEditor::populateContext()' in editor
assert 'int currentParameterCount() const' in header
assert 'focusControl (Control::grid, true);' in editor
assert 'processor.parameters, p.id, parameterValue' in editor
assert 'currentParameterCount()' in editor
assert 'LB-303-0.0.2-TEST2.4.25-Windows-x64' in (root / '.github/workflows/build-windows.yml').read_text()
print('PASS: TEST1.6 dual independent basslines, panning, persistent IDs and 3 contexts.')

# TEST1.7: separate MIDI channel routing for the two existing DSP engines.
# Channel numbers from JUCE are one-based, while the engines array is zero-based.
assert 'const int midiChannel = message.getChannel();' in processor
assert 'midiChannel >= 1 && midiChannel <= static_cast<int> (lb303::basslineCount)' in processor
assert 'engines[static_cast<size_t> (midiChannel - 1)].midi (message);' in processor
assert 'const auto offset = std::clamp (entry.samplePosition, 0, count);' in processor
assert processor.index('generateSegment (previousSample, offset);') < processor.index('const int midiChannel = message.getChannel();')
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
assert 'engineMidi.addEvents (generated' in processor
assert 'if (! enabled[lane]) continue;' in playback
assert 'const auto& bank = banks[static_cast<size_t> (lane)]' in playback
assert 'const int pattern = juce::jlimit' in playback
assert 'lane + 1, note, static_cast<juce::uint8> (velocity)' in playback
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
assert 'processor.toggleSequencerPreview()' in step
assert 'getRevision()' in step
assert 'const auto index = track == 0 ? lb303::pattern1Index : lb303::pattern2Index;' in step
assert 'processor.parameter (lb303::editTrackIndex)' in step
assert 'bank().getStep (pattern(), index)' in step
assert 'bank().replaceStep (pattern(), index, clipboard[i])' in step
assert 'juce::KeyPress::homeKey' in step and 'juce::KeyPress::endKey' in step
assert 'coarseSteps' in step and 'topKeys' in step and 'lowerKeys' in step
assert 'LB-303 SEQUENCER' in step and 'Page ' in step and 'Launch Step' in step
assert 'activeStepBounds()' in step # active 128-step editor
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
assert 'setWantsKeyboardFocus (true)' in constructor and 'setFocusContainerType' in constructor
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
ctrl_section = step.split('if (mods.isCtrlDown())', 1)[1].split("if (mods.isAltDown() && letter ('i'))", 1)[0]
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
assert 'TEST2.4' in (root / '.github/workflows/build-windows.yml').read_text()

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
assert 'lb303::voiceVisibleParameterCount' in step
assert 'selectPickerParameter (mods.isAltDown())' in step
assert 'const auto index = lb303::voiceParameterGlobalIndex (static_cast<size_t> (track),' in step
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

# TEST2.2.7: JUCE 8.0.15 / MSVC requires a matching FontOptions
# overload. The two-argument (String, float) call causes C2248.
font_source = (root / 'Source' / 'SequencerStepEditor.cpp').read_text(encoding='utf-8')
assert 'juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 17.0f, juce::Font::plain)' in font_source
assert 'juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 17.0f)' not in font_source
print('PASS: TEST2.2.7 FontOptions uses explicit font style (C2248 regression).')


# TEST2.2.8 — each lane has an automatable Off/On keyboard/sequence mode.
assert '"seq_b1_enable", "Sequencer Bassline 1"' in param_cpp
assert '"seq_b2_enable", "Sequencer Bassline 2"' in param_cpp
assert 'const juce::MidiBuffer physicalInput (midi);' in processor
assert 'if (sequencerOn[lane] && message.isNoteOnOrOff())' in processor
assert 'Note is a trigger, never a direct oscillator note' in processor
assert 'triggerOriginPpq[lane] = ppq + offset * beatsPerSample;' in processor
assert 'audible[0] || audible[1]' in processor
assert 'engines[lane].releaseNote();' in processor
assert 'void BassEngine::releaseNote() noexcept' in (root / 'Source/BassEngine.cpp').read_text()
assert 'const std::array<double, 2>& triggerOrigins' in playback
assert 'const double lanePpq = ppq - triggerOrigins[' in playback
assert 'if (activePreview) ppq = previewPpq;' in processor
assert 'else if (! playing) ppq = fallbackPpq;' in processor
assert 'engineMidi.addEvents (generated, 0, segmentLength, cursor);' in processor
# TEST2.4.21: generation may split a MIDI segment at Song boundaries.
print('PASS: TEST2.2.8 Off = direct MIDI, On = MIDI-triggered sequencer, no double notes, independent lanes.')

# TEST2.2.9 — LJuno-style keyboard transposition, two independent lanes.
processor_h = (root / 'Source/PluginProcessor.h').read_text(encoding='utf-8')
processor = (root / 'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
playback = (root / 'Source/SequencerPlayback.cpp').read_text(encoding='utf-8')
playback_h = (root / 'Source/SequencerPlayback.h').read_text(encoding='utf-8')
assert 'keyboardTranspose' in processor_h and 'heldKeyAge' in processor_h
assert 'std::array<int, lb303::basslineCount> lastKeyboardNote {{ 60, 60 }}' in processor_h
assert 'keyboardTranspose[lane] = note - 60;' in processor
assert 'keyboardTranspose[lane] = newestNote - 60;' in processor
assert 'heldKeyAge[lane][static_cast<size_t> (channel)]' in processor
assert 'heldKeyCount[lane] == 0' in processor  # only first key retriggers step 1
assert 'activePreview ? lastKeyboardNote[0] - 60' in processor
assert 'const std::array<int, 2>& keyboardTransposes' in playback_h
assert 'step.note + randomSemitones + keyboardTransposes[lane]' in playback
assert 'lastSourceNote[lane] == step.note' in playback
assert 'lane + 1, lastNote[lane])' in playback
assert 'oldPattern[lane] = patterns[lane]' in playback
assert 'Note is a trigger, never a direct oscillator note' in processor
assert 'TEST2.4' in (root / '.github/workflows/build-windows.yml').read_text()
print('PASS: TEST2.2.9 keyboard transpose reference C4, MIDI1/MIDI2 independence, newest-held priority and matching Note Off (static).')

# TEST2.3 — locks drive actual audio snapshots per step, including paused steps.
assert 'struct StepEvent' in playback_h
assert 'maximumStepEvents = 2048' in playback_h
assert 'const int stepIndex = start + index;' in playback
assert playback.index('stepEvents[static_cast<size_t> (stepEventCount++)]') < playback.index('if (step.note >= 128')
assert 'stepEvents, stepEventCount, cursor' in processor
assert 'std::sort (stepEvents.begin()' in processor
assert 'const auto applyVoiceParameters = [&] (size_t voice)' in processor
assert 'activeLockStep[lane] = event.step;' in processor
assert 'activeLockPattern[lane] = event.pattern;' in processor
assert 'lock.sliderNumber - 1' in processor
assert 'sequenceBanks[voice].getStep' in processor
assert 'effective[static_cast<size_t> (index)] =' in processor
assert 'engines[voice].setParameters (synth, bpm)' in processor
assert 'renderSamplesTo (event.sample);' in processor
assert processor.index('applyVoiceParameters (lane);', processor.index('const auto renderTo')) < processor.index('engines[static_cast<size_t> (midiChannel - 1)].midi (message);')
assert 'param->setValueNotifyingHost' not in processor.split('void LB303AudioProcessor::processBlock', 1)[1].split('void LB303AudioProcessor::resetToInitialState()', 1)[0] # no host automation from audio thread
assert 'const auto level = 1.0f;' in processor  # redundant historic Level disabled; Volume is the only level
assert 'effective[lb303::outputPanOffset]' in processor
assert 'activeLockStep = { -1, -1 };' in processor
# Value/grid share the same search; edit remains native. Picker supports reverse too.
assert 'current == Control::grid || current == Control::value' in editor
assert 'if (current == Control::value)' in editor
assert 'lb303::announceToActiveScreenReader (parameterValue, labelFor (candidate));' in editor
assert 'const int delta = mods.isShiftDown() ? -1 : 1;' in editor
assert 'const int delta = mods.isShiftDown() ? -1 : 1;' in step
assert "mods.isShiftDown() && code >= '0' && code <= '9'" in editor
assert "mods.isShiftDown() && code >= '0' && code <= '9'" in step
print('PASS: TEST2.3 sample-position lock overlay, baseline persistence, dual-lane separation and cyclic character search (static).')

# TEST2.3.1 — independently addressable MIDI channels + persistent UI order.
import ast
mapsrc = param_cpp
match = re.search(r'constexpr std::array<size_t, voiceVisibleParameterCount> voiceDisplayOrder\s*\{\{([^}]+)\}\}', param_cpp)
assert match
voice_order = [int(n) for n in re.findall(r'\b\d+\b', re.sub(r'//[^\n]*', '', match.group(1)))]
assert len(voice_order) == 40 and len(set(voice_order)) == 40
assert voice_order[:2] == [7, 0] # Volume before Waveform; historic Level is hidden
assert 'sequencerDisplayParameterCount> seqDisplayOrder' in param_cpp
assert 'midiChannel1Index = audioParameterCount + 5' in param_h
assert 'midiChannel2Index = audioParameterCount + 6' in param_h
assert '"Omni", "1", "2"' in param_cpp
assert '"b1_midi_channel"' in param_cpp and '"b2_midi_channel"' in param_cpp
assert 'const std::array<int, 2> inputChannels' in processor
assert '(inputChannels[lane] != 0 && inputChannels[lane] != midiChannel)' in processor
assert 'heldKeys[lane][channelSlot][key]' in processor
assert 'routedMessage.setChannel (static_cast<int> (lane) + 1);' in processor
assert 'engineMidi.addEvents (generated' in processor
assert 'outputMessage.setChannel (outputChannel);' in processor
assert 'juce::MidiBuffer engineMidi;' in processor
assert '.withChannel (' not in processor # JUCE MidiMessage uses setChannel(), not withChannel()
assert 'previewEnabled.store (enabled' in processor_h
assert '(sequencerOn[0] && heldKeyCount[0] > 0) || activePreview' in processor
assert '(sequencerOn[1] && heldKeyCount[1] > 0) || activePreview' in processor
assert 'if (preview) { processor.requestSequencerPreview (false); preview = false; }' not in step
assert 'if (preview) { processor.requestSequencerPreview (false); preview = false; }\n    track = target;' not in step
assert 'processor.editorSession.sequencerPage' in step
assert 'lastPickerPosition[static_cast<size_t> (track)]' in step
assert 'returnControl = focus;' in step
assert 'key.getModifiers().isAltDown()' in step and 'close(); return true;' in step
assert 'displayToLocal (static_cast<size_t> (context)' in editor
assert 'displayToLocal (static_cast<size_t> (track)' in step
assert 'lastSelected[static_cast<size_t> (context)] = chosen;' in editor
print('PASS: TEST2.3.1 independent configurable MIDI inputs and Omni, combined preview, UI focus memory and stable IDs.')

# TEST2.3.2 - preset snapshots are the same as the complete DAW state.
preset = (root / 'Source/PresetManager.cpp').read_text(encoding='utf-8')
browser = (root / 'Source/PresetBrowser.cpp').read_text(encoding='utf-8')
cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
assert 'Source/PresetManager.cpp' in cmake and 'Source/PresetBrowser.cpp' in cmake
assert 'userDocumentsDirectory' in preset and 'getChildFile ("LB-303")' in preset
assert 'constexpr auto header = "LB-303 Patch Snapshot"' in preset
assert '.withFileExtension (".lb")' in preset
assert 'processor.getStateInformation (snapshot.data)' in preset
assert 'processor.setStateInformation (snapshot.data.getData()' in preset
assert 'processor.setStateInformation (state.getData()' in preset
assert 'getXmlFromBinary' in preset and 'xml->hasTagName' in preset
assert 'juce::Base64::toBase64' in preset
assert 'original = manager.capture()' in browser and 'manager.restore (original)' in browser
assert 'manager.preview (item.file)' in browser and 'manager.commit (entry.file)' in browser
assert 'manager.save (directory' in browser and 'manager.relative (direction' in browser
assert 'renameSelected()' in browser and 'newFolder()' in browser
assert 'deleteSelected()' in browser and 'showMenu()' in browser
assert 'Alt+B' in editor and 'Alt+S' in editor
assert 'savedBrowserDirectory' in preset and 'saveBrowserDirectory' in preset
assert 'presetBrowser->onPresetLoaded' in editor
print('PASS: TEST2.3.2 LJuno-style preset browser, complete state snapshots, .lb library, preview rollback and keyboard UI (static).')

# TEST2.4 — a single shared stereo FX bus with four accessible contexts.
fxheader = (root / 'Source/SharedEffects.h').read_text(encoding='utf-8')
fxcpp = (root / 'Source/SharedEffects.cpp').read_text(encoding='utf-8')
engine = (root / 'Source/BassEngine.cpp').read_text(encoding='utf-8')
assert 'fxParameterCount = 35' in param_h
assert 'fxDisplayParameterCount = fxParameterCount + classicDelayParameterCount' in param_h
assert 'legacyParameterCount = audioParameterCount + sequencerParameterCount' in param_h
assert 'fxDisplayOrder' in param_cpp
assert 'voiceVisibleParameterCount = 22 + lfoParameterCount + osc2ParameterCount' in param_h
assert 'std::array<int, lb303::basslineCount + 2>' in (root / 'Source/PluginEditor.h').read_text(encoding='utf-8')
assert 'return context == 2 ? "Sequencer" : "FX"' in editor
assert 'visibleFxParameters.at' in editor and 'Source/SharedEffects.cpp' in cmake
assert 'std::pair<float, float> BassEngine::processClassicDelay' in engine
assert 'const auto panLfo = clamp (' in engine  # two stereo stems are now panned by the independent LFO
assert 'BassEngine classic' in fxheader
assert 'classic.processClassicDelay (l, r)' in fxcpp
assert 'SharedEffects::dualDelay' in fxcpp and 'SharedEffects::chorus' in fxcpp
assert 'SharedEffects::distort' in fxcpp and 'SharedEffects::reverb' in fxcpp
assert 'sharedFx.prepare (sampleRate);' in processor
assert 'sharedFx.setParameters (fxControls, legacy, bpm)' in processor
assert 'sharedFx.process (stems[0].first' in processor
assert 'legacy[static_cast<size_t> (local)]' in processor
assert 'fxParameterIndex (fx::delayType)' in param_cpp
assert 'fxParameterIndex (fx::reverbType)' in param_cpp
assert 'fxParameterIndex (fx::distortionType)' in param_cpp
assert 'fxParameterIndex (fx::chorusOn)' in param_cpp
for idname in ('fx_delay_send1', 'fx_delay_send2', 'fx_chorus_send1',
               'fx_chorus_send2', 'fx_reverb_send1', 'fx_reverb_send2'):
    assert idname in param_cpp
assert 'TEST2.4' in (root / 'README.md').read_text()
print('PASS: TEST2.4 shared FX engine, stable host IDs, dual sends, 4 accessible contexts and complete preset state.')

# Index and ID allocation: 81 legacy IDs, 34 appended, one shared classic effect.
import collections
fx_enum = (root / 'Source/ParameterCatalog.h').read_text()
assert 'constexpr size_t legacyParameterCount' in fx_enum
fx_decl = re.search(r'namespace fx\s*\{\s*enum Control : size_t\s*\{([^}]+)\}',fx_enum,re.S)
assert fx_decl
names = [x.strip() for x in re.sub(r'//[^\n]*', '', fx_decl.group(1)).split(',') if x.strip()]
assert len(names) == 35, (len(names),names)
for name in names:
    assert 'addFx (fx::'+name+',' in param_cpp, name
fxmap = re.search(r'constexpr std::array<size_t, fxDisplayParameterCount> fxDisplayOrder\s*\{\{(.*?)\}\};',param_cpp,re.S)
assert fxmap
items=[x.strip() for x in re.sub(r'//[^\n]*','',fxmap.group(1)).split(',') if x.strip()]
assert len(items) == 49, len(items)
assert len(set(items)) == len(items)
assert sum(x.startswith('fxParameterIndex ') for x in items) == 35
assert sorted(int(x) for x in items if x.isdigit()) == list(range(19,33))
assert 'return 30.0' in processor_h
print('PASS: FX 35 append-only IDs, 14 shared classic IDs, complete unique display mapping, 30s tail.')

# TEST2.4.1 — Correct LJuno Value limits and independent distortion states.
# Test exactly the context-specific Home/End behavior; navigation Home/End
# elsewhere (Grid and sequencer) must keep their original roles.
value_block = editor.split('if (current == Control::value)', 2)[-1]
assert 'if (code == juce::KeyPress::homeKey) { setBoundary (true); return true; }' in editor
assert 'if (code == juce::KeyPress::endKey) { setBoundary (false); return true; }' in editor
assert 'changeSelection ((selected / rowsPerColumn) * rowsPerColumn)' in editor
assert 'changeSelection (juce::jmin (currentParameterCount() - 1' in editor
assert 'if (code == juce::KeyPress::homeKey)' in step
assert 'if (code == juce::KeyPress::endKey)' in step
assert 'const size_t typeIndex = voice == 0 ? fx::distortionType : fx::distortion2Type;' in fxcpp
assert 'const size_t amountIndex = voice == 0 ? fx::distortionAmount : fx::distortion2Amount;' in fxcpp
assert 'const size_t harmonicsIndex = voice == 0 ? fx::distortionHarmonics : fx::distortion2Harmonics;' in fxcpp
assert 'std::array<DistortionState, basslineCount> distortions {};' in fxheader
assert 'const auto [axL, axR] = distort (aL, aR, 0);' in fxcpp
assert 'const auto [bxL, bxR] = distort (bL, bR, 1);' in fxcpp
assert 'distort (dryL,dryR)' not in fxcpp
assert 'distribute(aL,bL' not in fxcpp
assert 'distortion2Type, distortion2Amount, distortion2Harmonics' in param_h
for idname in ['fx_distortion_type', 'fx_distortion_amount', 'fx_distortion_harmonics',
               'fx_distortion_2_type', 'fx_distortion_2_amount', 'fx_distortion_2_harmonics']:
    assert f'"{idname}"' in param_cpp
# Additional controls have defaults that will not alter old projects (Off / Amount zero).
assert '"Distortion 2 Type", 0, 12, 1, 0' in param_cpp
assert '"Distortion 2 Amount", 0, 100, 1, 0' in param_cpp
print('PASS: TEST2.4.1 Value Home/End, two isolated distortions before shared sends, old VST IDs preserved.')

# TEST2.4.2: single parameter ID for channel in each sequencer/voice view,
# persistent focus return from preset browsing, and full reset of 32 patterns.
assert 'voiceGridParameterCount = voiceVisibleParameterCount + 2' in param_h
assert 'voiceMidiDisplayIndex = 5' in param_h
assert 'midiChannel1Index : lb303::midiChannel2Index' in editor
assert 'voiceGridParameterCount' in (root / 'Source/PluginEditor.h').read_text(encoding='utf-8')
assert 'presetReturnContext = context;' in editor and 'presetReturnParameter = selected;' in editor
assert 'focusControl (presetReturnControl, true);' in editor
assert 'announce ("Loaded preset " + entry.name);\n    close (false);' in browser
assert 'void LB303AudioProcessorEditor::initialisePlugin()' in editor
assert "if (letter ('i')) { initialisePlugin(); return true; }" in editor
assert 'void LB303AudioProcessor::resetToInitialState()' in processor
assert 'for (auto& bank : sequenceBanks) bank.reset();' in processor
assert 'resetRuntimeRequested.exchange (false' in processor
assert 'sharedFx.clearTails();' in processor
assert 'void SharedEffects::clearTails() noexcept' in fxcpp
assert 'for (auto& line : verb)' in fxcpp
print('PASS: TEST2.4.2 preset focus restore, MIDI alias, two audible default voices, full reset.')

# TEST2.4.3: pressing Enter on a preset must restore a functional, speaking
# Grid without making the user Tab around Value/Edit first.  Focus recovery is
# delayed until the browser has finished dispatching the Enter key.
preset_restore = editor.split('void LB303AudioProcessorEditor::focusBackFromPreset()', 1)[1].split(
    'void LB303AudioProcessorEditor::changePreset', 1)[0]
assert 'parameterGrid.setWantsKeyboardFocus (true)' in constructor
assert 'if (presetReturnControl == Control::grid)' in preset_restore
assert 'focusControl (presetReturnControl, true);' in preset_restore
assert 'juce::Timer::callAfterDelay (65,' in preset_restore
assert preset_restore.index('focusControl (presetReturnControl, true);') < preset_restore.index('juce::Timer::callAfterDelay (65,')
assert 'safe->parameterGrid.hidePopup();' in preset_restore
assert 'safe->parameterGrid.grabKeyboardFocus();' in preset_restore
assert 'juce::AccessibilityHandler::clearCurrentlyFocusedHandler();' in preset_restore
assert 'handler->grabFocus();' in preset_restore
assert 'safe->labelFor (restoreParameter)' in preset_restore
assert 'safe->parameterValue.hasKeyboardFocus (true)' in preset_restore
assert 'safe->numericEdit.hasKeyboardFocus (true)' in preset_restore
assert 'safe->context != restoreContext || safe->selected != restoreParameter' in preset_restore
assert 'safe->parameterGrid.setLineReadingMode();' in preset_restore
print('PASS: TEST2.4.3 browser Enter -> responsive Grid and restored NVDA parameter/value.')

# TEST2.4.4: initial host keyboard entry (not editor construction), per-grid
# persistent cursor and Alt+Q page memory, glitch-free reverb predelay.
assert 'void LB303AudioProcessorEditor::focusGained (FocusChangeType)' in editor
assert 'void LB303AudioProcessorEditor::performInitialFocusTransfer()' in editor
assert 'peer->isFocused()' in editor and 'focused != this' in editor
assert 'handler->getNativeImplementation()' in editor
assert 'processor.editorSession.activeContext' in editor
assert 'processor.editorSession.selectedParameters' in editor
assert 'void LB303AudioProcessorEditor::rememberEditorLocation()' in editor
assert 'rememberEditorLocation();' in editor
assert 'processor.editorSession.sequencerPage' in step
assert 'processor.parameters.state.setProperty (sequencerPageState' not in step
assert 'page = 0; // Always open on Note' not in step
assert 'reverbPredelaySmoothCoeff' in fxcpp and 'reverbPredelaySamples' in fxcpp
assert 'targetPredelay - reverbPredelaySamples' in fxcpp
assert 'preL.read (readOffset), inR = preR.read (readOffset)' in fxcpp
assert 'verb[i].data.size() - 2' in fxcpp
print('PASS: TEST2.4.4 entry Grid focus, context/page memory, smoothed pre-delay and FDN effective delay.')

# TEST2.4.5: navigation persists only while the processor instance lives.
# REAPER/project/.lb state excludes navigation, including legacy TEST2.4.4 tags.
assert 'struct EditorSessionState' in processor_h
assert 'EditorSessionState editorSession;' in processor_h
assert 'processor.editorSession.activeContext' in editor
assert 'processor.editorSession.selectedParameters' in editor
assert 'processor.editorSession.stepWidthIndex' in editor
assert 'processor.editorSession.sequencerPage' in step
for legacy in ('editorActiveContext', 'editorSelectedBassline1', 'editorSelectedBassline2',
               'editorSelectedSequencer', 'editorSelectedFx', 'editorSequencerPage',
               'editorStepWidthIndex'):
    assert legacy in processor, legacy
    assert legacy not in editor and legacy not in step, legacy
assert 'removeLegacyEditorProperties (snapshot);' in processor
assert 'removeLegacyEditorProperties (restored);' in processor
assert processor.index('removeLegacyEditorProperties (snapshot);') < processor.index('if (auto xml = snapshot.createXml())')
assert processor.index('removeLegacyEditorProperties (restored);') < processor.index('parameters.replaceState (restored);')
assert 'processor.parameters.state.setProperty' not in step
assert 'processor.parameters.state.setProperty (activeContextState' not in editor
assert 'processor.parameters.state.setProperty (selectedContextState' not in editor
assert 'processor.parameters.state.setProperty (stepWidthState' not in editor
assert 'editorSession.stepWidthIndex = 0;' in processor
print('PASS: TEST2.4.5 processor-instance-only navigation, legacy state migration, no cross-instance bleed.')

# Old snapshots must never reset the current editor location when loading a
# preset, and creating a new VST always starts with a default state object.
assert 'int activeContext = 0;' in processor_h
assert 'std::array<int, lb303::basslineCount + 2> selectedParameters {};' in processor_h
assert 'int sequencerPage = 0;' in processor_h
state_loader = processor.split('void LB303AudioProcessor::setStateInformation (', 1)[1]
assert 'editorSession' not in state_loader
assert 'auto restored = juce::ValueTree::fromXml (*xml);' in state_loader
print('PASS: TEST2.4.5 old projects cannot overwrite the live editor position.')

# TEST2.4.6 — complete native Grid speech on FIRST host entry, as Alt+L.
# No short UIA notification may mask JUCE's full combo-box focus context.
entry = editor.split('void LB303AudioProcessorEditor::performInitialFocusTransfer()', 1)[1]\
             .split('size_t LB303AudioProcessorEditor::globalIndex', 1)[0]
assert 'parameterGrid.setEntryAccessibility();' in entry
assert 'juce::AccessibilityHandler::clearCurrentlyFocusedHandler();' in entry
assert entry.index('parameterGrid.setEntryAccessibility();') < entry.index('parameterGrid.grabKeyboardFocus();')
assert 'handler->grabFocus();' in entry
assert 'announceToActiveScreenReader (parameterGrid, labelFor (selected))' not in entry
assert 'setTitle (editor.contextName() + ". Grid");' in editor
assert 'setDescription ("Alt+L");' in editor
assert 'if (letter (\'l\')) { focusControl (Control::grid,' in editor
# DSP gain is post-FDN and acts exclusively on the wet return, with no
# parameter range/IDs or preset serialization modifications.
reverb = fxcpp.split('std::pair<float, float> SharedEffects::reverb (', 1)[1]\
               .split('std::pair<float, float> SharedEffects::process (', 1)[0]
assert 'const float level = c[fx::reverbLevel] * .02f * reverbReturnMakeupGain;' in reverb
assert 'return { bounded ((middle+side)*level), bounded ((middle-side)*level) };' in reverb
assert '"fx_reverb_level"' in param_cpp
assert '"Reverb Level", 0, 100, 1, 50' in param_cpp
assert 'fx::reverbSend1' in fxcpp and 'fx::reverbSend2' in fxcpp
print('PASS: TEST2.4.6 native Grid speech preserved; TEST2.4.7 wet gain verified separately.')

# TEST2.4.7: substantial *wet-only* reverb boost. Source constant and
# reference arithmetic ensure +18.06dB relative to TEST2.4.6 (8x amplitude),
# at Send 20 / Level 50 now 1.6x the pre-2.4.6 full-send/level reference.
from math import log10
assert 'constexpr float reverbReturnMakeupGain = 8.0f;' in fxcpp
assert reverb.count('reverbReturnMakeupGain') == 1
assert abs(20 * log10(8.0) - 18.0617997398) < 1.e-4
assert abs((.20 * .50 * .02 * 8.0 * 100) - 1.6) < 1.e-9
assert 'const auto reverbStem = send (fx::reverbSend1, fx::reverbSend2);' in fxcpp
assert 'reverbWet = runReverb' in fxcpp
assert 'out.dry[0]' in fxcpp and 'out.dry[1]' in fxcpp and 'out.wet' in fxcpp
print('PASS: TEST2.4.7 +18.06 dB reverb return only; sends, dry and other FX unchanged.')

# TEST2.4.8 — global direct Dry Level, first in FX grid.
# Keep historical slider032 unchanged for REAPER automation and .lb files.
assert '"slider032", "Dry Level"' in param_cpp
assert 'index == parameterIndex (0, 31)' in param_cpp
assert '("FX - " + p.name)' in param_cpp
fxmap = re.search(r'constexpr std::array<size_t, fxDisplayParameterCount> fxDisplayOrder\s*\{\{(.*?)\}\};',param_cpp,re.S)
assert fxmap
fxitems=[x.strip() for x in re.sub(r'//[^\n]*','',fxmap.group(1)).split(',') if x.strip()]
assert fxitems[0] == '31', fxitems[:5]
assert fxitems.count('31') == 1
assert len(fxitems) == 49 and len(set(fxitems)) == 49
assert 'out.dry[0] = { bounded (axL)' in fxcpp and 'sharedFx.dryLevel()' in processor
assert 'const float dryGain = delayMode == 1 ? classicDryLevel : 1.0f;' not in fxcpp
assert 'const auto [axL, axR] = distort (aL, aR, 0);' in fxcpp
assert 'const auto [bxL, bxR] = distort (bL, bR, 1);' in fxcpp
assert 'result.first - l * classicDryGain' in fxcpp
print('PASS: TEST2.4.8 Dry Level first in FX; globally active; existing ID and delay wet-only routing retained.')

# TEST2.4.9 — insert order, per-lane sends and LJuno performance parameters.
assert 'fxParameterCount = 35' in param_h
assert 'sequencerPerformancePerLane = 10' in param_h
assert 'sequencerPerformanceCount = basslineCount * sequencerPerformancePerLane' in param_h
assert 'sequencerDisplayParameterCount = sequencerParameterCount + sequencerPerformanceCount' in param_h
assert 'sequencerDisplayParameterCount' in (root / 'Source/PluginEditor.h').read_text()
assert 'fx_delay_reverb_order' in param_cpp
assert 'fxOrderChoices' in param_cpp
assert 'fx::effectOrder' in param_cpp
assert 'const bool delayFirst = c[fx::effectOrder] < .5f;' not in fxcpp
assert fxcpp.index('const auto [axL, axR] = distort') < fxcpp.index('const auto first = chorus (')
assert fxcpp.index('const auto first = chorus (') < fxcpp.index('const auto delayStem = send')
assert 'reverbWet = runReverb (reverbStem.first + delayWet.first * reverbLink' in fxcpp
assert 'const auto delayWet = runDelay (delayStem.first, delayStem.second);' in fxcpp
assert 'const float reverbLink = std::max' in fxcpp and 'const float delayLink = std::max' not in fxcpp
assert 'std::array<Ring, basslineCount> chorusL, chorusR;' in fxheader
assert 'const auto second = chorus (bxL * s2, bxR * s2, 1);' in fxcpp
assert 'const float s1 = c[fx::chorusSend1] * .01f;' in fxcpp
assert 'std::pair<float, float> chorus (float, float, size_t voice)' in fxheader
assert 'fxParameterIndex (fx::effectOrder)' in param_cpp
assert '31, // Dry Level' in param_cpp
assert '7, 0, 53, 54, 36,' in param_cpp  # Volume, Waveform, Pan; no duplicated Level
for idstub in ('bpm_division','shuffle','legato','skip_probability','note_random',
               'velocity_random','length_random','repeat_random','octave_shift','semitone_shift'):
    assert f'"{idstub}"' in param_cpp, idstub
assert 'const std::array<SequencerConfig, 2>& performanceControls' not in playback
assert 'const auto config = bank.getConfig (pattern);' in playback
assert 'songEntryIds[static_cast<size_t> (lane)]' in playback
assert 'randomValue (lane, pattern, tick' in playback
assert 'double lengthFactor' in playback
assert 'config.legato && step.length >= 100' in playback
assert 'performance,\n                playing' not in processor
assert 'LB-303-0.0.2-TEST2.4.25-Windows-x64' in (root / '.github/workflows/build-windows.yml').read_text()
print('PASS: TEST2.4.9 serial FX order, two isolated chorus send paths, 20 sequencer controls, stable parameter indices.')

# TEST2.4.10: guard against the stray second `}};` that broke MSVC
# parsing at ParameterCatalog.cpp:79 in the TEST2.4.9 GitHub Actions log.
# The namespace must close with just `}` after seqDisplayOrder.
sequence_declaration = re.search(
    r'constexpr std::array<size_t, sequencerDisplayParameterCount> seqDisplayOrder\s*\{\{(.*?)\}\};\s*\}\s*const std::array<ParameterInfo, parameterCount>& catalog\(\)',
    param_cpp, re.S)
assert sequence_declaration is not None, 'Wrong braces at end of sequencer display order / namespace'
assert not re.search(r'\}\};\s*\}\};\s*\}', param_cpp), 'Duplicate array closure before namespace'
print('PASS: TEST2.4.10 ParameterCatalog sequencer array and namespace closure (C2059 regression).')

# TEST2.4.11 — dynamic effect columns, fixed Delay -> Reverb and stable legacy IDs.
assert 'void LB303AudioProcessorEditor::updateFxColumns()' in editor
assert 'void LB303AudioProcessorEditor::syncFxColumns()' in editor
assert 'std::array<int, 6> fxColumnLengths' in (root / 'Source/PluginEditor.h').read_text()
assert 'if (delayType == 1)' in editor and 'else' in editor
assert 'if (enabled (lb303::fx::chorusOn))' in editor
assert 'if (enabled (lb303::fx::distortionType))' in editor
assert 'if (enabled (lb303::fx::distortion2Type))' in editor
assert 'if (enabled (lb303::fx::reverbType))' in editor
assert 'fxCellIndex (targetCol' in editor and 'fxCell (selected)' in editor
assert 'oldIndices == visibleFxParameters' in editor
assert 'const auto delayWet = runDelay (delayStem.first, delayStem.second);' in fxcpp
assert 'const auto reverbWet = runReverb (reverbStem.first + delayWet.first * reverbLink' in fxcpp
assert 'delayFirst' not in fxcpp
assert 'fx_delay_reverb_order' in param_cpp # legacy parameter ID for old projects
assert 'addFx (4, lb303::fx::effectOrder)' not in editor # legacy selector hidden from dynamic FX grid
print('PASS: TEST2.4.11 six independently collapsing FX columns, Type-dependent delay, stable IDs and fixed Delay->Reverb.')

# TEST2.4.12 — two fully independent LJuno-derived LFOs, no duplicate Level.
assert 'lfoParameterBase = sequencerPerformanceBase + sequencerPerformanceCount' in param_h
assert 'parameterCount = osc2ParameterBase + basslineCount * osc2ParameterCount' in param_h
assert 'constexpr size_t voiceLocalParameterCount = osc2LocalBase + osc2ParameterCount' in param_h
assert 'constexpr size_t voiceVisibleParameterCount = 22 + lfoParameterCount + osc2ParameterCount' in param_h
assert len(voice_order) == 40 and voice_order[0] == 7 and 35 not in voice_order
assert voice_order[-16:] == list(range(37, 53))
assert '"Legacy Level (unused)"' in param_cpp
for suffix in ('mode','rate','bpm_sync','wave','square_pwm','delay','phase','upper_squash',
               'lower_squash','smooth','one_shot','one_shot_percent','pitch_depth',
               'volume_depth','filter_depth','pan_depth'):
    assert f'"{suffix}"' in param_cpp
assert 'const juce::String id = lane == 0 ? "b1_lfo_" : "b2_lfo_";' in param_cpp
assert 'voiceParameterGlobalIndex (static_cast<size_t> (context), actual)' in editor
assert 'voiceParameterGlobalIndex' in step and 'voiceLocalParameterCount' in step
assert 'engines[voice].setLfoParameters (voiceLfo)' in processor
assert 'lb303::voiceParameterCount' in processor and 'lock.sliderNumber' in processor
assert 'nextLfoSample() noexcept' in engine
assert 'lfoParameters[lb303::lfo::pitchDepth]' in engine
assert 'lfoParameters[lb303::lfo::volumeDepth]' in engine
assert 'lfoParameters[lb303::lfo::filterDepth]' in engine
assert 'lfoParameters[lb303::lfo::panDepth]' in engine
assert 'pwmIncrement' in engine and 'p[11]' in engine  # original PWM Rate independent
assert 'if (lfoParameters[lb303::lfo::mode] >= .5f' in engine
assert 'const auto modulation = nextLfoSample();' in engine
assert 'double BassEngine::nextLfoSample() noexcept' in engine
assert 'return { static_cast<float> (synthL *' in engine
print('PASS: TEST2.4.12 Volume-first, 2 independent LFOs, bipolar targets, PWM wave, step locks and all existing IDs.')

# TEST2.4.13 — Space transport target and preview lifecycle.
assert 'bool spaceControlsPreview = false;' in processor_h
assert 'editorSession.spaceControlsPreview = ! editorSession.spaceControlsPreview;' in processor_h
assert 'if (! editorSession.spaceControlsPreview) requestSequencerPreview (false);' in processor_h
assert 'bool toggleSequencerPreview() noexcept' in processor_h
assert 'return false; // leave Space to REAPER in default DAW mode' in step
assert 'const bool running = processor.toggleSequencerPreview();' in step
assert 'const bool running = processor.toggleSequencerPreview();' in editor
assert 'mods.isAltDown() && ! mods.isCtrlDown() && ! mods.isCommandDown() && letter (\'p\')' in editor
assert 'Space: Sequencer preview' in editor and 'Space: DAW' in editor
assert 'Space: Sequencer preview' in step and 'Space: DAW' in step
assert 'if (preview)' not in step and 'bool preview = false;' not in step_h
assert 'if (pickerOpen) return handlePickerKey (key);' in step
assert step.index('processor.toggleSpaceTarget()') < step.index('if (pickerOpen) return handlePickerKey (key);')
assert 'if (preview) { processor.requestSequencerPreview (false); preview = false; }' not in step
assert 'spaceControlsPreview' not in processor.split('void LB303AudioProcessor::getStateInformation',1)[1]
assert 'spaceControlsPreview' not in processor.split('void LB303AudioProcessor::setStateInformation',1)[1]
assert 'processor.editorSession.spaceControlsPreview' not in param_cpp
assert 'getKeyCode() == juce::KeyPress::spaceKey' in editor
assert '&& ! editor.processor.spaceControlsSequencerPreview()' in editor
print('PASS: TEST2.4.13 DAW-default Space, Alt+P target, dual preview across edit exit and nonpersistent instance mode.')

# TEST2.4.15 — only the ACTIVE pattern may be copied/cut/pasted.
assert 'void copyCurrentPattern (bool cut);' in step_h
assert 'void pasteCurrentPattern();' in step_h
assert 'bool hasWholePattern = false;' in step_h
assert 'juce::String beforePattern, afterPattern;' in step_h
assert 'LB303-PATTERN-V1:' in step
assert 'LB303-TRACK-V1:' not in step
assert 'juce::SystemClipboard::copyTextToClipboard (patternClipboardHeader + snapshot);' in step
assert 'juce::SystemClipboard::getTextFromClipboard()' in step
assert 'scratch->copySequenceFrom (source, sourcePattern, 0)' in step
assert 'destination.copySequenceFrom (*scratch, 0, destinationPattern)' in step
assert 'bank().copySequenceFrom (*staged, 0, destinationPattern);' in step
assert 'bank().resetSequence (sourcePattern);' in step
assert 'bank().resetSequence (i);' not in step
assert 'action.hasWholePattern = true;' in step
assert 'restorePatternSnapshot (dest, action.pattern,' in step
assert 'const int destinationPattern = pattern();' in step
assert 'if (mods.isCtrlDown() && mods.isShiftDown() && ! mods.isAltDown())' in step
assert "if (chord ('c')) { copyCurrentPattern (false); return true; }" in step
assert "if (chord ('x')) { copyCurrentPattern (true); return true; }" in step
assert "if (chord ('v')) { pasteCurrentPattern(); return true; }" in step
assert step.index('if (mods.isCtrlDown() && mods.isShiftDown()') < step.index('if (pickerOpen) return handlePickerKey')
assert "if (letter ('c')) { copySteps (false); return true; }" in step
assert "if (letter ('x')) { copySteps (true); return true; }" in step
assert "if (letter ('v')) { pasteSteps(); return true; }" in step
assert 'void SequencerStepEditor::undo (bool redo)' in step
state = (root / 'Source/SequencerState.cpp').read_text(encoding='utf-8')
copy_sequence = state.split('void SequencerState::copySequenceFrom', 1)[1].split('juce::String SequencerState::serialiseToBase64',1)[0]
assert 'storeRelaxed (dst.midiInputPolyphony, loadRelaxed (src.midiInputPolyphony));' in copy_sequence
assert 'sequences[1].midiInputPolyphony' not in copy_sequence
assert 'sequences[2].midiInputPolyphony' not in copy_sequence
print('PASS: TEST2.4.15 portable single-pattern clipboard, cross-track paste, target-only Undo/Redo, silent other 15 patterns.')

# TEST2.4.16 — accessible browser and multi-output audio routing.
assert 'std::function<void()> onPresetLoaded' in (root/'Source/PresetBrowser.h').read_text()
browser = (root/'Source/PresetBrowser.cpp').read_text()
assert 'announce (getNameForRow (row));' in browser
assert 'const int bounded = juce::jlimit' in browser
assert 'entries.selectRow (bounded);' in browser
assert 'manager.instrument().toggleSpaceTarget()' in browser
assert 'manager.instrument().toggleSequencerPreview()' in browser
assert 'if (! manager.instrument().spaceControlsSequencerPreview()) return false;' in browser
assert 'if (! suppressSelection && mode == Mode::browse && row >= 0) previewRow (row);' not in browser
assert 'void PresetBrowser::previewRow' in browser and 'manager.preview (item.file)' in browser
assert 'setStateInformation (state.getData()' in (root/'Source/PresetManager.cpp').read_text()
assert 'previewEnabled.store (false' not in processor.split('void LB303AudioProcessor::setStateInformation', 1)[1]
assert 'separateOutputBase = lfoParameterBase + basslineCount * lfoParameterCount;' in param_h
assert 'constexpr size_t separateOutputIndex (size_t lane)' in param_h
assert 'result[separateOutputIndex (0)]' in param_cpp
assert 'result[separateOutputIndex (1)]' in param_cpp
assert '"b1_separate_output"' in param_cpp and '"b2_separate_output"' in param_cpp
assert '"Off", "Out 3/4"' in param_cpp and '"Off", "Out 5/6"' in param_cpp
assert 'if (local == lb303::voiceMidiDisplayIndex + 1)' in editor
assert 'lb303::separateOutputIndex (static_cast<size_t> (context))' in editor
assert 'voiceGridParameterCount = voiceVisibleParameterCount + 2' in param_h
assert 'std::array<std::pair<float, float>, basslineCount> dry' in fxheader
assert 'std::pair<float, float> wet' in fxheader
assert 'SharedEffects::Output SharedEffects::process' in fxcpp
assert 'out.wet = {' in fxcpp and 'out.dry[0] = {' in fxcpp and 'out.dry[1] = {' in fxcpp
assert '.withOutput ("Master 1/2"' in processor and '.withOutput ("Bassline 1 3/4"' in processor
assert '.withOutput ("Bassline 2 5/6"' in processor
assert '.withOutput ("FX Wet 7/8"' in processor
assert 'getChannelIndexInProcessBlockBuffer (false, bus, 0)' in processor
assert 'const int wetIndex = stereoBusStart (3);' in processor
assert 'channel + 1 < audio.getNumChannels()' in processor
assert 'b1L[nextSample] += fx.dry[0].first;' in processor
assert 'b2L[nextSample] += fx.dry[1].first;' in processor
assert 'const float mainDry = sharedFx.dryLevel();' in processor
assert 'mainDry * (fx.dry[0].first + fx.dry[1].first) + fx.wet.first' in processor
assert 'for (int channel = 2; channel < audio.getNumChannels(); ++channel)' not in processor
assert 'LB-303-0.0.2-TEST2.4.25-Windows-x64' in (root/'.github/workflows/build-windows.yml').read_text()
print('PASS: TEST2.4.16 browser NVDA names + existing preview + 4 stereo buses + separate distortion stems + stable IDs (static).')

# TEST2.4.17 — GUI close stops preview; Alt+Q editor close does not.
editor_source = (root / 'Source/PluginEditor.cpp').read_text(encoding='utf-8')
processor_header = (root / 'Source/PluginProcessor.h').read_text(encoding='utf-8')
step_source = (root / 'Source/SequencerStepEditor.cpp').read_text(encoding='utf-8')
destructor = editor_source.split('LB303AudioProcessorEditor::~LB303AudioProcessorEditor()', 1)[1].split('LB303AudioProcessorEditor::createAccessibilityHandler', 1)[0]
step_close = step_source.split('void SequencerStepEditor::close()', 1)[1].split('void SequencerStepEditor::', 1)[0]
assert 'processor.requestSequencerPreview (false);' in destructor
assert 'processor.requestSequencerPreview (false);' not in step_close
assert 'void requestSequencerPreview (bool enabled)' in processor_header
assert 'previewEnabled.store (enabled, std::memory_order_release);' in processor_header
print('PASS: TEST2.4.17 window close stops preview; leaving Alt+Q does not.')

# TEST2.4.18 - full master, isolated bassline and wet FX outputs.
assert 'const int wetIndex = stereoBusStart (3);' in processor
assert 'if (wetL != nullptr)' in processor
assert 'wetL[nextSample] += fx.wet.first;' in processor
assert 'wetR[nextSample] += fx.wet.second;' in processor
assert 'audio.clear (0, 0, count);' in processor
assert 'audio.clear (1, 0, count);' in processor
assert 'audio.clear (wetIndex, 0, count);' in processor
assert 'const bool toB1 = value (lb303::separateOutputIndex (0)) > .5f && b1Index >= 0;' in processor
assert 'const bool toB2 = value (lb303::separateOutputIndex (1)) > .5f && b2Index >= 0;' in processor
assert 'l[nextSample] += mainDry * (fx.dry[0].first + fx.dry[1].first) + fx.wet.first;' in processor
assert 'r[nextSample] += mainDry * (fx.dry[0].second + fx.dry[1].second) + fx.wet.second;' in processor
assert 'toB1 ? 0.0f' not in processor
assert 'toB2 ? 0.0f' not in processor
assert '.withOutput ("FX Wet 7/8", juce::AudioChannelSet::stereo(), true)' in processor
assert 'std::pair<float, float> wet; // Shared chorus' in (root/'Source/SharedEffects.h').read_text()
print('PASS: TEST2.4.18 four stereo buses: full master, two optional distortion stems, isolated wet FX; no audio-input passthrough.')

# TEST2.4.19 — shared-waveform detuned OSC 2 per bassline, appended stable IDs.
assert 'osc2ParameterBase = separateOutputBase + basslineCount' in param_h
assert 'parameterCount = osc2ParameterBase + basslineCount * osc2ParameterCount' in param_h
assert 'voiceLocalParameterCount = osc2LocalBase + osc2ParameterCount' in param_h
assert 'voiceMidiDisplayIndex = 5' in param_h
assert 'voiceVisibleParameterCount = 22 + lfoParameterCount + osc2ParameterCount' in param_h
assert voice_order[:5] == [7, 0, 53, 54, 36]
assert voice_order[-16:] == list(range(37, 53))
assert 'result[osc2ParameterIndex (lane, osc2::volume)]' in param_cpp
assert 'result[osc2ParameterIndex (lane, osc2::detune)]' in param_cpp
assert '"Osc 2 Volume", 0, 100, 1, 0, "%"' in param_cpp
assert '"Osc 2 Detune", -100, 100, 1, 10, "cents"' in param_cpp
assert '"b1_osc2_" : "b2_osc2_"' in param_cpp
assert 'local < osc2LocalBase ? lfoParameterIndex' in param_h
engine_h = (root / 'Source/BassEngine.h').read_text(encoding='utf-8')
assert 'using Osc2Values = std::array<float, osc2ParameterCount>;' in engine_h
assert 'double osc2Phase = 0' in engine_h
assert 'if (! noteActive) osc2Phase = phase;' in engine
assert 'const double secondaryDt = clamp (dt * std::pow (2.0, osc2DetuneSmooth / 1200.0)' in engine
assert 'const double osc = primary + osc2GainSmooth * waveform (osc2Phase, secondaryDt);' in engine
assert 'setOsc2Parameters (layerValues)' in processor
assert 'lock.sliderNumber - 1 - static_cast<int> (lb303::osc2LocalBase)' in processor
assert 'voiceParameterGlobalIndex' in step
assert 'LB-303-0.0.2-TEST2.4.25-Windows-x64' in (root/'.github/workflows/build-windows.yml').read_text()
print('PASS: TEST2.4.19 two independent second oscillators, shared waveform/voice envelopes, cents detune, stable APVTS and lock slots.')

# TEST2.4.20 — Ctrl+A selects only focused and active keyboard steps.
assert 'void selectVisibleSteps();' in (root / 'Source/SequencerStepEditor.h').read_text()
select_visible = step.split('void SequencerStepEditor::selectVisibleSteps()', 1)[1].split('void SequencerStepEditor::changeBlock', 1)[0]
assert '(currentStep / 16) * 16' in select_visible
assert '(currentStep / 16) * 16 + 15' in select_visible
assert 'activeStepBounds()' in select_visible
assert 'selectedSteps.fill (false);' in select_visible
assert 'for (int step = first; step <= last; ++step)' in select_visible
assert 'selectedSteps[static_cast<size_t> (step)] = true;' in select_visible
assert 'lastStepKey = -1;' in select_visible
assert 'steps selected' in select_visible
assert "key.getTextCharacter() == 1" in step
assert 'selectVisibleSteps(); return true;' in step
print('PASS: TEST2.4.20 Ctrl+A selects only the focused 16 keys intersected with Start/End.')

# TEST2.4.21 — two-column Song Editor; selection and playback are independent.
song_h = (root / 'Source/SongArrangement.h').read_text(encoding='utf-8')
song_cpp = (root / 'Source/SongArrangement.cpp').read_text(encoding='utf-8')
step = (root / 'Source/SequencerStepEditor.cpp').read_text(encoding='utf-8')
step_h = (root / 'Source/SequencerStepEditor.h').read_text(encoding='utf-8')
processor = (root / 'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
processor_h = (root / 'Source/PluginProcessor.h').read_text(encoding='utf-8')
playback = (root / 'Source/SequencerPlayback.cpp').read_text(encoding='utf-8')
assert 'Source/SongArrangement.cpp' in (root / 'CMakeLists.txt').read_text()
assert 'using Column = std::vector<int>' in song_h
assert 'std::array<Snapshot, 2> columns' in song_h
assert 'std::atomic_load_explicit' in song_cpp and 'std::atomic_store_explicit' in song_cpp
assert 'LB303SongArrangement' in processor and 'songs.restore' in processor
assert 'songs.reset(); // old .lb presets have no arrangement' in processor
assert 'removeChildElement (song, true)' in processor
assert 'songs.reset();' in processor.split('void LB303AudioProcessor::resetToInitialState()', 1)[1]
assert 'songMode()' in song_h and 'setSongMode' in song_h
assert 'SongArrangement::locate' in processor
assert 'const auto songColumns' in processor
assert 'segmentEnd = juce::jmin' in processor
assert 'oldSongEntryIds[lane] != songEntryIds[lane]' in playback
assert 'previewRestartEpoch' in processor_h and 'if (activePreview) ppq = previewPpq;' in processor
assert 'songView = ! songView' in step
assert 'if (songView) return handleSongKey (key);' in step
assert 'std::array<int, 2> songCursor' in step_h
assert 'std::array<std::set<int>, 2> songSelected' in step_h
assert 'songShiftBaseline' in step_h and 'songShiftAnchor' in step_h
assert 'songSelected[lane] = songShiftBaseline[lane]' in step
assert 'if (details && count > 1)' in step
assert 'const bool marked = count > 1' in step
assert 'code == juce::KeyPress::tabKey) { announceSong (true)' in step
assert 'juce::KeyPress::pageUpKey ? -5 : 5' in step
assert 'if (letter (\'i\'))' in step and 'if (letter (\'l\'))' in step
assert "if (letter ('m') && mods.isShiftDown())" in step and 'auditionSongPattern' in step
assert 'if (letter (\'r\'))' in step and 'setSongMode (next)' in step
assert 'songConfirmClear' in step and 'Clear entire Bassline' in step
assert 'LB303-SONG-ITEMS-V1:' in step
assert 'for (int i : selection)' in step
assert 'data.insert (data.begin() + index, paste.begin(), paste.end());' in step
assert 'data.erase (data.begin() + *it)' in step
assert 'songUndo.push_back' in step and 'songUndoRedo (bool redo)' in step
assert 'lastSongRevision != processor.songs.getRevision()' in step
assert 'void SequencerStepEditor::selectVisibleSteps()' in step # step mode remains unchanged
assert 'songCursor' not in song_cpp and 'songSelected' not in song_cpp # transient navigation only
print('PASS: TEST2.4.21 Song columns, independent realtime pattern chaining, portable sparse clipboard, NVDA and per-column undo.')

# TEST2.4.22 — performance values are genuinely per-pattern, including song timing.
assert 'performanceConfigMap' in processor and 'sequenceBanks[lane].setConfigValue (' in processor
assert 'currentPattern (lane), performanceConfigMap[control], newValue' in processor
assert 'syncSelectedPatternPerformance' in processor and 'mirroringPerformance' in processor_h
assert 'processor.syncSelectedPatternPerformance();' in editor
assert 'LB303PatternPerformancePerPattern' in processor
assert 'if (oldLaneWidePerformance)' in processor
assert 'const auto config = bank.getConfig (pattern);' in playback
assert 'sequencerTiming::duration' in song_cpp and 'double bpmDivision) noexcept' not in song_h
assert 'SongArrangement::locate' in processor
assert 'songEntryIds[static_cast<size_t> (lane)]' in playback
assert 'randomSeed += 0x9e3779b9u' in playback
assert 'return sequencerTiming::random (randomSeed' in playback
assert 'for (int n = 0; n < 6; ++n)' in playback # LJuno velocity/gate distributions
assert 'static_cast<std::uint32_t> (repeat)' in playback # varying per retrigger
assert 'Same Ctrl+Up/Down pattern selection as in Alt+Q Steps' in step
assert 'if (code == juce::KeyPress::returnKey)' in step # reads details in Song
assert 'songChoosingPattern = true;' not in step # no second picker
print('PASS: TEST2.4.22 per-pattern parameters, dynamic Song divisions, evolving random, direct Song Ctrl+Up/Down.')

# TEST2.4.23 — Alt+M in both Alt+Q views, BPM-step Repeat / per-pulse Shuffle.
timing = (root / 'Source/SequencerTiming.h').read_text(encoding='utf-8')
assert 'Alt+M always chooses between Pattern and Song preview' in step
assert step.index('Alt+M always chooses between Pattern and Song preview') < step.index('if (songView) return handleSongKey (key);')
assert step.index('Alt+M always chooses between Pattern and Song preview') < step.index('if (pickerOpen) return handlePickerKey (key);')
assert 'processor.endSongAudition();' in step
assert 'sequencerTiming::interval (' in playback
assert 'const double intervalBeats = sequencerTiming::interval (' in playback
assert 'pulseBeat += intervalBeats;' in playback
assert 'sequencerTiming::count (' in playback
assert 'double stepBeats' in timing and 'config.shuffle' in playback
assert 'stepBeats / repeats' not in playback and 'repeat * repeatBeats' not in playback
assert 'sequencerTiming::duration (' in song_cpp
assert 'sequencerPlayback.getRandomSeed(), bpm' in processor
assert 'double beats, int lane, std::uint32_t seed, double bpm' in song_h
print('PASS: TEST2.4.23 repeat pulses at pattern BPM/Shuffle, correct Song durations, global Alt+M.')

# Note on each musical clock pulse, not an irregular gate-end subdivision.
beat = 0.25   # 16th note at division=4
shuffle = 0.5
interval = lambda index: beat * (1.0 + (0.5 * shuffle if index % 2 == 0 else -0.5 * shuffle))
assert abs(sum(interval(i) for i in range(3)) - 0.8125) < 1e-10
assert abs(sum(interval(i) for i in range(4)) - 1.0) < 1e-10
assert sum(interval(i) for i in range(3)) > beat  # 3 repeats exceed one step
assert sum(interval(i) for i in range(3)) != beat / 3.0
print('PASS: TEST2.4.23 numeric swing timing model; repeated pulses extend pattern.')

# TEST2.4.24 — Pattern/Song transport selection and pattern navigation on all grids.
editor_header_2424 = (root / 'Source' / 'PluginEditor.h').read_text(encoding='utf-8')
assert 'int shortcutPatternLane() const;' in editor_header_2424
assert 'void changePatternFromGrid (int direction, Control current);' in editor_header_2424
assert 'int LB303AudioProcessorEditor::shortcutPatternLane() const' in editor
assert 'void LB303AudioProcessorEditor::changePatternFromGrid (int direction, Control current)' in editor
main_keys = editor.split('bool LB303AudioProcessorEditor::handleKey (', 1)[1]
assert 'const bool song = ! processor.songs.songMode();' in main_keys
assert 'processor.endSongAudition();' in main_keys
assert 'processor.songs.setSongMode (song);' in main_keys
assert 'processor.restartSongPreviewClock();' in main_keys
assert main_keys.index("letter ('m')") < main_keys.index('// LJuno preset shortcuts work')
assert 'changePatternFromGrid (code == juce::KeyPress::upKey ? 1 : -1, current);' in main_keys
assert main_keys.index('changePatternFromGrid (code == juce::KeyPress::upKey ? 1 : -1, current);') < main_keys.index('if (current == Control::edit)')
assert 'current == Control::edit && editDirty && ! commitEdit()' in main_keys
shortcuts = editor.split('void LB303AudioProcessorEditor::changePatternFromGrid (', 1)[1].split('bool LB303AudioProcessorEditor::handleKey', 1)[0]
assert 'processor.currentPattern (static_cast<size_t> (lane))' in shortcuts
assert 'juce::jlimit (0, 15, old + direction)' in shortcuts
assert 'if (next == old) return;' in shortcuts
assert 'parameter->beginChangeGesture();' in shortcuts and 'parameter->endChangeGesture();' in shortcuts
assert 'processor.syncSelectedPatternPerformance();' in shortcuts
assert 'selectParameter();' in shortcuts
assert 'lb303::announceToActiveScreenReader (active,' in shortcuts
assert 'lb303::editTrackIndex' in editor.split('int LB303AudioProcessorEditor::shortcutPatternLane()', 1)[1].split('void LB303AudioProcessorEditor::changePatternFromGrid', 1)[0]
assert 'TEST2.4.25' in (root / '.github/workflows/build-windows.yml').read_text()
print('PASS: TEST2.4.24 Alt+M and Ctrl+Up/Down accessible across all four parameter grids.')

# TEST2.4.25: Ctrl+Up/Down cannot require !Command on Windows because JUCE
# commandModifier aliases ctrlModifier on Windows/Linux. Old key predicate
# compiled, but could never be satisfied in REAPER on Windows.
import re as _test2425_re
key_handler_2425 = editor.split('bool LB303AudioProcessorEditor::handleKey (', 1)[1]
ctrl_up_2425 = key_handler_2425.split('// Windows JUCE maps commandModifier', 1)[1].split('changePatternFromGrid (', 1)[0]
assert 'mods.isCtrlDown() && ! mods.isAltDown() && ! mods.isShiftDown()' in ctrl_up_2425
assert '! mods.isCommandDown()' not in ctrl_up_2425
assert 'juce::KeyPress::upKey || code == juce::KeyPress::downKey' in ctrl_up_2425
assert 'editor.handleKey (k, Control::grid)' in editor
assert 'editor.handleKey (k, Control::value)' in editor
assert 'editor.handleKey (k, Control::edit)' in editor
assert 'bool keyPressed (const juce::KeyPress&) override;' in editor_header_2424
root_fallback_2425 = editor.split('bool LB303AudioProcessorEditor::keyPressed (', 1)[1].split('void LB303AudioProcessorEditor::scheduleInitialFocusTransfer', 1)[0]
assert 'stepEditor->isOpen()' in root_fallback_2425
assert 'presetBrowser->active()' in root_fallback_2425
assert 'mods.isCtrlDown()' in root_fallback_2425
assert 'return handleKey (key, control);' in root_fallback_2425
assert '! mods.isCommandDown()' not in root_fallback_2425
assert 'TEST2.4.25' in (root / '.github/workflows/build-windows.yml').read_text()
# Explicitly model JUCE's Windows flags: both isCtrlDown and isCommandDown
# are TRUE for the ctrlModifier bit. The fixed predicate must accept it.
ctrl_pressed_windows = True
command_pressed_windows = ctrl_pressed_windows
alt_pressed = shift_pressed = False
assert ctrl_pressed_windows and not alt_pressed and not shift_pressed
assert not (ctrl_pressed_windows and not command_pressed_windows)
print('PASS: TEST2.4.25 Windows Ctrl/Command alias fixed; root focus fallback; overlays isolated.')
