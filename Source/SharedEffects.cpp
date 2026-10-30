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
// TEST2.4.7: +18.06 dB on the reverb wet RETURN only, relative to TEST2.4.6.
// This makes moderate sends (e.g. 20% send, 50% level) clearly audible.
constexpr float reverbReturnMakeupGain = 8.0f;
// TEST2.4.49: -6.00 dB applied once at the final Reverb WET return,
// AFTER the FDN tail and early-reflection contributions are combined.
// Do not change the reverb feedback, dry bus, chorus or delay.
constexpr float reverbWetOutputTrim6dB = 0.501187234f; // 10^(-6/20)
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
    settingsPrepared = false;
    wetGuardSeconds = 0.5;
    insertOversamplingFactor = 1;
    for (size_t i = 0; i < basslineCount; ++i)
    {
        chorusL[i].initialise (static_cast<int> (fs * 0.05) + 8);
        chorusR[i].initialise (static_cast<int> (fs * 0.05) + 8);
    }
    tapeL.initialise (static_cast<int> (fs * 12.0) + 8);
    tapeR.initialise (static_cast<int> (fs * 12.0) + 8);
    preL.initialise (static_cast<int> (fs * 0.30) + 8);
    preR.initialise (static_cast<int> (fs * 0.30) + 8);
    for (size_t v = 0; v < basslineCount; ++v)
    {
        sourcePreL[v].initialise (static_cast<int> (fs * 0.30) + 8);
        sourcePreR[v].initialise (static_cast<int> (fs * 0.30) + 8);
    }
    sourcePositionsSmoothed.fill (0.0f);
    earlyReflectionsLevelSmoothed = 0.15f;
    earlyReflectionsLevelSmoothCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.025 * fs)));
    sourcePositionSmoothCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.012 * fs)));
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
    verbBassLow.fill (0);
    verbBassGainDeltaTarget.fill (0);
    verbBassGainDeltaSmoothed.fill (0);
    verbBassXoverTarget = verbBassXoverSmoothed = 0.0f;
    verbBassSmoothCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.035 * fs)));
    verbMotionAngles.fill (0.0f);
    verbMotionDepthSmoothed = 0.0f;
    verbMotionRateSmoothed = 0.35f;
    verbMotionPhase = verbMotionAge = 0.0;
    verbMotionSmoothCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.045 * fs)));
    tapeTime.fill (0); tapeLow.fill (0); tapeHighX.fill (0); tapeHighY.fill (0);
    tapeWetX.fill (0); tapeWetY.fill (0);
    chorusRateSmoothed = chorusTail = 0;
    chorusPhase = 0;
    for (auto& distortion : distortions) distortion = {};
    enabledDistortion = {{ false, false }};
    enabledSharedWetFx = false;
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
    for (auto* line : { &tapeL, &tapeR, &preL, &preR })
    {
        std::fill (line->data.begin(), line->data.end(), 0.0f);
        line->pos = 0;
    }
    for (auto& line : sourcePreL) { std::fill (line.data.begin(), line.data.end(), 0.0f); line.pos = 0; }
    for (auto& line : sourcePreR) { std::fill (line.data.begin(), line.data.end(), 0.0f); line.pos = 0; }
    sourcePositionsSmoothed.fill (0.0f);
    earlyReflectionsLevelSmoothed = clampf (c[fx::reverbEarlyLevel] * .01f, 0.0f, 1.0f);
    for (auto& line : chorusL) { std::fill (line.data.begin(), line.data.end(), 0.0f); line.pos = 0; }
    for (auto& line : chorusR) { std::fill (line.data.begin(), line.data.end(), 0.0f); line.pos = 0; }
    for (auto& line : verb)
    {
        std::fill (line.data.begin(), line.data.end(), 0.0f);
        line.pos = 0;
    }
    verbDamped.fill (0.0f);
    verbBassLow.fill (0.0f);
    verbBassGainDeltaTarget.fill (0.0f);
    verbBassGainDeltaSmoothed.fill (0.0f);
    verbBassXoverTarget = verbBassXoverSmoothed = 0.0f;
    verbMotionAngles.fill (0.0f);
    verbMotionDepthSmoothed = 0.0f;
    verbMotionRateSmoothed = 0.35f;
    verbMotionPhase = verbMotionAge = 0.0;
    reverbPredelayReady = false;
    reverbPredelaySamples = 0.0;
    tapeTime.fill (0.0f); tapeLow.fill (0.0f); tapeHighX.fill (0.0f); tapeHighY.fill (0.0f);
    tapeWetX.fill (0.0f); tapeWetY.fill (0.0f);
    chorusRateSmoothed = chorusTail = 0.0f;
    chorusHpXL.fill (0.0f); chorusHpYL.fill (0.0f);
    chorusHpXR.fill (0.0f); chorusHpYR.fill (0.0f);
    chorusPhase = 0.0;
    for (auto& distortion : distortions) distortion = {};
    lastDelayMode = 0;
    settingsPrepared = false;
}
void SharedEffects::setParameters (const Controls& settings,
                                   const ClassicParameters& original, double tempo)
{
    const double nextBpm = std::max (1.0, std::isfinite (tempo) ? tempo : 120.0);
    if (settingsPrepared && c == settings && lastClassicParameters == original
        && bpm == nextBpm) return;
    if (!settingsPrepared)
        earlyReflectionsLevelSmoothed = clampf (settings[fx::reverbEarlyLevel] * .01f, 0.0f, 1.0f);
    settingsPrepared = true;
    lastClassicParameters = original;
    bpm = nextBpm;
    c = settings;
    // Bound a delay's *silent gaps* by its actual time and feedback, rather
    // than paying a fixed 30 seconds of DSP after every short echo.
    // Continuous measurable wet output keeps the graph awake regardless.
    const auto echoGuard = [] (double delay, double feedback) noexcept
    {
        delay = std::clamp (delay, .001, 12.5);
        feedback = std::clamp (std::abs (feedback), 0.0, 1.0);
        if (feedback >= 0.999) return std::max (30.0, delay * 4.0);
        const double echoes = feedback <= 1.0e-5 ? 1.0
            : std::ceil (std::log (1.0e-8) / std::log (feedback)) + 2.0;
        return delay * std::clamp (echoes, 1.0, 200.0) + 0.5;
    };
    wetGuardSeconds = 0.5; // chorus headroom and stereo HP memories
    if (juce::roundToInt (c[fx::delayType]) == 1)
    {
        double delay = static_cast<double> (original[19]) * .00125;
        const int sync = juce::roundToInt (original[20]);
        constexpr int divisions[] = { 1, 16, 12, 8, 6, 4, 3, 2, 1 };
        if (sync >= 1 && sync <= 8)
            delay = (60.0 / bpm) * 4.0 / divisions[sync];
        delay = std::min (10.0, delay);
        wetGuardSeconds = std::max (wetGuardSeconds,
            echoGuard (delay + std::abs (original[23]) * .01, original[21] * .01));
    }
    else if (juce::roundToInt (c[fx::delayType]) == 2)
    {
        const int sync = juce::jlimit (0, 10, juce::roundToInt (c[fx::dualSync]));
        constexpr int divisions[] = { 1, 32, 24, 16, 12, 8, 6, 4, 3, 2, 1 };
        const double delay = sync > 0 ? (60.0 / bpm) * 4.0 / divisions[sync]
                                      : c[fx::dualTime] * .001;
        wetGuardSeconds = std::max (wetGuardSeconds,
            echoGuard (std::min (11.9, delay * c[fx::dualSpeed1]),
                       c[fx::dualFeedback1] * .004999));
        wetGuardSeconds = std::max (wetGuardSeconds,
            echoGuard (std::min (11.9, delay * c[fx::dualSpeed2]),
                       c[fx::dualFeedback2] * .004999));
    }
    if (c[fx::reverbType] > 0.5f)
    {
        const int mode = juce::jlimit (1, 4, juce::roundToInt (c[fx::reverbType]));
        const double tailFactor = mode == 1 ? .55 : mode == 2 ? .8
                                : mode == 3 ? 1.0 : 1.55;
        wetGuardSeconds = std::max (wetGuardSeconds,
            2.0 * static_cast<double> (c[fx::reverbDecay]) * tailFactor
                * std::max (1.0, static_cast<double> (c[fx::reverbBassDecay]))
            + .001 * static_cast<double> (c[fx::reverbPredelay]) + .5);
    }
    wetGuardSeconds = std::clamp (wetGuardSeconds, .5, 240.0);

    // LJuno Reverb 1 principle: independent RT60 for bass IN the FDN loop.
    // Each line's low feedback gain is computed from its effective delay time.
    // The main decay and the high-frequency damping are kept unchanged.
    if (c[fx::reverbType] > 0.5f)
    {
        const int mode = juce::jlimit (1, 4, juce::roundToInt (c[fx::reverbType]));
        const double size = mode == 1 ? .58 : mode == 2 ? .85 : mode == 3 ? 1.0 : 1.2;
        const double seconds = std::max (.15, static_cast<double> (c[fx::reverbDecay])
            * (mode == 1 ? .55 : mode == 2 ? .8 : mode == 3 ? 1.0 : 1.55));
        const double lowSeconds = seconds * std::clamp (
            static_cast<double> (c[fx::reverbBassDecay]), .5, 2.0);
        const double hz = std::clamp (static_cast<double> (c[fx::reverbBassXover]), 50.0, 1000.0);
        verbBassXoverTarget = static_cast<float> (1.0 - std::exp (-2.0 * pi * hz / fs));
        for (size_t i = 0; i < verb.size(); ++i)
        {
            const double delay = std::clamp (static_cast<double> (verbTimes[i]) * size,
                2.0 / fs, static_cast<double> (verb[i].data.size() - 2) / fs);
            const double midGain = std::pow (10.0, -3.0 * delay / seconds);
            const double lowGain = std::pow (10.0, -3.0 * delay / lowSeconds);
            verbBassGainDeltaTarget[i] = static_cast<float> (lowGain - midGain);
        }
    }
    for (size_t voice = 0; voice < basslineCount; ++voice)
    {
        const auto type = voice == 0 ? fx::distortionType : fx::distortion2Type;
        const auto amount = voice == 0 ? fx::distortionAmount : fx::distortion2Amount;
        const bool on = juce::roundToInt (c[type]) != 0 && c[amount] > 0.0f;
        if (!on && enabledDistortion[voice]) distortions[voice] = {};
        enabledDistortion[voice] = on;
        if (on)
        {
            // Drive, curve centre and harmonic mix are fixed until an FX
            // control changes. At 4x they must NOT be recomputed per sample.
            auto& config = distortionConfigs[voice];
            config.mode = juce::roundToInt (c[type]);
            config.amount = c[amount] * .01f;
            const auto harmonics = voice == 0 ? fx::distortionHarmonics : fx::distortion2Harmonics;
            const float balance = c[harmonics] * .01f;
            config.oddGain = 1.0f + .85f*balance;
            config.evenGain = 1.0f - balance;
            config.drive = 1.0f + 19.0f*std::pow (config.amount, 1.35f);
            config.bias = config.mode == 9 ? 0.0f : 0.28f;
            config.centre = fxShape (config.mode, config.bias);
        }
    }
    enabledSharedWetFx = c[fx::chorusOn] > 0.5f
        || juce::roundToInt (c[fx::delayType]) != 0
        || c[fx::reverbType] > 0.5f;
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
void SharedEffects::setInsertOversamplingFactor (int factor) noexcept
{
    factor = factor == 2 || factor == 4 ? factor : 1;
    if (factor == insertOversamplingFactor) return;
    insertOversamplingFactor = factor;
    // Only the two insert distortion envelopes/DC filters run faster.
    // The shared chorus, dual delay, reverb and classic delay retain fs.
    const auto rate = fs * factor;
    distDcCoeff = static_cast<float> (std::exp (-2 * pi * 9.0 / (rate * 2.0)));
    distPowerCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.025 * rate)));
    distGainCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.035 * rate)));
    distMixCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (0.004 * rate)));
}

