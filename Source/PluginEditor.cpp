// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginEditor.h"
#include "ScreenReaderAnnouncer.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
constexpr int rowsPerColumn = 8;
constexpr int numberOfColumns = (static_cast<int> (lb303::parameterCount) + rowsPerColumn - 1) / rowsPerColumn;
constexpr int gridGap = 7;
constexpr int valuePageStep = 40;
constexpr int stepWidths[] { 1, 5, 10, 15, 20 };
const juce::Identifier stepWidthState { "editorStepWidthIndex" };
}

void LB303AudioProcessorEditor::ParameterGrid::setLineReadingMode()
{
    // As in LJuno: selected ComboBox item contains "parameter, value".
    // No persistent "Grid" prefix or repeated shortcut when reading the line.
    setTitle (juce::String());
    setDescription (juce::String());
}
void LB303AudioProcessorEditor::ParameterGrid::setEntryAccessibility()
{
    setTitle ("Grid");
    setDescription ("Alt+L");
}
void LB303AudioProcessorEditor::ParameterGrid::focusGained (FocusChangeType cause)
{
    setEntryAccessibility();
    juce::ComboBox::focusGained (cause);
    juce::Timer::callAfterDelay (1, [safe = juce::Component::SafePointer<ParameterGrid> (this)]
    {
        if (safe != nullptr && safe->hasKeyboardFocus (true))
            if (auto* handler = safe->getAccessibilityHandler()) handler->grabFocus();
    });
    juce::Timer::callAfterDelay (220, [safe = juce::Component::SafePointer<ParameterGrid> (this)]
    {
        if (safe != nullptr) safe->setLineReadingMode();
    });
}
bool LB303AudioProcessorEditor::ParameterGrid::keyPressed (const juce::KeyPress& k)
{
    return editor.handleKey (k, Control::grid) || juce::ComboBox::keyPressed (k);
}

void LB303AudioProcessorEditor::ParameterValue::setLineReadingMode()
{
    const auto name = juce::String (lb303::catalog()[static_cast<size_t> (editor.selected)].name);
    setName (name);
    setTitle (name);
    setDescription (juce::String());
}
void LB303AudioProcessorEditor::ParameterValue::setEntryAccessibility()
{
    const auto name = juce::String (lb303::catalog()[static_cast<size_t> (editor.selected)].name);
    setName ("Value. " + name);
    setTitle ("Value. " + name);
    setDescription ("Alt+V");
}
void LB303AudioProcessorEditor::ParameterValue::focusGained (FocusChangeType cause)
{
    setEntryAccessibility();
    juce::Slider::focusGained (cause);
    juce::Timer::callAfterDelay (150, [safe = juce::Component::SafePointer<ParameterValue> (this)]
    {
        if (safe != nullptr) safe->setLineReadingMode();
    });
}
bool LB303AudioProcessorEditor::ParameterValue::keyPressed (const juce::KeyPress& k)
{
    return editor.handleKey (k, Control::value) || juce::Slider::keyPressed (k);
}

void LB303AudioProcessorEditor::NumericEdit::focusGained (FocusChangeType why)
{
    setTitle ("Edit. " + juce::String (lb303::catalog()[static_cast<size_t> (editor.selected)].name));
    setDescription ("Alt+E");
    juce::TextEditor::focusGained (why);
    editor.editDirty = true;
    selectAll();
    juce::Timer::callAfterDelay (1, [safe = juce::Component::SafePointer<NumericEdit> (this)]
    {
        if (safe != nullptr && safe->hasKeyboardFocus (true))
            if (auto* handler = safe->getAccessibilityHandler()) handler->grabFocus();
    });
    juce::Timer::callAfterDelay (220, [safe = juce::Component::SafePointer<NumericEdit> (this)]
    {
        if (safe != nullptr) safe->setDescription (juce::String());
    });
}
bool LB303AudioProcessorEditor::NumericEdit::keyPressed (const juce::KeyPress& k)
{
    // Handle Alt shortcuts before the TextEditor can insert a letter.
    return editor.handleKey (k, Control::edit) || juce::TextEditor::keyPressed (k);
}

