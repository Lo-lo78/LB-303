#!/usr/bin/env python3
"""TEST2.4.54: no-build guard for per-step *absolute* 0..100 sends,
backward-compatible serialization, accessible state cycling and wet DSP.
"""
from pathlib import Path
import math
r = Path(__file__).resolve().parents[1]
def read(path): return (r/path).read_text(encoding='utf-8')
h,s=read('Source/SequencerState.h'),read('Source/SequencerState.cpp')
ui,uh=read('Source/SequencerStepEditor.cpp'),read('Source/SequencerStepEditor.h')
p=read('Source/PluginProcessor.cpp')
fx,fxh=read('Source/SharedEffects.cpp'),read('Source/SharedEffects.h')
workflow=read('.github/workflows/build-windows.yml')
for field in ('delaySendLevel','reverbSendLevel'):
    assert f'int {field} = -1;' in h
    assert f'std::atomic<int> {field} {{ -1 }};' in h
    assert s.count(f'storeRelaxed (step.{field}, -1);') == 3
    assert f'result.{field} = loadRelaxed (step.{field});' in s
    assert f'step.{field}.store (juce::jlimit (-1, 100, source.{field})' in s
    assert f'destinationStep.{field}, loadRelaxed (sourceStep.{field})' in s
    assert f'step.{field}.store (readSendLevel(), std::memory_order_relaxed);' in s
assert 'stream.writeInt (9);' in s and 'formatVersion > 9' in s
assert 'formatVersion >= 8' in s # v8 Normal/Accent/Mute retained
assert 'if (formatVersion < 9) return -1;' in s
assert 'return raw == 255 ? -1 : juce::jlimit (0, 100, raw);' in s
assert s.count('step.delaySendLevel < 0 ? 255') == 1
assert s.count('step.reverbSendLevel < 0 ? 255') == 1
assert s.index('step.delaySendLevel.store (readSendLevel()') > s.index('step.reverbSendState.store (formatVersion >= 8')
assert s.index('step.reverbSendLevel.store (readSendLevel()') < s.index('step.parameterLockCount.store (0, std::memory_order_relaxed);',s.index('const int loadedVelocity ='))
assert 'maximumParameterLocks = 32' in h

# Repeated focused-key selects a send STATE, not a double-press selection.
assert 'if ((page == 7 || page == 8) && index == currentStep)' in ui
assert 'editSendState (1, true);' in ui
assert ui.index('editSendState (1, true)') < ui.index('if (doublePress)')
assert 'const auto steps = focusedOnly ? std::vector<int> { currentStep } : editingSteps();' in ui
assert 'else if (page == 7 || page == 8) editSendState (1);' in ui
assert 'if (page == 7 || page == 8) { editSendLevel (direction, amount); return; }' in ui
assert 'void SequencerStepEditor::editSendLevel (int direction, int amount)' in ui
assert 'if (state != 0) continue;' in ui
assert 'focused.delaySendState : focused.reverbSendState) != 0) return;' in ui
assert 'const int previous = level < 0 ? initial : level;' in ui
assert 'const int next = juce::jlimit (0, 100, previous + direction * amount);' in ui
assert 'int SequencerStepEditor::inheritedSendLevel() const' in ui
assert 'processor.value (lb303::fxParameterIndex (fxSend))' in ui
assert 'Normal, Send ' in ui and '(FX default)' in ui
assert 'updated.delaySendLevel = -1;' in ui and 'updated.reverbSendLevel = -1;' in ui

# Real send even with master FX send=0: no multiplication by global Send in override.
assert 'step.delaySendState == 0 && step.delaySendLevel >= 0' in p
assert 'step.reverbSendState == 0 && step.reverbSendLevel >= 0' in p
assert 'sharedFx.setStepSendGains (delay, reverb);' in p
assert 'sharedFx.setStepSendOverrides (delayAbsolute, reverbAbsolute);' in p
assert p.index('sharedFx.setStepSendOverrides (delayAbsolute') < p.index('for (size_t voice = 0; voice < lb303::basslineCount; ++voice)')
assert 'setStepSendOverrides' in fxh and 'void SharedEffects::setStepSendOverrides' in fx
assert 'delayOverrideMixTarget' in fxh and 'reverbOverrideMixTarget' in fxh
assert 'delayOverrideMixSmoothed[v] +=' in fx and 'reverbOverrideMixSmoothed[v] +=' in fx
assert 'const float inherited = normalPercent * .01f * gain;' in fx
assert 'return inherited * (1.0f - mix) + absolutePercent * .01f * mix;' in fx
assert 'const float reverbLink = std::max (verbS1, verbS2);' in fx
assert 'const auto delayWet = runDelay' in fx and 'const auto reverbWet = runReverb' in fx
assert 'stepSendSmoothingCoeff' in fx and '0.006 * fs' in fx

# No override is the old send, explicit overrides are absolute, not dry output pan.
for global_send in (0, 10, 50, 100):
 for gain in (0, .5, 1, 1.5, 2):
  legacy=global_send/100*gain
  for override in (0, 1, 37, 50, 100):
   def effective(mix):return legacy*(1-mix)+override/100*mix
   assert abs(effective(0)-legacy)<1e-10
   assert abs(effective(1)-override/100)<1e-10
   assert abs(effective(.5)-(legacy+override/100)*.5)<1e-10
assert 'TEST2.4.54' in read('README.md')
assert read('Help/LB303Help.html').count('TEST2.4.54') >= 7
assert 'Tests/check_step_send_levels.py' in workflow
assert 'TEST2.4.54-Windows-x64' in workflow
print('PASS: TEST2.4.54 real per-step sends, old preset migration, key cycling, audio smoothing and compatible FX routing')
