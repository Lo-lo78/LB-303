"""TEST2.4.45: append-only independent reverb-source spatial excitation and LFO wiring.

Static-only regression; Windows VST3 compilation is owned by GitHub Actions.
"""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
def read(name):
    return (root / name).read_text(encoding='utf-8')
h = read('Source/ParameterCatalog.h')
c = read('Source/ParameterCatalog.cpp')
e = read('Source/PluginEditor.cpp')
p = read('Source/PluginProcessor.cpp')
eng_h = read('Source/BassEngine.h')
eng = read('Source/BassEngine.cpp')
fxh = read('Source/SharedEffects.h')
fx = read('Source/SharedEffects.cpp')
wf = read('.github/workflows/build-windows.yml')

# These old indices MUST remain fixed: user presets, REAPER automation,
# locks and song arrangement all depend on them.
assert 'fxParameterCount = 35' in h
assert 'reverbBassDecayIndex = oversamplingIndex + 1' in h
assert 'reverbBassXoverIndex = reverbBassDecayIndex + 1' in h
assert 'reverbMotionModeIndex = reverbBassXoverIndex + 1' in h
assert 'reverbMotionDepthIndex = reverbMotionModeIndex + 1' in h
assert 'reverbMotionRateIndex = reverbMotionDepthIndex + 1' in h
assert 'reverbSourcePan1Index = reverbMotionRateIndex + 1' in h
assert 'reverbSourcePan2Index = reverbSourcePan1Index + 1' in h
assert 'reverbLfoPan1Index = reverbSourcePan2Index + 1' in h
assert 'reverbLfoPan2Index = reverbLfoPan1Index + 1' in h
assert 'reverbEarlyLevelIndex = reverbLfoPan2Index + 1' in h
assert 'parameterCount = reverbEarlyLevelIndex + 1' in h
assert 'std::array<float, fxParameterCount + 10>' in fxh
assert 'fxDisplayParameterCount = fxParameterCount + classicDelayParameterCount + 1 + 2 + 3 + 4 + 1' in h
for index, identity, label in [
    ('reverbSourcePan1Index', 'fx_reverb_source_pan1', 'Reverb Source Pan 1'),
    ('reverbSourcePan2Index', 'fx_reverb_source_pan2', 'Reverb Source Pan 2'),
    ('reverbLfoPan1Index', 'fx_reverb_lfo_pan1', 'Reverb LFO Pan 1'),
    ('reverbLfoPan2Index', 'fx_reverb_lfo_pan2', 'Reverb LFO Pan 2')]:
    assert f'result[{index}]' in c
    assert f'"{identity}"' in c and f'"{label}"' in c
    assert f'add (5, lb303::{index})' in e
    assert f'fxControls[lb303::fx::{index[:-5]}] = value (lb303::{index})' in p
assert 'index == reverbSourcePan1Index || index == reverbSourcePan2Index' in c
assert 'index == reverbLfoPan1Index || index == reverbLfoPan2Index' in c
assert c.count('-100, 100, 1, 0, "%"') >= 4
assert 'reverbSourcePan1Index, reverbSourcePan2Index' in c
assert 'reverbLfoPan1Index, reverbLfoPan2Index' in c

# Input-side room excitation: individual stereo sends survive the pre-delay.
# Legacy direct/return audio and existing 8-line FDN network are unchanged.
assert 'std::array<Ring, basslineCount> sourcePreL, sourcePreR' in fxh
assert 'sourcePreL[v].push (voiceSends[v].first)' in fx
assert 'sourcePreR[v].push (voiceSends[v].second)' in fx
assert 'sourcePreL[v].read (readOffset)' in fx
assert 'sourcePreR[v].read (readOffset)' in fx
assert 'spatialExcitationDelta[i] += steered - original;' in fx
assert 'const float source = (((i & 1)==0 ? inL : inR) + spatialExcitationDelta[i]) * .18f;' in fx
assert 'const float leftWeight = std::max (0.0f, -position);' in fx
assert 'const float rightWeight = std::max (0.0f, position);' in fx
assert 'sourcePositionsSmoothed.fill (0.0f)' in fx
assert 'sourcePositionsSmoothed.fill (0.0f)' in fx
assert 'sourcePresenceSmoothed' not in fx
assert 'sourcePositionSmoothCoeff' in fx
assert 'if (std::abs (position) > 1.0e-7f)' in fx
assert 'reverb (l, r, reverbVoiceSends, lfoSamples)' in fx
assert 'Output out;' in fx and 'out.dry[0] = { bounded (axL), bounded (axR) }' in fx
assert 'const auto delayWet = runDelay (delayStem.first, delayStem.second);' in fx
assert 'const auto reverbWet = runReverb (reverbStem.first + delayWet.first * reverbLink,' in fx

# Uses the actual per-voice LFO, not a duplicate oscillator. Step locks,
# BPM Sync and oversampling all use the existing engine.
assert 'void BassEngine::setExternalLfoDestination (bool enabled)' in eng
assert 'lastLfoSample = static_cast<float> (modulation)' in eng
assert 'lfoHasDestination = externalLfoDestination || values[lfo::pitchDepth]' in eng
assert 'void BassEngine::advanceSilentLfo (int subSamples)' in eng
assert 'engines[voice].advanceSilentLfo (oversamplingFactor)' in p
assert 'engines[0].currentLfoSample(), engines[1].currentLfoSample()' in p
assert 'sharedFx.processPostDistortion (stems[0].first' in p
assert 'sharedFx.process (stems[0].first' in p
assert 'engines[voice].setExternalLfoDestination (connected)' in p
assert 'TEST2.4.51-Windows-x64' in wf
assert 'python Tests/check_reverb_source_spatial.py' in wf
for language in ('en', 'it', 'es', 'fr', 'ru', 'zh', 'ja'):
    html = read('Help/LB303Help.html')
    block = re.search(r'<article class="manual" id="' + language + r'".*?</article>', html, re.S)
    assert block and 'TEST2.4.45' in block.group(), language
print('PASS: TEST2.4.46 append-only IDs, independent source predelays, real shared LFO, FDN spatial injection, UI and seven help languages.')
