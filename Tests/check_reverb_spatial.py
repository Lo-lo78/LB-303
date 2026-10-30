"""TEST2.4.44: append-only spatial reverb, FDN rotations and defaults.

Cross-platform static checks on CI; native smoke test only when g++/clang++
is available locally. The real VST3 build is performed by GitHub Actions.
"""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
def read(name):
    return (root / name).read_text(encoding="utf-8")

h = read('Source/ParameterCatalog.h')
c = read('Source/ParameterCatalog.cpp')
e = read('Source/PluginEditor.cpp')
p = read('Source/PluginProcessor.cpp')
fxh = read('Source/SharedEffects.h')
fx = read('Source/SharedEffects.cpp')
workflow = read('.github/workflows/build-windows.yml')

# Do not disturb any legacy automations, original bass controls, or locks.
assert 'fxParameterCount = 35' in h
assert 'reverbBassXoverIndex = reverbBassDecayIndex + 1' in h
assert 'reverbMotionModeIndex = reverbBassXoverIndex + 1' in h
assert 'reverbMotionDepthIndex = reverbMotionModeIndex + 1' in h
assert 'reverbMotionRateIndex = reverbMotionDepthIndex + 1' in h
assert 'reverbSourcePan1Index = reverbMotionRateIndex + 1' in h
assert 'sequencerPerformanceBase = legacyParameterCount + fxParameterCount' in h
assert 'fxDisplayParameterCount = fxParameterCount + classicDelayParameterCount + 1 + 2 + 3 + 4 + 1' in h
assert 'std::array<float, fxParameterCount + 10>' in fxh
for index, label, ident in [
    ('reverbMotionModeIndex', 'Reverb Motion Mode', 'fx_reverb_motion_mode'),
    ('reverbMotionDepthIndex', 'Reverb Motion Depth', 'fx_reverb_motion_depth'),
    ('reverbMotionRateIndex', 'Reverb Motion Rate', 'fx_reverb_motion_rate')]:
    assert f'result[{index}]' in c and f'"{label}"' in c and f'"{ident}"' in c
    assert f'add (5, lb303::{index})' in e
    local = index.replace('Index', '')
    assert f'fxControls[lb303::fx::{local}] = value (lb303::{index})' in p
assert '"Off", "Left to Right", "Right to Left", "Bounce", "Orbit", "Expand", "Contract"' in c
assert '0, 6, 1, 0, "", reverbMotionChoices, 7' in c
assert '0, 100, 1, 60, "%"' in c
assert '0.02f, 2.0f, 0.01f, 0.35f, "Hz"' in c
assert 'reverbMotionModeIndex, reverbMotionDepthIndex, reverbMotionRateIndex' in c
assert 'index == reverbMotionModeIndex || index == reverbMotionDepthIndex' in c
assert 'index == reverbMotionRateIndex' in c

# Rotations happen on the Householder reflection vector before damping and
# in-loop bass splitting. The original final wet mid/side must be unchanged.
start = fx.index('std::pair<float, float> SharedEffects::reverb (')
end = fx.index('SharedEffects::Output SharedEffects::process (', start)
body = fx[start:end]
assert body.index('std::array<float, 8> reflected') < body.index('verbDamped[i] += (reflected[i]-verbDamped[i])*damp')
assert 'reflected[i] = sum * .25f - current[i]' in body
assert 'reflected[a] = cosine * x - sine * y;' in body
assert 'reflected[b] = sine * x + cosine * y;' in body
assert 'const size_t a = pair * 2, b = a + 1' in body
assert 'const size_t a = pair, b = pair + 4' in body
assert 'verbMotionAngles.fill (0.0f)' in fx and 'verbMotionPhase = verbMotionAge = 0.0' in fx
assert 'const float targetDepth = motionMode == 0 ? 0.0f' in body
assert 'verbMotionDepthSmoothed += (targetDepth - verbMotionDepthSmoothed)' in body
assert 'verbMotionAge = std::min (120.0, verbMotionAge + 1.0 / fs)' in body
assert 'verbMotionSmoothCoeff' in body
assert 'const float middle = .5f*(wetL+wetR), side = .5f*(wetL-wetR)*width;' in body
assert 'wetL *= .23f; wetR *= .23f;' in body
assert 'verbBassGainDeltaSmoothed[i]' in body
assert 'const float source = (((i & 1)==0 ? inL : inR) + spatialExcitationDelta[i]) * .18f' in body
assert 'verb[i].push (bounded (source + feedback));' in body
assert 'TEST2.4.49-Windows-x64' in workflow
assert 'python Tests/check_reverb_spatial.py' in workflow
for lang in ('en', 'it', 'es', 'fr', 'ru', 'zh', 'ja'):
    html = read('Help/LB303Help.html')
    block = re.search(r'<article class="manual" id="'+lang+r'".*?</article>', html, re.S)
    assert block and 'TEST2.4.44' in block.group()
print('PASS: TEST2.4.44 stable IDs, accessibility, 3 controls, 6 feedback reflection modes and help translations.')

compiler = (shutil.which('g++') or shutil.which('clang++')) if os.name != 'nt' else None
if compiler:
    with tempfile.TemporaryDirectory(prefix='lb303_spatial_') as tmp:
        binary = str(Path(tmp) / 'spatial_smoke')
        subprocess.run([compiler, '-std=c++20', '-O2', '-I', str(root/'Tests/Shim'),
            '-I', str(root/'Source'), str(root/'Source/SharedEffects.cpp'),
            str(root/'Source/BassEngine.cpp'), str(root/'Tests/test_reverb_spatial.cpp'),
            '-o', binary], check=True, timeout=90)
        report = subprocess.check_output([binary], text=True, timeout=60)
        assert 'PASS: spatial FDN movement' in report, report
    print('PASS: six audible modulation modes, energy/finite checks, precise Off invariance and runtime automation.')
else:
    print('SKIP: optional C++ DSP smoke tests; GitHub Actions builds the complete Windows VST3.')
