#!/usr/bin/env python3
"""TEST2.4.75: append-only AHDSR Hold, automation, step locks, clipboard and CI."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
read=lambda name:(root/name).read_text(encoding='utf-8')
h=read('Source/ParameterCatalog.h')
c=read('Source/ParameterCatalog.cpp')
e=read('Source/PluginEditor.cpp')
b=read('Source/BassEngine.cpp')
bh=read('Source/BassEngine.h')
p=read('Source/PluginProcessor.cpp')
w=read('.github/workflows/build-windows.yml')
build=read('Tools/build-windows.ps1')
cmake=read('CMakeLists.txt')
assert 'hold1Index = reverbDuckRecoveryIndex + 1' in h
assert 'hold2Index = hold1Index + 1' in h
assert 'parameterCount = hold2Index + 1' in h
assert 'holdLocalIndex = osc2LocalBase + osc2ParameterCount' in h
assert 'voiceLocalParameterCount = holdLocalIndex + 1' in h
assert 'local < holdLocalIndex ? osc2ParameterIndex' in h
assert 'holdIndex (voice)' in h
assert '"b1_env_hold", "Hold", 0.0f, 5.0f, 0.001f, 0.0f, "s"' in c
assert '"b2_env_hold", "Hold", 0.0f, 5.0f, 0.001f, 0.0f, "s"' in c
assert '7, 0, 53, 54, 36, 8, 9, 10, 14, 1, holdLocalIndex' in c
assert 'constexpr int curveDisplayIndex = 18' in e
assert 'setHoldSeconds (float seconds) noexcept' in bh
assert 'envelopeStage = holdSeconds > 0.0 ? 4 : 1' in b
assert 'case 4: // Hold' in b
assert 'holdElapsedSamples += 1.0' in b and 'holdSeconds * fs' in b
assert 'holdElapsedSamples *= static_cast<double> (factor) / oversamplingFactor' in b
assert 'if (! legato)' in b and 'holdElapsedSamples = 0.0;' in b
assert 'envelopeStage = 3;' in b
assert 'float hold = value (lb303::holdIndex (voice))' in p
assert 'lock.sliderNumber != static_cast<int> (lb303::holdLocalIndex + 1)' in p
assert 'engines[voice].setHoldSeconds (hold)' in p
assert 'legacyCount = count - 1' in e
assert 'storedCount != count && storedCount != legacyCount' in e
assert 'constexpr int holdDisplayIndex = 12' in e
assert 'display >= holdDisplayIndex' in e and 'display + 1 : display' in e
assert 'LB303HoldEnvelopeTest' in cmake and 'HoldEnvelopeTests.cpp' in cmake
assert 'LB303HoldEnvelopeTest.exe' in build
assert 'python Tests/check_test2475_ahdsr_hold.py' in w
assert w.count('LB-303-0.99.7-Windows-x64') == 3
assert read('Help/LB303Help.html').count('TEST2.4.75 — AHDSR Hold')==7
assert 'Doc/TEST2.4.75_AHDSR_HOLD.md' in read('README.md')
print('PASS TEST2.4.75: independent AHDSR Hold, stable IDs, locks, automation, legacy clipboard, 7 manuals, CI')
