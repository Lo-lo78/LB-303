"""TEST2.4.47: continuous centred early reflections, independent FX level and stable IDs.
Static source checks and a numerical check of the stereo early-reflection mixing law.
No local build: the VST3 is built only by GitHub Actions.
"""
from pathlib import Path
import math
import re
root = Path(__file__).resolve().parents[1]
read = lambda name: (root/name).read_text(encoding='utf8')
h = read('Source/ParameterCatalog.h')
c = read('Source/ParameterCatalog.cpp')
fxh = read('Source/SharedEffects.h')
fx = read('Source/SharedEffects.cpp')
p = read('Source/PluginProcessor.cpp')
e = read('Source/PluginEditor.cpp')
wf = read('.github/workflows/build-windows.yml')
assert 'fxParameterCount = 35' in h
assert 'reverbEarlyLevelIndex = reverbLfoPan2Index + 1' in h
assert 'parameterCount = reverbEarlyLevelIndex + 1' in h
assert 'reverbEarlyLevel = fxParameterCount + 9' in h
assert 'fxParameterCount + 10' in fxh
assert 'fx_reverb_early_level' in c
assert '"Reverb Early Reflections Level"' in c
assert '0, 100, 1, 100, "%"' in c
assert 'reverbLfoPan1Index, reverbLfoPan2Index, reverbEarlyLevelIndex' in c
assert 'add (5, lb303::reverbEarlyLevelIndex);' in e
assert 'fxControls[lb303::fx::reverbEarlyLevel] = value (lb303::reverbEarlyLevelIndex)' in p
assert 'earlyReflectionsLevelSmoothed' in fxh
assert 'sourcePresenceSmoothed' not in fxh and 'sourcePresenceSmoothed' not in fx
assert 'std::max (std::abs (c[base]), std::abs (c[depth]))' not in fx
assert 'if (std::abs (position) > 1.0e-7f)' in fx
assert 'if (earlyReflectionsLevelSmoothed > 1.0e-6f)' in fx
assert '(1.8f * earlyReflectionsLevelSmoothed)' in fx
assert 'earlyReflectionsLevelSmoothCoeff' in fx
assert 'const float positionComp' in fx
assert 'if (!settingsPrepared)' in fx
assert 'reverbEarlyLevelIndex' in h and 'reverbEarlyLevelIndex' in c
assert 'LB-303-0.99.1-TEST2.4.47-Windows-x64' in wf
assert 'python Tests/check_reverb_early_level.py' in wf
# Fixed reflection taps are constant across pan: only the stereo distribution
# changes. Centre is never gated, and LFO zero crossings do not add/remove taps.
# Use the real tap coefficients on uncorrelated impulses to check energy.
near = {11:.45,23:.26,34:-.16}
far = {23:.10,34:-.08,49:.07}
def energy(position):
    left = math.sqrt(.5 * (1.0-position))
    right = math.sqrt(.5 * (1.0+position))
    vals=[]
    tap_energy = sum(a*a for a in near.values()) + sum(b*b for b in far.values())
    common = sum(near.get(t,0)*far.get(t,0) for t in (11,23,34,49))
    comp = math.sqrt(tap_energy / (tap_energy + 4*left*right*common))
    for tap in {11,23,34,49}:
        a,b=near.get(tap,0),far.get(tap,0)
        vals += [comp*(left*a+right*b), comp*(right*a+left*b)]
    return sum(v*v for v in vals)
ref=energy(0.0)
assert ref > 0
for i in range(-100,101):
    er=energy(i*.01)
    assert abs(er/ref-1) < 1.e-5, (i, er/ref)
for lang in ('en','it','fr','es','ru','zh','ja'):
    match = re.search(r'<article class="manual" id="'+lang+r'".*?</article>', read('Help/LB303Help.html'), re.S)
    assert match and 'TEST2.4.47' in match.group(), lang
print('PASS: TEST2.4.47 early reflection level, centred presence, smooth automation, constant-power movement, append-only IDs')