LB303AudioProcessorEditor::LB303AudioProcessorEditor (LB303AudioProcessor& p)
    : juce::AudioProcessorEditor (p), processor (p)
{
    // Do NOT transfer focus when REAPER constructs or displays the editor.
    // Loading the FX must leave the host FX Chain as the keyboard context.
    setWantsKeyboardFocus (false);
    stepWidthIndex = juce::jlimit (0, 4, static_cast<int> (
        processor.parameters.state.getProperty (stepWidthState, 0)));

    parameterGrid.setTextWhenNothingSelected ("Select parameter");
    parameterGrid.setLineReadingMode();
    parameterGrid.setExplicitFocusOrder (1);
    parameterGrid.onChange = [this] { selectParameter(); };
    for (int i = 0; i < static_cast<int> (lb303::parameterCount); ++i)
        parameterGrid.addItem (labelFor (i), i + 1);
    parameterGrid.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (parameterGrid);

    parameterValue.setSliderStyle (juce::Slider::LinearHorizontal);
    parameterValue.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    parameterValue.setWantsKeyboardFocus (true);
    parameterValue.setExplicitFocusOrder (2);
    parameterValue.setLineReadingMode();
    parameterValue.onValueChange = [this]
    {
        if (changingParameter) return;
        refreshLabels();
        if (! numericEdit.hasKeyboardFocus (true))
        {
            numericEdit.setText (lb303::parameterText (static_cast<size_t> (selected),
                                                       processor.value (static_cast<size_t> (selected))),
                                 juce::dontSendNotification);
        }
    };
    addAndMakeVisible (parameterValue);

    numericEdit.setMultiLine (false);
    numericEdit.setReturnKeyStartsNewLine (false);
    numericEdit.setInputRestrictions (32);
    numericEdit.setWantsKeyboardFocus (true);
    numericEdit.setExplicitFocusOrder (3);
    numericEdit.setTitle ("Edit");
    numericEdit.setDescription (juce::String());
    addAndMakeVisible (numericEdit);

    selectParameter();
    // Children exist before JUCE's resize notifications (TEST1.2 crash fix).
    setResizable (true, true);
    setResizeLimits (690, 350, 1700, 1100);
    setSize (1050, 570);
    startTimerHz (8);
}
LB303AudioProcessorEditor::~LB303AudioProcessorEditor()
{
    stopTimer();
    valueAttachment.reset();
}
std::unique_ptr<juce::AccessibilityHandler> LB303AudioProcessorEditor::createAccessibilityHandler()
{
    // No duplicate editor/window title in NVDA; expose only the three controls.
    return createIgnoredAccessibilityHandler (*this);
}

