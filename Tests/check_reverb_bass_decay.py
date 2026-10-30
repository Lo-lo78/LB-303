"""TEST2.4.43: reverb bass-decay topology and append-only compatibility.

Static-only: the real MSVC/VST3 compilation runs in GitHub Actions.
"""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
read = lambda rel: (root / rel).read_text(encoding="utf-8")
h = read("Source/ParameterCatalog.h")
c = read("Source/ParameterCatalog.cpp")
e = read("Source/PluginEditor.cpp")
p = read("Source/PluginProcessor.cpp")
fxh = read("Source/SharedEffects.h")
fx = read("Source/SharedEffects.cpp")
workflow = read(".github/workflows/build-windows.yml")

# Every historic fx parameter retains its original local and global slot.
assert "fxParameterCount = 35" in h
assert "sequencerPerformanceBase = legacyParameterCount + fxParameterCount" in h
assert "oversamplingIndex = osc2ParameterBase + basslineCount * osc2ParameterCount" in h
assert "reverbBassDecayIndex = oversamplingIndex + 1" in h
assert "reverbBassXoverIndex = reverbBassDecayIndex + 1" in h
assert "parameterCount = reverbBassXoverIndex + 1" in h
assert '"fx_reverb_bass_decay"' in c and '"fx_reverb_bass_xover"' in c
assert '0.5f, 2.0f, 0.01f, 1.0f, "x"' in c
assert '50.0f, 1000.0f, 1.0f, 360.0f, "Hz"' in c
assert '(index == reverbBassDecayIndex || index == reverbBassXoverIndex)' in c
assert 'result[reverbBassDecayIndex]' in c and 'result[reverbBassXoverIndex]' in c
assert 'reverbBassDecayIndex, reverbBassXoverIndex' in c
assert 'add (5, lb303::reverbBassDecayIndex)' in e
assert 'add (5, lb303::reverbBassXoverIndex)' in e
assert 'if (enabled (lb303::fx::reverbType))' in e
assert 'std::array<float, fxParameterCount + 2>' in fxh
assert 'for (size_t i = 0; i < lb303::fxParameterCount; ++i)' in p
assert 'fxControls[lb303::fx::reverbBassDecay] = value (lb303::reverbBassDecayIndex)' in p
assert 'fxControls[lb303::fx::reverbBassXover] = value (lb303::reverbBassXoverIndex)' in p

# The low-band difference in feedback gains must be applied inside the loop,
# not implemented as an output high-pass or low-pass filter.
assert 'lowSeconds = seconds * std::clamp' in fx
assert 'const double lowGain = std::pow (10.0, -3.0 * delay / lowSeconds)' in fx
assert 'const double midGain = std::pow (10.0, -3.0 * delay / seconds)' in fx
assert 'verbBassGainDeltaTarget[i] = static_cast<float> (lowGain - midGain)' in fx
assert 'verbBassLow[i] += (verbDamped[i] - verbBassLow[i])' in fx
assert 'verbBassLow[i] * verbBassGainDeltaSmoothed[i]' in fx
assert 'verb[i].push (bounded (source + feedback))' in fx
assert 'verbBassSmoothCoeff' in fx and 'verbBassXoverSmoothed' in fx
assert 'verbBassLow.fill (0.0f)' in fx and 'verbBassGainDeltaSmoothed.fill (0.0f)' in fx
assert 'std::max (1.0, static_cast<double> (c[fx::reverbBassDecay]))' in fx
assert 'std::clamp (wetGuardSeconds, .5, 240.0)' in fx
assert 'TEST2.4.43-Windows-x64' in workflow
print('PASS: TEST2.4.43 append-only bass RT60/XOver, per-line feedback, smoothing, dynamic UI, wet tails.')
