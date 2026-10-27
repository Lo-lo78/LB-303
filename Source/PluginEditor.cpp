// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginEditor.h"
#include "ScreenReaderAnnouncer.h"
#include "SequencerStepEditor.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
constexpr int rowsPerColumn = 8;
constexpr int columnsFor (int count) { return (count + rowsPerColumn - 1) / rowsPerColumn; }
constexpr int gridGap = 7;
constexpr int valuePageStep = 40;
constexpr int stepWidths[] { 1, 5, 10, 15, 20 };
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
    setTitle (editor.contextName() + ". Grid");
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
    if (k.getKeyCode() == juce::KeyPress::spaceKey
        && ! editor.processor.spaceControlsSequencerPreview())
        return false; // REAPER host command, not ComboBox popup
    return editor.handleKey (k, Control::grid) || juce::ComboBox::keyPressed (k);
}

void LB303AudioProcessorEditor::ParameterValue::setLineReadingMode()
{
    const auto name = juce::String (lb303::catalog()[editor.globalIndex()].name);
    setName (name);
    setTitle (name);
    setDescription (juce::String());
}
void LB303AudioProcessorEditor::ParameterValue::setEntryAccessibility()
{
    const auto name = juce::String (lb303::catalog()[editor.globalIndex()].name);
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
    if (k.getKeyCode() == juce::KeyPress::spaceKey
        && ! editor.processor.spaceControlsSequencerPreview())
        return false;
    return editor.handleKey (k, Control::value) || juce::Slider::keyPressed (k);
}

void LB303AudioProcessorEditor::NumericEdit::focusGained (FocusChangeType why)
{
    setTitle ("Edit. " + juce::String (lb303::catalog()[editor.globalIndex()].name));
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
    if (k.getKeyCode() == juce::KeyPress::spaceKey
        && ! editor.processor.spaceControlsSequencerPreview())
        return false;
    return editor.handleKey (k, Control::edit) || juce::TextEditor::keyPressed (k);
}

LB303AudioProcessorEditor::LB303AudioProcessorEditor (LB303AudioProcessor& p)
    : juce::AudioProcessorEditor (p), processor (p)
{
    // LJuno-style host entry: be a keyboard focus container, but NEVER grab
    // focus in the constructor. REAPER's FX Chain retains focus until the user
    // actually enters this plug-in. Once the root receives focus, hand it to
    // the currently remembered parameter Grid on the native UIA message loop.
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    setWantsKeyboardFocus (true);
    // Restore only this processor instance's in-memory editing history.
    // The APVTS and preset snapshots must never contain these selections.
    context = juce::jlimit (0, 3, processor.editorSession.activeContext);
    for (int i = 0; i < 4; ++i)
        lastSelected[static_cast<size_t> (i)] = juce::jmax (0,
            processor.editorSession.selectedParameters[static_cast<size_t> (i)]);
    stepWidthIndex = juce::jlimit (0, 4, processor.editorSession.stepWidthIndex);
    rememberedFxGlobal = processor.editorSession.fxSelectedGlobal;
    rememberedFxColumn = juce::jlimit (0, 5, processor.editorSession.fxSelectedColumn);
    updateFxColumns();

    parameterGrid.setTextWhenNothingSelected ("Select parameter");
    parameterGrid.setWantsKeyboardFocus (true); // ensure Grid accepts arrows immediately after preset browser
    parameterGrid.setLineReadingMode();
    parameterGrid.setExplicitFocusOrder (1);
    parameterGrid.onChange = [this] { selectParameter(); };
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
            numericEdit.setText (lb303::parameterText (globalIndex(),
                                                       processor.value (globalIndex())),
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

    // Same main keyboard commands as LJuno: Alt+B browser, Alt+S save,
    // Alt+minus/plus previous/next. Buttons stay outside the 3-item Tab cycle.
    previousPreset.onClick = [this] { changePreset (-1); };
    nextPreset.onClick = [this] { changePreset (1); };
    browserButton.onClick = [this] { openPresetBrowser (false); };
    saveButton.onClick = [this] { openPresetBrowser (true); };
    for (auto* button : { &previousPreset, &nextPreset, &browserButton, &saveButton })
    {
        button->setWantsKeyboardFocus (false);
        addAndMakeVisible (*button);
    }
    presetBrowser = std::make_unique<lb303::PresetBrowser> (processor.presetManager);
    addAndMakeVisible (*presetBrowser);
    presetBrowser->setVisible (false);
    presetBrowser->onClose = [this] { focusBackFromPreset(); };
    presetBrowser->onPresetLoaded = [this] { processor.syncSelectedPatternPerformance (true); syncFxColumns(); refreshLabels (true); selectParameter(); };
    stepEditor = std::make_unique<SequencerStepEditor> (processor);
    addAndMakeVisible (*stepEditor);
    stepEditor->setVisible (false);
    processor.syncSelectedPatternPerformance (true);
    populateContext();
    // Children exist before JUCE's resize notifications (TEST1.2 crash fix).
    setResizable (true, true);
    setResizeLimits (690, 350, 1700, 1100);
    setSize (1050, 570);
    startTimerHz (8);
}
LB303AudioProcessorEditor::~LB303AudioProcessorEditor()
{
    stopTimer();
    // Closing the *whole VST3 window* stops the per-instance sequencer preview.
    // Closing the Alt+Q overlay alone must NOT stop it.
    processor.requestSequencerPreview (false);
    rememberEditorLocation();
    if (stepEditor != nullptr) stepEditor->close();
    if (presetBrowser != nullptr) { presetBrowser->onClose = {}; presetBrowser->onPresetLoaded = {}; presetBrowser->close(); }
    valueAttachment.reset();
}
std::unique_ptr<juce::AccessibilityHandler> LB303AudioProcessorEditor::createAccessibilityHandler()
{
    // No duplicate editor/window title in NVDA; expose only the three controls.
    return createIgnoredAccessibilityHandler (*this);
}


void LB303AudioProcessorEditor::rememberEditorLocation()
{
    lastSelected[static_cast<size_t> (context)] = selected;
    processor.editorSession.activeContext = context;
    processor.editorSession.selectedParameters = lastSelected;
    processor.editorSession.fxSelectedGlobal = rememberedFxGlobal;
    processor.editorSession.fxSelectedColumn = rememberedFxColumn;
}

void LB303AudioProcessorEditor::focusGained (FocusChangeType)
{
    // Unlike the constructor, this event means REAPER has intentionally
    // transferred keyboard focus into the plug-in's root component.
    scheduleInitialFocusTransfer();
}

// Root fallback for hosts that initially focus the editor itself instead of
// the native ComboBox/Slider/Edit child. JUCE bubbles only UNHANDLED keys to
// the parent; ordinary grid keys, NVDA focus and the Alt+Q overlay are intact.
bool LB303AudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    if ((stepEditor != nullptr && stepEditor->isOpen())
        || (presetBrowser != nullptr && presetBrowser->active()))
        return false;

    const auto mods = key.getModifiers();
    const auto code = key.getKeyCode();
    if (! mods.isCtrlDown() || mods.isAltDown() || mods.isShiftDown()
        || (code != juce::KeyPress::upKey && code != juce::KeyPress::downKey))
        return false;

    const auto control = numericEdit.hasKeyboardFocus (true) ? Control::edit
                       : parameterValue.hasKeyboardFocus (true) ? Control::value
                                                                : Control::grid;
    return handleKey (key, control);
}