juce::String LB303AudioProcessorEditor::labelFor (int index) const
{
    const auto& p = lb303::catalog()[static_cast<size_t> (index)];
    return juce::String (p.name) + ", "
         + lb303::parameterText (static_cast<size_t> (index),
                                 processor.value (static_cast<size_t> (index)));
}
void LB303AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff12161d));
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 23.0f, juce::Font::bold)));
    g.drawText ("LB-303", 18, 8, 190, 34, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffb5c9e2));
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::plain)));
    g.drawText ("Bassline | Grid → Value → Edit", 203, 12, getWidth() - 223, 27,
                juce::Justification::centredLeft);

    const int cw = (drawnGrid.getWidth() - (numberOfColumns - 1) * gridGap) / numberOfColumns;
    const int rh = (drawnGrid.getHeight() - (rowsPerColumn - 1) * gridGap) / rowsPerColumn;
    if (cw > 0 && rh > 0)
    {
        for (int i = 0; i < static_cast<int> (lb303::parameterCount); ++i)
        {
            const auto col = i / rowsPerColumn;
            const auto row = i % rowsPerColumn;
            const auto bounds = juce::Rectangle<int> (drawnGrid.getX() + col * (cw + gridGap),
                                                       drawnGrid.getY() + row * (rh + gridGap), cw, rh);
            g.setColour (i == selected ? juce::Colour (0xff395775) : juce::Colour (0xff202630));
            g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
            g.setColour (i == selected ? juce::Colour (0xfff0f5ff) : juce::Colour (0xffd2ddeb));
            g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain)));
            g.drawFittedText (labelFor (i), bounds.reduced (5, 2), juce::Justification::centredLeft, 1);
        }
    }
    g.setColour (juce::Colour (0xffb5c9e2));
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain)));
    g.drawText ("Arrows: grid | Enter: Value | Tab: Grid / Value / Edit | Alt+L / Alt+V / Alt+E",
                18, getHeight() - 26, getWidth() - 36, 18, juce::Justification::centredLeft);
}
void LB303AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    area.removeFromTop (41);
    parameterGrid.setBounds (area.removeFromTop (36));
    area.removeFromTop (7);
    auto controls = area.removeFromTop (38);
    const auto editorWidth = juce::jmin (200, juce::jmax (100, controls.getWidth() / 4));
    numericEdit.setBounds (controls.removeFromRight (editorWidth));
    controls.removeFromRight (9);
    parameterValue.setBounds (controls);
    area.removeFromTop (14);
    area.removeFromBottom (32);
    drawnGrid = area;
}
void LB303AudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    if (! drawnGrid.contains (event.getPosition())) return;
    const auto cw = (drawnGrid.getWidth() - (numberOfColumns - 1) * gridGap) / numberOfColumns;
    const auto rh = (drawnGrid.getHeight() - (rowsPerColumn - 1) * gridGap) / rowsPerColumn;
    if (cw <= 0 || rh <= 0) return;
    const auto p = event.getPosition() - drawnGrid.getPosition();
    const int col = p.x / (cw + gridGap);
    const int row = p.y / (rh + gridGap);
    const int index = col * rowsPerColumn + row;
    if (col >= 0 && col < numberOfColumns && row >= 0 && row < rowsPerColumn
        && index < static_cast<int> (lb303::parameterCount))
    {
        changeSelection (index);
        focusControl (Control::grid);
    }
}
void LB303AudioProcessorEditor::timerCallback() { refreshLabels(); }
void LB303AudioProcessorEditor::refreshLabels (bool force)
{
    // Maintain both the native ComboBox list and visible selection text. Merely
    // changing the popup's item text is NOT enough in JUCE (see LJuno TEST95).
    bool changed = false;
    for (int i = 0; i < static_cast<int> (lb303::parameterCount); ++i)
    {
        const auto label = labelFor (i);
        if (force || parameterGrid.getItemText (i) != label)
        {
            parameterGrid.changeItemText (i + 1, label);
            changed = true;
        }
    }
    if (changed)
    {
        parameterGrid.setSelectedId (selected + 1, juce::dontSendNotification);
        repaint (drawnGrid);
    }
}
void LB303AudioProcessorEditor::changeSelection (int index)
{
    index = juce::jlimit (0, static_cast<int> (lb303::parameterCount) - 1, index);
    if (index == selected) return; // silent borders; no repeated announcements
    parameterGrid.setLineReadingMode();
    parameterGrid.setSelectedId (index + 1, juce::sendNotificationSync);
}
void LB303AudioProcessorEditor::selectParameter()
{
    const int chosen = parameterGrid.getSelectedId() - 1;
    if (! juce::isPositiveAndBelow (chosen, static_cast<int> (lb303::parameterCount))) return;
    selected = chosen;
    changingParameter = true;
    valueAttachment.reset();
    const auto& p = lb303::catalog()[static_cast<size_t> (selected)];
    // Value is a real slider attached to the currently selected VST parameter.
    // The Edit field is an independent, keyboard-accessible native TextEditor.
    parameterValue.setRange (p.minimum, p.maximum, p.step);
    parameterValue.setValue (processor.value (static_cast<size_t> (selected)), juce::dontSendNotification);
    parameterValue.setLineReadingMode();
    numericEdit.setTitle ("Edit. " + juce::String (p.name));
    numericEdit.setText (lb303::parameterText (static_cast<size_t> (selected),
                                              processor.value (static_cast<size_t> (selected))),
                         juce::dontSendNotification);
    editDirty = false;
    valueAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, p.id, parameterValue);
    changingParameter = false;
    refreshLabels();
    repaint (drawnGrid);
}
void LB303AudioProcessorEditor::moveInGrid (int row, int column)
{
    const auto r = selected % rowsPerColumn;
    const auto target = selected + row + column * rowsPerColumn;
    if (row < 0 && r == 0) return;
    if (row > 0 && r == rowsPerColumn - 1) return;
    if (row != 0 && target / rowsPerColumn != selected / rowsPerColumn) return;
    if (juce::isPositiveAndBelow (target, static_cast<int> (lb303::parameterCount)))
        changeSelection (target);
}
void LB303AudioProcessorEditor::setValue (float value)
{
    if (auto* param = processor.parameter (static_cast<size_t> (selected)))
    {
        const auto range = param->getNormalisableRange();
        const auto legal = range.snapToLegalValue (juce::jlimit (range.start, range.end, value));
        const auto original = processor.value (static_cast<size_t> (selected));
        if (std::abs (legal - original) <= 1.0e-6f) return; // silent on limits
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (legal));
        param->endChangeGesture();
        refreshLabels (true);
        announceSelectedValueChanged();
    }
}
void LB303AudioProcessorEditor::announceSelectedValueChanged()
{
    // Match LJuno: report the ACTUAL parameter value after host notification,
    // regardless of whether the active editor control is Grid, Value or Edit.
    // A valueChanged event on the Value slider alone is not heard when keyboard
    // shortcuts are pressed while focus remains in the Grid (or in Edit).
    const auto text = lb303::parameterText (static_cast<size_t> (selected),
                                            processor.value (static_cast<size_t> (selected)));
    if (numericEdit.hasKeyboardFocus (true))
    {
        // An Alt shortcut in Edit is a committed parameter change, not part of
        // the uncommitted text. Keep its text synchronised like LJuno does.
        numericEdit.setText (text, juce::dontSendNotification);
        numericEdit.selectAll();
        editDirty = false;
    }

    juce::Component* active = nullptr;
    if (numericEdit.hasKeyboardFocus (true)) active = &numericEdit;
    else if (parameterValue.hasKeyboardFocus (true)) active = &parameterValue;
    else if (parameterGrid.hasKeyboardFocus (true)) active = &parameterGrid;
    if (active == nullptr) active = &parameterGrid;

    // Explicit UIA speech for all three contexts, rather than only a slider
    // accessibility valueChanged event. Announce the value once, not the full
    // Grid label, and never announce when no effective change happened.
    lb303::announceToActiveScreenReader (*active, text);
    juce::Timer::callAfterDelay (180,
        [safe = juce::Component::SafePointer<LB303AudioProcessorEditor> (this)]
        {
            if (safe == nullptr) return;
            if (safe->parameterGrid.hasKeyboardFocus (true))
                safe->parameterGrid.setLineReadingMode();
            else if (safe->parameterValue.hasKeyboardFocus (true))
                safe->parameterValue.setLineReadingMode();
            else if (safe->numericEdit.hasKeyboardFocus (true))
                safe->numericEdit.setDescription (juce::String());
        });
}
void LB303AudioProcessorEditor::changeValue (int direction, int speed)
{
    const auto& p = lb303::catalog()[static_cast<size_t> (selected)];
    // LJuno behavior: choice parameters always move by one item; continuous
    // parameters use the user-selected step width (1 / 5 / 10 / 15 / 20).
    // PageUp/PageDown apply LJuno's x40 page multiplier.
    const float multiplier = p.choiceCount > 0 ? 1.0f
                              : static_cast<float> (stepWidths[stepWidthIndex] * speed);
    setValue (processor.value (static_cast<size_t> (selected)) + direction * p.step * multiplier);
}
void LB303AudioProcessorEditor::changeStepWidth (int direction)
{
    const int next = juce::jlimit (0, 4, stepWidthIndex + direction);
    if (next == stepWidthIndex) return; // silent limit, also for NVDA/OSARA
    stepWidthIndex = next;
    processor.parameters.state.setProperty (stepWidthState, stepWidthIndex, nullptr);
    auto* active = juce::Component::getCurrentlyFocusedComponent();
    juce::Component& source = (active != nullptr && (active == this || isParentOf (active)))
        ? *active : static_cast<juce::Component&> (parameterGrid);
    lb303::announceToActiveScreenReader (source, "Step " + juce::String (stepWidths[stepWidthIndex]));
    juce::Timer::callAfterDelay (220,
        [safe = juce::Component::SafePointer<LB303AudioProcessorEditor> (this)]
        {
            if (safe == nullptr) return;
            if (safe->parameterGrid.hasKeyboardFocus (true))
                safe->parameterGrid.setLineReadingMode();
            else if (safe->parameterValue.hasKeyboardFocus (true))
                safe->parameterValue.setLineReadingMode();
            else if (safe->numericEdit.hasKeyboardFocus (true))
                safe->numericEdit.setDescription (juce::String());
        });
}
void LB303AudioProcessorEditor::setBoundary (bool maximum)
{
    const auto& p = lb303::catalog()[static_cast<size_t> (selected)];
    setValue (maximum ? p.maximum : p.minimum);
}
void LB303AudioProcessorEditor::resetParameter()
{
    setValue (lb303::catalog()[static_cast<size_t> (selected)].initial);
}
void LB303AudioProcessorEditor::focusControl (Control target, bool defer)
{
    const auto doFocus = [safe = juce::Component::SafePointer<LB303AudioProcessorEditor> (this), target]
    {
        if (safe == nullptr) return;
        juce::Component* control = target == Control::grid
            ? static_cast<juce::Component*> (&safe->parameterGrid)
            : target == Control::value
                ? static_cast<juce::Component*> (&safe->parameterValue)
                : static_cast<juce::Component*> (&safe->numericEdit);
        juce::AccessibilityHandler::clearCurrentlyFocusedHandler();
        control->grabKeyboardFocus();
        // Grid/Edit emit their native accessibility focus events from
        // focusGained(). Value follows LJuno's direct accessibility focus.
        if (target == Control::value)
            if (auto* handler = control->getAccessibilityHandler()) handler->grabFocus();
    };
    if (defer) juce::MessageManager::callAsync (doFocus);
    else doFocus();
}
void LB303AudioProcessorEditor::enterEdit()
{
    editDirty = false;
    numericEdit.setText (lb303::parameterText (static_cast<size_t> (selected),
                                              processor.value (static_cast<size_t> (selected))),
                         juce::dontSendNotification);
    focusControl (Control::edit);
    numericEdit.selectAll();
}
bool LB303AudioProcessorEditor::commitEdit()
{
    auto input = numericEdit.getText().trim();
    if (input.isEmpty()) return false;
    const auto& p = lb303::catalog()[static_cast<size_t> (selected)];
    if (p.choiceCount > 0)
    {
        for (int i = 0; i < p.choiceCount; ++i)
            if (input.equalsIgnoreCase (p.choices[i]))
            {
                setValue (static_cast<float> (i));
                editDirty = false;
                return true;
            }
    }
    char* end = nullptr;
    const auto utf8 = input.toRawUTF8();
    const auto result = std::strtof (utf8, &end);
    if (end == utf8 || ! std::isfinite (result))
    {
        lb303::announceToActiveScreenReader (numericEdit, "Invalid value");
        return false;
    }
    setValue (result);
    editDirty = false;
    numericEdit.setText (lb303::parameterText (static_cast<size_t> (selected),
                                              processor.value (static_cast<size_t> (selected))),
                         juce::dontSendNotification);
    return true;
}
void LB303AudioProcessorEditor::cancelEdit()
{
    editDirty = false;
    numericEdit.setText (lb303::parameterText (static_cast<size_t> (selected),
                                              processor.value (static_cast<size_t> (selected))),
                         juce::dontSendNotification);
}
void LB303AudioProcessorEditor::announceUnavailableSequencer()
{
    const auto source = juce::Component::getCurrentlyFocusedComponent();
    lb303::announceToActiveScreenReader (source != nullptr && isParentOf (source)
                                            ? *source : static_cast<juce::Component&> (parameterGrid),
                                        "Sequencer editor will be added in TEST2");
}
bool LB303AudioProcessorEditor::handleKey (const juce::KeyPress& key, Control current)
{
    const auto code = key.getKeyCode();
    const auto mods = key.getModifiers();
    const auto ch = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());
    const auto letter = [code, ch] (juce::juce_wchar c)
    {
        return ch == c || code == juce::CharacterFunctions::toUpperCase (c);
    };
    if (mods.isAltDown())
    {
        if (letter ('l')) { focusControl (Control::grid, current == Control::edit); return true; }
        if (letter ('v')) { focusControl (Control::value, current == Control::edit); return true; }
        if (letter ('e')) { enterEdit(); return true; }
        if (letter ('q')) { announceUnavailableSequencer(); return true; }
        if (code == juce::KeyPress::upKey) { changeValue (1, mods.isShiftDown() ? 10 : 1); return true; }
        if (code == juce::KeyPress::downKey) { changeValue (-1, mods.isShiftDown() ? 10 : 1); return true; }
        if (code == juce::KeyPress::leftKey) { changeStepWidth (-1); return true; }
        if (code == juce::KeyPress::rightKey) { changeStepWidth (1); return true; }
        // Mirror LJuno's Alt shortcuts: Home = maximum, End = minimum.
        if (code == juce::KeyPress::homeKey) { setBoundary (true); return true; }
        if (code == juce::KeyPress::endKey) { setBoundary (false); return true; }
        if (code == juce::KeyPress::pageUpKey) { changeValue (1, valuePageStep); return true; }
        if (code == juce::KeyPress::pageDownKey) { changeValue (-1, valuePageStep); return true; }
        return current == Control::edit; // no Alt text inserted in the numeric editor
    }
    if (mods.isCtrlDown() && code == juce::KeyPress::tabKey)
    {
        announceUnavailableSequencer(); // sequencer parameters in TEST2
        return true;
    }
    if (current == Control::edit)
    {
        if (code == juce::KeyPress::escapeKey)
        {
            cancelEdit();
            focusControl (Control::grid, true);
            return true;
        }
        if (code == juce::KeyPress::tabKey || code == juce::KeyPress::returnKey)
        {
            if (! commitEdit()) return true;
            focusControl (code == juce::KeyPress::tabKey && mods.isShiftDown()
                              ? Control::value : Control::grid, true);
            return true;
        }
        return false; // native text editing, arrow movement, clipboard, etc.
    }
    if (code == juce::KeyPress::tabKey && ! mods.isCtrlDown() && ! mods.isCommandDown())
    {
        if (current == Control::grid)
        {
            if (mods.isShiftDown()) enterEdit();
            else focusControl (Control::value);
        }
        else if (mods.isShiftDown()) focusControl (Control::grid);
        else enterEdit();
        return true;
    }
    if (code == juce::KeyPress::returnKey)
    {
        focusControl (current == Control::grid ? Control::value : Control::grid);
        return true;
    }
    if (code == juce::KeyPress::backspaceKey) { resetParameter(); return true; }
    if (current == Control::value)
    {
        if (code == juce::KeyPress::upKey) { changeValue (1); return true; }
        if (code == juce::KeyPress::downKey) { changeValue (-1); return true; }
        if (code == juce::KeyPress::leftKey) { changeStepWidth (-1); return true; }
        if (code == juce::KeyPress::rightKey) { changeStepWidth (1); return true; }
        if (code == juce::KeyPress::pageUpKey) { changeValue (1, valuePageStep); return true; }
        if (code == juce::KeyPress::pageDownKey) { changeValue (-1, valuePageStep); return true; }
        if (code == juce::KeyPress::homeKey) { setBoundary (false); return true; }
        if (code == juce::KeyPress::endKey) { setBoundary (true); return true; }
        return false;
    }
    parameterGrid.setLineReadingMode();
    if (mods.isCtrlDown() && code == juce::KeyPress::homeKey) { changeSelection (0); return true; }
    if (mods.isCtrlDown() && code == juce::KeyPress::endKey)
    { changeSelection (static_cast<int> (lb303::parameterCount) - 1); return true; }
    if (! mods.isCtrlDown() && ! mods.isCommandDown())
    {
        if (code == juce::KeyPress::upKey) { moveInGrid (-1, 0); return true; }
        if (code == juce::KeyPress::downKey) { moveInGrid (1, 0); return true; }
        if (code == juce::KeyPress::leftKey) { moveInGrid (0, -1); return true; }
        if (code == juce::KeyPress::rightKey) { moveInGrid (0, 1); return true; }
        if (code == juce::KeyPress::homeKey)
        { changeSelection ((selected / rowsPerColumn) * rowsPerColumn); return true; }
        if (code == juce::KeyPress::endKey)
        { changeSelection (juce::jmin (static_cast<int> (lb303::parameterCount) - 1,
                                     (selected / rowsPerColumn + 1) * rowsPerColumn - 1)); return true; }
        if (code == juce::KeyPress::pageUpKey)
        {
            changeSelection (juce::jmax ((selected / rowsPerColumn) * rowsPerColumn,
                                        selected - 5));
            return true;
        }
        if (code == juce::KeyPress::pageDownKey)
        {
            changeSelection (juce::jmin (juce::jmin (
                                    static_cast<int> (lb303::parameterCount) - 1,
                                    (selected / rowsPerColumn + 1) * rowsPerColumn - 1),
                                    selected + 5));
            return true;
        }
        if (juce::CharacterFunctions::isLetterOrDigit (ch))
        {
            const int count = static_cast<int> (lb303::parameterCount);
            const int delta = mods.isShiftDown() ? -1 : 1;
            for (int t = 1; t <= count; ++t)
            {
                const int next = (selected + delta * t + count * 2) % count;
                if (juce::CharacterFunctions::toLowerCase (
                        juce::String (lb303::catalog()[static_cast<size_t> (next)].name)[0]) == ch)
                {
                    changeSelection (next);
                    return true;
                }
            }
            return true; // don't activate the ComboBox's native popup search
        }
    }
    return false;
}
