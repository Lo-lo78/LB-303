// SPDX-License-Identifier: AGPL-3.0-or-later
#include "BassEngine.h"
#include <algorithm>
#include <cmath>

namespace lb303
{
namespace
{
constexpr double pi = 3.1415926535897932384626433832795;
double clamp (double v, double lo, double hi) { return std::clamp (v, lo, hi); }
}

void BassEngine::Biquad::clear() { x1 = x2 = y1 = y2 = 0; }
void BassEngine::Biquad::lowpass (double hz, double q, double fs)
{
    const auto w = 2.0 * pi * std::min (hz / fs, 0.49);
    const auto c = std::cos (w), a = std::sin (w) / (2.0 * std::max (q, 0.01));
    const auto s = 1.0 / (1.0 + a);
    b0 = (1.0 - c) * 0.5 * s; b1 = (1.0 - c) * s; b2 = b0;
    a1 = -2.0 * c * s; a2 = (1.0 - a) * s;
}
void BassEngine::Biquad::highpass (double hz, double q, double fs)
{
    const auto w = 2.0 * pi * std::min (hz / fs, 0.49);
    const auto c = std::cos (w), a = std::sin (w) / (2.0 * std::max (q, 0.01));
    const auto s = 1.0 / (1.0 + a);
    b0 = (1.0 + c) * 0.5 * s; b1 = -(1.0 + c) * s; b2 = b0;
    a1 = -2.0 * c * s; a2 = (1.0 - a) * s;
}
void BassEngine::Biquad::peak (double hz, double q, double db, double fs)
{
    const auto w = 2.0 * pi * std::min (hz / fs, 0.49);
    const auto a = std::pow (10.0, db / 40.0), c = std::cos (w);
    const auto alpha = std::sin (w) / (2.0 * q);
    const auto s = 1.0 / (1.0 + alpha / a);
    b0 = (1.0 + alpha * a) * s;
    b1 = -2.0 * c * s;
    b2 = (1.0 - alpha * a) * s;
    a1 = -2.0 * c * s;
    a2 = (1.0 - alpha / a) * s;
}
float BassEngine::Biquad::process (float sample)
{
    const double x = sample;
    const double y = b0*x + b1*x1 + b2*x2 - a1*y1 - a2*y2;
    x2 = x1; x1 = x; y2 = y1; y1 = std::isfinite (y) ? y : 0.0;
    return static_cast<float> (clamp (y1, -1000, 1000));
}
void BassEngine::DelayLine::resize (size_t samples)
{
    memory.assign (std::max<size_t> (samples, 16), 0.0f);
    writeIndex = 0;
}
void BassEngine::DelayLine::clear()
{
    std::fill (memory.begin(), memory.end(), 0.0f);
    writeIndex = 0;
}
void BassEngine::DelayLine::write (float value)
{
    if (memory.empty()) return;
    memory[writeIndex] = value;
    if (++writeIndex >= memory.size()) writeIndex = 0;
}
float BassEngine::DelayLine::readCubic (double samples) const
{
    if (memory.empty()) return 0.0f;
    const auto count = static_cast<int64_t> (memory.size());
    if (samples < 0.0 || samples >= static_cast<double> (count - 3)) return 0.0f;
    auto pos = static_cast<double> (writeIndex) - samples;
    if (pos < 0.0) pos += count;
    const auto base = static_cast<int64_t> (std::floor (pos));
    const auto f = pos - std::floor (pos);
    const auto at = [this, count] (int64_t index) -> double
    {
        index = ((index % count) + count) % count;
        return memory[static_cast<size_t> (index)];
    };
    // Exact four-point CookDSP/JSFX interpolation weights.
    double a3 = (f*f - 1.0) / 6.0;
    double a2 = (f + 1.0) * 0.5;
    double a0 = a2 - 1.0;
    double a1 = a3 * 3.0;
    a2 -= a1; a0 -= a3; a1 -= f;
    a0 *= f; a1 = a1*f + 1.0; a2 *= f; a3 *= f;
    return static_cast<float> (a0*at (base - 1) + a1*at (base)
                             + a2*at (base + 1) + a3*at (base + 2));
}
void BassEngine::PitchShift::prepare()
{
    left.resize (8192); right.resize (8192); phase = 0;
}
void BassEngine::PitchShift::reset()
{
    left.clear(); right.clear(); phase = 0;
}
std::pair<float, float> BassEngine::PitchShift::process (float l, float r, int semitones)
{
    // TEST1: deterministic dual-grain, cross-faded delay transposition.
    // Unlike the Saike JSFX WSOLA+ACF routine, this does not align grain phases.
    // Kept isolated for a later exact fidelity pass; zero semitones bypasses it.
    left.write (l); right.write (r);
    if (semitones == 0) return { l, r };
    constexpr double grain = 1024.0;
    const auto ratio = std::pow (2.0, semitones / 12.0);
    const auto a = phase;
    const auto b = std::fmod (phase + 0.5, 1.0);
    const auto delayA = 2048.0 - (ratio - 1.0) * grain * a;
    const auto delayB = 2048.0 - (ratio - 1.0) * grain * b;
    const auto weightA = 1.0 - std::abs (2.0 * a - 1.0);
    const auto weightB = 1.0 - std::abs (2.0 * b - 1.0);
    const auto sum = std::max (weightA + weightB, 1.0e-10);
    const auto outL = (weightA*left.readCubic (delayA) + weightB*left.readCubic (delayB)) / sum;
    const auto outR = (weightA*right.readCubic (delayA) + weightB*right.readCubic (delayB)) / sum;
    phase += 1.0 / grain;
    if (phase >= 1.0) phase -= 1.0;
    return { static_cast<float> (outL), static_cast<float> (outR) };
}

double BassEngine::polyblep (double t, double dt)
{
    if (t < dt)
    {
        auto x = t / dt;
        return x + x - x*x - 1.0;
    }
    if (t > 1.0 - dt)
    {
        auto x = (t - 1.0) / dt;
        return x*x + x + x + 1.0;
    }
    return 0.0;
}

void BassEngine::prepare (double sampleRate)
{
    fs = std::max (sampleRate, 8000.0);
    delayL.resize (static_cast<size_t> (fs * 12.0) + 64);
    delayR.resize (static_cast<size_t> (fs * 12.0) + 64);
    pitchShift.prepare();
    airPeakL.peak (8000, 0.7, 4.0, fs);
    airPeakR.peak (8000, 0.7, 4.0, fs);
    outputLpfL.lowpass (std::min (7000.0, fs*0.49), 0.70710678118655, fs);
    outputLpfR.lowpass (std::min (7000.0, fs*0.49), 0.70710678118655, fs);
    reset();
}

void BassEngine::reset()
{
    freq = targetFreq = phase = pwmPhase = 0;
    envelope = lpgMemory = accentEnvelope = velNorm = 0;
    envelopeStage = 3;
    stage3 = stage4 = 0;
    lastNote = -1; noteActive = legato = accent = false;
    smoothDelay = delayTargetSamples = delayPhase = 0;
    delayL.clear(); delayR.clear(); pitchShift.reset();
    for (auto* filter : { &airPeakL, &airPeakR, &outputLpfL, &outputLpfR,
                          &delayLpL, &delayLpR, &delayHpL, &delayHpR })
        filter->clear();
}

void BassEngine::setParameters (const Values& values, double newBpm)
{
    p = values;
    tempo = std::max (1.0, std::isfinite (newBpm) ? newBpm : 120.0);
    delayLpL.lowpass (p[24], p[25], fs); delayLpR.lowpass (p[24], p[25], fs);
    delayHpL.highpass (p[26], p[27], fs); delayHpR.highpass (p[26], p[27], fs);
    double delayMs = p[19] * 1.25; // intentional 1.25x free-time scaling in JSFX
    const int sync = static_cast<int> (std::round (p[20]));
    constexpr int divisions[] = { 1, 16, 12, 8, 6, 4, 3, 2, 1 };
    if (sync >= 1 && sync <= 8)
        delayMs = (60000.0 / tempo) * 4.0 / divisions[sync];
    delayTargetSamples = std::min (10000.0, delayMs) * 0.001 * fs;
}

void BassEngine::releaseNote() noexcept
{
    // Do not reset effect tails, oscillators or delay: only release the held voice.
    noteActive = false;
    legato = false;
    envelopeStage = 3;
    lastNote = -1;
}

void BassEngine::midi (const juce::MidiMessage& message)
{
    if (message.isController() && message.getControllerNumber() == 123)
    {
        delayL.clear(); delayR.clear();
        return;
    }
    if (message.isNoteOn())
    {
        targetFreq = 440.0 * std::pow (2.0, (message.getNoteNumber() - 69.0) / 12.0);
        const auto velocity = message.getVelocity();
        velNorm = std::pow (velocity / 127.0, 0.6);
        accent = velocity >= p[15];
        accentEnvelope = accent ? 1.0 : 0.0;
        legato = noteActive;
        noteActive = true;
        lastNote = message.getNoteNumber();
        if (! legato) envelopeStage = 0;
        return;
    }
    if (message.isNoteOff() && message.getNoteNumber() == lastNote)
    {
        noteActive = legato = false;
        envelopeStage = 3;
    }
}

std::pair<float, float> BassEngine::tick (float inputL, float inputR)
{
    // Oscillator, amplitude/LP envelopes and nonlinear filter track the JSFX.
    const bool glide = p[5] > 0 && (p[6] < 0.5f || legato);
    const auto glideCoeff = glide ? std::max (1.0 / (1.0 + p[5]*150.0), 0.0002) : 1.0;
    freq += (targetFreq - freq) * glideCoeff;
    const auto tuned = freq * std::pow (2.0, p[14] / 12.0);
    const auto dt = clamp ((tuned * 0.5) / fs, 0.000001, 0.49);
    phase += dt; if (phase >= 1.0) phase -= 1.0;

    double pwmLfo = 0.0;
    if (p[12] > 0)
    {
        const auto pwmIncrement = std::max (0.001, static_cast<double> (p[11])) * 0.25
                                / (fs * 60.0 / tempo);
        pwmPhase += pwmIncrement;
        pwmPhase -= std::floor (pwmPhase);
        pwmLfo = pwmPhase < 0.5 ? pwmPhase * 4.0 - 1.0 : 3.0 - pwmPhase * 4.0;
    }
    const auto saw1 = (2.0*phase - 1.0)*0.5 - polyblep (phase, dt);
    double osc = saw1;
    if (p[0] > 0.5f)
    {
        const auto duty = clamp (0.5 + pwmLfo*p[12], 0.01, 0.99);
        auto t2 = phase + 1.0 - duty;
        t2 -= std::floor (t2);
        const auto saw2 = 2.0*t2 - 1.0 - polyblep (t2, dt);
        osc = 0.4 * (saw1 - saw2);
    }

    const double transient = p[18] < 0.5f ? 0.7 : p[18] > 1.5f ? 1.6 : 1.0;
    const double decayMult = accent ? 1.8 : 1.0;
    switch (envelopeStage)
    {
        case 0:
            envelope += p[1] > 0 ? transient / (p[1]*fs) : 1.0;
            if (envelope >= 1.0) { envelope = 1.0; envelopeStage = 1; }
            break;
        case 1:
            envelope -= p[2] > 0 ? transient * (1.0-p[3]) * decayMult / (p[2]*fs) : 1.0;
            if (envelope <= p[3]) { envelope = p[3]; envelopeStage = 2; }
            break;
        case 2:
            envelope = p[3];
            break;
        default:
            envelope -= p[4] > 0 ? transient * p[3] / (p[4]*fs) : 1.0;
            if (envelope < 0) envelope = 0;
            break;
    }
    const auto ampEnv = envelopeStage < 2 ? envelope : (envelopeStage == 2 ? 0.25 : envelope);
    const auto velEnergy = 0.35 + velNorm*0.65;
    const auto lpgSpeed = 0.03 + velNorm*0.12;
    lpgMemory += (ampEnv*velEnergy - lpgMemory)*lpgSpeed;
    const auto lpgEnv = std::max (0.0, lpgMemory);
    const auto inLpg = p[33] > 0.5f;
    const auto ampFinal = inLpg ? std::pow (lpgEnv, 1.6) : ampEnv;
    const auto lpgGain = inLpg ? 1.8 + lpgEnv*0.25 : 1.0;
    const auto cutEnv = inLpg ? lpgEnv*0.85 : envelope;
    const auto velDepth = p[17] / 127.0;
    const auto shaped = std::pow (velNorm, 1.0 + velDepth*3.0);
    const auto velFactor = 1.0 - velDepth + shaped*velDepth;
    const auto signal = osc*ampFinal*lpgGain*p[7]*(accent ? 1.35 : 1.0)*velFactor;

    // Preserve source accent envelope, including its repeated renewal on
    // accented notes (quirk of the original implementation).
    accentEnvelope -= accentEnvelope*0.25;
    if (accentEnvelope < 0.0001) accentEnvelope = 0;
    if (accent) accentEnvelope = 1.0;
    accentEnvelope -= accentEnvelope * (1.0 - std::exp (-1.0 / (0.01*fs)));
    if (accentEnvelope < 0.0001) accentEnvelope = 0;
    const auto classicCut = p[8] + cutEnv*p[10] + accentEnvelope*(p[16]*0.18);
    const auto reduction = !inLpg ? (1.0-velNorm)*(p[34]/127.0)*p[10] : 0.0;
    const auto combined = clamp (classicCut-reduction, 0.0, 1.0);
    const auto cut = std::min ((std::pow (2.0, combined*14.28771)*2.0)/fs, 0.99);
    const auto resonance = p[9]*(7.0 + accentEnvelope*4.0*p[16]);
    const auto fb = resonance + resonance / (1.0-cut);
    stage3 += cut*(signal - stage3 + fb*(stage3-stage4));
    stage4 += cut*(stage3-stage4);
    if (!std::isfinite (stage3) || !std::isfinite (stage4)) stage3 = stage4 = 0;
    const auto filteredL = airPeakL.process (static_cast<float> (stage4));
    const auto filteredR = airPeakR.process (static_cast<float> (stage4));
    const auto drive = p[13];
    const auto clip = [drive] (float s) -> float
    {
        return drive > 0.0f ? (s*drive) / (1.0f+std::abs (s*drive)) : s;
    };
    const auto synthL = outputLpfL.process (2.0f*clip (filteredL));
    const auto synthR = outputLpfR.process (2.0f*clip (filteredR));

    // CookDSP delay interpolation, time modulation and per-channel filters.
    // JSFX rc_set(0.5) deliberately smooths time over a long (~0.5 s) period.
    smoothDelay += (delayTargetSamples - smoothDelay) / (0.5*fs+1.0);
    const auto modAmp = p[28];
    const auto sine = modAmp > 0.0f ? std::sin (2.0*pi*delayPhase) : 0.0;
    if (modAmp > 0.0f)
    {
        delayPhase += p[29] / fs;
        if (delayPhase > 1.0) delayPhase -= 1.0;
    }
    const bool pingPong = p[22] > 0.5f;
    const auto spread = p[23];
    const auto lfactor = spread > 0 ? 1.0 - spread/100.0 : 1.0;
    const auto rfactor = spread < 0 ? 1.0 + spread/100.0 : 1.0;
    const auto leftHaas = (!pingPong && spread > 0) ? fs/100.0*spread/100.0 : 0.0;
    const auto rightHaas = (!pingPong && spread < 0) ? fs/100.0*(-spread)/100.0 : 0.0;
    auto val0 = delayL.readCubic (smoothDelay + sine*modAmp + leftHaas);
    auto val1 = delayR.readCubic (smoothDelay + sine*modAmp + rightHaas);
    if (p[24] < 20000.0f) { val0 = delayLpL.process (val0); val1 = delayLpR.process (val1); }
    if (p[26] > 20.0f) { val0 = delayHpL.process (val0); val1 = delayHpR.process (val1); }
    const double feedback = p[21] / 100.0;
    auto writeL = pingPong ? synthL*lfactor + val1*feedback : synthL + val0*feedback;
    auto writeR = pingPong ? synthR*rfactor + val0*feedback : synthR + val1*feedback;
    if (const auto semitones = juce::roundToInt (p[30]); semitones != 0)
    {
        const auto shifted = pitchShift.process (static_cast<float> (writeL), static_cast<float> (writeR), semitones);
        writeL = shifted.first; writeR = shifted.second;
    }
    delayL.write (static_cast<float> (writeL));
    delayR.write (static_cast<float> (writeR));
    const auto dry = p[31]/100.0, wet = p[32]/100.0;
    const auto outGain = wet > 0 ? 1.0 : 1.3;
    const auto outL = (synthL*dry + val0*wet)*outGain;
    const auto outR = (synthR*dry + val1*wet)*outGain;
    const auto safe = [] (double x) { return static_cast<float> (std::isfinite (x) ? clamp (x, -1000, 1000) : 0.0); };
    // JSFX sums incoming audio after the synth+delay output.
    return { safe (inputL+outL), safe (inputR+outR) };
}
}
