#!/usr/bin/env python3
"""Static TEST1 source invariants. Does not compile/build anything."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
original = (root / 'Reference' / 'LB-303.jsfx').read_text(encoding='utf-8')
catalog = (root / 'Source' / 'ParameterCatalog.cpp').read_text(encoding='utf-8')
ids = re.findall(r'"slider(\d{3})"', catalog)
expected = [f'{n:03}' for n in range(1, 36)]
assert ids == expected, (len(ids), ids)
# JUCE 8 uses juce::Optional, which does not implement std::optional::value_or.
assert '.value_or(' not in (root / 'Source' / 'PluginProcessor.cpp').read_text(encoding='utf-8')
assert len(re.findall(r'^slider\d+:', original, re.M)) == 35
for src in ['ParameterCatalog.cpp', 'BassEngine.cpp', 'PluginEditor.cpp', 'PluginProcessor.cpp', 'ScreenReaderAnnouncer.cpp']:
    assert (root / 'Source' / src).is_file(), src
assert (root / '.github/workflows/build-windows.yml').is_file()
assert 'LB303_artefacts' in (root / '.github/workflows/build-windows.yml').read_text()
assert 'Alt+Q' in (root / 'README.md').read_text()

# Regression test TEST1.2: JUCE setResizeLimits() can synchronously call
# resized(); creating all cells must precede ALL resizing-related setup.
editor = (root / 'Source' / 'PluginEditor.cpp').read_text(encoding='utf-8')
constructor = editor.split('LB303AudioProcessorEditor::LB303AudioProcessorEditor (', 1)[1].split('LB303AudioProcessorEditor::~', 1)[0]
assert constructor.find('cells[i] = std::make_unique<ParameterCell>') < constructor.find('setResizable (true, true)')
assert constructor.find('cells[i] = std::make_unique<ParameterCell>') < constructor.find('setResizeLimits (')
assert 'if (cells[i] != nullptr)' in editor.split('void LB303AudioProcessorEditor::resized()', 1)[1].split('void LB303AudioProcessorEditor::timerCallback()', 1)[0]

print('PASS: parameter and safe editor construction checks; no compilation performed.')
