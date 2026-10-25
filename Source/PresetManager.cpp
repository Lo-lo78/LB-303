// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PresetManager.h"
#include "PluginProcessor.h"
#include <algorithm>

namespace lb303
{
namespace
{
constexpr auto header = "LB-303 Patch Snapshot";
const juce::Identifier currentKey { "currentPresetRelativePath" };
const juce::Identifier folderKey { "presetBrowserDirectory" };
const juce::Identifier rowKey { "presetBrowserRow" };
// JUCE XML state is already used by REAPER; preserving its binary blob also
// keeps the two independent 16-pattern banks, every step, and every lock.
}

PresetManager::PresetManager (LB303AudioProcessor& p)
    : processor (p),
      library (juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("LB-303")) {}

juce::Result PresetManager::ensureLibrary() const
{
    return library.createDirectory();
}

bool PresetManager::inside (const juce::File& file) const
{
    return file == library || file.isAChildOf (library);
}

bool PresetManager::validPreset (const juce::File& file) const
{
    return inside (file) && file.existsAsFile() && file.hasFileExtension ("lb");
}

std::vector<PresetManager::BrowserEntry> PresetManager::list (const juce::File& folder) const
{
    std::vector<BrowserEntry> entries;
    if (! inside (folder) || ! folder.isDirectory()) return entries;
    const auto dirs = folder.findChildFiles (juce::File::findDirectories | juce::File::ignoreHiddenFiles,
                                             false, "*", juce::File::FollowSymlinks::no);
    for (const auto& d : dirs)
        if (inside (d)) entries.push_back ({ d, d.getFileName(), true });
    const auto files = folder.findChildFiles (juce::File::findFiles | juce::File::ignoreHiddenFiles,
                                              false, "*.lb", juce::File::FollowSymlinks::no);
    for (const auto& f : files)
        if (validPreset (f)) entries.push_back ({ f, f.getFileNameWithoutExtension(), false });
    std::sort (entries.begin(), entries.end(), [] (const auto& a, const auto& b)
    {
        if (a.directory != b.directory) return a.directory;
        return a.name.compareNatural (b.name, false) < 0;
    });
    return entries;
}

std::vector<juce::File> PresetManager::allPresets() const
{
    std::vector<juce::File> files;
    if (! library.isDirectory()) return files;
    for (const auto& f : library.findChildFiles (juce::File::findFiles | juce::File::ignoreHiddenFiles,
                                                 true, "*.lb", juce::File::FollowSymlinks::no))
        if (validPreset (f)) files.push_back (f);
    std::sort (files.begin(), files.end(), [this] (const auto& a, const auto& b)
    {
        return a.getRelativePathFrom (library).compareNatural (
                   b.getRelativePathFrom (library), false) < 0;
    });
    return files;
}

PresetManager::Snapshot PresetManager::capture() const
{
    Snapshot snapshot;
    processor.getStateInformation (snapshot.data);
    return snapshot;
}

void PresetManager::restore (const Snapshot& snapshot) const
{
    if (snapshot.valid())
        processor.setStateInformation (snapshot.data.getData(),
                                       static_cast<int> (snapshot.data.getSize()));
}

juce::Result PresetManager::readState (const juce::File& file, juce::MemoryBlock& state) const
{
    if (! validPreset (file)) return juce::Result::fail ("Invalid LB-303 preset");
    if (file.getSize() > 16 * 1024 * 1024)
        return juce::Result::fail ("Preset file is too large");
    auto lines = juce::StringArray::fromLines (file.loadFileAsString());
    if (lines.isEmpty() || lines[0].trim() != header)
        return juce::Result::fail ("Not an LB-303 .lb preset");
    juce::String encoded;
    for (const auto& line : lines)
        if (line.startsWith ("State: ")) encoded = line.fromFirstOccurrenceOf ("State: ", false, false).trim();
    if (encoded.isEmpty()) return juce::Result::fail ("Preset has no instrument state");
    juce::MemoryOutputStream decoded;
    if (! juce::Base64::convertFromBase64 (decoded, encoded))
        return juce::Result::fail ("Corrupt preset data");
    state = decoded.getMemoryBlock();
    // Validate the XML envelope BEFORE changing the processor state.
    const auto xml = juce::AudioProcessor::getXmlFromBinary (
        state.getData(), static_cast<int> (state.getSize()));
    if (state.getSize() == 0 || xml == nullptr
        || ! xml->hasTagName (processor.parameters.state.getType()))
        return juce::Result::fail ("Invalid LB-303 preset state");
    return juce::Result::ok();
}

juce::Result PresetManager::preview (const juce::File& file)
{
    juce::MemoryBlock state;
    auto result = readState (file, state);
    if (result.wasOk()) processor.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    return result;
}

void PresetManager::remember (const juce::File& file)
{
    processor.parameters.state.setProperty (currentKey, file.getRelativePathFrom (library), nullptr);
}

juce::Result PresetManager::commit (const juce::File& file)
{
    if (! validPreset (file)) return juce::Result::fail ("Preset is not available");
    remember (file);
    return juce::Result::ok();
}

juce::Result PresetManager::load (const juce::File& file)
{
    auto result = preview (file);
    if (result.wasOk()) remember (file);
    return result;
}

juce::String PresetManager::cleanName (const juce::String& name)
{
    return juce::File::createLegalFileName (name.trim()).trim();
}

juce::Result PresetManager::save (const juce::File& folder, const juce::String& name,
                                  juce::File& destination, bool overwrite)
{
    auto result = ensureLibrary();
    if (result.failed()) return result;
    if (! inside (folder) || ! folder.isDirectory())
        return juce::Result::fail ("Invalid preset folder");
    const auto clean = cleanName (name);
    if (clean.isEmpty() || clean == "." || clean == "..")
        return juce::Result::fail ("Enter a valid preset name");
    destination = folder.getChildFile (clean).withFileExtension (".lb");
    if (! inside (destination)) return juce::Result::fail ("Invalid preset path");
    if (destination.existsAsFile() && ! overwrite)
        return juce::Result::fail ("Preset exists");
    auto snapshot = capture();
    if (! snapshot.valid()) return juce::Result::fail ("Cannot capture instrument state");
    const auto text = juce::String (header) + "\nVersion: 1\nName: " + clean
        + "\nState: " + juce::Base64::toBase64 (snapshot.data.getData(), snapshot.data.getSize()) + "\n";
    if (! destination.replaceWithText (text))
        return juce::Result::fail ("Cannot write preset file");
    remember (destination);
    return juce::Result::ok();
}

juce::Result PresetManager::remove (const juce::File& file)
{
    if (! validPreset (file)) return juce::Result::fail ("Select a preset to delete");
    if (! file.deleteFile()) return juce::Result::fail ("Cannot delete preset");
    if (currentFile() == file) forgetCurrent();
    return juce::Result::ok();
}

juce::Result PresetManager::rename (const juce::File& file, const juce::String& name,
                                    juce::File& result)
{
    if (! inside (file) || file == library || ! file.exists())
        return juce::Result::fail ("Invalid item");
    const auto clean = cleanName (name);
    if (clean.isEmpty() || clean == "." || clean == "..")
        return juce::Result::fail ("Invalid name");
    result = file.getSiblingFile (clean + (file.isDirectory() ? juce::String() : juce::String (".lb")));
    if (! inside (result) || result.exists())
        return juce::Result::fail ("An item with this name already exists");
    const auto current = currentFile();
    const auto wasCurrent = current == file;
    const auto currentInsideRenamedFolder = file.isDirectory() && current.isAChildOf (file);
    const auto childSuffix = currentInsideRenamedFolder ? current.getRelativePathFrom (file)
                                                         : juce::String();
    if (! file.moveFileTo (result)) return juce::Result::fail ("Cannot rename item");
    if (wasCurrent) remember (result);
    else if (currentInsideRenamedFolder) remember (result.getChildFile (childSuffix));
    return juce::Result::ok();
}

juce::Result PresetManager::newFolder (const juce::File& folder,
                                       const juce::String& name, juce::File& result)
{
    if (! inside (folder) || ! folder.isDirectory())
        return juce::Result::fail ("Invalid folder");
    const auto clean = cleanName (name);
    if (clean.isEmpty() || clean == "." || clean == "..")
        return juce::Result::fail ("Invalid folder name");
    result = folder.getChildFile (clean);
    if (! inside (result) || result.exists())
        return juce::Result::fail ("Folder already exists");
    return result.createDirectory();
}

juce::File PresetManager::currentFile() const
{
    const auto name = processor.parameters.state.getProperty (currentKey).toString();
    if (name.isEmpty()) return {};
    const auto file = library.getChildFile (name);
    return inside (file) ? file : juce::File();
}

juce::File PresetManager::savedBrowserDirectory() const
{
    const auto path = processor.parameters.state.getProperty (folderKey).toString();
    if (path.isEmpty() || path == ".") return library;
    const auto file = library.getChildFile (path);
    return inside (file) && file.isDirectory() ? file : library;
}
void PresetManager::saveBrowserDirectory (const juce::File& folder)
{
    if (! inside (folder) || ! folder.isDirectory()) return;
    processor.parameters.state.setProperty (folderKey,
       folder == library ? juce::String (".") : folder.getRelativePathFrom (library), nullptr);
}
int PresetManager::savedBrowserRow() const
{
    return static_cast<int> (processor.parameters.state.getProperty (rowKey, 0));
}
void PresetManager::saveBrowserRow (int row)
{
    processor.parameters.state.setProperty (rowKey, std::max (0, row), nullptr);
}

void PresetManager::forgetCurrent()
{
    processor.parameters.state.removeProperty (currentKey, nullptr);
}

juce::Result PresetManager::relative (int direction, juce::String& name)
{
    const auto files = allPresets();
    if (files.empty()) return juce::Result::fail ("No LB-303 presets found");
    const auto current = currentFile();
    auto iterator = std::find (files.begin(), files.end(), current);
    int i = iterator == files.end() ? (direction > 0 ? -1 : 0)
                                    : static_cast<int> (std::distance (files.begin(), iterator));
    const int count = static_cast<int> (files.size());
    i = (i + (direction > 0 ? 1 : -1) + count * 2) % count;
    auto result = load (files[static_cast<size_t> (i)]);
    if (result.wasOk()) name = files[static_cast<size_t> (i)].getFileNameWithoutExtension();
    return result;
}
}
