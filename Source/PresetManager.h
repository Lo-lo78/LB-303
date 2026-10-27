// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

class LB303AudioProcessor;

namespace lb303
{
// LJuno-style preset library. Store the EXACT host snapshot, including both
// sequencer banks. A preset does not duplicate or reinterpret the DSP state.
class PresetManager
{
public:
    // Same instrument instance: browser can route Alt+P/Space to the existing
    // sequencer preview transport, without creating a second audition engine.
    LB303AudioProcessor& instrument() const noexcept { return processor; }
    struct BrowserEntry
    {
        juce::File file;
        juce::String name;
        bool directory = false;
    };
    struct Snapshot
    {
        juce::MemoryBlock data;
        bool valid() const noexcept { return data.getSize() != 0; }
    };

    explicit PresetManager (LB303AudioProcessor&);
    const juce::File& root() const noexcept { return library; }
    juce::Result ensureLibrary() const;
    bool inside (const juce::File&) const;
    bool validPreset (const juce::File&) const;
    std::vector<BrowserEntry> list (const juce::File&) const;
    std::vector<juce::File> allPresets() const;
    Snapshot capture() const;
    void restore (const Snapshot&) const;
    juce::Result preview (const juce::File&);
    juce::Result commit (const juce::File&);
    juce::Result load (const juce::File&);
    juce::Result save (const juce::File& folder, const juce::String& name,
                       juce::File& destination, bool overwrite = false);
    juce::Result remove (const juce::File&);
    juce::Result rename (const juce::File&, const juce::String&, juce::File& result);
    juce::Result newFolder (const juce::File&, const juce::String&, juce::File&);
    juce::Result relative (int direction, juce::String& name);
    juce::File currentFile() const;
    // Last explicitly loaded/saved preset across REAPER sessions and instances.
    juce::File lastLoadedPresetAcrossSessions() const;
    juce::File savedBrowserDirectory() const;
    void saveBrowserDirectory (const juce::File&);
    int savedBrowserRow() const;
    void saveBrowserRow (int);
    void forgetCurrent();
    static juce::String cleanName (const juce::String&);

private:
    LB303AudioProcessor& processor;
    juce::File library;
    juce::Result readState (const juce::File&, juce::MemoryBlock&) const;
    void remember (const juce::File&);
};
}
