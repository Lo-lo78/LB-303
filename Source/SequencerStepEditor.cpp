// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SequencerStepEditor.h"
#include "ScreenReaderAnnouncer.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr std::array<int, 9> coarseSteps { 1, 5, 10, 15, 20, 25, 30, 35, 40 };
constexpr std::array<char, 8> topKeys { 'q','w','e','r','t','y','u','i' };
constexpr std::array<char, 8> lowerKeys { 'a','s','d','f','g','h','j','k' };
const juce::String pageNames[] { "Note", "Length", "Velocity", "Repeat", "Shift", "Launch Step", "Parameters" };
constexpr int maxUndo = 50;
}

SequencerStepEditor::SequencerStepEditor (LB303AudioProcessor& p)
    : juce::Label ("Step editor"), processor (p)
{
    setWantsKeyboardFocus (true);
    setJustificationType (juce::Justification::topLeft);
    setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 17.0f, juce::Font::plain)));
    setColour (juce::Label::backgroundColourId, juce::Colour (0xff182439));
    setColour (juce::Label::textColourId, juce::Colours::white);
    setBorderSize (juce::BorderSize<int> (20));
    setTitle ("Sequencer step editor");
    setDescription ("Alt Q. Home and End choose bassline. Control Up and Down change pattern. 1 to 7 choose page. On Parameters, Enter opens parameter selection. Q to I and A to K choose step. Z/X change Start Step and C/V change End Step. Tab changes block within active Start/End. Escape closes.");
    setVisible (false);
}

