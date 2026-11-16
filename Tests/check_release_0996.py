#!/usr/bin/env python3
"""Public LB-303 0.99.6 release metadata and seven-language Help check."""
from pathlib import Path
from html.parser import HTMLParser
R=Path(__file__).resolve().parents[1]
def t(p):return (R/p).read_text(encoding='utf-8')
assert 'project(LB303 VERSION 0.99.6 LANGUAGES C CXX)' in t('CMakeLists.txt')
assert 'lb303Version = "0.99.6"' in t('Source/PluginEditor.cpp')
assert 'lb303ReleaseDate = "16 November 2026"' in t('Source/PluginEditor.cpp')
assert 'LB-303-0.99.6-Windows-x64' in t('.github/workflows/build-windows.yml')
assert (R/'RELEASE_NOTES_0.99.6.md').is_file()
assert '0.99.6' in t('README.md') and 'Ctrl+Shift+A' in t('README.md')
help=t('Help/LB303Help.html')
class Parser(HTMLParser):
  def error(self,message):raise AssertionError(message)
Parser().feed(help)
assert help.count('<article class="manual"')==7
for lang in ('en','it','es','fr','ru','zh','ja'):
  part=help.split(f'<article class="manual" id="{lang}"',1)[1].split('</article>',1)[0]
  for value in ('0.99.6','16 November 2026','AHDSR','Ctrl+Shift+A','Ctrl+Shift+Tab','44'):
    assert value in part,(lang,value)
assert '0.99.3' not in help and '9 November 2026' not in help
assert '"b1_env_hold", "Hold", 0.0f, 5.0f, 0.001f, 0.0f, "s"' in t('Source/ParameterCatalog.cpp')
assert '"b2_env_hold", "Hold", 0.0f, 5.0f, 0.001f, 0.0f, "s"' in t('Source/ParameterCatalog.cpp')
print('PASS 0.99.6: About, VST3 metadata, REAPER automation, seven Help languages, release notes')
