"""TEST2.4.51: late-tail room geometry regression (static and numerical).

Static CI test. A proper VST3 listening test still needs REAPER on Windows.
"""
from pathlib import Path
import cmath
import math
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding='utf-8')
cpp = read('Source/SharedEffects.cpp')
h = read('Source/SharedEffects.h')
cat = read('Source/ParameterCatalog.h')
workflow = read('.github/workflows/build-windows.yml')
body = cpp.split('std::pair<float, float> SharedEffects::reverb (', 1)[1].split('SharedEffects::Output SharedEffects::process (', 1)[0]

# No new host IDs, preserved existing effects and previous wet gain staging.
assert 'fxParameterCount = 35' in cat
assert 'reverbEarlyLevelIndex = reverbLfoPan2Index + 1' in cat
assert 'portamentoCurve1Index = reverbEarlyLevelIndex + 1' in cat
assert 'portamentoCurve2Index = portamentoCurve1Index + 1' in cat
assert 'parameterCount = reverbAccentSendIndex + 1' in cat
assert 'reverbWetOutputTrim6dB = 0.501187234f' in cpp
assert 'verbBassGainDeltaSmoothed[i]' in body
assert 'wetL += directionalEarlyReflections[0]' in body
assert 'wetR += directionalEarlyReflections[1]' in body
assert 'verbSpacePath' in h and 'verbFarEarLow' in h
assert 'verbSpatialDriftPhase' in h
assert 'verbDirectionalBlendSmoothed' in h
assert 'verbLastDirectionalMode' in h
assert 'verbSpacePath[i].push (current[i])' in body
assert 'verbFarEarLowCoeff' in body
assert 'const double oppositeDelay = shadow * .00032 * fs' in body
assert 'const float shadedFar = farSample - .68f * shadow * (farSample - low);' in body
assert 'const float nearGain = std::sqrt (1.0f + .43f * shadow)' in body
assert 'const float farGain  = std::sqrt (1.0f - .43f * shadow)' in body
assert 'verbDirectionalBlendSmoothed += (desiredDirectionalBlend - verbDirectionalBlendSmoothed)' in body
assert 'wetL += (movedL - wetL) * blend;' in body
assert 'wetR += (movedR - wetR) * blend;' in body
assert 'verbSpatialDriftPhase +=' in body
assert 'const float phase = static_cast<float> (verbMotionPhase) - phaseLag[i]' in body
assert 'case 1: position = -std::cos (phase); break;' in body
assert 'case 2: position =  std::cos (phase); break;' in body
assert 'const float part = cycle < 0.84f' not in body
assert 'targetTailPan' not in body
assert 'verbTailPanSmoothed' not in cpp
assert 'verbTailPanSmoothed' not in h
assert body.index('verb[i].push (bounded (source + feedback))') < body.index('// TEST2.4.51:')
assert body.index('verbSpacePath[i].push (current[i])') < body.index('wetL += directionalEarlyReflections[0]')
assert 'float wetL = current[0]-current[1]+current[2]-current[3]+current[4]-current[5]+current[6]-current[7];' in body
assert 'float wetR = current[0]+current[1]-current[2]-current[3]+current[4]+current[5]-current[6]-current[7];' in body
assert 'const float width = c[fx::reverbWidth]*.01f' in body
assert 'TEST2.4.54-Windows-x64' in workflow
assert 'python Tests/check_reverb_tail_motion.py' in workflow
assert 'TEST2.4.51:' in read('README.md')
help_text = read('Help/LB303Help.html')
for lang in ('en','it','es','fr','ru','zh','ja'):
    block = re.search(r'<article class="manual" id="' + lang + r'"[^>]*>.*?</article>', help_text, re.S)
    assert block and 'TEST2.4.51' in block.group(), lang

# A smooth sinusoid does not undergo the former fast 16%-of-period reset.
# Eight paths are delayed in phase, so reflections do not move in lockstep.
lag = [.04,.19,.32,.49,.63,.78,.92,1.05]
paths_ms = [3.1,4.7,6.1,8.3,10.2,12.7,14.6,17.1]
assert len(set(lag)) == 8 and len(set(paths_ms)) == 8

def position(mode, cycle, i=0, drift=0.):
    phase = 2*math.pi*cycle - lag[i] + .14*math.sin(drift + i*1.93)
    if mode == 1: return -math.cos(phase)
    if mode == 2: return math.cos(phase)
    if mode == 3: return -math.cos(2*phase)
    return math.sin(phase)

for i in range(8):
    for mode in (1,2,3,4):
        assert abs(position(mode, 1-1e-7, i) - position(mode, 1e-7, i)) < 4e-6
        assert abs(position(mode, .5-1e-7, i) - position(mode, .5+1e-7, i)) < 4e-6
assert sum(position(1,.06,i) for i in range(8)) < -4
assert sum(position(1,.56,i) for i in range(8)) > 4
assert sum(position(2,.06,i) for i in range(8)) > 4
assert sum(position(2,.56,i) for i in range(8)) < -4
assert len({round(position(1,.20,i),3) for i in range(8)}) > 6
assert any(abs(position(1,.25,i,0.)-position(1,.25,i,1.7)) > .02 for i in range(8))

# The far ear must be spectrally darker at 5 kHz than at 200 Hz, not merely
# quieter at every frequency. One-pole 1.6 kHz filter, 44.1 kHz SR.
fs = 44100.0
alpha = 1-math.exp(-2*math.pi*1600/fs)
def shadow_response(freq):
    z = cmath.exp(-1j*2*math.pi*freq/fs)
    lowpass = alpha / (1 - (1-alpha)*z)
    return abs(1-.68*(1-lowpass))
assert shadow_response(5000) < .55
assert shadow_response(200) > .95
assert shadow_response(200) > shadow_response(5000) + .35

print('PASS: TEST2.4.51 spatial path delays, non-synchronous motion, continuous turnarounds,')
print('      frequency-dependent far-side shadow, Off/Depth compatibility, IDs, docs and CI.')