void SequencerStepEditor::open (int selectedTrack, juce::Component* focus)
{
    if (opened) return;
    track = juce::jlimit (0, 1, selectedTrack);
    returnControl = focus;
    opened = true;
    lastStepKey = -1;
    pickerOpen = false;
    selectedLockIndex = 0;
    lastRevision = 0;
    lastPattern = -1;
    setVisible (true);
    toFront (false);
    refresh();
    grabKeyboardFocus();
    juce::AccessibilityHandler::clearCurrentlyFocusedHandler();
    if (auto* handler = getAccessibilityHandler()) handler->grabFocus();
    announce ("Sequencer editor. Bassline " + juce::String (track + 1)
              + ", Pattern " + juce::String (pattern() + 1)
              + ", " + pageName() + ", Step " + juce::String (currentStep + 1));
}
void SequencerStepEditor::close()
{
    if (! opened) return;
    opened = false;
    pickerOpen = false;
    if (preview) { processor.requestSequencerPreview (-1); preview = false; }
    setVisible (false);
    if (returnControl != nullptr)
    {
        auto safe = returnControl;
        juce::MessageManager::callAsync ([safe]
        {
            if (safe != nullptr)
            {
                safe->grabKeyboardFocus();
                if (auto* handler = safe->getAccessibilityHandler()) handler->grabFocus();
            }
        });
    }
    returnControl = nullptr;
}
void SequencerStepEditor::focusGained (FocusChangeType cause)
{
    juce::Label::focusGained (cause);
    if (opened) refresh();
}
juce::String SequencerStepEditor::pageName() const { return pageNames[page]; }
void SequencerStepEditor::announce (const juce::String& msg)
{
    if (opened) lb303::announceToActiveScreenReader (*this, msg);
}
juce::String SequencerStepEditor::valueText (int index) const
{
    const auto step = bank().getStep (pattern(), index);
    switch (page)
    {
        case 0: return step.note >= 128 ? "Pause" : juce::String (step.note);
        case 1: return step.length == 101 ? "101 Legato" : juce::String (step.length);
        case 2: return juce::String (step.velocity);
        case 3: return juce::String (step.repeat);
        case 4: return juce::String ((step.shift - 0.5f) * 2.0f, 2);
        case 6: return index == currentStep ? currentLockText()
                         : juce::String (step.parameterLockCount) + " parameter locks";
        default: return "";
    }
}
std::pair<int, int> SequencerStepEditor::activeStepBounds() const
{
    const auto config = bank().getConfig (pattern());
    const int first = juce::jlimit (0, lb303::SequencerState::stepsPerSequence - 1,
                                    config.startStep - 1);
    const int last = juce::jlimit (first, lb303::SequencerState::stepsPerSequence - 1,
                                   config.endStep - 1);
    return { first, last }; // inclusive, zero-based
}
void SequencerStepEditor::refresh()
{
    if (! opened) return;
    if (pickerOpen) { renderParameterPicker(); return; }
    // The range belongs to the selected pattern in the selected bassline.
    // Also handle external edits/host state restore while the editor is open.
    const auto [firstActive, lastActive] = activeStepBounds();
    const int bounded = juce::jlimit (firstActive, lastActive, currentStep);
    if (bounded != currentStep) { currentStep = bounded; lastStepKey = -1; }
    const int pat = pattern();
    const auto revision = bank().getRevision();
    if (revision == lastRevision && lastPattern == pat) return;
    lastRevision = revision;
    lastPattern = pat;
    const auto block = currentStep / 16;
    const auto config = bank().getConfig (pat);
    juce::String text;
    text << "LB-303 SEQUENCER — BASSLINE " << (track + 1)
         << " — PATTERN " << (pat + 1) << "/16 — PAGE " << (page + 1)
         << " " << pageName() << "\n\n";
    if (page == 5)
        text << "Launch Step " << config.launchStep << "\n"
             << "Up/Down: change launch step. Page 1–5, 7: select page.\n";
    else
    {
        text << "STEPS " << juce::jmax (firstActive + 1, block * 16 + 1)
             << "–" << juce::jmin (lastActive + 1, block * 16 + 16) << "\n";
        for (int row = 0; row < 2; ++row)
        {
            for (int col = 0; col < 8; ++col)
            {
                const int step = block * 16 + row * 8 + col;
                // Do not present steps outside the active playback range as editable.
                if (step < firstActive || step > lastActive)
                    text << " ";
                else
                    text << (step == currentStep ? ">" : selectedSteps[static_cast<size_t> (step)] ? "*" : " ")
                         << (step + 1) << ":" << valueText (step);
                if (col != 7) text << "   ";
            }
            text << "\n";
        }
        if (page == 6)
            text << "\nEnter: select a synth parameter, Up/Down: edit current lock. "
                 << "Alt+Home/End: previous/next lock. Delete: remove, Alt+Delete: remove all."
                 << "\nPlayback of locks: TEST2.3.\n";
    }
    text << "\nStart " << config.startStep << "  End " << config.endStep
         << "  Division " << juce::String (config.bpmDivision, 4)
         << "  Step " << coarseSteps[static_cast<size_t> (valueStepIndex)]
         << "\nHome/End: bassline  |  Ctrl+Up/Down: pattern +/-  |  Tab: active blocks"
         << "\nZ/X: Start Step -/+  |  C/V: End Step -/+"
         << "\nCtrl+C/V/X: Copy/Paste/Cut  |  Ctrl+Z: Undo  |  Ctrl+Shift+Z: Redo"
         << "\nSpace: preview  |  Esc: return to parameter grid";
    setText (text, juce::dontSendNotification);
}
void SequencerStepEditor::announceCurrent()
{
    if (page == 5)
    {
        announce ("Bassline " + juce::String (track + 1) + ", Pattern "
            + juce::String (pattern() + 1) + ", Launch Step "
            + juce::String (bank().getConfig (pattern()).launchStep));
        return;
    }
    announce ("Step " + juce::String (currentStep + 1)
        + (selectedSteps[static_cast<size_t> (currentStep)] ? ", selected" : "")
        + ", " + pageName() + " " + valueText (currentStep));
}
void SequencerStepEditor::selectStep (int inBlock)
{
    const int index = currentStep / 16 * 16 + juce::jlimit (0, 15, inBlock);
    const auto [firstActive, lastActive] = activeStepBounds();
    if (index < firstActive || index > lastActive) return; // silent inactive cells
    const auto now = juce::Time::getMillisecondCounterHiRes();
    const bool doublePress = index == lastStepKey && now - lastPressTime >= 40.0
                          && now - lastPressTime <= 350.0;
    currentStep = index;
    if (doublePress)
    {
        auto& item = selectedSteps[static_cast<size_t> (index)];
        item = ! item;
        lastStepKey = -1;
        lastRevision = 0;
        refresh();
        announce ("Step " + juce::String (index + 1) + (item ? ", selected" : ", deselected"));
        return;
    }
    lastStepKey = index;
    lastPressTime = now;
    lastRevision = 0;
    refresh();
    announceCurrent();
}
void SequencerStepEditor::changeBlock (int direction)
{
    const auto [firstActive, lastActive] = activeStepBounds();
    const int block = currentStep / 16;
    const int firstBlock = firstActive / 16;
    const int lastBlock = lastActive / 16;
    const int next = juce::jlimit (firstBlock, lastBlock, block + direction);
    if (next == block) return; // silent active-range boundary
    // Preserve the local step position when possible, but never leave the range.
    currentStep = juce::jlimit (firstActive, lastActive, next * 16 + currentStep % 16);
    lastStepKey = -1;
    lastRevision = 0;
    refresh();
    announce ("Bassline " + juce::String (track + 1) + ", Pattern "
              + juce::String (pattern() + 1) + ", Steps "
              + juce::String (juce::jmax (firstActive + 1, next * 16 + 1))
              + " to " + juce::String (juce::jmin (lastActive + 1, next * 16 + 16)));
}
void SequencerStepEditor::changeActiveRange (bool editStart, int direction)
{
    const int pat = pattern();
    const auto old = bank().getConfig (pat);
    const int previous = editStart ? old.startStep : old.endStep;
    const int target = juce::jlimit (1, lb303::SequencerState::stepsPerSequence,
                                     previous + direction);
    if (target == previous) return; // no repeated speech at 1 or 128
    bank().setConfigValue (pat,
        editStart ? lb303::SequencerState::ConfigParameter::startStep
                  : lb303::SequencerState::ConfigParameter::endStep,
        static_cast<float> (target));
    const auto [firstActive, lastActive] = activeStepBounds();
    currentStep = juce::jlimit (firstActive, lastActive, currentStep);
    for (int i = 0; i < lb303::SequencerState::stepsPerSequence; ++i)
        if (i < firstActive || i > lastActive)
            selectedSteps[static_cast<size_t> (i)] = false;
    lastStepKey = -1;
    lastRevision = 0;
    refresh();
    const auto updated = bank().getConfig (pat);
    announce (juce::String (editStart ? "Start Step " : "End Step ")
              + juce::String (editStart ? updated.startStep : updated.endStep));
}
void SequencerStepEditor::changeTrack (int direction)
{
    // Home/End navigate between the two independent lanes, with silent edges.
    const int target = juce::jlimit (0, static_cast<int> (lb303::basslineCount) - 1,
                                     track + (direction < 0 ? -1 : 1));
    if (target == track) return;
    if (preview) { processor.requestSequencerPreview (-1); preview = false; }
    track = target;
    if (auto* param = processor.parameter (lb303::editTrackIndex))
        param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (track)));
    selectedSteps.fill (false);
    lastStepKey = -1;
    lastRevision = 0;
    refresh();
    announce ("Bassline " + juce::String (track + 1) + ", Pattern "
             + juce::String (pattern() + 1) + ", " + pageName());
}
void SequencerStepEditor::changePattern (int direction)
{
    const int old = pattern();
    const int next = juce::jlimit (0, 15, old + direction);
    if (next == old) return;
    // Only the selector for the current bassline is touched.
    const auto index = track == 0 ? lb303::pattern1Index : lb303::pattern2Index;
    if (auto* param = processor.parameter (index))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (next)));
        param->endChangeGesture();
    }
    selectedSteps.fill (false);
    lastStepKey = -1;
    lastRevision = 0;
    refresh();
    announce ("Bassline " + juce::String (track + 1) + ", Pattern " + juce::String (next + 1));
}
std::vector<int> SequencerStepEditor::editingSteps() const
{
    std::vector<int> indices;
    const auto [firstActive, lastActive] = activeStepBounds();
    for (int i = firstActive; i <= lastActive; ++i)
        if (selectedSteps[static_cast<size_t> (i)]) indices.push_back (i);
    const int bounded = juce::jlimit (firstActive, lastActive, currentStep);
    if (std::find (indices.begin(), indices.end(), bounded) == indices.end())
        indices.push_back (bounded);
    std::sort (indices.begin(), indices.end());
    return indices;
}
void SequencerStepEditor::commitAction (Action&& action)
{
    if (action.changes.empty() && ! action.hasLaunch) return;
    undoStack.push_back (std::move (action));
    if (undoStack.size() > maxUndo) undoStack.erase (undoStack.begin());
    redoStack.clear();
    lastRevision = 0;
    refresh();
}
void SequencerStepEditor::applyAction (const Action& action, bool backwards)
{
    auto& dest = processor.patternBank (static_cast<size_t> (action.track));
    for (const auto& step : action.changes)
        dest.replaceStep (action.pattern, step.index, backwards ? step.before : step.after);
    if (action.hasLaunch)
        dest.setConfigValue (action.pattern, lb303::SequencerState::ConfigParameter::launchStep,
                             static_cast<float> (backwards ? action.launchBefore : action.launchAfter));
    lastRevision = 0;
    refresh();
}
void SequencerStepEditor::undo (bool redo)
{
    auto& source = redo ? redoStack : undoStack;
    if (source.empty()) return;
    Action action = std::move (source.back());
    source.pop_back();
    applyAction (action, ! redo);
    auto& dest = redo ? undoStack : redoStack;
    dest.push_back (std::move (action));
    announce (redo ? "Redo" : "Undo");
}
void SequencerStepEditor::modifyValue (int direction, int amount)
{
    if (page == 5) { changeLaunch (direction, amount); return; }
    if (page == 6) { editSelectedLock (direction, amount); return; }
    Action action; action.track = track; action.pattern = pattern();
    const auto layer = static_cast<lb303::SequencerLayer> (page);
    const float delta = page == 4 ? 0.01f * direction * amount : static_cast<float> (direction * amount);
    for (auto index : editingSteps())
    {
        StepChange change; change.index = index;
        change.before = bank().getStep (pattern(), index);
        const auto before = bank().getStepValue (pattern(), index, layer);
        bank().addStepDelta (pattern(), index, layer, delta);
        const auto after = bank().getStepValue (pattern(), index, layer);
        if (std::abs (after - before) > 0.000001f)
        {
            change.after = bank().getStep (pattern(), index);
            action.changes.push_back (std::move (change));
        }
    }
    if (action.changes.empty()) return; // silent limits
    commitAction (std::move (action));
    announceCurrent();
}
void SequencerStepEditor::changeLaunch (int direction, int amount)
{
    const int before = bank().getConfig (pattern()).launchStep;
    const int target = juce::jlimit (1, 128, before + direction * amount);
    if (target == before) return;
    Action action; action.track = track; action.pattern = pattern();
    action.hasLaunch = true; action.launchBefore = before; action.launchAfter = target;
    bank().setConfigValue (pattern(), lb303::SequencerState::ConfigParameter::launchStep,
                           static_cast<float> (target));
    commitAction (std::move (action));
    announceCurrent();
}
void SequencerStepEditor::resetCurrent()
{
    if (page == 5)
    {
        const int before = bank().getConfig (pattern()).launchStep;
        if (before != 1) changeLaunch (-1, before - 1);
        return;
    }
    if (page == 6) { resetSelectedLock(); return; }
    Action action; action.track = track; action.pattern = pattern();
    for (int i : editingSteps())
    {
        StepChange change; change.index = i;
        change.before = bank().getStep (pattern(), i);
        lb303::SequencerStep reset = change.before;
        reset.note = 128; reset.length = 50; reset.velocity = 100;
        reset.repeat = 1; reset.shift = 0.5f;
        bank().replaceStep (pattern(), i, reset);
        change.after = bank().getStep (pattern(), i);
        action.changes.push_back (std::move (change));
    }
    commitAction (std::move (action));
    announce ("Reset. Step " + juce::String (currentStep + 1));
}
void SequencerStepEditor::toggleCurrentNote()
{
    // Empty patterns start as rests (MIDI 128). Enter immediately creates C4
    // without requiring the user to decrement 68 times using the arrow keys.
    Action action; action.track = track; action.pattern = pattern();
    for (int index : editingSteps())
    {
        StepChange change; change.index = index;
        change.before = bank().getStep (pattern(), index);
        const int target = change.before.note >= 128 ? 60 : 128;
        bank().setStepValue (pattern(), index, lb303::SequencerLayer::note,
                             static_cast<float> (target));
        change.after = bank().getStep (pattern(), index);
        action.changes.push_back (std::move (change));
    }
    commitAction (std::move (action));
    announceCurrent();
}
void SequencerStepEditor::copySteps (bool cut)
{
    clipboard.clear();
    const auto indices = editingSteps();
    for (int i : indices) clipboard.push_back (bank().getStep (pattern(), i));
    if (! cut)
    {
        announce (juce::String (clipboard.size()) + " steps copied");
        return;
    }
    Action action; action.track = track; action.pattern = pattern();
    for (int i : indices)
    {
        StepChange change; change.index = i;
        change.before = bank().getStep (pattern(), i);
        bank().resetStep (pattern(), i);
        change.after = bank().getStep (pattern(), i);
        action.changes.push_back (std::move (change));
    }
    commitAction (std::move (action));
    announce (juce::String (clipboard.size()) + " steps cut");
}
void SequencerStepEditor::pasteSteps()
{
    if (clipboard.empty()) return;
    Action action; action.track = track; action.pattern = pattern();
    // Contiguous placement starts at the current step, within Start/End Step.
    const auto [firstActive, lastActive] = activeStepBounds();
    const int insertion = juce::jlimit (firstActive, lastActive, currentStep);
    for (size_t i = 0; i < clipboard.size(); ++i)
    {
        const int index = insertion + static_cast<int> (i);
        if (index > lastActive) break;
        StepChange change; change.index = index;
        change.before = bank().getStep (pattern(), index);
        bank().replaceStep (pattern(), index, clipboard[i]);
        change.after = bank().getStep (pattern(), index);
        action.changes.push_back (std::move (change));
    }
    const int count = static_cast<int> (action.changes.size());
    commitAction (std::move (action));
    announce (juce::String (count) + " steps pasted");
}
// Parameter selectors store the 1-based local catalog slot in sliderNumber.
// The two independent banks translate that slot to the corresponding voice.
juce::String SequencerStepEditor::currentLockText() const
{
    const int count = bank().getStepParameterCount (pattern(), currentStep);
    if (count == 0) return "0 parameter locks";
    const int index = juce::jlimit (0, count - 1, selectedLockIndex);
    const auto lock = bank().getStepParameterLock (pattern(), currentStep, index);
    const int local = lock.sliderNumber - 1;
    if (! juce::isPositiveAndBelow (local, static_cast<int> (lb303::voiceParameterCount)))
        return juce::String (count) + " parameter locks";
    const auto catalogIndex = lb303::parameterIndex (static_cast<size_t> (track),
                                                   static_cast<size_t> (local));
    return juce::String (index + 1) + " of " + juce::String (count) + ", "
        + lb303::catalog()[catalogIndex].name + " "
        + lb303::parameterText (catalogIndex, lock.value);
}
void SequencerStepEditor::openParameterPicker()
{
    if (page != 6 || pickerOpen) return;
    pickerOpen = true;
    const auto lockCount = bank().getStepParameterCount (pattern(), currentStep);
    if (lockCount > 0)
    {
        const auto lock = bank().getStepParameterLock (pattern(), currentStep,
            juce::jlimit (0, lockCount - 1, selectedLockIndex));
        pickerIndex = juce::jlimit (0, static_cast<int> (lb303::voiceParameterCount) - 1,
                                   lock.sliderNumber - 1);
    }
    else pickerIndex = 0;
    renderParameterPicker();
    announce ("Select Parameter. Bassline " + juce::String (track + 1)
        + ", Step " + juce::String (currentStep + 1) + ". "
        + "Use arrows, Enter to assign and close, Alt+Enter to toggle and stay, Escape to return. "
        + pickerItemText (pickerIndex));
}
void SequencerStepEditor::closeParameterPicker()
{
    if (! pickerOpen) return;
    pickerOpen = false;
    lastRevision = 0;
    refresh();
    announce ("Parameters. Step " + juce::String (currentStep + 1) + ", "
        + currentLockText());
}
juce::String SequencerStepEditor::pickerItemText (int localIndex) const
{
    const auto index = lb303::parameterIndex (static_cast<size_t> (track),
                                               static_cast<size_t> (localIndex));
    const bool assigned = bank().findStepParameterLock (pattern(), currentStep,
                                                        localIndex + 1) >= 0;
    // The prefix refers to a parameter already locked on the current step,
    // not merely the row currently highlighted with the arrow keys.
    return juce::String (assigned ? "selected " : "") + lb303::catalog()[index].name;
}

