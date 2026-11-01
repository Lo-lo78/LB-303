#!/usr/bin/env python3
"""TEST2.4.57: independent 64-pattern banks, legacy v9 and musical clipboard UI."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
def read(f): return (root/f).read_text(encoding='utf-8')
h=read('Source/SequencerState.h')
s=read('Source/SequencerState.cpp')
p=read('Source/ParameterCatalog.cpp')
e=read('Source/PluginEditor.cpp')
head=read('Source/PluginEditor.h')
step=read('Source/SequencerStepEditor.cpp')
song=read('Source/SongArrangement.cpp')
processor=read('Source/PluginProcessor.cpp')
w=read('.github/workflows/build-windows.yml')
assert 'maximumSequences = 64' in h and 'stepsPerSequence = 128' in h
assert 'std::array<AtomicSequence, maximumSequences> sequences' in h
assert 'stream.writeInt (10);' in s
assert 'formatVersion < 1 || formatVersion > 10' in s
assert 'const int savedSequenceCount = formatVersion >= 10 ? maximumSequences : 16;' in s
assert 'for (int sequenceIndex = 0; sequenceIndex < savedSequenceCount; ++sequenceIndex)' in s
assert 'for (int p = savedSequenceCount; p < maximumSequences; ++p)' in s and 'resetSequence (p)' in s
assert s.index('for (int p = savedSequenceCount; p < maximumSequences; ++p)') < s.index('selectedSequence.store (restoredSelected')
assert 'stream.writeByte (static_cast<char> (step.reverbSendLevel < 0 ? 255' in s
choices=p.split('constexpr const char* patternChoices[] = {',1)[1].split('};',1)[0]
assert re.findall(r'"(\d+)"',choices)==[str(i) for i in range(1,65)]
assert '"Pattern Bassline 1", 0, 63, 1, 0, "", patternChoices, 64' in p
assert '"Pattern Bassline 2", 0, 63, 1, 0, "", patternChoices, 64' in p
assert 'juce::jlimit (0, lb303::SequencerState::maximumSequences - 1, old + direction)' in e
assert 'juce::jlimit (0, lb303::SequencerState::maximumSequences - 1, old + direction)' in step
assert 'juce::jlimit (0, lb303::SequencerState::maximumSequences - 1, old + (code == juce::KeyPress::upKey ? 1 : -1))' in step
assert 'p > lb303::SequencerState::maximumSequences' in step
assert 'juce::jlimit (0, SequencerState::maximumSequences - 1, p)' in song
assert 'std::array<double, SequencerState::maximumSequences> fixedDurations' in song
assert 'const int savedSequenceCount' in s
assert 'sequenceBanks[0].serialiseToBase64()' in processor and 'sequenceBanks[1].serialiseToBase64()' in processor
# No new parameter IDs; old selectors stay in their original slots.
paramh=read('Source/ParameterCatalog.h')
assert 'pattern1Index = audioParameterCount' in paramh
assert 'pattern2Index = audioParameterCount + 1' in paramh
assert 'parameterCount = reverbDuckRecoveryIndex + 1' in paramh
# Keyboard copy/paste available only on Bassline pages and before legacy Alt+V.
assert 'basslineClipboardHeader = "LB303-BASSLINE-PARAMS-V1:"' in e
assert 'copyBasslineParameters (Control)' in head and 'pasteBasslineParameters (Control)' in head
assert 'juce::SystemClipboard::copyTextToClipboard' in e
assert 'juce::SystemClipboard::getTextFromClipboard()' in e
assert 'if (context >= 2) return;' in e
assert 'if (context < 2)' in e
assert 'if (letter (\'c\')) copyBasslineParameters (current);' in e
assert 'else pasteBasslineParameters (current);' in e
assert 'if (current == Control::edit && editDirty && ! commitEdit())' in e
assert 'LB303-BASSLINE-PARAMS-V1:' not in step # step clipboard remains independent
assert 'constexpr int count = static_cast<int> (lb303::voiceGridParameterCount)' in e
assert 'decoded.getDataSize() != static_cast<size_t> (12 + count * 4)' in e
assert 'if (! std::isfinite (v))' in e
assert 'juce::jlimit (info.minimum, info.maximum' in e
assert 'param->beginChangeGesture();' in e and 'param->endChangeGesture();' in e
assert 'Tab' in e and 'setDescription ("Alt+V")' not in e
assert 'song->setAttribute ("data", songs.serialise())' in processor
assert 'TEST2.4.62-Windows-x64' in w
assert 'python Tests/check_bassline_clipboard_64_patterns.py' in w
manual=read('Help/LB303Help.html')
for lang in ('en','it','es','fr','ru','zh','ja'):
    section=manual.split('<article class="manual" id="'+lang+'"',1)[1].split('</article>',1)[0]
    assert 'TEST2.4.57' in section and 'Alt+C' in section and 'Alt+V' in section and 'P1–P64' in section
print('PASS: TEST2.4.57 64 patterns per lane; LSQ2 legacy load; Song P64; secure Bassline clipboard; 7 manuals.')
