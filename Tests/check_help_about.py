#!/usr/bin/env python3
"""Regression checks for LJuno-equivalent Help/7 languages/About in LB-303."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
cm=(root/'CMakeLists.txt').read_text()
h=(root/'Source/PluginEditor.h').read_text()
editor=(root/'Source/PluginEditor.cpp').read_text()
help=(root/'Help/LB303Help.html').read_text(encoding='utf8')
assert 'project(LB303 VERSION 0.99.1' in cm
assert 'juce_add_binary_data(LB303HelpData' in cm
assert 'Help/LB303Help.html' in cm and 'LB303HelpData' in cm
assert 'BinaryData::getNamedResource ("LB303Help_html"' in editor
assert 'lb303ReleaseDate = "27 October 2026"' in editor
assert 'lb303Version = "0.99.1"' in editor
assert 'lb303ProjectUrl = "https://github.com/Lo-lo78/LB-303"' in editor
assert 'lb303ContactEmail = "vmanolo301@gmail.com"' in editor
assert 'setDescription ("Alt+H")' in editor and 'setDescription ("Alt+A")' in editor
assert "if (letter ('h')) { showHelpLanguageMenu(); return true; }" in editor
assert "if (letter ('a')) { openAbout(); return true; }" in editor
assert 'void LB303AudioProcessorEditor::showHelpLanguageMenu()' in editor
assert 'void LB303AudioProcessorEditor::openHelp ' in editor
assert 'juce::PopupMenu::Options().withTargetComponent (&help)' in editor
assert 'const requestedLanguage=' in editor
assert 'file.startAsProcess()' in editor
assert 'void LB303AudioProcessorEditor::openAbout()' in editor
assert 'void LB303AudioProcessorEditor::closeAbout()' in editor
assert 'navigateAboutText' in editor and 'handleAboutKey' in editor
assert 'if (aboutOpen) return;' in editor # do not steal overlay focus
assert 'closeAbout();' in editor
assert 'bool aboutOpen = false' in h
assert 'class LB303AboutTextEditor' in h
codes=['en','it','es','fr','ru','zh','ja']
assert len(re.findall(r'<article class="manual"',help))==7
for code in codes:
 assert f'id="{code}"' in help
 assert f'href="#{code}"' in help
assert "const supported=['en','it','es','fr','ru','zh','ja'];" in help
for term in ['0.99.1','27 October 2026','Alt+H','Alt+A','Alt+Q','Alt+B','101','Oversampling']:
 assert help.count(term)>=7,term
wf=(root/'.github/workflows/build-windows.yml').read_text()
assert 'python Tests/check_help_about.py' in wf
assert 'LB-303-0.99.1-TEST2.4.38-Windows-x64' in wf
assert (root/'.gitignore').exists()
print('PASS: Help HTML embedded, seven real translations, popup menu, Alt+H, About, Alt+A, version/date, NVDA navigation, CI and dotfiles')