void SequencerStepEditor::renderParameterPicker()
{
    if (! opened || ! pickerOpen) return;
    juce::String text;
    text << "LB-303 SELECT PARAMETER — BASSLINE " << (track + 1)
         << ", PATTERN " << (pattern() + 1) << ", STEP " << (currentStep + 1)
         << "\\n\\n";
    const int rows = 8;
    const int count = static_cast<int> (lb303::voiceParameterCount);
    const int columns = (count + rows - 1) / rows;
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < columns; ++col)
        {
            const int item = col * rows + row;
            if (item >= count) continue;
            text << (pickerIndex == item ? "> " : "  ")
                 << pickerItemText (item);
            if (col != columns - 1) text << "     ";
        }
        text << "\\n";
    }
    text << "\\nArrows: navigate | Enter: add/select lock | Alt+Enter: toggle lock"
         << "\\nHome/End: column limits | Ctrl+Home/End: first/last | Esc: return";
    text = text.replace ("\\n", "\n");
    if (getText() != text) setText (text, juce::dontSendNotification);
}
void SequencerStepEditor::announcePickerItem()
{
    renderParameterPicker();
    announce (pickerItemText (pickerIndex));
}
void SequencerStepEditor::selectPickerParameter (bool toggle)
{
    const int pat = pattern();
    const int stepIndex = currentStep;
    const int local = pickerIndex;
    const int prior = bank().findStepParameterLock (pat, stepIndex, local + 1);
    Action action; action.track = track; action.pattern = pat;
    StepChange change; change.index = stepIndex; change.before = bank().getStep (pat, stepIndex);
    if (prior >= 0 && toggle)
    {
        bank().removeStepParameterLock (pat, stepIndex, prior);
        selectedLockIndex = 0;
    }
    else if (prior >= 0)
        selectedLockIndex = prior;
    else
    {
        const auto index = lb303::parameterIndex (static_cast<size_t> (track),
                                                    static_cast<size_t> (local));
        const int added = bank().addStepParameterLock (pat, stepIndex, local + 1,
                                                       processor.value (index));
        if (added < 0) { announce ("Maximum 32 parameter locks for this step"); return; }
        selectedLockIndex = added;
    }
    change.after = bank().getStep (pat, stepIndex);
    if (change.before.parameterLockCount != change.after.parameterLockCount)
    {
        action.changes.push_back (std::move (change));
        commitAction (std::move (action));
    }
    if (toggle)
        announcePickerItem();
    else
        closeParameterPicker();
}
void SequencerStepEditor::editSelectedLock (int direction, int amount)
{
    const auto count = bank().getStepParameterCount (pattern(), currentStep);
    if (count <= 0) return;
    selectedLockIndex = juce::jlimit (0, count - 1, selectedLockIndex);
    const auto lock = bank().getStepParameterLock (pattern(), currentStep, selectedLockIndex);
    const int local = lock.sliderNumber - 1;
    if (! juce::isPositiveAndBelow (local, static_cast<int> (lb303::voiceParameterCount))) return;
    const auto& info = lb303::catalog()[lb303::parameterIndex (static_cast<size_t> (track),
                                                               static_cast<size_t> (local))];
    const float delta = (info.choiceCount > 0 ? 1.0f : info.step) * direction * amount;
    const float next = juce::jlimit (info.minimum, info.maximum, lock.value + delta);
    if (std::abs (next - lock.value) < 0.0000001f) return; // silent edges
    Action action; action.track = track; action.pattern = pattern();
    StepChange change; change.index = currentStep;
    change.before = bank().getStep (pattern(), currentStep);
    bank().setStepParameterLockValue (pattern(), currentStep, selectedLockIndex, next);
    change.after = bank().getStep (pattern(), currentStep);
    action.changes.push_back (std::move (change));
    commitAction (std::move (action));
    announceCurrent();
}
void SequencerStepEditor::changeSelectedLock (int direction)
{
    const int count = bank().getStepParameterCount (pattern(), currentStep);
    if (count <= 0) return;
    const int next = juce::jlimit (0, count - 1, selectedLockIndex + direction);
    if (next == selectedLockIndex) return;
    selectedLockIndex = next;
    lastRevision = 0;
    refresh();
    announceCurrent();
}
void SequencerStepEditor::deleteSelectedLock (bool all)
{
    const int count = bank().getStepParameterCount (pattern(), currentStep);
    if (count <= 0) return;
    Action action; action.track = track; action.pattern = pattern();
    StepChange change; change.index = currentStep;
    change.before = bank().getStep (pattern(), currentStep);
    if (all)
        for (int index = count - 1; index >= 0; --index)
            bank().removeStepParameterLock (pattern(), currentStep, index);
    else
        bank().removeStepParameterLock (pattern(), currentStep,
                                      juce::jlimit (0, count - 1, selectedLockIndex));
    change.after = bank().getStep (pattern(), currentStep);
    action.changes.push_back (std::move (change));
    selectedLockIndex = 0;
    commitAction (std::move (action));
    announceCurrent();
}
void SequencerStepEditor::resetSelectedLock()
{
    const int count = bank().getStepParameterCount (pattern(), currentStep);
    if (count <= 0) return;
    selectedLockIndex = juce::jlimit (0, count - 1, selectedLockIndex);
    const auto lock = bank().getStepParameterLock (pattern(), currentStep, selectedLockIndex);
    const int local = lock.sliderNumber - 1;
    if (! juce::isPositiveAndBelow (local, static_cast<int> (lb303::voiceParameterCount))) return;
    const float initial = lb303::catalog()[lb303::parameterIndex (
        static_cast<size_t> (track), static_cast<size_t> (local))].initial;
    if (std::abs (initial - lock.value) < 0.0000001f) return;
    Action action; action.track = track; action.pattern = pattern();
    StepChange change; change.index = currentStep; change.before = bank().getStep (pattern(), currentStep);
    bank().setStepParameterLockValue (pattern(), currentStep, selectedLockIndex, initial);
    change.after = bank().getStep (pattern(), currentStep);
    action.changes.push_back (std::move (change));
    commitAction (std::move (action));
    announceCurrent();
}
bool SequencerStepEditor::handlePickerKey (const juce::KeyPress& key)
{
    const int code = key.getKeyCode();
    const auto mods = key.getModifiers();
    if (code == juce::KeyPress::escapeKey) { closeParameterPicker(); return true; }
    if (code == juce::KeyPress::returnKey) { selectPickerParameter (mods.isAltDown()); return true; }
    // Ctrl+Up/Down is deliberately unassigned in the parameter picker.
    // Consume it silently: do not change track, pattern or highlighted row,
    // and do not pass it through to the REAPER host.
    if (mods.isCtrlDown() && (code == juce::KeyPress::upKey
                             || code == juce::KeyPress::downKey))
        return true;
    const int count = static_cast<int> (lb303::voiceParameterCount);
    const int firstInColumn = (pickerIndex / 8) * 8;
    const int lastInColumn = juce::jmin (count - 1, firstInColumn + 7);
    int target = pickerIndex;
    if (mods.isCtrlDown() && code == juce::KeyPress::homeKey) target = 0;
    else if (mods.isCtrlDown() && code == juce::KeyPress::endKey) target = count - 1;
    else if (code == juce::KeyPress::homeKey) target = firstInColumn;
    else if (code == juce::KeyPress::endKey) target = lastInColumn;
    else if (code == juce::KeyPress::pageUpKey) target = juce::jmax (firstInColumn, pickerIndex - 5);
    else if (code == juce::KeyPress::pageDownKey) target = juce::jmin (lastInColumn, pickerIndex + 5);
    else if (code == juce::KeyPress::upKey) target = juce::jmax (firstInColumn, pickerIndex - 1);
    else if (code == juce::KeyPress::downKey) target = juce::jmin (lastInColumn, pickerIndex + 1);
    else if (code == juce::KeyPress::leftKey) target = juce::jmax (0, pickerIndex - 8);
    else if (code == juce::KeyPress::rightKey) target = juce::jmin (count - 1, pickerIndex + 8);
    else if (! mods.isAltDown() && ! mods.isCtrlDown())
    {
        auto ch = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());
        if (mods.isShiftDown() && code >= '0' && code <= '9')
            ch = static_cast<juce::juce_wchar> (code);
        if (juce::CharacterFunctions::isLetterOrDigit (ch))
        {
            const int delta = mods.isShiftDown() ? -1 : 1;
            for (int i = 1; i <= count; ++i)
            {
                const int candidate = (pickerIndex + delta * i + count * 2) % count;
                const auto index = lb303::parameterIndex (static_cast<size_t> (track),
                                                         static_cast<size_t> (candidate));
                const auto& name = lb303::catalog()[index].name;
                if (name.isNotEmpty() && juce::CharacterFunctions::toLowerCase (name[0]) == ch)
                { target = candidate; break; }
            }
        }
    }
    if (target != pickerIndex) { pickerIndex = target; announcePickerItem(); }
    return true;
}

