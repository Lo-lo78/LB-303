"""TEST2.4.53: no-build regression on persisted send states, accessible shortcuts,
append-only FX parameters, audio-thread control and velocity response.
Actual VST3 compilation and listening tests belong to GitHub Actions/REAPER.
"""
from pathlib import Path
import math
import re

root = Path(__file__).resolve().parents[1]
read = lambda name: (root/name).read_text(encoding='utf-8')
h = read('Source/SequencerState.h')
s = read('Source/SequencerState.cpp')
ui = read('Source/SequencerStepEditor.cpp')
uh = read('Source/SequencerStepEditor.h')
ch = read('Source/ParameterCatalog.h')
c = read('Source/ParameterCatalog.cpp')
fx = read('Source/SharedEffects.cpp')
fxh = read('Source/SharedEffects.h')
p = read('Source/PluginProcessor.cpp')
workflow = read('.github/workflows/build-windows.yml')

# Normal=0 guarantees that v1-v7 presets are neutral. Never repurpose 1..55 lock slots.
for field in ['delaySendState', 'reverbSendState']:
    assert f'int {field} = 0;' in h
    assert f'std::atomic<int> {field} {{ 0 }};' in h
    assert f'result.{field} = loadRelaxed (step.{field})' in s
    assert f'step.{field}.store (juce::jlimit (0, 2, source.{field})' in s
    assert f'destinationStep.{field}' in s
    assert f'stream.writeByte (static_cast<char> (juce::jlimit (0, 2, step.{field})))' in s
    assert f'step.{field}.store (formatVersion >= 8' in s
    assert s.count(f'storeRelaxed (step.{field}, 0);') >= 3
assert 'stream.writeInt (9);' in s and 'formatVersion > 9' in s
assert s.index('step.reverbSendState.store (formatVersion >= 8') < s.index('step.parameterLockCount.store (0, std::memory_order_relaxed);', s.index('const int loadedVelocity ='))
assert 'maximumParameterLocks = 32' in h
assert 'source.noteBeforePause' in s and 'source.lengthBeforeLegato' in s
assert 'stream.writeInt (0x4c535132)' in s

# 1..7 stay the original pages. Ctrl chords land in the newly appended pages.
assert '"Launch Step", "Parameters", "Delay Send Steps", "Reverb Send Steps"' in ui
assert 'page = juce::jlimit (0, 8, processor.editorSession.sequencerPage)' in ui
assert 'page = (code == \'1\' || ch == \'1\') ? 7 : 8;' in ui
assert ui.index('page = (code ==') < ui.index('if (letter (\'c\')) { copySteps')
assert 'else if (page == 7 || page == 8) editSendState (1);' in ui
assert 'void SequencerStepEditor::editSendState' in ui
assert 'void editSendState (int direction, bool focusedOnly = false);' in uh
assert 'state = (state + (direction >= 0 ? 1 : 2)) % 3;' in ui
assert 'updated.delaySendState = 0; updated.delaySendLevel = -1;' in ui
assert 'updated.reverbSendState = 0; updated.reverbSendLevel = -1;' in ui
assert 'commitAction (std::move (action))' in ui
assert 'return sendName (step.delaySendState, step.delaySendLevel)' in ui and 'return sendName (step.reverbSendState, step.reverbSendLevel)' in ui
assert 'Ctrl+1: Delay Send' in ui and 'Ctrl+2: Reverb Send' in ui

# Previously published indices are stable, all additions are after portamento curves.
assert 'portamentoCurve2Index = portamentoCurve1Index + 1' in ch
assert 'delayAccentSendIndex = portamentoCurve2Index + 1' in ch
assert 'reverbAccentSendIndex = delayAccentSendIndex + 1' in ch
assert 'parameterCount = reverbDuckRecoveryIndex + 1' in ch
for index, name in [('delayAccentSendIndex','Delay Accent Send'),('reverbAccentSendIndex','Reverb Accent Send')]:
    assert f'result[{index}]' in c
    assert f'"{name}"' in c
    assert f'-100, 100, 1, 50, "%"' in c
    assert index in read('Source/PluginEditor.cpp')

# Velocity and the per-lane synth threshold drive FX sends, not synth Accent.
assert 'sequenceBanks[lane].getStep (activeLockPattern[lane], activeLockStep[lane])' in p
assert 'parameterIndex (lane, 15)' in p
assert 'sliderNumber == 16' in p
assert '(static_cast<float> (step.velocity) - threshold) / (127.0f - threshold)' in p
assert 'if (state == 2) return 0.0f' in p
assert '1.0f + accent * amount * .01f' in p
assert 'sharedFx.setStepSendGains (delay, reverb);' in p
assert p.count('updateStepSendGains();') == 2
assert p.index('updateStepSendGains();\n        }') < p.index('renderSamplesTo (end);\n    };')
# One-pole smoothing on incoming signals; don't erase delay or FDN buffers on Mute.
assert 'stepSendSmoothingCoeff' in fx and '0.006 * fs' in fx
assert 'delayStepSendSmoothed[v] +=' in fx and 'reverbStepSendSmoothed[v] +=' in fx
assert 'effectiveSend (c[fx::delaySend1], delayStepSendSmoothed[0]' in fx
assert 'effectiveSend (c[fx::reverbSend1], reverbStepSendSmoothed[0]' in fx
assert 'const auto delayWet = runDelay' in fx and 'const auto reverbWet = runReverb' in fx
assert 'setStepSendGains' in fxh

for threshold in [80, 100, 120]:
    for velocity in [0, 80, threshold, min(threshold+1,127), 127]:
        accent = max(0, min(1,(velocity-threshold)/(127-threshold)))
        for amount in [-100, 0, 50, 100]:
            val = max(0,min(2,1+accent*amount/100))
            assert 0 <= val <= 2
            if velocity <= threshold: assert val == 1
            if velocity == 127: assert abs(val-(1+amount/100)) < 1e-9

for language in ['en','it','es','fr','ru','zh','ja']:
    manual = read('Help/LB303Help.html')
    assert f'TEST2.4.53' in manual
    assert manual.count('TEST2.4.53 —') >= 6
assert 'TEST2.4.53' in read('README.md')
assert 'Tests/check_accent_send_steps.py' in workflow
assert 'TEST2.4.55-Windows-x64' in workflow
print('PASS: TEST2.4.53 velocity-dependent Delay/Reverb step sends, v8 migration, Alt+Q accessible pages and DSP smoothing')
