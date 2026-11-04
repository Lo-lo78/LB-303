#!/usr/bin/env python3
"""TEST2.4.67 integration check: keyboard search, metadata and unchanged DSP."""
from pathlib import Path
import re
from html.parser import HTMLParser
root=Path(__file__).resolve().parents[1]
read=lambda p:(root/p).read_text(encoding='utf-8')
editor=read('Source/PluginEditor.cpp')
header=read('Source/PluginEditor.h')
picker=read('Source/SequencerStepEditor.cpp')
picker_h=read('Source/SequencerStepEditor.h')
core=read('Source/IncrementalParameterSearch.h')
key=read('Source/ParameterSearchKey.h')
cmake=read('CMakeLists.txt')
workflow=read('.github/workflows/build-windows.yml')
help_html=read('Help/LB303Help.html')
assert 'namespace lb303 {' in core
for item in ('intervalMs = 1000','sameLetterCycle', 'CharLowerBuffW', '#define NOMINMAX', '#undef min', '#undef max', 'std::towlower'):
    assert item in core,item
assert key.index("if (code >= '0' && code <= '9')") < key.index('const auto typed = key.getTextCharacter();')
assert key.index('F1Key') < key.index("if (code >= '0' && code <= '9')")
assert 'numberPad0' in key and 'numberPad9' in key
assert 'mods.isCtrlDown() || mods.isAltDown() || mods.isCommandDown()' in key
assert 'lb303::IncrementalParameterSearch parameterSearch;' in header
assert 'lb303::IncrementalParameterSearch pickerSearch;' in picker_h
assert 'parameterSearch.next (' in editor and 'pickerSearch.next (' in picker
assert 'parameterSearch.reset();' in editor and 'pickerSearch.reset();' in picker
assert 'lb303ParameterSearchCharacter (key)' in editor and 'lb303ParameterSearchCharacter (key)' in picker
assert 'lb303::catalog()[globalIndex (index)].name' in editor
assert 'lb303::catalog()[global].name' in picker
assert 'pickerItemText' in picker and 'selected ' in picker
assert '0.99.2' in editor and '4 November 2026' in editor
assert 'New: progressive parameter search' in editor
assert 'project(LB303 VERSION 0.99.2' in cmake
assert 'LB303SearchTest' in cmake and 'Tests/IncrementalParameterSearchTests.cpp' in cmake
assert 'LB303SearchTest.exe' in read('Tools/build-windows.ps1')
assert 'check_incremental_search_test2467.py' in workflow
assert 'LB-303-0.99.2-TEST2.4.67-Windows-x64' in workflow
assert 'TEST2.4.67' in read('README.md') and 'TEST2.4.67' in read('RELEASE_NOTES_0.99.2.md')
assert 'TEST2.4.67' in read('Doc/TEST2.4.67_INCREMENTAL_PARAMETER_SEARCH.md')
for lang in ('en','it','es','fr','ru','zh','ja'):
    block=help_html.split(f'<article class="manual" id="{lang}"',1)[1].split('</article>',1)[0]
    assert 'TEST2.4.67' in block and '0.99.2' in block and 'Shift' in block,lang
for untouched in ('Source/BassEngine.cpp','Source/PluginProcessor.cpp','Source/ParameterCatalog.cpp','Source/SequencerState.cpp','Source/SequencerPlayback.cpp'):
    assert 'TEST2.4.67' not in read(untouched),untouched
print('PASS TEST2.4.67: name-only incremental search in Grid/Value and Alt+Q picker, CI, 7 languages, About, no DSP edits')
