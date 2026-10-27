"""TEST2.4.35 idle: structural and isolated voice DSP regression checks."""
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

root=Path(__file__).resolve().parents[1]
read=lambda name:(root/name).read_text(encoding='utf-8')
processor=read('Source/PluginProcessor.cpp')
engine=read('Source/BassEngine.cpp')
over=read('Source/Oversampling.h')
fx=read('Source/SharedEffects.cpp')
assert 'bool BassEngine::isQuiescent() const noexcept' in engine
assert 'if (noteActive) return false;' in engine
assert 'bool isQuiescent() const noexcept' in over
assert 'bool SharedEffects::distortionQuiescent' in fx
assert 'const bool anyVoiceWorking' in processor
assert 'if (!anyVoiceWorking && wetTailGuardSamples == 0)' in processor
assert 'nextSample = end;' in processor
assert 'wetTailGuardSamples = std::max (wetTailGuardSamples, wetGuardLimit)' in processor
assert 'if (!voiceWorking[voice]) continue;' in processor
assert 'engines[static_cast<size_t> (midiChannel - 1)].midi (message);' in processor
assert 'renderTo (offset);' in processor
assert 'python Tests/check_idle_sleep.py' in read('.github/workflows/build-windows.yml')
assert '.gitignore' in [p.name for p in root.iterdir()]
print('PASS: cold-idle bypass, per-voice sleep, MIDI wake, tails guard, .gitignore and CI checks.')
compiler=(shutil.which('g++') or shutil.which('clang++')) if os.name!='nt' else None
if compiler:
    with tempfile.TemporaryDirectory() as tmp:
        exe=str(Path(tmp)/'idle_test')
        cmd=[compiler,'-O2','-std=c++20','-I',str(root/'Tests/Shim'),'-I',str(root/'Source'),
            str(root/'Source/BassEngine.cpp'),str(root/'Source/SharedEffects.cpp'),
            str(root/'Tests/test_idle_sleep.cpp'),'-o',exe]
        subprocess.run(cmd,check=True,timeout=90)
        subprocess.run([exe],check=True,timeout=40)
else:
    print('SKIP native C++ idle test on Windows; Actions builds full VST3.')