void LB303AudioProcessorEditor::scheduleInitialFocusTransfer()
{
    if (initialFocusTransferPending) return;
    initialFocusTransferPending = true;
    juce::Timer::callAfterDelay (110,
        [safe = juce::Component::SafePointer<LB303AudioProcessorEditor> (this)]
        {
            if (safe == nullptr) return;
            safe->initialFocusTransferPending = false;
            safe->performInitialFocusTransfer();
        });
}

void LB303AudioProcessorEditor::performInitialFocusTransfer()
{
    if (! isShowing() || (presetBrowser != nullptr && presetBrowser->active())
        || (stepEditor != nullptr && stepEditor->isOpen())) return;

    // Never steal focus from Value, Edit or from the host FX Chain.
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (focused != nullptr && focused != this)
        return;
    auto* peer = getPeer();
    auto* handler = parameterGrid.getAccessibilityHandler();
    if (peer == nullptr || ! peer->isFocused() || handler == nullptr
        || handler->getNativeImplementation() == nullptr)
    {
        if (focused == this && ++initialFocusTransferAttempts < 8)
            scheduleInitialFocusTransfer();
        return;
    }

    initialFocusTransferAttempts = 0;
    parameterGrid.hidePopup();
    // Match Alt+L and LJuno exactly: let NVDA read the NATIVE ComboBox
    // focus event (page, Grid, control role, selected parameter/value,
    // collapsed state, shortcut). A manual notification with only
    // labelFor(selected) takes priority and hides the entry context.
    // Normal arrow navigation keeps the concise item/value announcement.
    parameterGrid.setEntryAccessibility();
    juce::AccessibilityHandler::clearCurrentlyFocusedHandler();
    parameterGrid.grabKeyboardFocus();
    if (parameterGrid.hasKeyboardFocus (false))
        handler->grabFocus();
    else if (++initialFocusTransferAttempts < 8)
        scheduleInitialFocusTransfer();
}

