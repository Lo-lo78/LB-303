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
print('PASS: 35 stable params and expected TEST1 sources present; no compilation performed.')
