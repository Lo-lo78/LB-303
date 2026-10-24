// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>

class LB303AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit LB303AudioProcessorEditor (LB303AudioProcessor&);
    ~LB303AudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct ParameterCell final : juce::TextButton
    {
        ParameterCell (LB303AudioProcessorEditor&, int index);
        bool keyPressed (const juce::KeyPress&) override;
        void focusGained (FocusChangeType) override;
        LB303AudioProcessorEditor& parent;
        int index;
    };
    LB303AudioProcessor& processor;
    std::array<std::unique_ptr<ParameterCell>, lb303::parameterCount> cells;
    int selected = 0;
    bool updating = false;
    void timerCallback() override;
    void refresh (bool force = false);
    bool handleKey (const juce::KeyPress&, int index);
    void focusIndex (int index);
    void moveValue (int index, int direction, bool large = false);
    void setValue (int index, float plain);
    void announce (juce::Component&, const juce::String&);
    void focusChanged (int index, juce::Component&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LB303AudioProcessorEditor)
};