size_t LB303AudioProcessorEditor::globalIndex (int local) const
{
    if (context == static_cast<int> (lb303::fxContextIndex))
        return visibleFxParameters.at (static_cast<size_t> (local));
    if (context < 2)
    {
        // Reuse the existing, automated MIDI channel ID in each voice grid,
        // without changing voice DSP local indices or the lock picker mapping.
        if (local == lb303::voiceMidiDisplayIndex)
            return context == 0 ? lb303::midiChannel1Index : lb303::midiChannel2Index;
        if (local == lb303::voiceMidiDisplayIndex + 1)
            return lb303::separateOutputIndex (static_cast<size_t> (context));
        const int voiceDisplay = local > lb303::voiceMidiDisplayIndex + 1 ? local - 2 : local;
        const auto actual = lb303::displayToLocal (static_cast<size_t> (context),
                                                   static_cast<size_t> (voiceDisplay));
        return lb303::voiceParameterGlobalIndex (static_cast<size_t> (context), actual);
    }
    return lb303::sequencerParameterIndex (lb303::displayToLocal (
        static_cast<size_t> (context), static_cast<size_t> (local)));
}
juce::String LB303AudioProcessorEditor::contextName() const
{
    if (context == 0) return "Bassline 1";
    if (context == 1) return "Bassline 2";
    return context == 2 ? "Sequencer" : "FX";
}
void LB303AudioProcessorEditor::updateFxColumns()
{
    // The FX grid is a filtered VIEW of stable host parameters, not a second
    // parameter layout. Do not change any AudioProcessorValueTreeState IDs.
    visibleFxParameters.clear();
    fxColumnLengths.fill (0);
    const auto add = [this] (int column, size_t global)
    {
        visibleFxParameters.push_back (global);
        ++fxColumnLengths[static_cast<size_t> (column)];
    };
    const auto addFx = [&] (int column, lb303::fx::Control which)
    { add (column, lb303::fxParameterIndex (which)); };
    const auto enabled = [&] (lb303::fx::Control which)
    { return processor.value (lb303::fxParameterIndex (which)) > 0.5f; };

    // Column 0: the output dry return is always available.
    add (0, lb303::parameterIndex (0, 31)); // historical slider032, Dry Level
    add (0, lb303::oversamplingIndex); // 1x/2x/4x, visible to NVDA as a choice
    // Columns 1 and 2: independent distortion inserts.
    addFx (1, lb303::fx::distortionType);
    if (enabled (lb303::fx::distortionType))
    {
        addFx (1, lb303::fx::distortionAmount);
        addFx (1, lb303::fx::distortionHarmonics);
    }
    addFx (2, lb303::fx::distortion2Type);
    if (enabled (lb303::fx::distortion2Type))
    {
        addFx (2, lb303::fx::distortion2Amount);
        addFx (2, lb303::fx::distortion2Harmonics);
    }
    // Column 3: chorus is absent when Off, except its On/Off control.
    addFx (3, lb303::fx::chorusOn);
    if (enabled (lb303::fx::chorusOn))
    {
        addFx (3, lb303::fx::chorusLevel);
        addFx (3, lb303::fx::chorusRate);
        addFx (3, lb303::fx::chorusWidth);
        addFx (3, lb303::fx::chorusSend1);
        addFx (3, lb303::fx::chorusSend2);
    }
    // Column 4: one delay, whose controls depend on its selected algorithm.
    addFx (4, lb303::fx::delayType);
    const int delayType = juce::jlimit (0, 2,
        juce::roundToInt (processor.value (lb303::fxParameterIndex (lb303::fx::delayType))));
    if (delayType != 0)
    {
        addFx (4, lb303::fx::delaySend1);
        addFx (4, lb303::fx::delaySend2);
        if (delayType == 1)
        {
            // Retain original (Bassline 1) Classic IDs; slider032 is Dry Level
            // and belongs ONLY to column zero, not to the delay.
            for (size_t local = 19; local <= 30; ++local)
                add (4, lb303::parameterIndex (0, local));
            add (4, lb303::parameterIndex (0, 32));
        }
        else
        {
            for (const auto which : { lb303::fx::dualTime, lb303::fx::dualSync,
                lb303::fx::dualSpeed1, lb303::fx::dualSpeed2, lb303::fx::dualGlide,
                lb303::fx::dualFeedback1, lb303::fx::dualFeedback2,
                lb303::fx::dualTone1, lb303::fx::dualTone2,
                lb303::fx::dualSpread, lb303::fx::dualDrive })
                addFx (4, which);
        }
    }
    // Column 5: reverb type picker and only the parameters of an active type.
    addFx (5, lb303::fx::reverbType);
    if (enabled (lb303::fx::reverbType))
        for (const auto which : { lb303::fx::reverbSend1, lb303::fx::reverbSend2,
             lb303::fx::reverbPredelay, lb303::fx::reverbDecay,
             lb303::fx::reverbDamping, lb303::fx::reverbWidth,
             lb303::fx::reverbLevel })
            addFx (5, which);
    // Historical fx::effectOrder deliberately absent from this view.
}

std::pair<int, int> LB303AudioProcessorEditor::fxCell (int index) const
{
    int first = 0;
    for (int col = 0; col < static_cast<int> (fxColumnLengths.size()); ++col)
    {
        const int length = fxColumnLengths[static_cast<size_t> (col)];
        if (index < first + length) return { col, index - first };
        first += length;
    }
    return { 0, 0 };
}
int LB303AudioProcessorEditor::fxCellIndex (int column, int row) const
{
    if (! juce::isPositiveAndBelow (column, static_cast<int> (fxColumnLengths.size())))
        return -1;
    const int length = fxColumnLengths[static_cast<size_t> (column)];
    if (! juce::isPositiveAndBelow (row, length)) return -1;
    int start = 0;
    for (int col = 0; col < column; ++col)
        start += fxColumnLengths[static_cast<size_t> (col)];
    return start + row;
}
void LB303AudioProcessorEditor::syncFxColumns()
{
    if (context != static_cast<int> (lb303::fxContextIndex)) return;
    const auto oldIndices = visibleFxParameters;
    const size_t previousGlobal = oldIndices.empty()
        ? lb303::parameterIndex (0, 31)
        : oldIndices[static_cast<size_t> (juce::jlimit (0, static_cast<int> (oldIndices.size()) - 1, selected))];
    const auto oldCell = fxCell (selected);
    updateFxColumns();
    if (oldIndices == visibleFxParameters) return;

    // Retain the same parameter when possible. When it becomes hidden (FX Off
    // or switched Classic/Dual), keep the selection in that column's Type picker.
    auto found = std::find (visibleFxParameters.begin(), visibleFxParameters.end(), previousGlobal);
    int nextSelected = found != visibleFxParameters.end()
        ? static_cast<int> (std::distance (visibleFxParameters.begin(), found))
        : fxCellIndex (oldCell.first, 0);
    if (nextSelected < 0) nextSelected = 0;
    selected = nextSelected;
    lastSelected[static_cast<size_t> (context)] = selected;
    changingParameter = true;
    valueAttachment.reset();
    parameterGrid.clear (juce::dontSendNotification);
    for (int i = 0; i < currentParameterCount(); ++i)
        parameterGrid.addItem (labelFor (i), i + 1);
    parameterGrid.setSelectedId (selected + 1, juce::dontSendNotification);
    changingParameter = false;
    selectParameter();
    rememberEditorLocation();
    repaint (drawnGrid);
}