std::pair<float, float> SharedEffects::processDistortionInsert (float l, float r,
                                                                size_t voice) noexcept
{
    return distort (l, r, voice);
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
bool SharedEffects::distortionQuiescent (size_t voice) const noexcept
{
    if (!enabledDistortion[voice]) return true;
    const auto& state = distortions[voice];
    constexpr float threshold = 1.0e-9f;
    const auto quiet = [] (const DistSide& side)
    {
        return std::abs (side.previous) < 1.0e-9f
            && std::abs (side.prevEven) < 1.0e-9f
            && std::abs (side.evenDc) < 1.0e-9f;
    };
    return quiet (state.left) && quiet (state.right)
        && state.dryPower < threshold && state.processedPower < threshold;
}

std::pair<float, float> SharedEffects::distort (float l, float r, size_t voice) noexcept
{
    auto& state = distortions[voice];
    if (!enabledDistortion[voice]) return { l, r };
    const auto& config = distortionConfigs[voice];
    state.mix += (config.amount - state.mix) * distMixCoeff;
    const int mode = config.mode;
    const float oddGain = config.oddGain, evenGain = config.evenGain;
    const float drive = config.drive, bias = config.bias, centre = config.centre;
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
std::pair<float, float> SharedEffects::chorus (float l, float r, size_t voice) noexcept
{
    // LJuno-116 TEST95 stereo chorus: quadrature LFO, fractional delay and
    // separate 45 Hz HP on each wet channel, wet-only aux return.
    if (chorusL[voice].data.empty()) return { 0, 0 };
    // The common chorus LFO advances once per audio sample in process().
    // Voice-specific delay lines keep both chorus sends isolated.
    const double a = std::sin (2.0*pi*chorusPhase);
    const double b = std::sin (2.0*pi*(chorusPhase+.25));
    const double dl = .006+(.018-.006)*(.5+.5*a*.65);
    const double dr = .006+(.018-.006)*(.5+.5*b*.65);
    const float fromL = chorusL[voice].read (dl*fs), fromR = chorusR[voice].read (dr*fs);
    chorusL[voice].push (l); chorusR[voice].push (r);
    const float wetL = fromL-chorusHpXL[voice]+chorusHpCoefficient*chorusHpYL[voice];
    const float wetR = fromR-chorusHpXR[voice]+chorusHpCoefficient*chorusHpYR[voice];
    chorusHpXL[voice] = fromL; chorusHpYL[voice] = wetL;
    chorusHpXR[voice] = fromR; chorusHpYR[voice] = wetR;
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
std::pair<float, float> SharedEffects::reverb (float l, float r,
    const std::array<std::pair<float, float>, basslineCount>& voiceSends,
    const LfoSamples& lfoSamples) noexcept
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
    for (size_t v = 0; v < basslineCount; ++v)
    {
        sourcePreL[v].push (voiceSends[v].first);
        sourcePreR[v].push (voiceSends[v].second);
    }

    // Source-specific directional excitation BEFORE feedback diffusion.
    // The decoding basis is the same signed geometry used by the FDN wet
    // outputs. The previous pair rotations tended to cancel direction when
    // passed through Householder mixing and the two output projections.
    // Encode position onto the left/right listening bases instead, preserving
    // the total energy for a mono bassline and never panning the wet return.
    // At position zero the original input is EXACTLY unchanged.
    std::array<float, 8> spatialExcitationDelta {};
    std::array<float, 2> directionalEarlyReflections {};
    // Independent of source pan/LFO: the early-reflection field is ALWAYS
    // present, including at the exact centre. Automation is ramped to avoid
    // clicks. This controls only the early reflections, NOT the diffuse FDN.
    const float earlyTarget = clampf (c[fx::reverbEarlyLevel] * .01f, 0.0f, 1.0f);
    earlyReflectionsLevelSmoothed += (earlyTarget - earlyReflectionsLevelSmoothed)
                                   * earlyReflectionsLevelSmoothCoeff;
    constexpr std::array<float, 8> leftBasis {{ 1,-1, 1,-1, 1,-1, 1,-1 }};
    constexpr std::array<float, 8> rightBasis {{ 1, 1,-1,-1, 1, 1,-1,-1 }};
    for (size_t v = 0; v < basslineCount; ++v)
    {
        const size_t base = v == 0 ? fx::reverbSourcePan1 : fx::reverbSourcePan2;
        const size_t depth = v == 0 ? fx::reverbLfoPan1 : fx::reverbLfoPan2;
        const float target = clampf ((c[base] + c[depth] * lfoSamples[v]) * .01f, -1.0f, 1.0f);
        float& position = sourcePositionsSmoothed[v];
        position += (target - position) * sourcePositionSmoothCoeff;
        // Keep the original FDN exactly unchanged when position is zero.
        // Early reflections are independent: centred sources need the same
        // reflection field as moving ones, with no sudden level increase.
        if (std::abs (position) > 1.0e-7f)
        {
            const float voiceL = sourcePreL[v].read (readOffset);
            const float voiceR = sourcePreR[v].read (readOffset);
            const float centre = 0.5f * (voiceL + voiceR);
            const float originalWeight = std::sqrt (std::max (0.0f, 1.0f - position * position));
            const float leftWeight = std::max (0.0f, -position);
            const float rightWeight = std::max (0.0f, position);
            for (size_t i = 0; i < 8; ++i)
            {
                const float original = (i & 1) == 0 ? voiceL : voiceR;
                const float steered = original * originalWeight
                    + centre * (leftWeight * leftBasis[i] + rightWeight * rightBasis[i]);
                spatialExcitationDelta[i] += steered - original;
            }
        }

        if (earlyReflectionsLevelSmoothed > 1.0e-6f)
        {
            // Four real pre-delay reflections for each source. Their arrival
            // times are different, so the listener hears a moving origin in
            // the room, not a gain-panned copy of the completed wet tail.
            // Each side receives the near-wall and opposite-wall reflections;
            // the opposite wall is weaker and arrives on different paths.
            // Reads use the SAME source pre-delay history as FDN injection.
            const auto reflectionTap = [&] (double ms) noexcept
            {
                const double offset = readOffset + ms * .001 * fs;
                return .5f * (sourcePreL[v].read (offset)
                            + sourcePreR[v].read (offset));
            };
            const float t1 = reflectionTap (11.0);
            const float t2 = reflectionTap (23.0);
            const float t3 = reflectionTap (34.0);
            const float t4 = reflectionTap (49.0);
            const float nearWall = .45f * t1 + .26f * t2 - .16f * t3;
            const float oppositeWall = .10f * t2 - .08f * t3 + .07f * t4;
            const float nearLeft = std::sqrt (.5f * (1.0f - position));
            const float nearRight = std::sqrt (.5f * (1.0f + position));
            // The near and opposite walls share two tap delays. Their
            // correlation would otherwise make the centre slightly louder
            // than the sides. Normalise the tap impulse-response energy so
            // turning Source Pan/LFO Pan changes placement, NOT ER volume.
            constexpr float tapEnergy = .45f*.45f + .26f*.26f + .16f*.16f
                                      + .10f*.10f + .08f*.08f + .07f*.07f;
            constexpr float commonTapProduct = .26f*.10f + .16f*.08f;
            const float positionComp = std::sqrt (tapEnergy /
                 (tapEnergy + 4.0f * nearLeft * nearRight * commonTapProduct));
            directionalEarlyReflections[0] += positionComp
                * (nearLeft * nearWall + nearRight * oppositeWall);
            directionalEarlyReflections[1] += positionComp
                * (nearRight * nearWall + nearLeft * oppositeWall);
        }
    }
    const double size = mode==1 ? .58 : mode==2 ? .85 : mode==3 ? 1.0 : 1.2;
    const double seconds = std::max (.15, static_cast<double> (c[fx::reverbDecay])
                                    * (mode==1 ? .55 : mode==2 ? .8 : mode==3 ? 1 : 1.55));
    const double cutoff = c[fx::reverbDamping]*(mode==2 ? 1.3 : mode==4 ? .75 : 1);
    const float damp = static_cast<float> (1-std::exp (-2*pi*std::min (cutoff,fs*.45)/fs));
    // Smooth automations to avoid a step in the reverb feedback gain/crossover.
    verbBassXoverSmoothed += (verbBassXoverTarget - verbBassXoverSmoothed)
                                * verbBassSmoothCoeff;
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
    // The original Householder network is orthogonal. The OPTIONAL moving
    // reflection paths below consist only of Givens (orthogonal) rotations;
    // unlike wet pan or a post-FDN chorus, the sound travels on every return
    // through the room. No rotation = exactly the original FDN feedback.
    std::array<float, 8> reflected {};
    for (size_t i = 0; i < reflected.size(); ++i)
        reflected[i] = sum * .25f - current[i];

    const int motionMode = juce::jlimit (0, 6, juce::roundToInt (c[fx::reverbMotionMode]));
    const float targetDepth = motionMode == 0 ? 0.0f
        : clampf (c[fx::reverbMotionDepth] * .01f, 0.0f, 1.0f);
    verbMotionDepthSmoothed += (targetDepth - verbMotionDepthSmoothed) * verbMotionSmoothCoeff;
    const float targetRate = clampf (c[fx::reverbMotionRate], .02f, 2.0f);
    verbMotionRateSmoothed += (targetRate - verbMotionRateSmoothed) * verbMotionSmoothCoeff;
    verbMotionPhase += (2.0 * pi * verbMotionRateSmoothed) / fs;
    if (verbMotionPhase >= 2.0 * pi) verbMotionPhase -= 2.0 * pi;

    // A new wet excitation begins a new spatial expansion/contraction.
    // Long tails keep progressing after the source has stopped, with no output
    // amplitude gating; sustained notes keep the room near its initial size.
    if (std::abs (inL) + std::abs (inR) > 1.0e-5f) verbMotionAge = 0.0;
    else verbMotionAge = std::min (120.0, verbMotionAge + 1.0 / fs);

    if (verbMotionDepthSmoothed > 1.0e-6f ||
        std::any_of (verbMotionAngles.begin(), verbMotionAngles.end(),
                     [] (float angle) { return std::abs (angle) > 1.0e-6f; }))
    {
        const float s = static_cast<float> (std::sin (verbMotionPhase));
        const float q = static_cast<float> (std::cos (verbMotionPhase));
        const float progress = clampf (static_cast<float> (verbMotionAge * verbMotionRateSmoothed), 0.0f, 1.0f);
        const float ageCurve = progress * progress * (3.0f - 2.0f * progress);
        std::array<float, 4> pattern {};
        switch (motionMode)
        {
            case 1: pattern = { s, q, -s, -q }; break; // L -> R travelling reflections
            case 2: pattern = { s, -q, -s, q }; break; // R -> L, reversed circulation
            case 3: pattern = { s, s, s, s }; break; // back-and-forth bounce
            case 4: pattern = { s, q, -s, -q }; break; // orbit in two crossing planes
            case 5: pattern = { ageCurve, -ageCurve, ageCurve, -ageCurve }; break;
            case 6: pattern = { 1.0f - ageCurve, ageCurve - 1.0f,
                                1.0f - ageCurve, ageCurve - 1.0f }; break;
            default: break; // Off, smooth rotations back to identity
        }
        // Across left/right-injected lines, then across distant FDN lines.
        // Each pair rotation preserves sum-of-squares feedback energy.
        for (size_t pair = 0; pair < 4; ++pair)
        {
            const float target = .90f * verbMotionDepthSmoothed * pattern[pair];
            float& angle = verbMotionAngles[pair];
            angle += (target - angle) * verbMotionSmoothCoeff;
            const float sine = std::sin (angle), cosine = std::cos (angle);
            const size_t a = pair * 2, b = a + 1;
            const float x = reflected[a], y = reflected[b];
            reflected[a] = cosine * x - sine * y;
            reflected[b] = sine * x + cosine * y;
        }
        for (size_t pair = 0; pair < 4; ++pair)
        {
            // The second plane drives orbit/dispersion rather than output pan.
            const float travel = motionMode == 4
                ? (pair % 2 == 0 ? q : -s)
                : (motionMode == 1 || motionMode == 2)
                    ? pattern[(pair + 1) % 4]
                    : (motionMode == 3 ? (pair % 2 == 0 ? s : -s) : pattern[pair]);
            const float target = .34f * verbMotionDepthSmoothed * travel;
            float& angle = verbMotionAngles[4 + pair];
            angle += (target - angle) * verbMotionSmoothCoeff;
            const float sine = std::sin (angle), cosine = std::cos (angle);
            const size_t a = pair, b = pair + 4;
            const float x = reflected[a], y = reflected[b];
            reflected[a] = cosine * x - sine * y;
            reflected[b] = sine * x + cosine * y;
        }
    }
    for (size_t i=0;i<8;++i)
    {
        verbDamped[i] += (reflected[i]-verbDamped[i])*damp;
        const double effectiveSeconds = std::clamp (
            static_cast<double> (verbTimes[i]) * size, 2.0 / fs,
            static_cast<double> (verb[i].data.size() - 2) / fs);
        const auto gain = static_cast<float> (std::pow (10.0, -3.0*effectiveSeconds/seconds));
        // Split the damped feedback *inside* the network. A shorter bass RT60
        // subtracts low feedback on successive circulations, while leaving
        // the initial reflections and mid/high decay untouched.
        verbBassLow[i] += (verbDamped[i] - verbBassLow[i]) * verbBassXoverSmoothed;
        verbBassGainDeltaSmoothed[i] += (verbBassGainDeltaTarget[i] - verbBassGainDeltaSmoothed[i])
                                        * verbBassSmoothCoeff;
        const float feedback = verbDamped[i] * std::min (.9995f, gain)
                             + verbBassLow[i] * verbBassGainDeltaSmoothed[i];
        const float source = (((i & 1)==0 ? inL : inR) + spatialExcitationDelta[i]) * .18f;
        verb[i].push (bounded (source + feedback));
    }
    // Decorrelated stereo output. Diffuse tail continues even with zero input.
    float wetL = current[0]-current[1]+current[2]-current[3]+current[4]-current[5]+current[6]-current[7];
    float wetR = current[0]+current[1]-current[2]-current[3]+current[4]+current[5]-current[6]-current[7];
    wetL *= .23f; wetR *= .23f;
    // Source-specific early-reflection level independent from Source Pan/LFO Pan.
    // At zero: the original FDN reverb remains audible, but the four early taps
    // vanish. At centre: the early taps are as present as at other positions.
    wetL += directionalEarlyReflections[0] * (1.8f * earlyReflectionsLevelSmoothed);
    wetR += directionalEarlyReflections[1] * (1.8f * earlyReflectionsLevelSmoothed);
    const float width = c[fx::reverbWidth]*.01f;
    const float middle = .5f*(wetL+wetR), side = .5f*(wetL-wetR)*width;
    // Historical TEST2.4.7 makeup (+18.06 dB) stays in place. TEST2.4.49
    // applies a separate exact -6 dB final wet trim to BOTH diffuse tail
    // and early reflections. Sends, the 0-100% Reverb Level control, dry,
    // reverb feedback, chorus and delay keep their original behaviour.
    const float level = c[fx::reverbLevel] * .02f * reverbReturnMakeupGain
                      * reverbWetOutputTrim6dB;
    return { bounded ((middle+side)*level), bounded ((middle-side)*level) };
}
SharedEffects::Output SharedEffects::process (float aL, float aR, float bL, float bR,
                                                        const LfoSamples& lfoSamples) noexcept
{
    // INSERT: two distortion processors act independently, before chorus.
    const auto [axL, axR] = enabledDistortion[0] ? distort (aL, aR, 0)
                                                      : std::pair<float,float> { aL, aR };
    const auto [bxL, bxR] = enabledDistortion[1] ? distort (bL, bR, 1)
                                                      : std::pair<float,float> { bL, bR };
    return processPostDistortion (axL, axR, bxL, bxR, lfoSamples);
}
SharedEffects::Output SharedEffects::processPostDistortion (float axL, float axR,
                                               float bxL, float bxR,
                                               const LfoSamples& lfoSamples) noexcept
{
    // Whole wet graph is nonexistent when chorus, delay and reverb are off.
    // This leaves the original stereo stems and 1x rendering untouched.
    if (!enabledSharedWetFx)
    {
        Output out;
        out.wet = { 0.0f, 0.0f };
        out.dry[0] = { bounded (axL), bounded (axR) };
        out.dry[1] = { bounded (bxL), bounded (bxR) };
        return out;
    }
    // AUX CHORUS: one global phase/rate, separate voice wet lines. This allows
    // Delay/Reverb send 1 and 2 to remain genuinely isolated even on tails.
    float ch1L = 0.0f, ch1R = 0.0f, ch2L = 0.0f, ch2R = 0.0f;
    if (c[fx::chorusOn] > .5f)
    {
        chorusRateSmoothed += (c[fx::chorusRate] * .01f - chorusRateSmoothed) * .0005f;
        const double rateHz = .10 + chorusRateSmoothed * 1.40;
        chorusPhase += rateHz / fs;
        if (chorusPhase >= 1.0) chorusPhase -= 1.0;
        const float s1 = c[fx::chorusSend1] * .01f;
        const float s2 = c[fx::chorusSend2] * .01f;
        const auto first = chorus (axL * s1, axR * s1, 0);
        const auto second = chorus (bxL * s2, bxR * s2, 1);
        ch1L = first.first; ch1R = first.second;
        ch2L = second.first; ch2R = second.second;
    }
    // The wet chorus is part of the sound being sent to later effects;
    // Dry Level still attenuates only the DIRECT bassline stems.
    const float composed1L = axL + ch1L, composed1R = axR + ch1R;
    const float composed2L = bxL + ch2L, composed2R = bxR + ch2R;
    const auto send = [&] (size_t one, size_t two)
    {
        const float s1 = c[one] * .01f, s2 = c[two] * .01f;
        return std::pair<float,float> { composed1L * s1 + composed2L * s2,
                                        composed1R * s1 + composed2R * s2 };
    };
    const auto delayStem = send (fx::delaySend1, fx::delaySend2);
    const float verbS1 = c[fx::reverbSend1] * .01f;
    const float verbS2 = c[fx::reverbSend2] * .01f;
    const std::array<std::pair<float, float>, basslineCount> reverbVoiceSends {{
        { composed1L * verbS1, composed1R * verbS1 },
        { composed2L * verbS2, composed2R * verbS2 }
    }};
    // Preserve the original floating-point summation order of the reverb send.
    const auto reverbStem = send (fx::reverbSend1, fx::reverbSend2);
    const int delayMode = juce::jlimit (0, 2, juce::roundToInt (c[fx::delayType]));
    const bool verbEnabled = c[fx::reverbType] > .5f;
    // Master links respect destination send controls: both Reverb Sends at
    // zero must silence new reverb excitation, even in serial FX routing.
    const float reverbLink = std::max (c[fx::reverbSend1], c[fx::reverbSend2]) * .01f;

    const auto runDelay = [&] (float l, float r) -> std::pair<float, float>
    {
        if (delayMode == 0) return { 0.0f, 0.0f };
        if (delayMode == 1)
        {
            const auto result = classic.processClassicDelay (l, r);
            // The Classic delay core includes its own dry term; remove it so
            // this bus always carries ONLY delay repeats, not duplicate dry.
            return { bounded (result.first - l * classicDryGain),
                     bounded (result.second - r * classicDryGain) };
        }
        return dualDelay (l, r);
    };
    const auto runReverb = [&] (float l, float r) -> std::pair<float, float>
    {
        return verbEnabled ? reverb (l, r, reverbVoiceSends, lfoSamples)
                           : std::pair<float,float> { 0.0f, 0.0f };
    };

    // Fixed insert order: Delay -> Reverb. An old saved fx::effectOrder
    // host parameter may still exist for backward-compatible snapshots,
    // but no longer changes the routing or appears in the FX grid.
    const auto delayWet = runDelay (delayStem.first, delayStem.second);
    const auto reverbWet = runReverb (reverbStem.first + delayWet.first * reverbLink,
                                      reverbStem.second + delayWet.second * reverbLink);
    Output out;
    // Exactly one shared wet bus, even when both voices are sent to stems.
    out.wet = { bounded (ch1L + ch2L + delayWet.first + reverbWet.first),
                bounded (ch1R + ch2R + delayWet.second + reverbWet.second) };
    out.dry[0] = { bounded (axL), bounded (axR) };
    out.dry[1] = { bounded (bxL), bounded (bxR) };
    return out;
}
}
