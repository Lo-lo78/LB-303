// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginEditor.h"
#include "ScreenReaderAnnouncer.h"
#include <algorithm>

LB303AudioProcessorEditor::ParameterCell::ParameterCell (LB303AudioProcessorEditor& editor, int i)
    : juce::TextButton (lb303::catalog()[static_cast<size_t> (i)].name), parent (editor), index (i)
{
    setWantsKeyboardFocus (true);
    setClickingTogglesState (false);
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202630));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff343e50));
    setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    onClick = [this]
    {
        grabKeyboardFocus();
        // Enter/Space on an enum advances to the next selection; on a number
        // it increments by one step.  Alt+Up/Down works for every control.
        parent.moveValue (index, 1);
    };
}
bool LB303AudioProcessorEditor::ParameterCell::keyPressed (const juce::KeyPress& key)
{
    return parent.handleKey (key, index) || juce::TextButton::keyPressed (key);
}
void LB303AudioProcessorEditor::ParameterCell::focusGained (FocusChangeType why)
{
    juce::TextButton::focusGained (why);
    parent.focusChanged (index, *this);
}

LB303AudioProcessorEditor::LB303AudioProcessorEditor (LB303AudioProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    setWantsKeyboardFocus (false);
    setResizable (true, true);
    setResizeLimits (720, 390, 1800, 1300);
    for (size_t i = 0; i < cells.size(); ++i)
    {
        cells[i] = std::make_unique<ParameterCell> (*this, static_cast<int> (i));
        addAndMakeVisible (*cells[i]);
    }
    setSize (1100, 610);
    refresh (true);
    startTimerHz (8);
    juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<LB303AudioProcessorEditor> (this)]
    {
        if (safe != nullptr) safe->focusIndex (safe->selected);
    });
}
LB303AudioProcessorEditor::~LB303AudioProcessorEditor() { stopTimer(); }
void LB303AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff12161d));
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 23.0f, juce::Font::bold)));
    g.drawText ("LB-303", 20, 8, 200, 35, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 13.0f, juce::Font::plain)));
    g.setColour (juce::Colour (0xffb6c6df));
    g.drawText ("Bassline grid - 35 parameters - 8 rows per column", 222, 14,
                getWidth()-240, 24, juce::Justification::centredLeft);
    g.drawText ("Arrows: navigate  |  Alt+Up/Down: value  |  Enter: increase  |  Home/End: min/max  |  Ctrl+Home/End: first/last",
                18, getHeight()-52, getWidth()-36, 19, juce::Justification::centredLeft);
    g.drawText ("Alt+L: focus grid  |  Alt+Q and Ctrl+Tab: sequencer in TEST2  |  JSFX fidelity test build",
                18, getHeight()-32, getWidth()-36, 20, juce::Justification::centredLeft);
}
void LB303AudioProcessorEditor::resized()
{
    const auto margin = 18;
    auto area = getLocalBounds().reduced (margin);
    area.removeFromTop (48);
    area.removeFromBottom (57);
    const auto columnWidth = std::max (1, (area.getWidth()-4*9)/5);
    const auto rowHeight = std::max (1, (area.getHeight()-7*6)/8);
    for (size_t i = 0; i < cells.size(); ++i)
    {
        const auto col = static_cast<int> (i / 8);
        const auto row = static_cast<int> (i % 8);
        cells[i]->setBounds (area.getX() + col*(columnWidth+9),
                             area.getY() + row*(rowHeight+6), columnWidth, rowHeight);
    }
}
void LB303AudioProcessorEditor::timerCallback() { refresh(); }
void LB303AudioProcessorEditor::refresh (bool force)
{
    if (updating) return;
    for (size_t i = 0; i < cells.size(); ++i)
    {
        const auto& p = lb303::catalog()[i];
        const auto text = juce::String (p.name) + ", " + lb303::parameterText (i, processor.value (i));
        if (force || cells[i]->getButtonText() != text)
        {
            cells[i]->setButtonText (text);
            cells[i]->setTitle (text);
            cells[i]->setDescription (juce::String());
        }
    }
}
void LB303AudioProcessorEditor::announce (juce::Component& source, const juce::String& what)
{
    lb303::announceToActiveScreenReader (source, what);
}
void LB303AudioProcessorEditor::focusChanged (int index, juce::Component& component)
{
    selected = index;
    refresh();
    const auto& p = lb303::catalog()[static_cast<size_t> (index)];
    announce (component, juce::String (p.name) + ", "
                              + lb303::parameterText (static_cast<size_t> (index), processor.value (index)));
}
void LB303AudioProcessorEditor::focusIndex (int index)
{
    selected = std::clamp (index, 0, static_cast<int> (cells.size()) - 1);
    cells[static_cast<size_t> (selected)]->grabKeyboardFocus();
}
void LB303AudioProcessorEditor::setValue (int index, float plain)
{
    if (auto* param = processor.parameter (static_cast<size_t> (index)))
    {
        const auto range = param->getNormalisableRange();
        const auto legal = range.snapToLegalValue (juce::jlimit (range.start, range.end, plain));
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (legal));
        param->endChangeGesture();
        refresh (true);
        focusChanged (index, *cells[static_cast<size_t> (index)]);
    }
}
void LB303AudioProcessorEditor::moveValue (int index, int dir, bool large)
{
    const auto& p = lb303::catalog()[static_cast<size_t> (index)];
    const auto step = p.step * (large ? 10.0f : 1.0f);
    setValue (index, processor.value (static_cast<size_t> (index)) + dir*step);
}
bool LB303AudioProcessorEditor::handleKey (const juce::KeyPress& key, int index)
{
    const auto mods = key.getModifiers();
    const auto k = key.getKeyCode();
    if (mods.isAltDown() && (key.getTextCharacter() == 'q' || key.getTextCharacter() == 'Q' || k == 'Q'))
    {
        announce (*cells[static_cast<size_t> (index)], "Sequencer editor is planned for TEST2");
        return true;
    }
    if (mods.isAltDown() && (key.getTextCharacter() == 'l' || key.getTextCharacter() == 'L' || k == 'L'))
    {
        focusIndex (selected);
        return true;
    }
    if (mods.isCtrlDown() && k == juce::KeyPress::tabKey)
    {
        announce (*cells[static_cast<size_t> (index)], "Sequencer parameter grid is planned for TEST2");
        return true;
    }
    if (mods.isAltDown() && k == juce::KeyPress::upKey) { moveValue (index, 1, mods.isShiftDown()); return true; }
    if (mods.isAltDown() && k == juce::KeyPress::downKey) { moveValue (index, -1, mods.isShiftDown()); return true; }
    if (mods.isCtrlDown() && k == juce::KeyPress::homeKey) { focusIndex (0); return true; }
    if (mods.isCtrlDown() && k == juce::KeyPress::endKey) { focusIndex (34); return true; }
    if (!mods.isCtrlDown() && !mods.isAltDown())
    {
        if (k == juce::KeyPress::upKey) { focusIndex (index > 0 && index%8 != 0 ? index-1 : index); return true; }
        if (k == juce::KeyPress::downKey) { focusIndex (index < 34 && index%8 != 7 ? index+1 : index); return true; }
        if (k == juce::KeyPress::leftKey) { focusIndex (index >= 8 ? index-8 : index); return true; }
        if (k == juce::KeyPress::rightKey) { focusIndex (index+8 < 35 ? index+8 : index); return true; }
        if (k == juce::KeyPress::homeKey) { setValue (index, lb303::catalog()[index].minimum); return true; }
        if (k == juce::KeyPress::endKey) { setValue (index, lb303::catalog()[index].maximum); return true; }
        if (k == juce::KeyPress::pageUpKey) { moveValue (index, 1, true); return true; }
        if (k == juce::KeyPress::pageDownKey) { moveValue (index, -1, true); return true; }
    }
    return false;
}
