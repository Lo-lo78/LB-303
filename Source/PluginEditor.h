// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PresetBrowser.h"
#include <memory>
#include <array>
#include <vector>
#include <functional>
#include <utility>

class SequencerStepEditor;

// Same read-only, keyboard-readable About text control as LJuno-116.
class LB303AboutTextEditor final : public juce::TextEditor
{
public:
    using KeyHandler = std::function<bool (const juce::KeyPress&)>;
    explicit LB303AboutTextEditor (const juce::String& name) : juce::TextEditor (name) {}
    void setKeyHandler (KeyHandler handler) { keyHandler = std::move (handler); }
    bool keyPressed (const juce::KeyPress& key) override
    {
        if (keyHandler && keyHandler (key)) return true;
        return juce::TextEditor::keyPressed (key);
    }
private:
    KeyHandler keyHandler;
};

class LB303AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::Timer,
                                         private juce::KeyListener
{
public:
    explicit LB303AudioProcessorEditor (LB303AudioProcessor&);
    ~LB303AudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void focusGained (FocusChangeType) override;
    bool keyPressed (const juce::KeyPress&) override;
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
    std::unique_ptr<SequencerStepEditor> stepEditor;
    std::unique_ptr<lb303::PresetBrowser> presetBrowser;
    juce::TextButton previousPreset { "Previous" };
    juce::TextButton nextPreset { "Next" };
    juce::TextButton browserButton { "Browser" };
    juce::TextButton saveButton { "Save preset" };
    juce::TextButton help { "Help" };
    juce::TextButton aboutButton { "About" };
    LB303AboutTextEditor aboutInfo { "About LB-303" };
    juce::TextButton aboutProject { "Project" };
    juce::TextButton aboutContact { "Contact" };
    juce::TextButton aboutClose { "Close" };
    std::unique_ptr<juce::LookAndFeel_V4> helpMenuLookAndFeel;
    bool aboutOpen = false;
    Control aboutReturnControl = Control::grid;
    Control presetReturnControl = Control::grid;
    int presetReturnContext = 0;
    int presetReturnParameter = 0;
    juce::Rectangle<int> drawnGrid;
    int context = 0; // 0 = Bassline 1, 1 = Bassline 2, 2 = Sequencer, 3 = FX
    std::array<int, lb303::basslineCount + 2> lastSelected { 0, 0, 0, 0 };
    int selected = 0; // per-context, not global parameter index
    // Runtime FX display only: no changes to the stable AudioProcessor parameter IDs.
    // One column per effect; hidden options are still stored in presets/automation.
    std::vector<size_t> visibleFxParameters;
    std::array<int, 6> fxColumnLengths { 1, 1, 1, 1, 1, 1 };
    size_t rememberedFxGlobal = lb303::parameterIndex (0, 31);
    int rememberedFxColumn = 0;
    int stepWidthIndex = 0; // LJuno: 1 / 5 / 10 / 15 / 20, saved with the project.
    bool changingParameter = false;
    bool editDirty = false;
    bool initialFocusTransferPending = false;
    int initialFocusTransferAttempts = 0;

    size_t globalIndex (int local) const;
    size_t globalIndex() const { return globalIndex (selected); }
    int currentParameterCount() const { return context < 2 ? static_cast<int> (lb303::voiceGridParameterCount)
        : context == 2 ? static_cast<int> (lb303::sequencerDisplayParameterCount)
                       : static_cast<int> (visibleFxParameters.size()); }
    void updateFxColumns();
    void syncFxColumns();
    std::pair<int, int> fxCell (int index) const;
    int fxCellIndex (int column, int row) const;
    juce::String contextName() const;
    void changeContext();
    void populateContext();
    void timerCallback() override;
    void refreshLabels (bool force = false);
    void changeSelection (int index);
    void selectParameter();
    void moveInGrid (int row, int column);
    void changeValue (int direction, int speed = 1);
    void changeStepWidth (int direction);
    void setValue (float value);
    void announceSelectedValueChanged();
    void setBoundary (bool maximum);
    void resetParameter();
    void enterEdit();
    bool commitEdit();
    void cancelEdit();
    void focusControl (Control, bool defer = false);
    void scheduleInitialFocusTransfer();
    void performInitialFocusTransfer();
    void rememberEditorLocation();
    bool handleKey (const juce::KeyPress&, Control);
    int shortcutPatternLane() const;
    void changePatternFromGrid (int direction, Control current);
    void openStepEditor();
    void openPresetBrowser (bool save);
    void focusBackFromPreset();
    void changePreset (int direction);
    void initialisePlugin();
    void showHelpLanguageMenu();
    void openHelp (const juce::String& languageCode);
    void openAbout();
    void closeAbout();
    bool handleAboutKey (const juce::KeyPress&, juce::Component* source);
    bool navigateAboutText (const juce::KeyPress&);
    void openProjectPage();
    void openContactEmail();
    bool keyPressed (const juce::KeyPress&, juce::Component*) override;
    juce::String labelFor (int index) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LB303AudioProcessorEditor)
};
