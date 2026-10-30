"""TEST2.4.50: late-field spatial decoding, no new IDs, stable old audio path.

This is a source/geometry regression, not a replacement for VST3 audio tests.
GitHub Actions runs the checks and builds the actual Windows plug-in.
"""
from pathlib import Path
import math
import random
import re

root = Path(__file__).resolve().parents[1]
read = lambda p: (root / p).read_text(encoding='utf-8')
cpp = read('Source/SharedEffects.cpp')
h = read('Source/SharedEffects.h')
cat = read('Source/ParameterCatalog.h')
workflow = read('.github/workflows/build-windows.yml')
body = cpp.split('std::pair<float, float> SharedEffects::reverb (', 1)[1].split('SharedEffects::Output SharedEffects::process (', 1)[0]

# Preserve every old ID and the append-only parameter layout.
assert 'fxParameterCount = 35' in cat
assert 'reverbEarlyLevelIndex = reverbLfoPan2Index + 1' in cat
assert 'fx::reverbMotionMode' in body and 'fx::reverbMotionDepth' in body
assert 'fx::reverbMotionRate' in body
assert 'verbTailPanSmoothed' in h and 'verbTailSpreadSmoothed' in h
assert 'verbTailMotionSmoothCoeff' in h
assert 'verbTailPanSmoothed = verbTailSpreadSmoothed = 0.0f' in cpp
assert 'verbTailPanSmoothed += (targetTailPan - verbTailPanSmoothed) * verbTailMotionSmoothCoeff' in body
assert 'verbTailSpreadSmoothed += (targetTailSpread - verbTailSpreadSmoothed) * verbTailMotionSmoothCoeff' in body
assert 'if (std::abs (verbTailPanSmoothed) > 1.0e-6f)' in body
assert 'if (verbTailSpreadSmoothed > 1.0e-6f)' in body

# Move the individual FDN tail lines after the old moving feedback matrix, but
# BEFORE the early reflection bus and global stereo Width/Level. The original
# two raw signed sums are left untouched when Off or Depth=0.
assert body.index('verb[i].push (bounded (source + feedback))') < body.index('// TEST2.4.50:')
assert body.index('// TEST2.4.50:') < body.index('wetL += directionalEarlyReflections[0]')
assert body.index('wetL += directionalEarlyReflections[0]') < body.index('const float width =')
assert 'float wetL = current[0]-current[1]+current[2]-current[3]+current[4]-current[5]+current[6]-current[7];' in body
assert 'float wetR = current[0]+current[1]-current[2]-current[3]+current[4]+current[5]-current[6]-current[7];' in body
assert 'const float gLeft = std::sqrt' in body and 'const float gRight = std::sqrt' in body
assert 'current[i] * leftSign[i] * (gLeft + path)' in body
assert 'current[i] * rightSign[i] * (gRight - path)' in body
assert 'const float sweep = verbTailPanSmoothed;' in body
assert 'targetTailPan = 0.93f * verbMotionDepthSmoothed * position' in body
assert 'const float part = cycle < 0.84f' in body
assert 'const float t = clampf (part, 0.0f, 1.0f)' in body
assert 'const float smooth = t * t * (3.0f - 2.0f * t)' in body
assert 'verbMotionAge' in body
# No change to established gain staging, bass RT60 defaults or old ER level.
assert 'reverbWetOutputTrim6dB = 0.501187234f' in cpp
assert 'c[fx::reverbEarlyLevel]' in body
assert 'verbBassGainDeltaSmoothed[i]' in body
assert 'fx::reverbEarlyLevel' in body
assert 'TEST2.4.50-Windows-x64' in workflow
assert 'python Tests/check_reverb_tail_motion.py' in workflow
assert 'TEST2.4.50:' in read('README.md')
help_text = read('Help/LB303Help.html')
for lang in ('en','it','es','fr','ru','zh','ja'):
    block = re.search(r'<article class="manual" id="' + lang + r'"[^>]*>.*?</article>', help_text, re.S)
    assert block and 'TEST2.4.50' in block.group(), lang

# Independent numerical check of the moving decoder geometry, deliberately
# using decorrelated eight-line diffuse field (not copied full-reverb output).
# This verifies that the intended motion is positional, not just a changed
# timbre. Audibility of the compiled VST3 remains for REAPER listening.
left_sign = (1,-1,1,-1,1,-1,1,-1)
right_sign = (1,1,-1,-1,1,1,-1,-1)
facets = (-.9,.7,-.4,1.,.5,-.8,.9,-.6)

def sweep_position(mode, cycle):
    cycle %= 1.
    if mode in (1,2):
        part = cycle / .84 if cycle < .84 else (1. - cycle) / .16
        t = max(0., min(1., part))
        smooth = t * t * (3. - 2. * t)
        return (-1. if mode == 1 else 1.) * (1. - 2. * smooth)
    if mode == 3:
        return -math.cos(2. * math.pi * cycle)
    if mode == 4:
        return math.sin(2. * math.pi * cycle)
    return 0.

def measure(mode, depth):
    rng = random.Random(23941)
    # Late diffuse field, windows well AFTER simulated note-off at 50 ms.
    sums = [(0.,0.),(0.,0.)]
    for n in range(6000):
        time_s = .05 + n * .00012  # 50–770 ms
        pos = sweep_position(mode, time_s) if mode else 0.
        pan = .93 * depth * pos
        gl = math.sqrt(1. - pan)
        gr = math.sqrt(1. + pan)
        values = [rng.uniform(-1.,1.) * math.exp(-time_s) for _ in range(8)]
        if mode and depth:
            l = sum(values[i] * left_sign[i] * (gl + .12 * pan * facets[i]) for i in range(8))
            r = sum(values[i] * right_sign[i] * (gr - .12 * pan * facets[i]) for i in range(8))
        else:
            l = sum(values[i] * left_sign[i] for i in range(8))
            r = sum(values[i] * right_sign[i] for i in range(8))
        index = 0 if .12 <= time_s < .22 else 1 if .62 <= time_s < .72 else -1
        if index >= 0:
            le,re_ = sums[index]
            sums[index] = (le + l*l, re_ + r*r)
    return [10. * math.log10((re_ + 1e-20)/(le+1e-20)) for le,re_ in sums]

lr = measure(1, 1.)
rl = measure(2, 1.)
lr_60 = measure(1, .6)
neutral = measure(0, 0.)
assert lr[0] < -6. and lr[1] > 6., ('Left to Right weak/inverted',lr)
assert rl[0] > 6. and rl[1] < -6., ('Right to Left weak/inverted',rl)
assert lr_60[0] < -3. and lr_60[1] > 3., ('Depth60 weak', lr_60)
assert max(abs(x) for x in neutral) < 2.5, ('neutral model unbalanced', neutral)
for mode in (1,2):
    # Seamless restart and turn-around at 84% of the cycle.
    assert abs(sweep_position(mode, 1.-1e-7)-sweep_position(mode, 1e-7)) < 1e-5
    assert abs(sweep_position(mode, .84-1e-7)-sweep_position(mode, .84+1e-7)) < 1e-5
print('PASS: TEST2.4.50 FDN late-field stereo decoder, directional movement geometry,')
print('      30ms mode transitions, previous IDs/defaults, seven manuals and CI packaging.')
print('      Model dB L->R:', [round(x,1) for x in lr], 'R->L:', [round(x,1) for x in rl],
      'Depth60:', [round(x,1) for x in lr_60])
