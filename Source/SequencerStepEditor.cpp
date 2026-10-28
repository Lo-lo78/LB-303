// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SequencerStepEditor.h"
#include "ScreenReaderAnnouncer.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <limits>

namespace
{
constexpr std::array<int, 9> coarseSteps { 1, 5, 10, 15, 20, 25, 30, 35, 40 };
constexpr std::array<char, 8> topKeys { 'q','w','e','r','t','y','u','i' };
constexpr std::array<char, 8> lowerKeys { 'a','s','d','f','g','h','j','k' };
const juce::String pageNames[] { "Note", "Length", "Velocity", "Repeat", "Shift", "Launch Step", "Parameters" };
constexpr int maxUndo = 50;
// Versioned clipboard marker prevents pasting ordinary text as a sequencer.
const juce::String patternClipboardHeader = "LB303-PATTERN-V1:";
const juce::String songClipboardHeader = "LB303-SONG-ITEMS-V1:";
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
    setDescription ("Enter toggles Pause/last Note on page 1, 101/last Length on page 2, and 127/last Velocity on page 3. Alt M switches Pattern/Song preview anywhere in Alt Q. Alt S switches Steps and Song editor. Ctrl A selects only the 16 steps of the focused keyboard block, bounded by Start and End. Delete clears selected steps. Alt Q opens/closes the editor. Alt P selects Space for REAPER transport or dual-bassline preview. Home and End choose bassline. Control Up and Down change pattern. 1 to 7 choose page. On Parameters, Enter opens parameter selection. Q to I and A to K choose step. Z/X change Start Step and C/V change End Step. Tab changes block within active Start/End. Escape closes without stopping preview.");
    setVisible (false);
}

