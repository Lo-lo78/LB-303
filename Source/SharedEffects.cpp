// SPDX-License-Identifier: AGPL-3.0-or-later
// Shared FX: LB-303 classic delay; chorus, dual tape and FX distortion adapted
// from LJuno-116 TEST95. Reverb is a new four-voicing 8-line stereo FDN.
#include "SharedEffects.h"
#include <algorithm>
#include <cmath>

namespace lb303
{
namespace
{
constexpr double pi = juce::MathConstants<double>::pi;
float clampf (float x, float lo, float hi) noexcept { return std::clamp (x, lo, hi); }
float lerp (float a, float b, float t) noexcept { return a + (b - a) * t; }
}
float SharedEffects::bounded (float v) noexcept
{
    return std::isfinite (v) ? clampf (v, -8.0f, 8.0f) : 0.0f;
}
float SharedEffects::Ring::read (double delay) const noexcept
{
    if (data.empty()) return 0.0f;
    const int size = static_cast<int> (data.size());
    delay = std::clamp (delay, 1.0, static_cast<double> (size - 2));
    double at = static_cast<double> (pos) - delay;
    if (at < 0.0) at += size;
    const auto lower = static_cast<int> (at);
    const int higher = lower + 1 == size ? 0 : lower + 1;
    return lerp (data[static_cast<size_t> (lower)], data[static_cast<size_t> (higher)],
                 static_cast<float> (at - lower));
}
void SharedEffects::prepare (double sampleRate)
{
    fs = std::max (8000.0, sampleRate);
    classic.prepare (fs);
    chorusL.initialise (static_cast<int> (fs * 0.05) + 8);
    chorusR.initialise (static_cast<int> (fs * 0.05) + 8);
    tapeL.initialise (static_cast<int> (fs * 12.0) + 8);
    tapeR.initialise (static_cast<int> (fs * 12.0) + 8);
    preL.initialise (static_cast<int> (fs * 0.30) + 8);
    preR.initialise (static_cast<int> (fs * 0.30) + 8);
    reverbPredelaySmoothCoeff = 1.0 - std::exp (-1.0 / (0.035 * fs));
    reverbPredelaySamples = 0.0;
    reverbPredelayReady = false;
    constexpr std::array<double, 8> lengths {{ .0311, .0377, .0411, .0437, .0531, .0593, .0679, .0719 }};
    for (size_t i = 0; i < verb.size(); ++i)
    {
        verb[i].initialise (static_cast<int> (fs * lengths[i]) + 8);
        verbTimes[i] = static_cast<float> (lengths[i]);
    }
    verbDamped.fill (0);
    tapeTime.fill (0); tapeLow.fill (0); tapeHighX.fill (0); tapeHighY.fill (0);
    tapeWetX.fill (0); tapeWetY.fill (0);
    chorusRateSmoothed = chorusTail = 0;
    chorusPhase = 0;
    for (auto& distortion : distortions) distortion = {};
    lastDelayMode = 0;
    chorusHpCoefficient = static_cast<float> (std::exp (-2 * pi * 120.0 / fs));
    distDcCoeff = static_cast<float> (std::exp (-2 * pi * 9.0 / (fs * 2.0)));
    distPowerCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.025 * fs)));
    distGainCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.035 * fs)));
    distMixCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.004 * fs)));
}
void SharedEffects::clearTails() noexcept
{
    classic.reset();
    for (auto* line : { &chorusL, &chorusR, &tapeL, &tapeR, &preL, &preR })
    {
        std::fill (line->data.begin(), line->data.end(), 0.0f);
        line->pos = 0;
    }
    for (auto& line : verb)
    {
        std::fill (line.data.begin(), line.data.end(), 0.0f);
        line.pos = 0;
    }
    verbDamped.fill (0.0f);
    reverbPredelayReady = false;
    reverbPredelaySamples = 0.0;
    tapeTime.fill (0.0f); tapeLow.fill (0.0f); tapeHighX.fill (0.0f); tapeHighY.fill (0.0f);
    tapeWetX.fill (0.0f); tapeWetY.fill (0.0f);
    chorusRateSmoothed = chorusTail = 0.0f;
    chorusHpXL = chorusHpYL = chorusHpXR = chorusHpYR = 0.0f;
    chorusPhase = 0.0;
    for (auto& distortion : distortions) distortion = {};
    lastDelayMode = 0;
}
void SharedEffects::setParameters (const Controls& settings,
                                   const ClassicParameters& original, double tempo)
{
    c = settings;
    bpm = std::max (1.0, tempo);
    classic.setParameters (original, bpm);
    classicDryLevel = original[31]*.01f;
    classicDryGain = (original[31]*.01f) * (original[32] > 0 ? 1.0f : 1.3f);
    const int newMode = juce::jlimit (0, 2, juce::roundToInt (c[fx::delayType]));
    if (newMode != lastDelayMode)
    {
        // Both delay buffers retain tails while the selector changes, but
        // dual tape always resets its time ramp to avoid stale interpolation.
        tapeTime.fill (0);
        lastDelayMode = newMode;
    }
}
float SharedEffects::fxShape (int mode, float x) noexcept
{
    const float v = clampf (x, -24.0f, 24.0f), a = std::abs (v);
    switch (mode)
    {
        case 1: return std::tanh (v);
        case 2: return 2.0f / juce::MathConstants<float>::pi * std::atan (1.45f*v);
        case 3: return std::tanh (v * (1.0f+0.18f*a));
        case 4: return 1.5f*v/(1.0f+a);
        case 5: return clampf (v, -1, 1);
        case 6: return v >= 0 ? std::tanh (2.0f*v) : 0.65f*std::tanh (v);
        case 7: return std::tanh (3.8f*v);
        case 8: return 2.0f/juce::MathConstants<float>::pi*std::asin (std::sin (1.8f*v));
        case 9: return 0.55f*v+0.85f*a;
        case 10: return std::floor (clampf (v, -1.4f, 1.4f)*12.0f+0.5f)/12.0f;
        case 11: return std::sin (2.25f*v);
        case 12: return 0.58f*std::sin (3.0f*v)+0.42f*std::tanh (4.0f*v)
                           +0.12f*std::sin (9.0f*v);
        default: return v;
    }
}
std::pair<float, float> SharedEffects::distort (float l, float r, size_t voice) noexcept
{
    auto& state = distortions[voice];
    const size_t typeIndex = voice == 0 ? fx::distortionType : fx::distortion2Type;
    const size_t amountIndex = voice == 0 ? fx::distortionAmount : fx::distortion2Amount;
    const size_t harmonicsIndex = voice == 0 ? fx::distortionHarmonics : fx::distortion2Harmonics;
    const int mode = juce::roundToInt (c[typeIndex]);
    const float amount = c[amountIndex] * .01f;
    if (mode == 0 || amount <= 0)
    {
        state = {}; // Clean restart when this bassline's distortion is turned on.
        return { l, r };
    }
    state.mix += (amount - state.mix) * distMixCoeff;
    const float balance = c[harmonicsIndex] * .01f;
    const float oddGain = 1.0f + .85f*balance, evenGain = 1.0f-balance;
    const float drive = 1.0f+19.0f*std::pow (amount, 1.35f);
    const float bias = mode == 9 ? 0.0f : 0.28f;
    const float centre = fxShape (mode, bias);
    const auto side = [&] (float v, DistSide& channel)
    {
        const float midpoint = .5f*(v+channel.previous);
        channel.previous = v;
        const auto evaluate = [&] (float x)
        {
            const float driven = x*drive;
            const float positive = fxShape (mode, driven+bias)-centre;
            const float negative = fxShape (mode, -driven+bias)-centre;
            const float odd = .5f*(positive-negative);
            const float even = .5f*(positive+negative);
            const float hp = even-channel.prevEven+distDcCoeff*channel.evenDc;
            channel.prevEven = even; channel.evenDc = hp;
            return oddGain*odd+2.0f*evenGain*hp;
        };
        return .23f*v+.77f*(.35f*evaluate (midpoint)+.65f*evaluate (v));
    };
    float wetL = side (l, state.left), wetR = side (r, state.right);
    const float dryLevel = .5f*(l*l+r*r);
    const float processedLevel = .5f*(wetL*wetL+wetR*wetR);
    state.dryPower += (dryLevel-state.dryPower)*distPowerCoeff;
    state.processedPower += (processedLevel-state.processedPower)*distPowerCoeff;
    const float target = clampf (std::sqrt ((state.dryPower+1.e-10f)/(state.processedPower+1.e-10f)), .2f, 1.35f);
    state.gain += (target-state.gain)*distGainCoeff;
    wetL *= state.gain; wetR *= state.gain;
    return { bounded (l+state.mix*(wetL-l)), bounded (r+state.mix*(wetR-r)) };
}
std::pair<float, float> SharedEffects::chorus (float l, float r) noexcept
{
    // LJuno-116 TEST95 stereo chorus: quadrature LFO, fractional delay and
    // separate 45 Hz HP on each wet channel, wet-only aux return.
    if (chorusL.data.empty()) return { 0, 0 };
    chorusRateSmoothed += (c[fx::chorusRate] * .01f - chorusRateSmoothed) * .0005f;
    const double rateHz = .10 + chorusRateSmoothed * 1.40;
    chorusPhase += rateHz / fs;
    if (chorusPhase >= 1.0) chorusPhase -= 1.0;
    const double a = std::sin (2.0*pi*chorusPhase);
    const double b = std::sin (2.0*pi*(chorusPhase+.25));
    const double dl = .006+(.018-.006)*(.5+.5*a*.65);
    const double dr = .006+(.018-.006)*(.5+.5*b*.65);
    const float fromL = chorusL.read (dl*fs), fromR = chorusR.read (dr*fs);
    chorusL.push (l); chorusR.push (r);
    const float wetL = fromL-chorusHpXL+chorusHpCoefficient*chorusHpYL;
    const float wetR = fromR-chorusHpXR+chorusHpCoefficient*chorusHpYR;
    chorusHpXL = fromL; chorusHpYL = wetL;
    chorusHpXR = fromR; chorusHpYR = wetR;
    const float gain = c[fx::chorusLevel]*.01f*(.5f+.5f*c[fx::chorusWidth]*.01f);
    return { bounded (wetL*gain), bounded (wetR*gain) };
}
float SharedEffects::toneFeedback (float v, float tone, size_t side) noexcept
{
    tone = clampf (tone*.01f, 0, 2);
    if (tone < .4999f)
    {
        const float norm = tone/.5f;
        const float coeff = static_cast<float> (1-std::exp (-2*pi*(600+norm*7000)/fs));
        tapeLow[side] += (v-tapeLow[side])*coeff;
        tapeHighX[side] = tapeHighY[side] = tapeLow[side];
        return tapeLow[side];
    }
    if (tone > .5001f)
    {
        const float norm = clampf ((tone-.5f)/1.5f, 0, 1);
        const float coeff = static_cast<float> (std::exp (-2*pi*(40+norm*1800)/fs));
        const float out = v - tapeHighX[side] + coeff*tapeHighY[side];
        tapeHighX[side] = v; tapeHighY[side] = out; tapeLow[side] = v;
        return out;
    }
    tapeLow[side] = tapeHighX[side] = tapeHighY[side] = v;
    return v;
}
float SharedEffects::saturateTape (float v) const noexcept
{
    const float amt = c[fx::dualDrive]*.01f;
    const float squared = amt*amt, gain = 1+79*squared, bias = .185f*squared;
    const float u = clampf (v*gain+bias, -16, 16);
    const float a = std::abs (u);
    const float soft = u>=0 ? u/(1+a) : .78f*u/(.78f+a);
    const float hard = clampf (u, -1, 1);
    const float zeroSoft = bias/(1+std::abs (bias));
    const float zeroHard = clampf (bias, -1, 1);
    const float shaped = soft*(1-squared)+hard*squared
                       - (zeroSoft*(1-squared)+zeroHard*squared);
    return v+(shaped-v)*amt;
}
std::pair<float, float> SharedEffects::dualDelay (float l, float r) noexcept
{
    constexpr std::array<int,11> divisions {{ 1, 32, 24, 16, 12, 8, 6, 4, 3, 2, 1 }};
    const int sync = juce::jlimit (0,10,juce::roundToInt (c[fx::dualSync]));
    const double seconds = sync > 0 ? 60.0/bpm*4.0/divisions[static_cast<size_t> (sync)]
                                    : c[fx::dualTime]*.001;
    const float coeff = static_cast<float> (1.0-std::exp (-1.0/std::max (1.0, c[fx::dualGlide]*.001*fs)));
    const std::array<float,2> target {{ static_cast<float> (std::clamp (seconds*c[fx::dualSpeed1]*fs, 16.0, fs*11.9)),
                                        static_cast<float> (std::clamp (seconds*c[fx::dualSpeed2]*fs, 16.0, fs*11.9)) }};
    for (size_t i=0;i<2;++i)
    {
        if (tapeTime[i] <= 0) tapeTime[i] = target[i];
        tapeTime[i] += (target[i]-tapeTime[i])*coeff;
    }
    const float raw1 = tapeL.read (tapeTime[0]), raw2 = tapeR.read (tapeTime[1]);
    const float wet1 = raw1-tapeWetX[0]+.995f*tapeWetY[0];
    const float wet2 = raw2-tapeWetX[1]+.995f*tapeWetY[1];
    tapeWetX = { raw1, raw2 }; tapeWetY = { wet1, wet2 };
    const float tone1 = toneFeedback (raw1,c[fx::dualTone1],0);
    const float tone2 = toneFeedback (raw2,c[fx::dualTone2],1);
    const float feedback1 = clampf (tone1 * c[fx::dualFeedback1] * .01f*.4999f, -2, 2);
    const float feedback2 = clampf (tone2 * c[fx::dualFeedback2] * .01f*.4999f, -2, 2);
    const float mono = .5f*(l+r);
    tapeL.push (clampf (mono+feedback1,-4,4));
    tapeR.push (clampf (mono+feedback2,-4,4));
    const float spread = c[fx::dualSpread]*.01f;
    const float a1 = (-spread+1.0f)*juce::MathConstants<float>::pi*.25f;
    const float a2 = (spread+1.0f)*juce::MathConstants<float>::pi*.25f;
    const std::array<float,4> targetPan {{ std::cos(a1), std::sin(a1), std::cos(a2), std::sin(a2) }};
    const float panCoeff = static_cast<float> (1-std::exp (-1.0/(.005*fs)));
    for (size_t i=0;i<4;++i) tapePan[i] += (targetPan[i]-tapePan[i])*panCoeff;
    const float out1 = saturateTape (wet1), out2 = saturateTape (wet2);
    constexpr float makeup = .35355339059f;
    return { bounded ((out1*tapePan[0]+out2*tapePan[2])*makeup),
             bounded ((out1*tapePan[1]+out2*tapePan[3])*makeup) };
}
std::pair<float, float> SharedEffects::reverb (float l, float r) noexcept
{
    // Four tunings of one Householder FDN. Decay times, diffusion and stereo
    // width differ: Room tight, Plate bright, Hall wide, Cathedral prolonged.
    const int mode = juce::jlimit (1,4,juce::roundToInt (c[fx::reverbType]));
    // Before TEST2.4.4 each 1 ms change made the read head jump instantly
    // (44/48 samples at common sample rates). That discontinuity sounds like
    // a click; the FDN then repeats and emphasises it. Keep one continuous
    // fractional delay head and smoothly approach the new user setting.
    const double targetPredelay = std::clamp (
        static_cast<double> (c[fx::reverbPredelay]) * .001 * fs,
        0.0, .20 * fs);
    if (! reverbPredelayReady)
    {
        reverbPredelaySamples = targetPredelay;
        reverbPredelayReady = true;
    }
    reverbPredelaySamples += (targetPredelay - reverbPredelaySamples)
                             * reverbPredelaySmoothCoeff;
    const double readOffset = 1.0 + reverbPredelaySamples;
    const float inL = preL.read (readOffset), inR = preR.read (readOffset);
    preL.push (l); preR.push (r);
    const double size = mode==1 ? .58 : mode==2 ? .85 : mode==3 ? 1.0 : 1.2;
    const double seconds = std::max (.15, static_cast<double> (c[fx::reverbDecay])
                                    * (mode==1 ? .55 : mode==2 ? .8 : mode==3 ? 1 : 1.55));
    const double cutoff = c[fx::reverbDamping]*(mode==2 ? 1.3 : mode==4 ? .75 : 1);
    const float damp = static_cast<float> (1-std::exp (-2*pi*std::min (cutoff,fs*.45)/fs));
    std::array<float,8> current {};
    float sum=0;
    for (size_t i=0;i<8;++i)
    {
        // Each buffer length is fixed and noncommensurate. Size changes the read offset.
        current[i] = verb[i].read (std::clamp (static_cast<double> (verbTimes[i]) * size * fs,
                                              2.0,
                                              static_cast<double> (verb[i].data.size() - 2)));
        sum += current[i];
    }
    for (size_t i=0;i<8;++i)
    {
        const float reflected = sum*.25f-current[i];
        verbDamped[i] += (reflected-verbDamped[i])*damp;
        const double effectiveSeconds = std::clamp (
            static_cast<double> (verbTimes[i]) * size, 2.0 / fs,
            static_cast<double> (verb[i].data.size() - 2) / fs);
        const auto gain = static_cast<float> (std::pow (10.0, -3.0*effectiveSeconds/seconds));
        const float source = ((i & 1)==0 ? inL : inR) * .18f;
        verb[i].push (bounded (source+verbDamped[i]*std::min (.9995f,gain)));
    }
    // Decorrelated stereo output. Diffuse tail continues even with zero input.
    float wetL = current[0]-current[1]+current[2]-current[3]+current[4]-current[5]+current[6]-current[7];
    float wetR = current[0]+current[1]-current[2]-current[3]+current[4]+current[5]-current[6]-current[7];
    wetL *= .23f; wetR *= .23f;
    const float width = c[fx::reverbWidth]*.01f;
    const float middle = .5f*(wetL+wetR), side = .5f*(wetL-wetR)*width;
    // TEST2.4.6: double ONLY the stereo wet return gain. Existing 100% now
    // corresponds to the new 50%, with up to 200% of the previous return at
    // 100%. No changes to decay, predelay, feedback, dry mix or other FX.
    const float level = c[fx::reverbLevel]*.02f;
    return { bounded ((middle+side)*level), bounded ((middle-side)*level) };
}
std::pair<float, float> SharedEffects::process (float aL, float aR, float bL, float bR) noexcept
{
    // Distortion is independent for each voice, before the shared send buses.
    const auto [axL, axR] = distort (aL, aR, 0);
    const auto [bxL, bxR] = distort (bL, bR, 1);
    const auto send = [&] (size_t one, size_t two)
    {
        const float l1=c[one]*.01f, l2=c[two]*.01f;
        return std::pair<float,float> { axL*l1+bxL*l2, axR*l1+bxR*l2 };
    };
    float wetL=0,wetR=0;
    const int delayMode=juce::roundToInt (c[fx::delayType]);
    if (delayMode != 0)
    {
        const auto [sl,sr]=send (fx::delaySend1,fx::delaySend2);
        if (delayMode == 1)
        {
            const auto [dl,dr] = classic.processClassicDelay(sl,sr);
            // Send return contains only the delayed sound, not the dry input.
            // With Wet 0 the feedback engine can run but there is no delay return.
            // Classic processClassicDelay retains the source's original Dry and
            // Wet controls, but dry is removed from the send bus here.
            // Classic dry factor and its gain are read from the classic values.
            wetL += dl-sl*classicDryGain;
            wetR += dr-sr*classicDryGain;
        }
        else
        {
            const auto [dl,dr]=dualDelay(sl,sr);
            wetL += dl; wetR += dr;
        }
    }
    if (c[fx::chorusOn] > .5f)
    {
        const auto [sl,sr]=send (fx::chorusSend1,fx::chorusSend2);
        const auto [cl,cr]=chorus(sl,sr);
        wetL += cl; wetR += cr;
    }
    if (c[fx::reverbType] > .5f)
    {
        const auto [sl,sr]=send (fx::reverbSend1,fx::reverbSend2);
        const auto [rl,rr]=reverb(sl,sr);
        wetL += rl; wetR += rr;
    }
    // Classic Dry controls the single dry instrument bus, while the return
    // contains just the wet repeats (never duplicating the raw stems).
    const float dryGain = delayMode == 1 ? classicDryLevel : 1.0f;
    return { bounded ((axL+bxL)*dryGain+wetL), bounded ((axR+bxR)*dryGain+wetR) };
}
}
