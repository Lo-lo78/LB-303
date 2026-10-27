"""TEST2.4.33: static regression plus optional native DSP smoke tests.

No JUCE download is needed to compile the DSP tests: Tests/Shim only
implements the small JUCE types used by BassEngine and SharedEffects.
The full JUCE VST3 is compiled by the following Windows Actions build step.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import os

root = Path(__file__).resolve().parents[1]
get = lambda rel: (root / rel).read_text(encoding='utf-8')
h = get('Source/ParameterCatalog.h')
c = get('Source/ParameterCatalog.cpp')
voice = get('Source/BassEngine.cpp')
fx = get('Source/SharedEffects.cpp')
processor = get('Source/PluginProcessor.cpp')
ed = get('Source/PluginEditor.cpp')
workflow = get('.github/workflows/build-windows.yml')

assert 'oversamplingIndex = osc2ParameterBase + basslineCount * osc2ParameterCount' in h
assert 'parameterCount = oversamplingIndex + 1' in h
assert 'fx_oversampling' in c and 'oversamplingChoices, 3' in c
assert '"1x", "2x", "4x"' in c
assert 'add (0, lb303::oversamplingIndex)' in ed
assert 'index == oversamplingIndex' in c and '("FX - " + p.name)' in c
assert 'currentOversamplingFactor' in processor
assert 'engines[voice].setOversamplingFactor (oversamplingFactor)' in processor
assert 'sharedFx.setInsertOversamplingFactor (oversamplingFactor)' in processor
assert 'voiceDecimators[voice].setFactor (oversamplingFactor)' in processor
assert 'for (int sub = 0; sub < oversamplingFactor; ++sub)' in processor
assert 'const auto distorted = sharedFx.processDistortionInsert (' in processor
assert 'stems[voice] = voiceDecimators[voice].process (' in processor
assert 'sharedFx.processPostDistortion (' in processor
assert 'sharedFx.process (stems[0].first, stems[0].second' in processor
assert 'return processPostDistortion (axL, axR, bxL, bxR);' in fx
assert 'return distort (l, r, voice);' in fx
assert 'const auto rate = fs * factor;' in fx
assert 'fs = hostFs * factor;' in voice
assert 'voiceDecimators[voice].setFactor (oversamplingFactor)' in processor
assert 'setOversamplingFactor (int factor) noexcept' in voice
assert 'BassEngine::tick' in voice and 'osc2Phase' in voice
assert 'setParameters (fxControls, legacy, bpm)' in processor
assert '  push:' in workflow and 'TEST2.4.33-Windows-x64' in workflow
assert 'python Tests/check_oversampling.py' in workflow
assert 'VoiceDecimator' in get('Source/Oversampling.h')
print('PASS: append-only 1x/2x/4x accessible parameter; both native-rate synths and distortion inserts; shared FX stay at host rate.')

# Windows CI validates these sources with the actual JUCE/MSVC VST3 build.
# A visible clang++ executable there may lack the Visual Studio SDK environment.
compiler = (shutil.which('g++') or shutil.which('clang++')) if os.name != 'nt' else None
if not compiler:
    print('SKIP optional standalone DSP tests (toolchain/platform); GitHub Actions compiles the full VST3 with MSVC.')
else:
    with tempfile.TemporaryDirectory(prefix='lb303_oversampling_') as tmp:
        exe_ext = '.exe' if os.name == 'nt' else ''
        def build_and_run(name, sources, include_shim):
            executable = str(Path(tmp) / (name + exe_ext))
            args = [compiler, '-std=c++20', '-O2', '-I', str(root / 'Source')]
            if include_shim: args += ['-I', str(root / 'Tests/Shim')]
            args += [str(root / source) for source in sources]
            args += ['-o', executable]
            subprocess.run(args, check=True, timeout=90)
            subprocess.run([executable], check=True, timeout=30)
        build_and_run('filter', ['Tests/test_oversampling_dsp.cpp'], False)
        build_and_run('voice', ['Source/BassEngine.cpp', 'Source/SharedEffects.cpp',
                               'Tests/test_oversampling_voice.cpp'], True)
print('PASS: TEST2.4.33 oversampling regression checks.')
