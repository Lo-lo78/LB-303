// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <memory>

class LB303AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit LB303AudioProcessorEditor (LB303AudioProcessor&);
    ~LB303AudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    // LJuno-style: ONE accessible parameter ComboBox, not 35 focusable buttons.
    struct ParameterGrid final : juce::ComboBox
    {
        explicit ParameterGrid (LB303AudioProcessorEditor& owner) : editor (owner) {}
        bool keyPressed (const juce::KeyPress&) override;
        void focusGained (FocusChangeType) override;
        void setEntryAccessibility();
        void setLineReadingMode();
        LB303AudioProcessorEditor& editor;
    };

    struct ParameterValue final : juce::Slider
    {
        explicit ParameterValue (LB303AudioProcessorEditor& owner) : editor (owner) {}
        bool keyPressed (const juce::KeyPress&) override;
        void focusGained (FocusChangeType) override;
        void setEntryAccessibility();
        void setLineReadingMode();
        LB303AudioProcessorEditor& editor;
    };

    struct NumericEdit final : juce::TextEditor
    {
        explicit NumericEdit (LB303AudioProcessorEditor& owner)
            : juce::TextEditor ("Edit"), editor (owner) {}
        bool keyPressed (const juce::KeyPress&) override;
        void focusGained (FocusChangeType) override;
        LB303AudioProcessorEditor& editor;
    };

    enum class Control { grid, value, edit };
    LB303AudioProcessor& processor;
    ParameterGrid parameterGrid { *this };
    ParameterValue parameterValue { *this };
    NumericEdit numericEdit { *this };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> valueAttachment;
    juce::Rectangle<int> drawnGrid;
    int selected = 0;
    int stepWidthIndex = 0; // LJuno: 1 / 5 / 10 / 15 / 20, saved with the project.
    bool changingParameter = false;
    bool editDirty = false;

    void timerCallback() override;
    void refreshLabels (bool force = false);
    void changeSelection (int index);
    void selectParameter();
    void moveInGrid (int row, int column);
    void changeValue (int direction, int speed = 1);
    void changeStepWidth (int direction);
    void setValue (float value);
    void setBoundary (bool maximum);
    void resetParameter();
    void enterEdit();
    bool commitEdit();
    void cancelEdit();
    void focusControl (Control, bool defer = false);
    bool handleKey (const juce::KeyPress&, Control);
    void announceUnavailableSequencer();
    juce::String labelFor (int index) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LB303AudioProcessorEditor)
};
