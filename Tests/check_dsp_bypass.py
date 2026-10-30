"""TEST2.4.34: verify precomputed DSP routing plus optional standalone smoke cases.

JUCE/MSVC VST3 builds are handled by the Windows GitHub Actions job.
Native C++ smoke binaries require a C++20 compiler; on Windows CI they are
skipped, since the Actions C++ toolchain is installed through the build script.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile
import os

root = Path(__file__).resolve().parents[1]
engine_h = (root/'Source/BassEngine.h').read_text(encoding='utf-8')
engine = (root/'Source/BassEngine.cpp').read_text(encoding='utf-8')
fx_h = (root/'Source/SharedEffects.h').read_text(encoding='utf-8')
fx = (root/'Source/SharedEffects.cpp').read_text(encoding='utf-8')
processor = (root/'Source/PluginProcessor.cpp').read_text(encoding='utf-8')
workflow = (root/'.github/workflows/build-windows.yml').read_text(encoding='utf-8')
assert 'lfoHasDestination' in engine_h
assert 'lfoHasDestination ? nextLfoSample() : 0.0' in engine
assert 'lfo::pitchDepth] != 0.0f' in engine
assert 'lfo::volumeDepth] != 0.0f' in engine
assert 'lfo::filterDepth] != 0.0f' in engine
assert 'lfo::panDepth] != 0.0f' in engine
assert 'const bool osc2Needed = osc2Target > 0.0 || osc2GainSmooth > 1.0e-7' in engine
assert 'if (osc2Needed)' in engine
assert 'glideFreeCoeff' in engine and 'lpgSpeedCoeff' in engine
assert 'staticPitchRatio' in engine and 'accentDecayCoeff' in engine
assert 'std::array<DistortionConfig, basslineCount> distortionConfigs' in fx_h
assert 'if (!enabledDistortion[voice]) return { l, r };' in fx
assert 'if (!enabledSharedWetFx)' in fx
assert 'sharedFx.distortionActive (0)' in processor
assert 'const auto distorted = distortionOn[voice]' in processor
assert 'python Tests/check_dsp_bypass.py' in workflow
assert 'TEST2.4.43-Windows-x64' in workflow
print('PASS: dormant LFO, Osc2, distortions, shared wet FX and rate-dependent coefficients are routed outside expensive DSP.')

compiler = (shutil.which('g++') or shutil.which('clang++')) if os.name != 'nt' else None
if compiler:
    with tempfile.TemporaryDirectory(prefix='lb303_dsp_bypass_') as tmp:
        binary = str(Path(tmp)/'dsp_bypass')
        cmd = [compiler,'-O2','-std=c++20','-I',str(root/'Tests/Shim'),'-I',str(root/'Source'),
               str(root/'Source/BassEngine.cpp'),str(root/'Source/SharedEffects.cpp'),
               str(root/'Tests/test_dsp_bypass.cpp'),'-o',binary]
        subprocess.run(cmd, check=True, timeout=90)
        for factor in (1,2,4):
            for scenario in range(10):
                out = subprocess.check_output([binary,str(scenario),str(factor),'1000'],
                                              text=True, timeout=30)
                assert f'scenario {scenario}, {factor}x, hash ' in out
    print('PASS: 30 standalone C++ smoke runs, finite audio at all oversampling factors.')
else:
    print('SKIP optional native DSP smoke tests; Windows Actions builds the complete VST3.')
