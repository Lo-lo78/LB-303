"""TEST2.4.61: Space starts and stops preview without speaking."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
for name, call in [('PluginEditor.cpp','processor.toggleSequencerPreview()'),
                   ('SequencerStepEditor.cpp','processor.toggleSequencerPreview()'),
                   ('PresetBrowser.cpp','manager.instrument().toggleSequencerPreview()')]:
    code=(root/'Source'/name).read_text(encoding='utf-8')
    assert call in code, name
    assert 'Both basslines preview playing' not in code, name
    assert 'Both basslines preview stopped' not in code, name
    assert 'juce::ignoreUnused (' in code, name
assert 'TEST2.4.72' in (root/'.github/workflows/build-windows.yml').read_text(encoding='utf-8')
print('PASS: LB303 preview still toggles in all three views without announcement')
