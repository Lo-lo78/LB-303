// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include <array>
#include <functional>
#include <vector>
#include <utility>

// Keyboard-centric overlay derived from the LJuno step editor. Its 16-key
// matrix is an accessible focused Label, not 128 focusable buttons.
class SequencerStepEditor final : public juce::Label
{
public:
    explicit SequencerStepEditor (LB303AudioProcessor&);
    void open (int track, juce::Component* returnFocus);
    void close();
    bool isOpen() const noexcept { return opened; }
    bool keyPressed (const juce::KeyPress&) override;
    void focusGained (FocusChangeType) override;
    void refresh();

private:
    struct StepChange
    {
        int index = 0;
        lb303::SequencerStep before, after;
    };
    struct Action
    {
        int track = 0;
        int pattern = 0;
        std::vector<StepChange> changes;
        bool hasLaunch = false;
        int launchBefore = 1, launchAfter = 1;
    };
    LB303AudioProcessor& processor;
    juce::Component::SafePointer<juce::Component> returnControl;
    bool opened = false;
    int track = 0;
    int page = 0; // 0..4 steps; 5 Launch Step; 6 Parameter Locks (TEST2.3)
    int currentStep = 0;
    int valueStepIndex = 0;
    int lastStepKey = -1;
    double lastPressTime = 0.0;
    std::array<bool, lb303::SequencerState::stepsPerSequence> selectedSteps {};
    std::vector<lb303::SequencerStep> clipboard;
    std::vector<Action> undoStack, redoStack;
    bool preview = false;
    bool pickerOpen = false;
    int pickerIndex = 0; // local parameter index (0..36), per current bassline
    int selectedLockIndex = 0; // current step's lock slot, 0..31
    std::uint64_t lastRevision = 0;
    int lastPattern = -1;

    int pattern() const noexcept { return processor.currentPattern (static_cast<size_t> (track)); }
    lb303::SequencerState& bank() noexcept { return processor.patternBank (static_cast<size_t> (track)); }
    const lb303::SequencerState& bank() const noexcept { return processor.patternBank (static_cast<size_t> (track)); }
    juce::String pageName() const;
    juce::String valueText (int step) const;
    void announce (const juce::String&);
    void announceCurrent();
    void selectStep (int inBlock);
    std::pair<int, int> activeStepBounds() const;
    void changeBlock (int direction);
    void changeActiveRange (bool editStart, int direction);
    void changeTrack (int direction);
    void changePattern (int direction);
    void modifyValue (int direction, int amount);
    void resetCurrent();
    void toggleCurrentNote();
    std::vector<int> editingSteps() const;
    void commitAction (Action&&);
    void applyAction (const Action&, bool backwards);
    void undo (bool redo);
    void copySteps (bool cut);
    void pasteSteps();
    void changeLaunch (int direction, int amount);
    juce::String currentLockText() const;
    void openParameterPicker();
    void closeParameterPicker();
    juce::String pickerItemText (int localIndex) const;
    void renderParameterPicker();
    void announcePickerItem();
    bool handlePickerKey (const juce::KeyPress&);
    void selectPickerParameter (bool toggle);
    void editSelectedLock (int direction, int amount);
    void changeSelectedLock (int direction);
    void deleteSelectedLock (bool all);
    void resetSelectedLock();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerStepEditor)
};
