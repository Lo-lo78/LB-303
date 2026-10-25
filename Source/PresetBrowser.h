// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PresetManager.h"
#include <functional>

namespace lb303
{
// Keyboard-first preset browser inspired by LJuno-116. The complete instrument
// snapshot (parameters and both pattern banks) is previewed atomically.
class PresetBrowser final : public juce::Component, private juce::ListBoxModel
{
public:
    explicit PresetBrowser (PresetManager&);
    ~PresetBrowser() override;
    std::function<void()> onClose;
    std::function<void()> onPresetLoaded;
    bool active() const noexcept { return mode != Mode::closed; }
    void browse();
    void saveAs();
    void close (bool revertPreview = true);
    void previousNext (int direction);
    bool shortcut (const juce::KeyPress&);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    enum class Mode { closed, browse, save, rename, folder, overwrite, remove };
    struct BrowserList final : juce::ListBox
    {
        explicit BrowserList (PresetBrowser& p) : juce::ListBox ("Presets", &p), parent (p) {}
        bool keyPressed (const juce::KeyPress& k) override;
        PresetBrowser& parent;
    };
    struct NameEdit final : juce::TextEditor
    {
        explicit NameEdit (PresetBrowser& p) : juce::TextEditor ("Preset name"), parent (p) {}
        bool keyPressed (const juce::KeyPress& k) override;
        PresetBrowser& parent;
    };
    PresetManager& manager;
    Mode mode = Mode::closed;
    Mode confirmReturn = Mode::browse;
    BrowserList entries { *this };
    NameEdit nameEdit { *this };
    juce::Label folderLabel, promptLabel;
    juce::TextButton accept { "OK" }, cancel { "Cancel" }, back { "Back" };
    juce::File directory, previewFile, pendingFile, editFile;
    PresetManager::Snapshot original;
    std::vector<PresetManager::BrowserEntry> items;
    bool suppressSelection = false;
    bool hasPreview = false;
    bool isPreviewing = false;
    int rememberedRow = 0;

    int getNumRows() override;
    void paintListBoxItem (int, juce::Graphics&, int, int, bool) override;
    juce::String getNameForRow (int) override;
    void selectedRowsChanged (int) override;
    void listBoxItemDoubleClicked (int, const juce::MouseEvent&) override;
    bool listKey (const juce::KeyPress&);
    bool textKey (const juce::KeyPress&);
    bool selectByInitial (juce::juce_wchar, bool backwards);
    void showMode (Mode);
    void refresh (int row = 0);
    void focusList();
    void focusName (const juce::String&);
    void previewRow (int);
    void activateRow (int);
    void parentFolder();
    void renameSelected();
    void newFolder();
    void deleteSelected();
    void commitName();
    void confirmAction();
    void showMenu();
    void announce (const juce::String&);
    void rememberDirectory();
    juce::File rememberedDirectory() const;
    void refreshAndSelectFile (const juce::File&);
};
}
