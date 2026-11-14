#!/usr/bin/env python3
"""TEST2.4.72: protect cached classic delay time from preview clearTails()."""
from pathlib import Path
import os, subprocess, shutil, tempfile
root = Path(__file__).resolve().parents[1]
bass = (root/'Source/BassEngine.cpp').read_text(encoding='utf-8')
fx = (root/'Source/SharedEffects.cpp').read_text(encoding='utf-8')
processor = (root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
workflow = (root/'.github/workflows/build-windows.yml').read_text(encoding='utf-8')
cmake = (root/'CMakeLists.txt').read_text(encoding='utf-8')
script = (root/'Tools/build-windows.ps1').read_text(encoding='utf-8')
reset = bass.split('void BassEngine::reset()\n{', 1)[1].split('\nvoid BassEngine::setParameters',1)[0]
assert 'parametersPrepared = false;' in reset, 'reset must invalidate cached delay time'
assert reset.index('parametersPrepared = false;') < reset.index('delayTargetSamples = delayPhase = 0')
assert 'parametersPrepared && p == values && tempo == nextTempo' in bass
assert 'classic.reset();' in fx and 'settingsPrepared = false;' in fx
assert 'sharedFx.clearTails(); // Isolate selected step' in processor
assert 'LB303DelayResetTest' in cmake and 'LB303DelayResetTest.exe' in script
assert 'python Tests/check_test2472_delay_reset.py' in workflow
assert 'TEST2.4.72-Windows-x64' in workflow
compiler = (shutil.which('g++') or shutil.which('clang++')) if os.name != 'nt' else None
if compiler:
    with tempfile.TemporaryDirectory(prefix='lb303_delay_reset_') as tmp:
        binary = str(Path(tmp)/'delay_reset')
        args = [compiler, '-O2', '-std=c++20', '-I',str(root/'Tests/Shim'),
                '-I',str(root/'Source'),str(root/'Source/BassEngine.cpp'),
                str(root/'Source/SharedEffects.cpp'),
                str(root/'Tests/DelayResetAuditionTests.cpp'),'-o',binary]
        subprocess.run(args,check=True,timeout=90)
        subprocess.run([binary],check=True,timeout=60)
else:
    print('MSVC release build will run LB303DelayResetTest.exe')
print('PASS TEST2.4.72: invalidate cached delay after preview reset, real DSP native regression')
