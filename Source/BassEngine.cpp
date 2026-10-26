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
    freq = targetFreq = phase = pwmPhase = osc2Phase = osc2GainSmooth = 0;
    osc2DetuneSmooth = 10;
    lfo = {};
    envelope = lpgMemory = accentEnvelope = velNorm = 0;
    envelopeStage = 3;
    stage3 = stage4 = 0;
    lastNote = -1; noteActive = legato = accent = false;
    pendingSequencerAccent = -1;
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
    if (message.isController() && message.getControllerNumber() == 119)
    {
        pendingSequencerAccent = message.getControllerValue() >= 64 ? 1 : 0;
        return;
    }
    if (message.isNoteOn())
    {
        if (lfoParameters[lb303::lfo::mode] >= .5f || lfoParameters[lb303::lfo::oneShot] >= .5f)
        {
            lfo.phase = 0.0;
            lfo.shotPosition = 0.0;
            lfo.shotDone = false;
            lfo.delayEnvelope = 0.0;
            lfo.previousPhase = 1.0;
        }
        // Align the layer at the start of a new non-legato note. The second
        // oscillator then runs freely at its detuned rate for audible beating;
        // it has no independent MIDI voice, envelope or waveform selector.
        if (! noteActive) osc2Phase = phase;
        targetFreq = 440.0 * std::pow (2.0, (message.getNoteNumber() - 69.0) / 12.0);
        const auto velocity = message.getVelocity();
        velNorm = std::pow (velocity / 127.0, 0.6);
        accent = pendingSequencerAccent >= 0 ? pendingSequencerAccent != 0 : velocity >= p[15];
        pendingSequencerAccent = -1;
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

double BassEngine::nextLfoSample() noexcept
{
    // LJuno-116 LFO wave, PWM, phase, one-shot, delay and smoothing logic,
    // adapted for one monophonic LFO per bassline without allocations.
    const auto& v = lfoParameters;
    const auto syncedRate = v[lb303::lfo::rate] *
        (v[lb303::lfo::bpmSync] >= .5f ? tempo / 120.0 : 1.0);
    const double step = std::max (0.0, syncedRate) / fs;
    const bool shot = v[lb303::lfo::oneShot] >= .5f && v[lb303::lfo::oneShotPercent] > 0;
    if (! (shot && lfo.shotDone))
    {
        double increment = step;
        if (shot)
        {
            const auto limit = clamp (v[lb303::lfo::oneShotPercent] * .01, 0.0, 1.0);
            increment = std::min (step, std::max (0.0, limit - lfo.shotPosition));
            lfo.shotPosition += increment;
            if (lfo.shotPosition >= limit) lfo.shotDone = true;
        }
        lfo.phase += increment;
        lfo.phase -= std::floor (lfo.phase);
    }
    const double phi = lfo.phase + v[lb303::lfo::phase] -
                       std::floor (lfo.phase + v[lb303::lfo::phase]);
    double raw = 0.0;
    switch (juce::roundToInt (v[lb303::lfo::wave]))
    {
        case 0: raw = std::sin (2.0 * pi * phi); break;
        case 1: raw = phi < .5 ? phi * 4.0 - 1.0 : 3.0 - phi * 4.0; break;
        case 2: raw = phi * 2.0 - 1.0; break;
        case 3: raw = 1.0 - phi * 2.0; break;
        case 4: raw = phi < clamp (v[lb303::lfo::squarePwm], .05, .95) ? 1.0 : -1.0; break;
        default:
            if (phi < lfo.previousPhase)
            {
                lfo.rng ^= lfo.rng << 13;
                lfo.rng ^= lfo.rng >> 17;
                lfo.rng ^= lfo.rng << 5;
                lfo.held = static_cast<double> (lfo.rng) / 2147483648.0 - 1.0;
            }
            raw = lfo.held;
            break;
    }
    lfo.previousPhase = phi;
    const auto smooth = static_cast<double> (v[lb303::lfo::smooth]);
    if (smooth > 0.0)
    {
        // LJuno stores a 0..100 Smooth parameter; scale to its 0..1 DSP unit.
        const auto amount = smooth * .01;
        const double coefficient = 1.0 /
            (1.0 + amount*amount * (v[lb303::lfo::wave] >= 4.5f ? 2000.0 : 400.0));
        lfo.smooth += (raw - lfo.smooth) * coefficient;
    }
    else lfo.smooth = raw;
    double output = lfo.smooth;
    output *= output >= 0.0 ? 1.0 - v[lb303::lfo::upperSquash]
                            : 1.0 - v[lb303::lfo::lowerSquash];
    if (v[lb303::lfo::delay] > 0.0f)
    {
        lfo.delayEnvelope += (1.0 - lfo.delayEnvelope) /
            std::max (1.0, static_cast<double> (v[lb303::lfo::delay]) * fs);
        output *= lfo.delayEnvelope;
    }
    else lfo.delayEnvelope = 1.0;
    return output;
}

std::pair<float, float> BassEngine::tick (float inputL, float inputR)
{
    // Oscillator, amplitude/LP envelopes and nonlinear filter track the JSFX.
    const bool glide = p[5] > 0 && (p[6] < 0.5f || legato);
    const auto glideCoeff = glide ? std::max (1.0 / (1.0 + p[5]*150.0), 0.0002) : 1.0;
    freq += (targetFreq - freq) * glideCoeff;
    const auto modulation = nextLfoSample();
    const auto tuned = freq * std::pow (2.0,
        (p[14] + modulation * lfoParameters[lb303::lfo::pitchDepth]) / 12.0);
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
    // Shared Saw/Square selection and PWM: no separate waveform controls
    // or independent filter/envelopes for the detuned layer.
    const auto duty = clamp (0.5 + pwmLfo*p[12], 0.01, 0.99);
    const auto waveform = [this, duty] (double currentPhase, double increment)
    {
        const auto saw = (2.0*currentPhase - 1.0)*0.5 - polyblep (currentPhase, increment);
        if (p[0] <= 0.5f) return saw;
        auto offsetPhase = currentPhase + 1.0 - duty;
        offsetPhase -= std::floor (offsetPhase);
        const auto otherSaw = 2.0*offsetPhase - 1.0 - polyblep (offsetPhase, increment);
        return 0.4 * (saw - otherSaw);
    };
    const double primary = waveform (phase, dt);
    // 5 ms gain ramp and 10 ms pitch ramp avoid abrupt automation/lock steps.
    const auto gainCoeff = std::min (1.0, 1.0 / (0.005 * fs));
    const auto tuneCoeff = std::min (1.0, 1.0 / (0.010 * fs));
    osc2GainSmooth += (clamp (osc2Parameters[osc2::volume], 0.0, 100.0) * .01
                       - osc2GainSmooth) * gainCoeff;
    osc2DetuneSmooth += (clamp (osc2Parameters[osc2::detune], -100.0, 100.0)
                         - osc2DetuneSmooth) * tuneCoeff;
    // +/-100 cents = +/-1 semitone; do NOT quantize to MIDI semitones.
    const double secondaryDt = clamp (dt * std::pow (2.0, osc2DetuneSmooth / 1200.0),
                                      0.000001, 0.49);
    osc2Phase += secondaryDt;
    if (osc2Phase >= 1.0) osc2Phase -= 1.0;
    // Both oscillators feed the SAME filter/envelope. Volume=0 reproduces the
    // original single-oscillator DSP exactly, including its original gain.
    const double osc = primary + osc2GainSmooth * waveform (osc2Phase, secondaryDt);

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
    const auto lfoVolume = clamp (1.0 + modulation * lfoParameters[lb303::lfo::volumeDepth], 0.0, 2.0);
    const auto signal = osc*ampFinal*lpgGain*p[7]*lfoVolume*(accent ? 1.35 : 1.0)*velFactor;

    // Preserve source accent envelope, including its repeated renewal on
    // accented notes (quirk of the original implementation).
    accentEnvelope -= accentEnvelope*0.25;
    if (accentEnvelope < 0.0001) accentEnvelope = 0;
    if (accent) accentEnvelope = 1.0;
    accentEnvelope -= accentEnvelope * (1.0 - std::exp (-1.0 / (0.01*fs)));
    if (accentEnvelope < 0.0001) accentEnvelope = 0;
    const auto classicCut = p[8] + cutEnv*p[10] + accentEnvelope*(p[16]*0.18)
                            + modulation * lfoParameters[lb303::lfo::filterDepth];
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

    // Raw stereo synth; both historic delay instances are now removed from the voice bus.
    const auto panLfo = clamp (modulation * lfoParameters[lb303::lfo::panDepth], -1.0, 1.0);
    return { static_cast<float> (synthL * (panLfo > 0 ? 1.0 - panLfo : 1.0)),
             static_cast<float> (synthR * (panLfo < 0 ? 1.0 + panLfo : 1.0)) };
}

std::pair<float, float> BassEngine::processClassicDelay (float synthL, float synthR)
{
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
    return { safe (outL), safe (outR) };
}
}