void LB303AudioProcessorEditor::populateContext()
{
    changingParameter = true;
    valueAttachment.reset();
    if (context == static_cast<int> (lb303::fxContextIndex)) updateFxColumns();
    parameterGrid.clear (juce::dontSendNotification);
    selected = juce::jlimit (0, currentParameterCount() - 1,
                             lastSelected[static_cast<size_t> (context)]);
    if (context == static_cast<int> (lb303::fxContextIndex))
    {
        // Remember by global VST parameter ID rather than a shifting display
        // index; when hidden, return to that effect's Type/Off selector.
        const auto found = std::find (visibleFxParameters.begin(),
                                     visibleFxParameters.end(), rememberedFxGlobal);
        selected = found != visibleFxParameters.end()
            ? static_cast<int> (std::distance (visibleFxParameters.begin(), found))
            : juce::jmax (0, fxCellIndex (rememberedFxColumn, 0));
    }
    for (int i = 0; i < currentParameterCount(); ++i)
        parameterGrid.addItem (labelFor (i), i + 1);
    parameterGrid.setSelectedId (selected + 1, juce::dontSendNotification);
    parameterValue.setVisible (true);
    numericEdit.setVisible (true);
    changingParameter = false;
    selectParameter();
    parameterGrid.setLineReadingMode();
    repaint();
}
void LB303AudioProcessorEditor::changeContext()
{
    lastSelected[static_cast<size_t> (context)] = selected;
    // Ctrl+Tab: Bassline 1 -> Bassline 2 -> Sequencer -> FX -> Bassline 1.
    context = (context + 1) % static_cast<int> (lb303::basslineCount + 2);
    populateContext();
    rememberEditorLocation();
    focusControl (Control::grid, true);
    juce::Timer::callAfterDelay (50,
        [safe = juce::Component::SafePointer<LB303AudioProcessorEditor> (this)]
        {
            if (safe != nullptr && safe->parameterGrid.hasKeyboardFocus (true))
                lb303::announceToActiveScreenReader (safe->parameterGrid,
                                                     safe->contextName());
        });
}
juce::String LB303AudioProcessorEditor::labelFor (int index) const
{
    const auto& p = lb303::catalog()[globalIndex (index)];
    return juce::String (p.name) + ", "
         + lb303::parameterText (globalIndex (index),
                                 processor.value (globalIndex (index)));
}
void LB303AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff12161d));
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 23.0f, juce::Font::bold)));
    g.drawText ("LB-303", 18, 8, 190, 34, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xffb5c9e2));
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::plain)));
    g.drawText (contextName() + " | Ctrl+Tab: Bassline 1 / Bassline 2 / Sequencer / FX", 203, 12, getWidth() - 223, 27,
                juce::Justification::centredLeft);

    const bool fxView = context == static_cast<int> (lb303::fxContextIndex);
    const int columns = fxView ? static_cast<int> (fxColumnLengths.size())
                               : columnsFor (currentParameterCount());
    const int maxRows = fxView ? *std::max_element (fxColumnLengths.begin(), fxColumnLengths.end())
                                : rowsPerColumn;
    const int gap = fxView ? 2 : gridGap;
    const int cw = (drawnGrid.getWidth() - (columns - 1) * gap) / columns;
    const int rh = (drawnGrid.getHeight() - (maxRows - 1) * gap) / maxRows;
    if (cw > 0 && rh > 0)
    {
        for (int i = 0; i < currentParameterCount(); ++i)
        {
            const auto cell = fxView ? fxCell (i)
                                     : std::pair<int,int> { i / rowsPerColumn, i % rowsPerColumn };
            const auto col = cell.first;
            const auto row = cell.second;
            const auto bounds = juce::Rectangle<int> (drawnGrid.getX() + col * (cw + gap),
                                                       drawnGrid.getY() + row * (rh + gap), cw, rh);
            g.setColour (i == selected ? juce::Colour (0xff395775) : juce::Colour (0xff202630));
            g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
            g.setColour (i == selected ? juce::Colour (0xfff0f5ff) : juce::Colour (0xffd2ddeb));
            g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(),
                fxView && maxRows > 12 ? 10.0f : 12.0f, juce::Font::plain)));
            g.drawFittedText (labelFor (i), bounds.reduced (5, 2), juce::Justification::centredLeft, 1);
        }
    }
    g.setColour (juce::Colour (0xffb5c9e2));
    g.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), 12.0f, juce::Font::plain)));
    g.drawText ("Grid / Value / Edit | Alt+M: Pattern / Song | Ctrl+Up/Down: pattern | Alt+P: Space target | Alt+Q: steps",
                18, getHeight() - 26, getWidth() - 36, 18, juce::Justification::centredLeft);
}
void LB303AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    area.removeFromTop (41);
    auto presetRow = area.removeFromTop (32);
    previousPreset.setBounds (presetRow.removeFromLeft (98));
    presetRow.removeFromLeft (7);
    nextPreset.setBounds (presetRow.removeFromLeft (98));
    presetRow.removeFromLeft (7);
    browserButton.setBounds (presetRow.removeFromLeft (105));
    presetRow.removeFromLeft (7);
    saveButton.setBounds (presetRow.removeFromLeft (135));
    area.removeFromTop (8);
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
    if (stepEditor != nullptr) stepEditor->setBounds (getLocalBounds().reduced (24));
    if (presetBrowser != nullptr) presetBrowser->setBounds (getLocalBounds().reduced (12));
}
void LB303AudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    if (! drawnGrid.contains (event.getPosition())) return;
    const bool fxView = context == static_cast<int> (lb303::fxContextIndex);
    const int columns = fxView ? static_cast<int> (fxColumnLengths.size())
                               : columnsFor (currentParameterCount());
    const int maxRows = fxView ? *std::max_element (fxColumnLengths.begin(), fxColumnLengths.end())
                                : rowsPerColumn;
    const int gap = fxView ? 2 : gridGap;
    const auto cw = (drawnGrid.getWidth() - (columns - 1) * gap) / columns;
    const auto rh = (drawnGrid.getHeight() - (maxRows - 1) * gap) / maxRows;
    if (cw <= 0 || rh <= 0) return;
    const auto p = event.getPosition() - drawnGrid.getPosition();
    const int col = p.x / (cw + gap);
    const int row = p.y / (rh + gap);
    const int index = fxView ? fxCellIndex (col, row) : col * rowsPerColumn + row;
    if (col >= 0 && col < columns && row >= 0 && row < maxRows
        && juce::isPositiveAndBelow (index, currentParameterCount()))
    {
        changeSelection (index);
        focusControl (Control::grid);
    }
}
void LB303AudioProcessorEditor::timerCallback()
{
    processor.syncSelectedPatternPerformance();
    if (stepEditor != nullptr && stepEditor->isOpen()) stepEditor->refresh();
    else { syncFxColumns(); refreshLabels(); }
}
void LB303AudioProcessorEditor::refreshLabels (bool force)
{
    // Maintain both the native ComboBox list and visible selection text. Merely
    // changing the popup's item text is NOT enough in JUCE (see LJuno TEST95).
    bool changed = false;
    for (int i = 0; i < currentParameterCount(); ++i)
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
    index = juce::jlimit (0, currentParameterCount() - 1, index);
    if (index == selected) return; // silent borders; no repeated announcements
    parameterGrid.setLineReadingMode();
    parameterGrid.setSelectedId (index + 1, juce::sendNotificationSync);
}
void LB303AudioProcessorEditor::selectParameter()
{
    processor.syncSelectedPatternPerformance();
    const int chosen = parameterGrid.getSelectedId() - 1;
    if (! juce::isPositiveAndBelow (chosen, currentParameterCount())) return;
    selected = chosen;
    lastSelected[static_cast<size_t> (context)] = chosen;
    if (context == static_cast<int> (lb303::fxContextIndex))
    {
        rememberedFxGlobal = visibleFxParameters[static_cast<size_t> (chosen)];
        rememberedFxColumn = fxCell (chosen).first;
    }
    rememberEditorLocation();
    changingParameter = true;
    valueAttachment.reset();
    const auto& p = lb303::catalog()[globalIndex()];
    // Value is a real slider attached to the currently selected VST parameter.
    // The Edit field is an independent, keyboard-accessible native TextEditor.
    parameterValue.setRange (p.minimum, p.maximum, p.step);
    parameterValue.setValue (processor.value (globalIndex()), juce::dontSendNotification);
    parameterValue.setLineReadingMode();
    numericEdit.setTitle ("Edit. " + juce::String (p.name));
    numericEdit.setText (lb303::parameterText (globalIndex(),
                                              processor.value (globalIndex())),
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
    if (context == static_cast<int> (lb303::fxContextIndex))
    {
        const auto cell = fxCell (selected);
        const int targetCol = cell.first + column;
        if (! juce::isPositiveAndBelow (targetCol, static_cast<int> (fxColumnLengths.size())))
            return; // silent edge
        const int desiredRow = cell.second + row;
        if (column == 0 && ! juce::isPositiveAndBelow (
                                  desiredRow, fxColumnLengths[static_cast<size_t> (targetCol)]))
            return; // silent bottom/top of this effect's column
        const int target = fxCellIndex (targetCol, column == 0 ? desiredRow
            : juce::jmin (cell.second, fxColumnLengths[static_cast<size_t> (targetCol)] - 1));
        if (target >= 0) changeSelection (target);
        return;
    }
    const auto r = selected % rowsPerColumn;
    const auto target = selected + row + column * rowsPerColumn;
    if (row < 0 && r == 0) return;
    if (row > 0 && r == rowsPerColumn - 1) return;
    if (row != 0 && target / rowsPerColumn != selected / rowsPerColumn) return;
    if (juce::isPositiveAndBelow (target, currentParameterCount()))
        changeSelection (target);
}
void LB303AudioProcessorEditor::setValue (float value)
{
    if (auto* param = processor.parameter (globalIndex()))
    {
        const auto range = param->getNormalisableRange();
        const auto legal = range.snapToLegalValue (juce::jlimit (range.start, range.end, value));
        const auto original = processor.value (globalIndex());
        if (std::abs (legal - original) <= 1.0e-6f) return; // silent on limits
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (legal));
        param->endChangeGesture();
        refreshLabels (true);
        announceSelectedValueChanged();
        if (context == static_cast<int> (lb303::fxContextIndex))
            juce::Timer::callAfterDelay (1,
                [safe = juce::Component::SafePointer<LB303AudioProcessorEditor> (this)]
                { if (safe != nullptr) safe->syncFxColumns(); });
    }
}
void LB303AudioProcessorEditor::announceSelectedValueChanged()
{
    // Match LJuno: report the ACTUAL parameter value after host notification,
    // regardless of whether the active editor control is Grid, Value or Edit.
    // A valueChanged event on the Value slider alone is not heard when keyboard
    // shortcuts are pressed while focus remains in the Grid (or in Edit).
    const auto text = lb303::parameterText (globalIndex(),
                                            processor.value (globalIndex()));
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
    const auto& p = lb303::catalog()[globalIndex()];
    // LJuno behavior: choice parameters always move by one item; continuous
    // parameters use the user-selected step width (1 / 5 / 10 / 15 / 20).
    // PageUp/PageDown apply LJuno's x40 page multiplier.
    const float multiplier = p.choiceCount > 0 ? 1.0f
                              : static_cast<float> (stepWidths[stepWidthIndex] * speed);
    setValue (processor.value (globalIndex()) + direction * p.step * multiplier);
}
void LB303AudioProcessorEditor::changeStepWidth (int direction)
{
    const int next = juce::jlimit (0, 4, stepWidthIndex + direction);
    if (next == stepWidthIndex) return; // silent limit, also for NVDA/OSARA
    stepWidthIndex = next;
    processor.editorSession.stepWidthIndex = stepWidthIndex;
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
    const auto& p = lb303::catalog()[globalIndex()];
    setValue (maximum ? p.maximum : p.minimum);
}
void LB303AudioProcessorEditor::resetParameter()
{
    setValue (lb303::catalog()[globalIndex()].initial);
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
    numericEdit.setText (lb303::parameterText (globalIndex(),
                                              processor.value (globalIndex())),
                         juce::dontSendNotification);
    focusControl (Control::edit);
    numericEdit.selectAll();
}
bool LB303AudioProcessorEditor::commitEdit()
{
    auto input = numericEdit.getText().trim();
    if (input.isEmpty()) return false;
    const auto& p = lb303::catalog()[globalIndex()];
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
    numericEdit.setText (lb303::parameterText (globalIndex(),
                                              processor.value (globalIndex())),
                         juce::dontSendNotification);
    return true;
}
void LB303AudioProcessorEditor::cancelEdit()
{
    editDirty = false;
    numericEdit.setText (lb303::parameterText (globalIndex(),
                                              processor.value (globalIndex())),
                         juce::dontSendNotification);
}
void LB303AudioProcessorEditor::openPresetBrowser (bool save)
{
    if (presetBrowser == nullptr || (stepEditor != nullptr && stepEditor->isOpen())) return;
    if (presetBrowser->active()) { presetBrowser->close(); return; }
    if (numericEdit.hasKeyboardFocus (true)) presetReturnControl = Control::edit;
    else if (parameterValue.hasKeyboardFocus (true)) presetReturnControl = Control::value;
    else presetReturnControl = Control::grid;
    // Store grid selection; do not steal focus from REAPER just by constructing.
    presetReturnContext = context;
    presetReturnParameter = selected;
    if (save) presetBrowser->saveAs(); else presetBrowser->browse();
}
void LB303AudioProcessorEditor::focusBackFromPreset()
{
    if (presetBrowser != nullptr && presetBrowser->active()) return;
    // Preset snapshots affect parameter values and pattern banks, not the
    // original page, selected parameter or Grid / Value / Edit target.
    if (context != presetReturnContext)
    {
        context = presetReturnContext;
        lastSelected[static_cast<size_t> (context)] = presetReturnParameter;
        populateContext();
    }
    else if (selected != presetReturnParameter)
    {
        selected = presetReturnParameter;
        parameterGrid.setSelectedId (selected + 1, juce::dontSendNotification);
        selectParameter();
    }
    refreshLabels (true);
    selectParameter();
    rememberEditorLocation();
    // Hiding the browser while its ListBox processes Enter can leave JUCE's
    // native ComboBox / UIA focus stale.  Value and Edit do not need recovery,
    // but Grid must receive a SECOND, post-event focus handoff.  Without it
    // REAPER/NVDA can appear to be on the Grid while arrow keys still go to
    // the closed list, until the user cycles Tab through Value and Edit.
    if (presetReturnControl == Control::grid)
        parameterGrid.hidePopup();
    focusControl (presetReturnControl, true);
    if (presetReturnControl == Control::grid)
    {
        const int restoreContext = context;
        const int restoreParameter = selected;
        juce::Timer::callAfterDelay (65,
            [safe = juce::Component::SafePointer<LB303AudioProcessorEditor> (this),
             restoreContext, restoreParameter]
            {
                if (safe == nullptr || (safe->presetBrowser != nullptr && safe->presetBrowser->active())
                    || safe->context != restoreContext || safe->selected != restoreParameter)
                    return;
                // Never override a later deliberate Tab into Value or Edit.
                if (safe->parameterValue.hasKeyboardFocus (true)
                    || safe->numericEdit.hasKeyboardFocus (true))
                    return;
                safe->parameterGrid.hidePopup();
                safe->parameterGrid.setSelectedId (restoreParameter + 1, juce::dontSendNotification);
                safe->parameterGrid.setEntryAccessibility();
                juce::AccessibilityHandler::clearCurrentlyFocusedHandler();
                safe->parameterGrid.grabKeyboardFocus();
                if (auto* handler = safe->parameterGrid.getAccessibilityHandler())
                    handler->grabFocus();
                // Always announce exactly the restored parameter and its
                // CURRENT value (including changes from the loaded preset).
                lb303::announceToActiveScreenReader (
                    safe->parameterGrid, safe->labelFor (restoreParameter));
                juce::Timer::callAfterDelay (250,
                    [safe, restoreContext, restoreParameter]
                    {
                        if (safe != nullptr && safe->context == restoreContext
                            && safe->selected == restoreParameter
                            && safe->parameterGrid.hasKeyboardFocus (true))
                            safe->parameterGrid.setLineReadingMode();
                    });
            });
    }
}
void LB303AudioProcessorEditor::changePreset (int direction)
{
    if (presetBrowser == nullptr || (stepEditor != nullptr && stepEditor->isOpen())) return;
    if (presetBrowser->active()) return;
    presetBrowser->previousNext (direction);
    refreshLabels (true);
    selectParameter();
}
void LB303AudioProcessorEditor::initialisePlugin()
{
    // Explicit user command only. Never steal REAPER focus at construction.
    processor.resetToInitialState();
    stepWidthIndex = 0;
    refreshLabels (true);
    selectParameter();
    juce::Component* focused = juce::Component::getCurrentlyFocusedComponent();
    auto& speaking = focused == &parameterValue ? static_cast<juce::Component&> (parameterValue)
                   : focused == &numericEdit ? static_cast<juce::Component&> (numericEdit)
                   : static_cast<juce::Component&> (parameterGrid);
    lb303::announceToActiveScreenReader (speaking, "LB-303 initialized. Both basslines, effects and 32 patterns reset");
}
void LB303AudioProcessorEditor::openStepEditor()
{
    if (stepEditor == nullptr || stepEditor->isOpen()) return;
    int lane = context < 2 ? context
        : juce::jlimit (0, 1, juce::roundToInt (processor.value (lb303::editTrackIndex)));
    juce::Component* origin = juce::Component::getCurrentlyFocusedComponent();
    if (origin == nullptr || (origin != &parameterGrid && origin != &parameterValue
                             && origin != &numericEdit)) origin = &parameterGrid;
    stepEditor->open (lane, origin);
}
// Pattern shortcuts are available from all parameter contexts, even when a
// different parameter has keyboard focus. In the Sequencer context, a B1/B2
// parameter targets its own lane; the shared Editor Track parameter (and FX)
// targets the lane chosen by Editor Track. This never changes GUI focus.
int LB303AudioProcessorEditor::shortcutPatternLane() const
{
    if (context == 0 || context == 1) return context;
    if (context == static_cast<int> (lb303::sequencerContextIndex))
    {
        const auto index = globalIndex();
        if (index == lb303::pattern1Index || index == lb303::sequencer1EnabledIndex
            || index == lb303::midiChannel1Index
            || (index >= lb303::seqPerformanceIndex (0, 0)
                && index < lb303::seqPerformanceIndex (0, lb303::sequencerPerformancePerLane)))
            return 0;
        if (index == lb303::pattern2Index || index == lb303::sequencer2EnabledIndex
            || index == lb303::midiChannel2Index
            || (index >= lb303::seqPerformanceIndex (1, 0)
                && index < lb303::seqPerformanceIndex (1, lb303::sequencerPerformancePerLane)))
            return 1;
    }
    return juce::jlimit (0, 1, juce::roundToInt (processor.value (lb303::editTrackIndex)));
}

