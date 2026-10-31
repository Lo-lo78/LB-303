"""TEST2.4.49: bass RT60 default 0.50x and -6 dB on reverb-only output.

No native compilation. The VST3 is built in GitHub Actions.
"""
from pathlib import Path
import math
import re
root=Path(__file__).resolve().parents[1]
read=lambda name: (root/name).read_text(encoding='utf-8')
c=read('Source/ParameterCatalog.cpp')
fx=read('Source/SharedEffects.cpp')
w=read('.github/workflows/build-windows.yml')
help=read('Help/LB303Help.html')
assert 'result[reverbBassDecayIndex] = { "fx_reverb_bass_decay", "Reverb Bass Decay",\n                                         0.5f, 2.0f, 0.01f, 0.5f, "x" };' in c
assert 'result[reverbEarlyLevelIndex]' in c and '0, 100, 1, 15, "%"' in c
assert '"Reverb Level", 0, 100, 1, 50' in c
assert 'constexpr float reverbReturnMakeupGain = 8.0f;' in fx
m=re.search(r'constexpr float reverbWetOutputTrim6dB = ([0-9.]+)f;',fx)
assert m, 'missing separate -6 dB trim'
trim=float(m.group(1))
assert abs(20*math.log10(trim)+6.0)<0.00001
reverb=fx.split('std::pair<float, float> SharedEffects::reverb (',1)[1].split('SharedEffects::Output SharedEffects::process (',1)[0]
assert reverb.count('reverbWetOutputTrim6dB') == 1
assert 'const float level = c[fx::reverbLevel] * .02f * reverbReturnMakeupGain\n                      * reverbWetOutputTrim6dB;' in reverb
assert reverb.index('wetL += directionalEarlyReflections') < reverb.index('const float level =')
assert reverb.index('const float middle =') < reverb.index('const float level =')
assert 'return { bounded ((middle+side)*level), bounded ((middle-side)*level) };' in reverb
assert 'verbBassLow[i] * verbBassGainDeltaSmoothed[i]' in reverb
assert 'reverbWet = runReverb' in fx and 'delayWet.first + reverbWet.first' in fx
assert 'reverbWetOutputTrim6dB' not in fx.split('SharedEffects::Output SharedEffects::process (',1)[1]
assert 'TEST2.4.55-Windows-x64' in w
assert 'python Tests/check_reverb_defaults_trim.py' in w
for lang in ('en','it','es','fr','ru','zh','ja'):
    block=re.search(r'<article class="manual" id="'+lang+r'".*?</article>',help,re.S)
    assert block and 'TEST2.4.49' in block.group(),lang
    assert '-6 dB' in block.group(), lang
print('PASS: TEST2.4.49 bass default 0.50x, 15% early default, reverb-only -6 dB wet trim, unchanged IDs and guides.')
