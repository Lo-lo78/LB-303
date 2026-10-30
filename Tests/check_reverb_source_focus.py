"""TEST2.4.46: pronounced directional reflections, unchanged historic ID map.

Static CI checks and native DSP measurement when a C++ compiler is available.
The actual Windows VST3 is built exclusively by GitHub Actions.
"""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import re
root = Path(__file__).resolve().parents[1]
def read(name):
    return (root / name).read_text(encoding='utf-8')
fx = read('Source/SharedEffects.cpp')
h = read('Source/SharedEffects.h')
params = read('Source/ParameterCatalog.h')
workflow = read('.github/workflows/build-windows.yml')
assert 'earlyReflectionsLevelSmoothed' in h
assert 'sourcePresenceSmoothed' not in fx
assert 'directionalEarlyReflections' in fx
assert 'leftBasis' in fx and 'rightBasis' in fx
assert 'spatialExcitationDelta[i] += steered - original;' in fx
assert 'nearLeft * nearWall + nearRight * oppositeWall' in fx
assert 'const float positionComp' in fx
assert 'nearRight * nearWall + nearLeft * oppositeWall' in fx
assert 'const double offset = readOffset + ms * .001 * fs;' in fx
assert fx.index('wetL += directionalEarlyReflections[0]') < fx.index('const float width =')
assert 'Reverb Source Pan 1' in read('Source/ParameterCatalog.cpp')
assert 'parameterCount = reverbEarlyLevelIndex + 1' in params
assert 'fxParameterCount = 35' in params
assert 'python Tests/check_reverb_source_focus.py' in workflow
assert 'TEST2.4.47-Windows-x64' in workflow
for lang in ('en', 'it', 'es', 'fr', 'ru', 'zh', 'ja'):
    block = re.search(r'<article class="manual" id="' + lang + r'".*?</article>',
                      read('Help/LB303Help.html'), re.S)
    assert block and 'TEST2.4.46' in block.group(), lang
print('PASS: TEST2.4.46 early spatial reflections, original parameters and CI verified')
compiler = ((shutil.which('g++') or shutil.which('clang++'))
            if os.environ.get('LB303_NATIVE_DSP_TEST') == '1' and os.name != 'nt' else None)
if compiler:
    with tempfile.TemporaryDirectory(prefix='lb303_focus_') as tmp:
        binary = str(Path(tmp)/'source_focus')
        subprocess.run([compiler, '-std=c++20', '-O2', '-I', str(root/'Tests/Shim'),
            '-I', str(root/'Source'), str(root/'Source/SharedEffects.cpp'),
            str(root/'Source/BassEngine.cpp'), str(root/'Tests/test_reverb_source_focus.cpp'),
            '-o', binary], check=True, timeout=120)
        print(subprocess.check_output([binary], text=True, timeout=120).strip())
else:
    print('SKIP: optional native C++ DSP focus test; Windows VST3 build on Actions')