void LB303AudioProcessorEditor::changePatternFromGrid (int direction, Control current)
{
    const int lane = shortcutPatternLane();
    const auto patternParameter = lane == 0 ? lb303::pattern1Index : lb303::pattern2Index;
    const int old = processor.currentPattern (static_cast<size_t> (lane));
    const int next = juce::jlimit (0, 15, old + direction);
    if (next == old) return; // identical to Alt+Q: silent at first/last pattern
    if (auto* parameter = processor.parameter (patternParameter))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (next)));
        parameter->endChangeGesture();
    }
    // On TEST2.4.22+ these controls are per pattern, so update their proxy
    // values immediately rather than waiting for the editor's polling timer.
    processor.syncSelectedPatternPerformance();
    selectParameter(); // update Value/Edit when the selected parameter also changed
    auto& active = current == Control::grid ? static_cast<juce::Component&> (parameterGrid)
                 : current == Control::value ? static_cast<juce::Component&> (parameterValue)
                                             : static_cast<juce::Component&> (numericEdit);
    lb303::announceToActiveScreenReader (active,
        "Bassline " + juce::String (lane + 1) + ", Pattern " + juce::String (next + 1));
    if (current == Control::edit)
        numericEdit.selectAll();
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
    // Space target is shared by all four parameter pages and the Alt+Q editor.
    // It is instance-only and does not change the selected parameter or focus.
    if (mods.isAltDown() && ! mods.isCtrlDown() && ! mods.isCommandDown() && letter ('p'))
    {
        const bool toPreview = processor.toggleSpaceTarget();
        auto* active = current == Control::grid ? static_cast<juce::Component*> (&parameterGrid)
                     : current == Control::value ? static_cast<juce::Component*> (&parameterValue)
                                                  : static_cast<juce::Component*> (&numericEdit);
        lb303::announceToActiveScreenReader (*active,
            toPreview ? "Space: Sequencer preview" : "Space: DAW");
        return true;
    }
    if (code == juce::KeyPress::spaceKey && ! mods.isAltDown()
        && ! mods.isCtrlDown() && ! mods.isCommandDown() && ! mods.isShiftDown())
    {
        if (! processor.spaceControlsSequencerPreview()) return false;
        const bool running = processor.toggleSequencerPreview();
        auto* active = current == Control::grid ? static_cast<juce::Component*> (&parameterGrid)
                     : current == Control::value ? static_cast<juce::Component*> (&parameterValue)
                                                  : static_cast<juce::Component*> (&numericEdit);
        lb303::announceToActiveScreenReader (*active,
            running ? "Both basslines preview playing" : "Both basslines preview stopped");
        return true;
    }
    // TEST2.4.24: share the same Song/Pattern state as Alt+Q, but keep Alt+P
    // completely independent. In DAW mode Space remains REAPER's key. In
    // Preview mode Space plays whichever of Song/Pattern Alt+M selected here.
    if (mods.isAltDown() && ! mods.isCtrlDown() && ! mods.isCommandDown()
        && ! mods.isShiftDown() && letter ('m'))
    {
        const bool song = ! processor.songs.songMode();
        processor.endSongAudition();
        processor.songs.setSongMode (song);
        processor.restartSongPreviewClock();
        auto& active = current == Control::grid ? static_cast<juce::Component&> (parameterGrid)
                     : current == Control::value ? static_cast<juce::Component&> (parameterValue)
                                                 : static_cast<juce::Component&> (numericEdit);
        lb303::announceToActiveScreenReader (active,
            song ? "Sequencer preview: Song" : "Sequencer preview: Pattern");
        return true;
    }
    // Windows JUCE maps commandModifier to ctrlModifier. Requiring Ctrl
    // while forbidding Command makes this shortcut IMPOSSIBLE on Windows.
    // Handle the actual Ctrl+Up/Down chord before the native ComboBox/Slider.
    if (mods.isCtrlDown() && ! mods.isAltDown() && ! mods.isShiftDown()
        && (code == juce::KeyPress::upKey || code == juce::KeyPress::downKey))
    {
        // If an Edit value is being typed, commit it before switching patterns
        // so the new pattern's stored value is not overwritten by stale text.
        if (current == Control::edit && editDirty && ! commitEdit()) return true;
        changePatternFromGrid (code == juce::KeyPress::upKey ? 1 : -1, current);
        return true;
    }
    // LJuno preset shortcuts work from Grid, Value and Edit, while preserving
    // the current parameter, context and the native three-control Tab cycle.
    if (mods.isAltDown() && ! mods.isCtrlDown() && ! mods.isCommandDown())
    {
        if (letter ('b')) { openPresetBrowser (false); return true; }
        if (letter ('s')) { openPresetBrowser (true); return true; }
        if (letter ('i')) { initialisePlugin(); return true; }
        if (letter ('+') || code == juce::KeyPress::numberPadAdd
            || (code == '=' && mods.isShiftDown()))
        { changePreset (1); return true; }
        if (letter ('-') || code == '-' || code == juce::KeyPress::numberPadSubtract)
        { changePreset (-1); return true; }
    }
    if (mods.isCtrlDown() && code == juce::KeyPress::tabKey)
    {
        if (current == Control::edit && editDirty && ! commitEdit()) return true;
        changeContext();
        return true;
    }
    if (mods.isAltDown() && letter ('q')) { openStepEditor(); return true; }
    if (mods.isAltDown())
    {
        if (letter ('l')) { focusControl (Control::grid, current == Control::edit); return true; }
        if (letter ('v')) { focusControl (Control::value, current == Control::edit); return true; }
        if (letter ('e')) { enterEdit(); return true; }
        if (letter ('q')) { openStepEditor(); return true; }
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
    // Character search works identically from Grid AND Value. Repeated
    // characters cycle forward; Shift+character cycles backward. Do not
    // steal typing from the native numeric Edit field.
    if ((current == Control::grid || current == Control::value)
        && ! mods.isCtrlDown() && ! mods.isAltDown() && ! mods.isCommandDown())
    {
        auto needle = juce::CharacterFunctions::toLowerCase (ch);
        // Shift+digit may produce punctuation as the text character; search
        // by the underlying numeric key to preserve reverse alphanumeric cycling.
        if (mods.isShiftDown() && code >= '0' && code <= '9')
            needle = static_cast<juce::juce_wchar> (code);
        if (juce::CharacterFunctions::isLetterOrDigit (needle))
        {
            const int count = currentParameterCount();
            const int delta = mods.isShiftDown() ? -1 : 1;
            for (int offset = 1; offset <= count; ++offset)
            {
                const int candidate = (selected + delta * offset + count * 2) % count;
                const auto& name = lb303::catalog()[globalIndex (candidate)].name;
                if (name.isNotEmpty() && juce::CharacterFunctions::toLowerCase (name[0]) == needle)
                {
                    changeSelection (candidate);
                    if (current == Control::value)
                    {
                        // Keep the Value slider focused while changing parameter.
                        parameterValue.setLineReadingMode();
                        lb303::announceToActiveScreenReader (parameterValue, labelFor (candidate));
                    }
                    return true;
                }
            }
            return true; // consume unmatched keys: silence rather than popup
        }
    }
    if (current == Control::value)
    {
        if (code == juce::KeyPress::upKey) { changeValue (1); return true; }
        if (code == juce::KeyPress::downKey) { changeValue (-1); return true; }
        if (code == juce::KeyPress::leftKey) { changeStepWidth (-1); return true; }
        if (code == juce::KeyPress::rightKey) { changeStepWidth (1); return true; }
        if (code == juce::KeyPress::pageUpKey) { changeValue (1, valuePageStep); return true; }
        if (code == juce::KeyPress::pageDownKey) { changeValue (-1, valuePageStep); return true; }
        // Match LJuno: Home=max, End=min, both on Value and with Alt.
        if (code == juce::KeyPress::homeKey) { setBoundary (true); return true; }
        if (code == juce::KeyPress::endKey) { setBoundary (false); return true; }
        return false;
    }
    parameterGrid.setLineReadingMode();
    if (mods.isCtrlDown() && code == juce::KeyPress::homeKey) { changeSelection (0); return true; }
    if (mods.isCtrlDown() && code == juce::KeyPress::endKey)
    { changeSelection (currentParameterCount() - 1); return true; }
    if (! mods.isCtrlDown() && ! mods.isCommandDown())
    {
        if (context == static_cast<int> (lb303::fxContextIndex))
        {
            const auto cell = fxCell (selected);
            const int count = fxColumnLengths[static_cast<size_t> (cell.first)];
            if (code == juce::KeyPress::homeKey)
            { const int pos = fxCellIndex (cell.first, 0); if (pos >= 0) changeSelection (pos); return true; }
            if (code == juce::KeyPress::endKey)
            { const int pos = fxCellIndex (cell.first, count - 1); if (pos >= 0) changeSelection (pos); return true; }
            if (code == juce::KeyPress::pageUpKey || code == juce::KeyPress::pageDownKey)
            {
                const int offset = code == juce::KeyPress::pageUpKey ? -5 : 5;
                const int pos = fxCellIndex (cell.first, juce::jlimit (0, count - 1, cell.second + offset));
                if (pos >= 0) changeSelection (pos);
                return true;
            }
        }
        if (code == juce::KeyPress::upKey) { moveInGrid (-1, 0); return true; }
        if (code == juce::KeyPress::downKey) { moveInGrid (1, 0); return true; }
        if (code == juce::KeyPress::leftKey) { moveInGrid (0, -1); return true; }
        if (code == juce::KeyPress::rightKey) { moveInGrid (0, 1); return true; }
        if (code == juce::KeyPress::homeKey)
        { changeSelection ((selected / rowsPerColumn) * rowsPerColumn); return true; }
        if (code == juce::KeyPress::endKey)
        { changeSelection (juce::jmin (currentParameterCount() - 1,
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
                                    currentParameterCount() - 1,
                                    (selected / rowsPerColumn + 1) * rowsPerColumn - 1),
                                    selected + 5));
            return true;
        }
    }
    return false;
}
