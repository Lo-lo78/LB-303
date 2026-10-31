"""TEST2.4.55: no-build checks of attack-triggered reverb ducking.
The real VST3 build runs in GitHub Actions; this only verifies source integration
and the intended envelope arithmetic (it does not render the JUCE DSP).
"""
from pathlib import Path
import math
root=Path(__file__).resolve().parent.parent
def read(p): return (root/p).read_text(encoding='utf-8')
h=read('Source/ParameterCatalog.h')
c=read('Source/ParameterCatalog.cpp')
fxh=read('Source/SharedEffects.h')
fx=read('Source/SharedEffects.cpp')
pp=read('Source/PluginProcessor.cpp')
seq=read('Source/SequencerPlayback.cpp')
w=read('.github/workflows/build-windows.yml')
assert 'constexpr size_t reverbDuckAmountIndex = reverbAccentSendIndex + 1;' in h
assert 'constexpr size_t reverbDuckRecoveryIndex = reverbDuckAmountIndex + 1;' in h
assert 'constexpr size_t parameterCount = reverbDuckRecoveryIndex + 1;' in h
assert 'constexpr size_t reverbDuckAmount = fxParameterCount + 10;' in h
assert 'constexpr size_t reverbDuckRecovery = fxParameterCount + 11;' in h
assert 'fxDisplayParameterCount = fxParameterCount + classicDelayParameterCount + 1 + 2 + 3 + 4 + 1 + 2 + 2;' in h
assert 'reverbDuckAmountIndex, reverbDuckRecoveryIndex' in c
assert '"fx_reverb_duck_amount", "Reverb Duck Amount",\n            0, 100, 1, 0, "%"' in c
assert '"fx_reverb_duck_recovery", "Reverb Duck Recovery",\n            10, 2000, 10, 350, "ms"' in c
for name in ('reverbDuckAmountIndex','reverbDuckRecoveryIndex'):
 assert name in c[c.index('juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()'):],name
for ref in ('fx::reverbDuckAmount','fx::reverbDuckRecovery'):
 assert ref in pp and ref in fx
assert 'using Controls = std::array<float, fxParameterCount + 12>;' in fxh
assert 'void triggerReverbDuck (int velocity) noexcept;' in fxh
assert 'void SharedEffects::triggerReverbDuck (int velocity) noexcept' in fx
assert 'void SharedEffects::clearTails() noexcept' in fx
assert 'reverbDuckReductionDb = reverbDuckAttackTargetDb = 0.0f;' in fx
# Trigger only when actual MIDI note onset is delivered to the synth engine.
pos=pp.index('for (const auto entry : engineMidi)')
block=pp[pos:pp.index('renderTo (count);',pos)]
assert block.index('if (message.isNoteOn())') < block.index('engines[static_cast<size_t> (midiChannel - 1)].midi (message)')
assert 'sharedFx.triggerReverbDuck (message.getVelocity());' in block
assert 'if (lastNote[lane] >= 0 && lastNote[lane] != note' in seq
assert 'if (lastNote[lane] != note' in seq
assert 'float SharedEffects::nextReverbDuckGain() noexcept' in fx
assert 'reverbDuckAttackSamplesLeft' in fx and 'reverbDuckRecoveryCoeff' in fx
assert 'if (c[fx::reverbDuckAmount] <= 0.0f)' in fx
assert 'const float duckGain = verbEnabled ? nextReverbDuckGain() : 1.0f;' in fx
assert 'delayWet.first + reverbWet.first * duckGain' in fx
assert 'delayWet.second + reverbWet.second * duckGain' in fx
assert 'reverbWet.first * duckGain' not in fx[:fx.index('const auto reverbWet = runReverb')]
# Check numeric envelope: target, 5ms attack, 99% in user-selected time.
for fs in (44100,48000,96000):
 n=max(1,round(.005*fs)); att=0.; peak=18.*(1.)*(127/127)
 for remaining in range(n,0,-1): att+=(peak-att)/remaining
 assert abs(att-18.0)<1.e-5
 for ms in (10,350,2000):
  k=1-math.exp(-math.log(100)/(.001*ms*fs))
  reduction=att*((1-k)**round(.001*ms*fs))
  assert 0.17 < reduction < 0.19,(fs,ms,reduction)
 assert 10**(-18/20) < 0.127
assert 'python Tests/check_reverb_ducking.py' in w
assert 'TEST2.4.55-Windows-x64' in w
assert 'TEST2.4.55' in read('README.md')
manual=read('Help/LB303Help.html')
assert manual.count('TEST2.4.55') >= 7
for locale in ('en','it','es','fr','ru','zh','ja'):
 sec=manual.split('<article class="manual" id="'+locale+'"',1)[1].split('</article>',1)[0]
 assert 'Reverb Duck Amount' in sec and 'Reverb Duck Recovery' in sec
print('PASS: TEST2.4.55 append-only reverb ducking, note-on detector, 5ms attack, recovery, routing, 0%-bypass, 7 guides')
