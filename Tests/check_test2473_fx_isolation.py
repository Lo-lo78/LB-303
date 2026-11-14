#!/usr/bin/env python3
"""TEST2.4.73: short step previews never reset or retime shared delay FX."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
s = (root / 'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
fx = (root / 'Source/SharedEffects.cpp').read_text(encoding='utf-8')
workflow = (root / '.github/workflows/build-windows.yml').read_text(encoding='utf-8')
cmake = (root / 'CMakeLists.txt').read_text(encoding='utf-8')
ps = (root / 'Tools/build-windows.ps1').read_text(encoding='utf-8')

start = s.index('if (const int command = stepAuditionCommand.exchange')
end = s.index('    const bool activePreview', start)
audition = s[start:end]
assert 'sequencerPlayback.reset();' in audition
assert 'const bool localStepAuditionBlock = stepAuditionActive;' in s, 'One-shot end in same buffer must not restore historic delay locks'
assert 'sharedFx.clearTails()' not in audition, 'Never reset shared FX when selecting steps'
assert s.count('sharedFx.clearTails();') == 1, 'Only explicit runtime reset may clear the shared FX'
assert 'for (auto& engine : engines) engine.releaseNote();' in audition

begin = s.index('const auto applyFxParameters = [&]')
end = s.index('const auto updateStepSendGains = [&]', begin)
params = s[begin:end]
assert 'if (! localStepAuditionBlock' in params
assert s.index('const bool localStepAuditionBlock = stepAuditionActive;') < s.index('if (stepAuditionActive && nextPpq >= stepAuditionDuration)'), 'Snapshot must precede audition stop'
assert params.index('if (! localStepAuditionBlock') < params.index('for (int i = 0; i < step.parameterLockCount; ++i)')
assert 'sharedFx.setParameters (fxControls, legacy, bpm);' in params
assert 'delay[lane] = gain' in s and 'sharedFx.setStepSendOverrides (delayAbsolute, reverbAbsolute)' in s
assert 'LB303StepPreviewFxIsolationTest' in cmake and 'LB303StepPreviewFxIsolationTest.exe' in ps
assert 'python Tests/check_test2473_fx_isolation.py' in workflow
assert 'TEST2.4.73-Windows-x64' in workflow

compiler = (shutil.which('g++') or shutil.which('clang++')) if os.name != 'nt' else None
if compiler:
    with tempfile.TemporaryDirectory(prefix='lb303_fx_isolation_') as tmp:
        exe = str(Path(tmp) / 'fx_isolation')
        subprocess.run([compiler, '-O2', '-std=c++20', '-I', str(root / 'Tests/Shim'),
            '-I', str(root / 'Source'), str(root / 'Source/BassEngine.cpp'),
            str(root / 'Source/SharedEffects.cpp'),
            str(root / 'Tests/StepPreviewFxIsolationTests.cpp'), '-o', exe],
            check=True, timeout=100)
        subprocess.run([exe], check=True, timeout=60)
else:
    print('MSVC will run LB303StepPreviewFxIsolationTest.exe')
print('PASS TEST2.4.73: step auditions preserve shared delay state, feedback and Send')
