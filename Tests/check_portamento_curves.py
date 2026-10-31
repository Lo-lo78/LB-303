#!/usr/bin/env python3
"""TEST2.4.53: static + numerical checks; full VST3 build occurs in GitHub Actions."""
from pathlib import Path
from math import exp, expm1, log, log1p, isclose
root=Path(__file__).resolve().parents[1]
read=lambda f:(root/f).read_text(encoding='utf-8')
h=read('Source/ParameterCatalog.h'); c=read('Source/ParameterCatalog.cpp')
b=read('Source/BassEngine.cpp'); bh=read('Source/BassEngine.h')
p=read('Source/PluginProcessor.cpp'); e=read('Source/PluginEditor.cpp')
w=read('.github/workflows/build-windows.yml')
assert 'portamentoCurve1Index = reverbEarlyLevelIndex + 1' in h
assert 'portamentoCurve2Index = portamentoCurve1Index + 1' in h
assert 'portamentoCurve2Index = portamentoCurve1Index + 1' in h
assert 'parameterCount = reverbAccentSendIndex + 1' in h
assert 'voiceGridParameterCount = voiceVisibleParameterCount + 3' in h
assert 'voiceLocalParameterCount = osc2LocalBase + osc2ParameterCount' in h
assert 'constexpr size_t synthParameterCount = 35' in h
assert 'constexpr const char* portaCurveChoices[] = { "Original", "Linear", "Exponential", "Logarithmic", "S-Curve" }' in c
assert '"b1_portamento_curve"' in c and '"b2_portamento_curve"' in c
assert c.count('"Portamento Curve"')==2
assert '0, 4, 1, 0, "", portaCurveChoices, 5' in c
assert 'constexpr int curveDisplayIndex = 17;' in e
assert 'if (local == curveDisplayIndex)' in e and 'if (local > curveDisplayIndex) --local;' in e
assert 'engines[voice].setPortamentoCurveMode' in p
assert 'value (lb303::portamentoCurveIndex (voice))' in p
assert 'void setPortamentoCurveMode (int mode) noexcept;' in bh
assert 'if (portamentoCurveMode == 0)\n        freq += (targetFreq - freq) * (legato ? glideLegatoCoeff : glideFreeCoeff);' in b
assert 'if (portamentoCurveMode != 0) beginCurvedGlide();' in b
assert 'if (coefficient >= 1.0 || freq <= 0.0 || targetFreq <= 0.0)' in b
assert 'const double coefficient = legato ? glideLegatoCoeff : glideFreeCoeff;' in b
assert 'std::log (0.01) / std::log1p (-coefficient)' in b
assert 'curvedElapsed *= ratio;' in b and 'curvedDuration *= ratio;' in b
assert 'if (portamentoCurveMode != 0 && curvedGlideActive && glideSettingsChanged)' in b
assert 'shaped = t*t;' in b and 'shaped = t*t*(3.0 - 2.0*t);' in b
assert 'TEST2.4.54-Windows-x64' in w
assert 'python Tests/check_portamento_curves.py' in w
assert 'TEST2.4.53' in read('README.md')
help=read('Help/LB303Help.html')
for locale in ('en','it','es','fr','ru','zh','ja'):
 block=help.split(f'<article class="manual" id="{locale}"',1)[1].split('</article>',1)[0]
 assert 'TEST2.4.53' in block, locale
# Validate that 99% of legacy one-pole motion takes the same number of
# samples as the new time base at 1x, 2x and 4x oversampling.
for time in (0.5,10.,50.,100.):
 original=max(1/(1+time*150),.0002)
 base_dur=log(.01)/log1p(-original)
 for n in (1,2,4):
  c=1-(1-original)**(1/n)
  samples=log(.01)/log1p(-c)
  assert isclose(samples,base_dur*n,rel_tol=1e-9)
  assert isclose((1-c)**samples,.01,rel_tol=1e-10)
# All four user curves are monotone, endpoint-correct and have different
# character (fast-first, slow-first, smoother both ends).
curves={
 'linear':lambda t:t,
 'exponential':lambda t:-expm1(-4*t)/-expm1(-4),
 'logarithmic':lambda t:t*t,
 's_curve':lambda t:t*t*(3-2*t),
}
for name, f in curves.items():
 vals=[f(i/1000) for i in range(1001)]
 assert abs(vals[0])<1e-12 and abs(vals[-1]-1)<1e-12,name
 assert all(a<=b for a,b in zip(vals,vals[1:])),name
assert curves['exponential'](.25)>.25
assert curves['logarithmic'](.25)<.25
assert curves['s_curve'](.25)<.25 and curves['s_curve'](.75)>.75
print('PASS: TEST2.4.53 append-only dual Portamento Curves, GUI, Original path, oversampling, 99% time, docs')
