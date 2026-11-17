from pathlib import Path
R=Path(__file__).resolve().parents[1]
def t(f):return (R/f).read_text(encoding="utf-8")
assert 'project(LB303 VERSION 0.99.7 LANGUAGES C CXX)' in t('CMakeLists.txt')
assert 'lb303Version = "0.99.7"' in t('Source/PluginEditor.cpp')
assert 'lb303ReleaseDate = "17 November 2026"' in t('Source/PluginEditor.cpp')
assert 'LB-303-0.99.7-Windows-x64' in t('.github/workflows/build-windows.yml')
assert 'RELEASE_NOTES_0.99.7.md' in t('README.md')
assert 'Fixed: sequencer Accent' in t('Source/PluginEditor.cpp')
assert (R/'RELEASE_NOTES_0.99.7.md').exists()
print('PASS 0.99.7 release metadata')