void SequencerStepEditor::open (int selectedTrack, juce::Component* focus)
{
    if (opened) return;
    track = juce::jlimit (0, 1, selectedTrack);
    // Preserve the visited page only in this live processor instance.
    // Opening another instance or loading a project starts at Note.
    page = juce::jlimit (0, 6, processor.editorSession.sequencerPage);
    returnControl = focus;
    opened = true;
    lastStepKey = -1;
    pickerOpen = false;
    selectedLockIndex = 0;
    lastRevision = 0;
    lastPattern = -1;
    songView = false;
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
    processor.editorSession.sequencerPage = page;
    opened = false;
    pickerOpen = false;
    // Preview stays active when Alt+Q/Esc closes this editing overlay.
    // Only Space in preview mode, Alt+P back to DAW or Alt+I stops it.
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
    if (songView)
    {
        const auto epoch = processor.songs.getLoadEpoch();
        if (lastSongLoadEpoch != epoch)
        {
            lastSongLoadEpoch = epoch;
            songUndo.clear(); songRedo.clear();
            songConfirmClear = false;
            for (auto& set : songSelected) set.clear();
            songShiftAnchor = { -1, -1 };
            lastSongRevision = 0;
        }
        if (lastSongRevision != processor.songs.getRevision()) renderSong();
        return;
    }
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
        if (page == 0)
            text << "\nEnter: Pause / last note of this step.\n";
        if (page == 1)
            text << "\nEnter: 101 Legato / last length of this step.\n";
        if (page == 2)
            text << "\nEnter: 127 maximum velocity / last velocity of this step.\n";
        if (page == 6)
            text << "\nEnter: select a synth parameter, Up/Down: edit current lock. "
                 << "Alt+Home/End: previous/next lock. Delete clears selected steps first; with no selected steps it removes the current lock. Alt+Delete: remove all."
                 << "\nPlayback of locks: enabled.\n";
    }
    text << "\nStart " << config.startStep << "  End " << config.endStep
         << "  Division " << juce::String (config.bpmDivision, 4)
         << "  Step " << coarseSteps[static_cast<size_t> (valueStepIndex)]
         << "\nHome/End: bassline  |  Ctrl+Up/Down: pattern +/-  |  Tab: active blocks"
         << "\nZ/X: Start Step -/+  |  C/V: End Step -/+"
         << "\nCtrl+A: select visible 16 steps  |  Delete: deselect all  |  Ctrl+C/V/X: steps  |  Ctrl+Shift+C/X/V: current pattern only"
         << "  |  Ctrl+Z: Undo  |  Ctrl+Shift+Z: Redo"
         << "\nAlt+P: Space target (" << (processor.spaceControlsSequencerPreview() ? "Preview" : "REAPER")
         << ")  |  Space: " << (processor.spaceControlsSequencerPreview() ? "play/stop both basslines" : "REAPER play/stop")
         << "  |  Alt+M: Pattern/Song preview | Esc / Alt+Q: return without stopping preview";
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
void SequencerStepEditor::selectVisibleSteps()
{
    // The computer keyboard contains exactly one 16-step block (Q..I/A..K).
    // Ctrl+A REPLACES any older selection, including on a different block.
    // Never select steps that are outside this pattern's active Start/End range.
    const auto [firstActive, lastActive] = activeStepBounds();
    const int first = juce::jmax (firstActive, (currentStep / 16) * 16);
    const int last = juce::jmin (lastActive, (currentStep / 16) * 16 + 15);
    selectedSteps.fill (false);
    for (int step = first; step <= last; ++step)
        selectedSteps[static_cast<size_t> (step)] = true;
    lastStepKey = -1; // Next physical press cannot accidentally toggle selection.
    lastRevision = 0;
    lastPattern = -1;
    refresh();
    const int count = juce::jmax (0, last - first + 1);
    announce (juce::String (count) + " steps selected, "
              + juce::String (first + 1) + " to " + juce::String (last + 1));
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
    // Preview keeps both basslines running when changing the editing track.
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
    processor.syncSelectedPatternPerformance();
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
    // Explicit selection takes precedence over the focused step. The cursor
    // can move away from selected cells without adding an unintended edit.
    if (indices.empty())
        indices.push_back (juce::jlimit (firstActive, lastActive, currentStep));
    return indices;
}
void SequencerStepEditor::commitAction (Action&& action)
{
    if (action.changes.empty() && ! action.hasLaunch && ! action.hasWholePattern) return;
    undoStack.push_back (std::move (action));
    if (undoStack.size() > maxUndo) undoStack.erase (undoStack.begin());
    redoStack.clear();
    lastRevision = 0;
    refresh();
}
void SequencerStepEditor::applyAction (const Action& action, bool backwards)
{
    auto& dest = processor.patternBank (static_cast<size_t> (action.track));
    if (action.hasWholePattern)
    {
        // Undo / Redo affects only the recorded pattern in the recorded lane.
        // Other patterns and active pattern selectors are never overwritten.
        restorePatternSnapshot (dest, action.pattern,
                                backwards ? action.beforePattern : action.afterPattern);
    }
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
    const auto indices = editingSteps();
    const bool multiple = indices.size() > 1;
    for (auto index : indices)
    {
        StepChange change; change.index = index;
        change.before = bank().getStep (pattern(), index);
        // During a relative multi-step pitch move, silence stays silence.
        // Enter on the Note page toggles every selected rest individually.
        if (multiple && page == 0 && change.before.note >= 128)
            continue;
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
    const int changed = static_cast<int> (action.changes.size());
    commitAction (std::move (action));
    if (multiple)
        announce (juce::String (changed) + " selected steps changed, " + pageName());
    else
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
        reset.noteBeforePause = 60; reset.lengthBeforeLegato = 50; reset.velocityBeforeMax = 100;
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
    // Restore each step's own previous note; only never-edited rests use C4 (60).
    Action action; action.track = track; action.pattern = pattern();
    for (int index : editingSteps())
    {
        StepChange change; change.index = index;
        change.before = bank().getStep (pattern(), index);
        const int target = change.before.note >= 128 ? change.before.noteBeforePause : 128;
        bank().setStepValue (pattern(), index, lb303::SequencerLayer::note,
                             static_cast<float> (target));
        change.after = bank().getStep (pattern(), index);
        action.changes.push_back (std::move (change));
    }
    const int changed = static_cast<int> (action.changes.size());
    commitAction (std::move (action));
    if (changed > 1)
        announce (juce::String (changed) + " selected notes toggled");
    else
        announceCurrent();
}
void SequencerStepEditor::toggleCurrentLength()
{
    Action action; action.track = track; action.pattern = pattern();
    for (int index : editingSteps())
    {
        StepChange change; change.index = index;
        change.before = bank().getStep (pattern(), index);
        // Enter alternates 101 and this individual step's last ordinary length.
        const int target = change.before.length == 101
            ? change.before.lengthBeforeLegato : 101;
        bank().setStepValue (pattern(), index, lb303::SequencerLayer::length,
                             static_cast<float> (target));
        change.after = bank().getStep (pattern(), index);
        action.changes.push_back (std::move (change));
    }
    const int changed = static_cast<int> (action.changes.size());
    if (changed == 0) return;
    commitAction (std::move (action));
    if (changed > 1)
        announce (juce::String (changed) + " selected lengths toggled");
    else
        announceCurrent();
}
void SequencerStepEditor::toggleCurrentVelocity()
{
    Action action; action.track = track; action.pattern = pattern();
    for (int index : editingSteps())
    {
        StepChange change; change.index = index;
        change.before = bank().getStep (pattern(), index);
        // A maximum-velocity accent is temporary: restore this step's own
        // most recent ordinary velocity rather than a hard-coded default.
        const int target = change.before.velocity == 127
            ? change.before.velocityBeforeMax : 127;
        bank().setStepValue (pattern(), index, lb303::SequencerLayer::velocity,
                             static_cast<float> (target));
        change.after = bank().getStep (pattern(), index);
        action.changes.push_back (std::move (change));
    }
    const int changed = static_cast<int> (action.changes.size());
    if (changed == 0) return;
    commitAction (std::move (action));
    if (changed > 1)
        announce (juce::String (changed) + " selected velocities toggled");
    else
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
// The versioned system clipboard contains a SINGLE source pattern (config,
// all 128 steps and all parameter locks), never a full 16-pattern bank.
// SequencerState already has a stable serializer and a copySequenceFrom
// primitive, so stage the copied pattern in temporary slot 0 on the HEAP.
// This supports pasting between patterns, basslines and plugin instances.
juce::String SequencerStepEditor::makePatternSnapshot (const lb303::SequencerState& source,
                                                       int sourcePattern)
{
    auto scratch = std::make_unique<lb303::SequencerState>();
    scratch->copySequenceFrom (source, sourcePattern, 0);
    return scratch->serialiseToBase64 (false);
}

bool SequencerStepEditor::restorePatternSnapshot (lb303::SequencerState& destination,
                                                   int destinationPattern,
                                                   const juce::String& snapshot)
{
    auto scratch = std::make_unique<lb303::SequencerState>();
    if (! scratch->restoreFromBase64 (snapshot)) return false;
    destination.copySequenceFrom (*scratch, 0, destinationPattern);
    return true;
}

void SequencerStepEditor::copyCurrentPattern (bool cut)
{
    const int sourcePattern = pattern();
    const auto snapshot = makePatternSnapshot (bank(), sourcePattern);
    juce::SystemClipboard::copyTextToClipboard (patternClipboardHeader + snapshot);
    if (! cut)
    {
        announce ("Bassline " + juce::String (track + 1) + ", Pattern "
                  + juce::String (sourcePattern + 1) + " copied");
        return;
    }

    Action action;
    action.track = track;
    action.pattern = sourcePattern;
    action.hasWholePattern = true;
    action.beforePattern = snapshot;
    bank().resetSequence (sourcePattern);
    action.afterPattern = makePatternSnapshot (bank(), sourcePattern);
    selectedSteps.fill (false);
    currentStep = 0;
    lastPattern = -1;
    commitAction (std::move (action));
    announce ("Bassline " + juce::String (track + 1) + ", Pattern "
              + juce::String (sourcePattern + 1) + " cut");
}

void SequencerStepEditor::pasteCurrentPattern()
{
    const juce::String text = juce::SystemClipboard::getTextFromClipboard();
    if (! text.startsWith (patternClipboardHeader) || text.length() > 4 * 1024 * 1024)
    {
        announce ("No LB-303 pattern in clipboard");
        return;
    }
    const auto payload = text.substring (patternClipboardHeader.length()).trim();

    // Validate clipboard before touching the destination, avoiding partial
    // updates with corrupt data. The large atomic bank lives on the heap.
    auto staged = std::make_unique<lb303::SequencerState>();
    if (! staged->restoreFromBase64 (payload))
    {
        announce ("Invalid LB-303 pattern clipboard");
        return;
    }

    const int destinationPattern = pattern();
    Action action;
    action.track = track;
    action.pattern = destinationPattern;
    action.hasWholePattern = true;
    action.beforePattern = makePatternSnapshot (bank(), destinationPattern);
    if (action.beforePattern == payload)
    {
        announce ("Bassline " + juce::String (track + 1) + ", Pattern "
                  + juce::String (destinationPattern + 1) + " already identical");
        return;
    }

    bank().copySequenceFrom (*staged, 0, destinationPattern);
    action.afterPattern = makePatternSnapshot (bank(), destinationPattern);
    selectedSteps.fill (false);
    currentStep = 0;
    lastPattern = -1;
    commitAction (std::move (action));
    announce ("Bassline " + juce::String (track + 1) + ", Pattern "
              + juce::String (destinationPattern + 1) + " pasted");
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
    if (! juce::isPositiveAndBelow (local, static_cast<int> (lb303::voiceLocalParameterCount)))
        return juce::String (count) + " parameter locks";
    const auto catalogIndex = lb303::voiceParameterGlobalIndex (static_cast<size_t> (track),
                                                   static_cast<size_t> (local));
    return juce::String (index + 1) + " of " + juce::String (count) + ", "
        + lb303::catalog()[catalogIndex].name + " "
        + lb303::parameterText (catalogIndex, lock.value);
}
void SequencerStepEditor::openParameterPicker()
{
    if (page != 6 || pickerOpen) return;
    pickerOpen = true;
    // Restore the last cursor for THIS bassline, even across editor open/close.
    pickerIndex = lastPickerPosition[static_cast<size_t> (track)];
    renderParameterPicker();
    announce ("Select Parameter. Bassline " + juce::String (track + 1)
        + ", Step " + juce::String (currentStep + 1) + ". "
        + "Use arrows, Enter to assign and close, Alt+Enter to toggle and stay, Escape to return. "
        + pickerItemText (pickerIndex));
}
void SequencerStepEditor::closeParameterPicker()
{
    if (! pickerOpen) return;
    lastPickerPosition[static_cast<size_t> (track)] = pickerIndex;
    pickerOpen = false;
    lastRevision = 0;
    refresh();
    announce ("Parameters. Step " + juce::String (currentStep + 1) + ", "
        + currentLockText());
}
juce::String SequencerStepEditor::pickerItemText (int localIndex) const
{
    const auto local = lb303::displayToLocal (static_cast<size_t> (track),
                                               static_cast<size_t> (localIndex));
    const auto index = lb303::voiceParameterGlobalIndex (static_cast<size_t> (track), local);
    const bool assigned = bank().findStepParameterLock (pattern(), currentStep,
                                                        static_cast<int> (local) + 1) >= 0;
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
    const int count = static_cast<int> (lb303::voiceVisibleParameterCount);
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
    const int local = static_cast<int> (lb303::displayToLocal (
        static_cast<size_t> (track), static_cast<size_t> (pickerIndex)));
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
        const auto index = lb303::voiceParameterGlobalIndex (static_cast<size_t> (track),
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
    if (! juce::isPositiveAndBelow (local, static_cast<int> (lb303::voiceLocalParameterCount))) return;
    const auto& info = lb303::catalog()[lb303::voiceParameterGlobalIndex (static_cast<size_t> (track),
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
    if (! juce::isPositiveAndBelow (local, static_cast<int> (lb303::voiceLocalParameterCount))) return;
    const float initial = lb303::catalog()[lb303::voiceParameterGlobalIndex (
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
    const int count = static_cast<int> (lb303::voiceVisibleParameterCount);
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
                const auto index = lb303::voiceParameterGlobalIndex (static_cast<size_t> (track),
                    lb303::displayToLocal (static_cast<size_t> (track),
                                           static_cast<size_t> (candidate)));
                const auto& name = lb303::catalog()[index].name;
                if (name.isNotEmpty() && juce::CharacterFunctions::toLowerCase (name[0]) == ch)
                { target = candidate; break; }
            }
        }
    }
    if (target != pickerIndex)
    {
        pickerIndex = target;
        lastPickerPosition[static_cast<size_t> (track)] = pickerIndex;
        announcePickerItem();
    }
    return true;
}

// A Song row holds one of this lane's sixteen patterns, NOT a copy of its steps.
// Selections can be discontinuous; Ctrl+C/X/V always compacts selected rows
// in ascending song order. Cursors and selections are runtime editor state.
void SequencerStepEditor::renderSong()
{
    lastSongRevision = processor.songs.getRevision();
    const auto a = processor.songs.column (0);
    const auto b = processor.songs.column (1);
    juce::String text = "LB-303 SONG — two independent bassline columns\n";
    text << "Bassline 1: " << (int) a->size() << " items, "
         << (processor.songs.loop (0) ? "Loop On" : "Loop Off")
         << "     Bassline 2: " << (int) b->size() << " items, "
         << (processor.songs.loop (1) ? "Loop On" : "Loop Off") << "\n";
    text << "Song playback: " << (processor.songs.songMode() ? "Song" : "Pattern")
         << "   Active column: Bassline " << (track + 1) << "\n\n";
    for (int n = 0; n < 17; ++n)
    {
        for (int lane = 0; lane < 2; ++lane)
        {
            const auto& v = lane == 0 ? *a : *b;
            const int start = juce::jmax (0, songCursor[static_cast<size_t> (lane)] - 8);
            const int i = start + n;
            juce::String cell;
            if (i < static_cast<int> (v.size()))
            {
                const bool selected = songSelected[static_cast<size_t> (lane)].count (i) != 0;
                cell << (songCursor[static_cast<size_t> (lane)] == i ? "> " : "  ")
                     << (selected ? "* " : "  ") << (i + 1) << ": P" << (v[static_cast<size_t> (i)] + 1);
            }
            if (lane == 0) text << cell.paddedRight (' ', 30) << " | ";
            else text << cell;
        }
        text << "\n";
    }
    text << "\nLeft/Right: remembered columns | Up/Down: item | PgUp/PgDn: five | Home/End: first/last"
         << "\nShift+Up/Down: select range | Shift+Space: toggle item | Ctrl+Shift+Up/Down: add range"
         << "\nCtrl+A: whole column | Ctrl+C/X/V: compact copy/cut/insert | Ctrl+Z/Shift+Z: undo/redo"
         << "\nAlt+I: insert | Delete: delete | Ctrl+Delete: clear with confirmation"
         << "\nCtrl+Up/Down: change P1–P16 | Alt+L: lane loop | Alt+M: Song/Pattern preview | Alt+Shift+M: audition focused pattern"
         << "\nAlt+R: Song/Pattern preview | Tab: full row details | Alt+S: Steps editor"
         << "\nAlt+P: Space target | Space: play/stop preview (when routed to Sequencer) | Esc: close";
    if (songConfirmClear) text << "\nConfirm clearing entire song column with Ctrl+Delete or Enter. Esc cancels.";
    setText (text, juce::dontSendNotification);
}

void SequencerStepEditor::announceSong (bool details)
{
    const auto lane = static_cast<size_t> (track);
    const auto column = processor.songs.column (track);
    if (column->empty())
    {
        announce (details ? "Song, Bassline " + juce::String (track + 1) + ", empty column, "
            + (processor.songs.loop (track) ? "Loop On" : "Loop Off") : "Empty column");
        return;
    }
    songCursor[lane] = juce::jlimit (0, static_cast<int> (column->size()) - 1, songCursor[lane]);
    const int cursor = songCursor[lane];
    const auto count = songSelected[lane].size();
    const bool marked = songSelected[lane].count (cursor) != 0;
    juce::String message;
    if (details)
        message << "Song, Bassline " << (track + 1) << ", element " << (cursor + 1)
                << " of " << static_cast<int> (column->size()) << ", Pattern "
                << (column->at (static_cast<size_t> (cursor)) + 1) << ", "
                << (processor.songs.loop (track) ? "Loop On" : "Loop Off")
                << ", " << (processor.songs.songMode() ? "Song playback" : "Pattern playback");
    else
        message << (marked ? "Selected " : "") << "P"
                << (column->at (static_cast<size_t> (cursor)) + 1)
                << ", " << (cursor + 1) << " of " << static_cast<int> (column->size());
    if (details && count > 0) message << ", " << static_cast<int> (count) << " selected";
    announce (message);
}

void SequencerStepEditor::changeSongCursor (int difference, bool extend, bool)
{
    const auto lane = static_cast<size_t> (track);
    const auto data = processor.songs.column (track);
    if (data->empty()) return;
    const int old = songCursor[lane];
    const int next = juce::jlimit (0, static_cast<int> (data->size()) - 1, old + difference);
    if (next == old) return; // silent boundaries
    if (extend)
    {
        if (songShiftAnchor[lane] < 0)
        {
            songShiftAnchor[lane] = old;
            songShiftBaseline[lane] = songSelected[lane];
            if (songShiftBaseline[lane].empty()) songShiftBaseline[lane].insert (old);
        }
        songSelected[lane] = songShiftBaseline[lane];
        for (int i = juce::jmin (songShiftAnchor[lane], next);
             i <= juce::jmax (songShiftAnchor[lane], next); ++i)
            songSelected[lane].insert (i);
    }
    else
        songShiftAnchor[lane] = -1; // keep the selection until copy/cut/explicit toggle
    songCursor[lane] = next;
    renderSong();
    announceSong();
}

void SequencerStepEditor::applySongEdit (lb303::SongArrangement::Column data)
{
    const auto before = processor.songs.column (track);
    if (*before == data) return;
    const int lane = track;
    const bool loop = processor.songs.loop (lane);
    processor.songs.replace (lane, std::move (data));
    songUndo.push_back ({ lane, before, processor.songs.column (lane), loop, loop });
    if (songUndo.size() > 64) songUndo.erase (songUndo.begin());
    songRedo.clear();
    const auto snapshot = processor.songs.column (lane);
    if (! snapshot->empty())
        songCursor[static_cast<size_t> (lane)] = juce::jlimit (0, static_cast<int> (snapshot->size()) - 1,
                                                              songCursor[static_cast<size_t> (lane)]);
    else songCursor[static_cast<size_t> (lane)] = 0;
    songSelected[static_cast<size_t> (lane)].clear();
    songShiftAnchor[static_cast<size_t> (lane)] = -1;
    renderSong();
}

void SequencerStepEditor::songUndoRedo (bool redo)
{
    auto& from = redo ? songRedo : songUndo;
    auto& to = redo ? songUndo : songRedo;
    if (from.empty()) { announce ("Nothing to " + juce::String (redo ? "redo" : "undo")); return; }
    auto action = from.back();
    from.pop_back();
    processor.songs.restoreSnapshot (action.lane, redo ? action.after : action.before);
    processor.songs.setLoop (action.lane, redo ? action.afterLoop : action.beforeLoop);
    to.push_back (std::move (action));
    track = to.back().lane;
    auto snapshot = processor.songs.column (track);
    auto& cursor = songCursor[static_cast<size_t> (track)];
    cursor = snapshot->empty() ? 0 : juce::jlimit (0, static_cast<int> (snapshot->size()) - 1, cursor);
    songSelected[static_cast<size_t> (track)].clear();
    songShiftAnchor[static_cast<size_t> (track)] = -1;
    renderSong();
    announce (redo ? "Song edit redone" : "Song edit undone");
}

bool SequencerStepEditor::handleSongKey (const juce::KeyPress& key)
{
    const auto code = key.getKeyCode();
    const auto mods = key.getModifiers();
    const auto ch = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());
    const auto letter = [ch, code] (juce::juce_wchar c)
    { return ch == c || code == c || code == juce::CharacterFunctions::toUpperCase (c); };
    auto data = *processor.songs.column (track);
    const auto lane = static_cast<size_t> (track);
    auto& cursor = songCursor[lane];
    if (songConfirmClear)
    {
        if (code == juce::KeyPress::escapeKey) { songConfirmClear = false; announce ("Clear cancelled"); renderSong(); return true; }
        if (code == juce::KeyPress::returnKey || (mods.isCtrlDown() && code == juce::KeyPress::deleteKey))
        {
            songConfirmClear = false;
            applySongEdit ({});
            announce ("Bassline " + juce::String (track + 1) + ", song column cleared");
            return true;
        }
        return true;
    }
    if (code == juce::KeyPress::escapeKey) { close(); return true; }
    if (code == juce::KeyPress::tabKey) { announceSong (true); return true; }
    // Shift+Space alone toggles explicit Song row selection, even one row.
    // Ctrl+Space is deliberately not a selection shortcut; bare Space is transport.
    const auto toggleSongRow = [&]()
    {
        if (data.empty()) return;
        const bool select = songSelected[lane].count (cursor) == 0;
        if (select) songSelected[lane].insert (cursor);
        else songSelected[lane].erase (cursor);
        songShiftAnchor[lane] = -1;
        renderSong();
        announce (juce::String (select ? "Selected P" : "Deselected P")
                  + juce::String (data[static_cast<size_t> (cursor)] + 1)
                  + ", " + juce::String (cursor + 1) + " of " + juce::String (data.size()));
    };
    if (code == juce::KeyPress::spaceKey && mods.isShiftDown()
        && ! mods.isCtrlDown() && ! mods.isAltDown())
    { toggleSongRow(); return true; }
    if (mods.isAltDown())
    {
        if (letter ('l'))
        {
            const bool before = processor.songs.loop (track);
            processor.songs.setLoop (track, ! before);
            songUndo.push_back ({ track, processor.songs.column (track),
                                  processor.songs.column (track), before, ! before });
            if (songUndo.size() > 64) songUndo.erase (songUndo.begin());
            songRedo.clear(); renderSong();
            announce ("Bassline " + juce::String (track + 1) + (before ? ", Loop Off" : ", Loop On"));
            return true;
        }
        if (letter ('r'))
        {
            const bool next = ! processor.songs.songMode();
            processor.endSongAudition();
            processor.songs.setSongMode (next);
            processor.restartSongPreviewClock(); renderSong();
            announce (next ? "Song playback" : "Pattern playback"); return true;
        }
        if (letter ('i'))
        {
            const int p = data.empty() ? 0 : data[static_cast<size_t> (cursor)];
            const auto pos = data.empty() ? data.begin() : data.begin() + cursor + 1;
            const int newIndex = data.empty() ? 0 : cursor + 1;
            data.insert (pos, p);
            applySongEdit (std::move (data));
            cursor = newIndex; renderSong(); announce ("Inserted P" + juce::String (p + 1)
                + ", " + juce::String (cursor + 1)); return true;
        }
        if (letter ('m') && mods.isShiftDown())
        {
            if (data.empty()) return true;
            processor.auditionSongPattern (track, data[static_cast<size_t> (cursor)]);
            announce (processor.isPreviewPlaying() ? "Audition P" + juce::String (data[static_cast<size_t> (cursor)] + 1)
                                                   : "Pattern audition stopped");
            return true;
        }
        return true;
    }
    if (mods.isCtrlDown())
    {
        // Same Ctrl+Up/Down pattern selection as in Alt+Q Steps. Change only
        // the current song ROW; all other occurrences of this pattern remain.
        if (! mods.isShiftDown() && ! mods.isAltDown() && (code == juce::KeyPress::upKey
                                      || code == juce::KeyPress::downKey))
        {
            if (data.empty()) return true;
            const int old = data[static_cast<size_t> (cursor)];
            const int next = juce::jlimit (0, 15, old + (code == juce::KeyPress::upKey ? 1 : -1));
            if (next == old) return true; // silent boundary
            data[static_cast<size_t> (cursor)] = next;
            applySongEdit (std::move (data));
            announceSong();
            return true;
        }
        if (code == juce::KeyPress::deleteKey)
        {
            if (data.empty()) return true;
            songConfirmClear = true; renderSong();
            announce ("Clear entire Bassline " + juce::String (track + 1)
                    + " song column? Enter or Ctrl Delete confirms, Escape cancels");
            return true;
        }
        if (letter ('z') || letter ('y')) { songUndoRedo (letter ('y') || mods.isShiftDown()); return true; }
        if (letter ('a'))
        {
            songSelected[lane].clear();
            for (int i = 0; i < static_cast<int> (data.size()); ++i) songSelected[lane].insert (i);
            songShiftAnchor[lane] = -1; renderSong();
            announce (juce::String (data.size()) + " song elements selected"); return true;
        }
        if ((code == juce::KeyPress::upKey || code == juce::KeyPress::downKey) && mods.isShiftDown())
        {
            changeSongCursor (code == juce::KeyPress::upKey ? -1 : 1, true, true);
            return true;
        }
        if (letter ('c') || letter ('x'))
        {
            songClipboard.clear();
            if (data.empty()) return true;
            const auto& selection = songSelected[lane];
            if (! selection.empty())
                for (int i : selection) if (i >= 0 && i < static_cast<int> (data.size()))
                    songClipboard.push_back (data[static_cast<size_t> (i)]);
            else songClipboard.push_back (data[static_cast<size_t> (cursor)]);
            juce::String text = songClipboardHeader;
            for (int p : songClipboard) text << "," << (p + 1);
            juce::SystemClipboard::copyTextToClipboard (text);
            const int copied = static_cast<int> (songClipboard.size());
            if (letter ('x'))
            {
                if (! selection.empty())
                {
                    for (auto it = selection.rbegin(); it != selection.rend(); ++it)
                        if (*it >= 0 && *it < static_cast<int> (data.size())) data.erase (data.begin() + *it);
                }
                else data.erase (data.begin() + cursor);
                applySongEdit (std::move (data));
            }
            else { songSelected[lane].clear(); songShiftAnchor[lane] = -1; renderSong(); }
            announce (juce::String (copied) + (letter ('x') ? " song elements cut" : " song elements copied"));
            return true;
        }
        if (letter ('v'))
        {
            const auto clipboardText = juce::SystemClipboard::getTextFromClipboard().trim();
            if (! clipboardText.startsWith (songClipboardHeader))
            { announce ("No LB-303 Song elements in clipboard"); return true; }
            const auto values = juce::StringArray::fromTokens (
                clipboardText.substring (songClipboardHeader.length()), ",", "");
            std::vector<int> paste;
            for (const auto& token : values)
            {
                if (token.isEmpty()) continue;
                const int p = token.getIntValue();
                if (p < 1 || p > 16 || ! token.trim().containsOnly ("0123456789"))
                { announce ("Invalid Song clipboard"); return true; }
                paste.push_back (p - 1);
            }
            if (paste.empty()) return true;
            const int index = data.empty() ? 0 : cursor + 1;
            data.insert (data.begin() + index, paste.begin(), paste.end());
            applySongEdit (std::move (data));
            cursor = index;
            renderSong(); announce (juce::String (paste.size()) + " song elements inserted"); return true;
        }
        return true;
    }
    if (code == juce::KeyPress::leftKey || code == juce::KeyPress::rightKey)
    {
        const int to = juce::jlimit (0, 1, track + (code == juce::KeyPress::rightKey ? 1 : -1));
        if (track == to) return true;
        songShiftAnchor[lane] = -1;
        track = to; renderSong(); announceSong (true); return true;
    }
    if (code == juce::KeyPress::upKey || code == juce::KeyPress::downKey)
    {
        changeSongCursor (code == juce::KeyPress::upKey ? -1 : 1, mods.isShiftDown(), false);
        return true;
    }
    if (code == juce::KeyPress::pageUpKey || code == juce::KeyPress::pageDownKey)
    { changeSongCursor (code == juce::KeyPress::pageUpKey ? -5 : 5, mods.isShiftDown(), false); return true; }
    if (code == juce::KeyPress::homeKey || code == juce::KeyPress::endKey)
    {
        if (data.empty()) return true;
        changeSongCursor ((code == juce::KeyPress::homeKey ? 0 : static_cast<int> (data.size()) - 1) - cursor,
                          mods.isShiftDown(), false);
        return true;
    }
    if (code == juce::KeyPress::returnKey)
    {
        // Enter no longer opens a second selector. Ctrl+Up/Down edits P1..P16
        // directly; Enter reads details just like Tab.
        announceSong (true);
        return true;
    }
    if (code == juce::KeyPress::deleteKey)
    {
        if (data.empty()) return true;
        auto& selection = songSelected[lane];
        if (selection.size() == data.size() && ! data.empty())
        {
            songConfirmClear = true;
            announce ("Delete all song elements? Enter confirms, Escape cancels"); return true;
        }
        const int n = ! selection.empty() ? static_cast<int> (selection.size()) : 1;
        if (! selection.empty())
        {
            for (auto it = selection.rbegin(); it != selection.rend(); ++it)
                if (*it >= 0 && *it < static_cast<int> (data.size())) data.erase (data.begin() + *it);
        }
        else data.erase (data.begin() + cursor);
        applySongEdit (std::move (data));
        announce (juce::String (n) + " song elements deleted"); return true;
    }
    return true;
}

bool SequencerStepEditor::keyPressed (const juce::KeyPress& key)
{
    if (! opened) return juce::Label::keyPressed (key);
    // Alt+Q closes the entire editor exactly like Esc, including from the picker.
    if (key.getModifiers().isAltDown()
        && (juce::CharacterFunctions::toLowerCase (key.getTextCharacter()) == 'q'
            || key.getKeyCode() == 'Q' || key.getKeyCode() == 'q'))
    { close(); return true; }
    const int code = key.getKeyCode();
    const auto mods = key.getModifiers();
    const auto character = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());
    if (mods.isAltDown() && ! mods.isCtrlDown() && ! mods.isCommandDown()
        && (character == 'p' || code == 'p' || code == 'P'))
    {
        const bool toPreview = processor.toggleSpaceTarget();
        lastRevision = 0;
        lastPattern = -1; // refresh transport hint even when pattern data is unchanged
        refresh();
        announce (toPreview ? "Space: Sequencer preview" : "Space: DAW");
        return true;
    }
    if (code == juce::KeyPress::spaceKey && ! mods.isAltDown()
        && ! mods.isCtrlDown() && ! mods.isCommandDown() && ! mods.isShiftDown())
    {
        if (! processor.spaceControlsSequencerPreview())
            return false; // leave Space to REAPER in default DAW mode
        const bool running = processor.toggleSequencerPreview();
        announce (running ? "Both basslines preview playing" : "Both basslines preview stopped");
        return true;
    }
    // Alt+M always chooses between Pattern and Song preview, regardless of
    // whether Steps, Song, or the Parameter Lock picker currently has focus.
    if (mods.isAltDown() && ! mods.isCtrlDown() && ! mods.isCommandDown()
        && ! mods.isShiftDown()
        && (character == 'm' || code == 'm' || code == 'M'))
    {
        const bool song = ! processor.songs.songMode();
        processor.endSongAudition();
        processor.songs.setSongMode (song);
        processor.restartSongPreviewClock();
        lastRevision = 0;
        lastPattern = -1;
        lastSongRevision = 0;
        refresh();
        announce (song ? "Sequencer preview: Song" : "Sequencer preview: Pattern");
        return true;
    }
    // Alt+S inside Alt+Q changes editor mode, never opens preset Save As.
    if (mods.isAltDown() && ! mods.isCtrlDown() && ! mods.isCommandDown()
        && (character == 's' || code == 's' || code == 'S'))
    {
        songView = ! songView;
        songConfirmClear = false;
        pickerOpen = false;
        if (songView)
        {
            processor.songs.setSongMode (true);
            processor.restartSongPreviewClock();
        }
        lastRevision = 0;
        refresh();
        announce (songView ? "Song editor. Bassline " + juce::String (track + 1)
                    : "Step editor. Bassline " + juce::String (track + 1));
        if (songView) announceSong (true);
        return true;
    }
    if (songView) return handleSongKey (key);
    // Whole-pattern clipboard shortcuts have priority even with the parameter
    // picker open. Ctrl+C/X/V without Shift still edit just selected steps.
    if (mods.isCtrlDown() && mods.isShiftDown() && ! mods.isAltDown())
    {
        const auto pressed = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());
        const auto chord = [code, pressed] (juce::juce_wchar ch)
        { return pressed == ch || code == juce::CharacterFunctions::toUpperCase (ch); };
        if (chord ('c')) { copyCurrentPattern (false); return true; }
        if (chord ('x')) { copyCurrentPattern (true); return true; }
        if (chord ('v')) { pasteCurrentPattern(); return true; }
    }
    if (pickerOpen) return handlePickerKey (key);
    // TEST2.4.29: Delete clears the entire explicit step selection, without
    // changing any note, value, pattern, or Undo history. Song Editor has its
    // own Delete handler above, so deleting Song entries is unaffected.
    // On the Parameters page, Delete with no selection keeps its old purpose
    // of deleting the selected Parameter Lock.
    if (code == juce::KeyPress::deleteKey && ! mods.isAnyModifierKeyDown())
    {
        const bool anySelected = std::any_of (selectedSteps.begin(), selectedSteps.end(),
                                              [] (bool selected) { return selected; });
        if (anySelected)
        {
            selectedSteps.fill (false);
            lastStepKey = -1;
            lastPressTime = 0.0;
            lastRevision = 0;
            refresh();
            announce ("All steps deselected");
            return true;
        }
        if (page != 6) return true; // Nothing to deselect: no step data is deleted.
    }
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
        if (! mods.isAltDown() && ! mods.isShiftDown()
            && (letter ('a') || code == 'a' || key.getTextCharacter() == 1))
        { selectVisibleSteps(); return true; }
        if (code == juce::KeyPress::upKey) { changePattern (1); return true; }
        if (code == juce::KeyPress::downKey) { changePattern (-1); return true; }
        if (letter ('c')) { copySteps (false); return true; }
        if (letter ('x')) { copySteps (true); return true; }
        if (letter ('v')) { pasteSteps(); return true; }
        if (letter ('z')) { undo (mods.isShiftDown()); return true; }
        if (letter ('y')) { undo (true); return true; }
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
        else if (page == 1) toggleCurrentLength();
        else if (page == 2) toggleCurrentVelocity();
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
            processor.editorSession.sequencerPage = page;
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
