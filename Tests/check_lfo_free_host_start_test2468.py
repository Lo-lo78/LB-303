#!/usr/bin/env python3
"""TEST2.4.68: host transport Free LFO restart and native DSP regression."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
read = lambda name: (root / name).read_text(encoding='utf-8')
processor = read('Source/PluginProcessor.cpp')
header = read('Source/PluginProcessor.h')
engine = read('Source/BassEngine.cpp')
engine_header = read('Source/BassEngine.h')
workflow = read('.github/workflows/build-windows.yml')
cmake = read('CMakeLists.txt')
script = read('Tools/build-windows.ps1')
assert 'bool previousHostPlaying = false;' in header
assert 'hostTransportAvailable = true;' in processor
assert 'hostPlaybackStarted = playing && ! previousHostPlaying;' in processor
assert 'previousHostPlaying = playing;' in processor
assert 'if (hostPlaybackStarted && ! activePreview)' in processor
assert 'engine.restartFreeLfoFromHostTransport();' in processor
assert processor.index('updateStepSendGains();') < processor.index('if (hostPlaybackStarted && ! activePreview)')
assert processor.index('if (hostPlaybackStarted && ! activePreview)') < processor.index('const int quality =')
assert 'void BassEngine::restartFreeLfoFromHostTransport() noexcept' in engine
assert 'if (lfoParameters[lb303::lfo::mode] >= .5f) return;' in engine
assert 'lfo = LfoState {};' in engine
assert 'lfo.previousPhase = 1.0;' in engine
assert 'lastLfoSample = 0.0f;' in engine
assert 'void restartFreeLfoFromHostTransport() noexcept;' in engine_header
assert 'LB303FreeLfoTest' in cmake and 'FreeLfoHostStartTests.cpp' in cmake
assert 'LB303FreeLfoTest.exe' in script
assert 'check_lfo_free_host_start_test2468.py' in workflow
assert 'LB-303-0.99.5-TEST2.4.73-Windows-x64' in workflow
assert 'TEST2.4.68' in read('README.md') and 'TEST2.4.68' in read('RELEASE_NOTES_0.99.2.md')
assert 'TEST2.4.68' in read('Doc/TEST2.4.68_FREE_LFO_HOST_START.md')
for lang in ('en','it','es','fr','ru','zh','ja'):
    article = read('Help/LB303Help.html').split(f'<article class="manual" id="{lang}"',1)[1].split('</article>',1)[0]
    assert 'TEST2.4.68' in article, lang
print('PASS TEST2.4.68: host transport edge, Free-only LFO reset, docs and CI integration')
compiler = shutil.which('g++') or shutil.which('clang++')
if compiler and os.name != 'nt':
    with tempfile.TemporaryDirectory() as tmp:
        exe = str(Path(tmp) / 'test_lfo_free')
        cmd = [compiler, '-O2', '-std=c++20', '-I', str(root/'Tests/Shim'),
            '-I', str(root/'Source'), str(root/'Source/BassEngine.cpp'),
            str(root/'Tests/FreeLfoHostStartTests.cpp'), '-o', exe]
        subprocess.run(cmd, check=True, timeout=60)
        subprocess.run([exe], check=True, timeout=15)
else:
    print('Native regression executes as LB303FreeLfoTest.exe in Windows GitHub Actions build.')