bool SequencerStepEditor::keyPressed (const juce::KeyPress& key)
{
    if (! opened) return juce::Label::keyPressed (key);
    if (pickerOpen) return handlePickerKey (key);
    const int code = key.getKeyCode();
    const auto mods = key.getModifiers();
    const auto ch = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());
    const auto letter = [code, ch] (juce::juce_wchar c)
    { return ch == c || code == juce::CharacterFunctions::toUpperCase (c); };
    if (code == juce::KeyPress::escapeKey) { close(); return true; }
    if (page == 6 && mods.isAltDown())
    {
        if (code == juce::KeyPress::homeKey) { changeSelectedLock (-1); return true; }
        if (code == juce::KeyPress::endKey) { changeSelectedLock (1); return true; }
        if (code == juce::KeyPress::deleteKey) { deleteSelectedLock (true); return true; }
    }
    if (page == 6 && code == juce::KeyPress::deleteKey)
    { deleteSelectedLock (false); return true; }
    // In the step editor Ctrl+Tab is intentionally unassigned, as are other
    // unhandled Ctrl combinations; the Ctrl catch-all below consumes them
    // so focus cannot escape to REAPER or the underlying parameter grids.
    if (mods.isCtrlDown())
    {
        if (code == juce::KeyPress::upKey) { changePattern (1); return true; }
        if (code == juce::KeyPress::downKey) { changePattern (-1); return true; }
        if (letter ('c')) { copySteps (false); return true; }
        if (letter ('x')) { copySteps (true); return true; }
        if (letter ('v')) { pasteSteps(); return true; }
        if (letter ('z')) { undo (mods.isShiftDown()); return true; }
        if (letter ('y')) { undo (true); return true; }
        return true;
    }
    if (code == juce::KeyPress::spaceKey)
    {
        preview = ! preview;
        processor.requestSequencerPreview (preview ? track : -1);
        announce (preview ? "Bassline " + juce::String (track + 1) + " preview on" : "Preview off");
        return true;
    }
    if (mods.isAltDown() && letter ('i'))
    {
        announce ("Bassline " + juce::String (track + 1) + ", Pattern "
            + juce::String (pattern() + 1) + ", " + pageName() + ", Step "
            + juce::String (currentStep + 1) + ", Start "
            + juce::String (bank().getConfig (pattern()).startStep) + ", End "
            + juce::String (bank().getConfig (pattern()).endStep));
        return true;
    }
    // Same start/end shortcuts as LJuno. These define the actual length of
    // the current lane's current pattern and therefore the Tab navigation range.
    if (! mods.isAltDown())
    {
        if (letter ('z')) { changeActiveRange (true, -1); return true; }
        if (letter ('x')) { changeActiveRange (true, 1); return true; }
        if (letter ('c')) { changeActiveRange (false, -1); return true; }
        if (letter ('v')) { changeActiveRange (false, 1); return true; }
    }
    if (code == juce::KeyPress::tabKey)
    { changeBlock (mods.isShiftDown() ? -1 : 1); return true; }
    if (code == juce::KeyPress::homeKey) { changeTrack (-1); return true; }
    if (code == juce::KeyPress::endKey) { changeTrack (1); return true; }
    if (code == juce::KeyPress::returnKey)
    {
        if (page == 0) toggleCurrentNote();
        else if (page == 6) openParameterPicker();
        return true;
    }
    if (code == juce::KeyPress::backspaceKey) { resetCurrent(); return true; }
    if (code == juce::KeyPress::upKey || code == juce::KeyPress::downKey
        || code == juce::KeyPress::pageUpKey || code == juce::KeyPress::pageDownKey)
    {
        const int direction = code == juce::KeyPress::upKey || code == juce::KeyPress::pageUpKey ? 1 : -1;
        const int amount = code == juce::KeyPress::pageUpKey || code == juce::KeyPress::pageDownKey
            ? coarseSteps[static_cast<size_t> (valueStepIndex)] : 1;
        modifyValue (direction, amount);
        return true;
    }
    if (code == juce::KeyPress::leftKey || code == juce::KeyPress::rightKey)
    {
        const auto target = juce::jlimit (0, 8, valueStepIndex + (code == juce::KeyPress::rightKey ? 1 : -1));
        if (target != valueStepIndex)
        {
            valueStepIndex = target;
            lastRevision = 0;
            refresh();
            announce ("Value Step " + juce::String (coarseSteps[static_cast<size_t> (valueStepIndex)]));
        }
        return true;
    }
    if (ch >= '1' && ch <= '7' && ! mods.isAltDown())
    {
        const int target = static_cast<int> (ch - '1');
        if (page != target)
        {
            page = target;
            lastRevision = 0;
            refresh();
        }
        announce ("Page " + pageName() + (page == 6 ? ". Enter opens parameter selection" : ""));
        return true;
    }
    for (int i = 0; i < 8; ++i)
    {
        if (ch == topKeys[static_cast<size_t> (i)]) { selectStep (i); return true; }
        if (ch == lowerKeys[static_cast<size_t> (i)]) { selectStep (8 + i); return true; }
    }
    return true; // prevent focus escape into REAPER or underlying grids
}
