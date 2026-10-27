// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "ParameterCatalog.h"
#include <array>
#include <vector>
#include <utility>
#include <cstddef>
#include <cstdint>

namespace lb303
{
class BassEngine
{
public:
    using Values = std::array<float, synthParameterCount>;
    void prepare (double sampleRate);
    void setOversamplingFactor (int factor) noexcept; // 1/2/4, no allocation
    void reset();
    void setParameters (const Values& values, double bpm);
    using LfoValues = std::array<float, lfoParameterCount>;
    void setLfoParameters (const LfoValues& values) noexcept;
    using Osc2Values = std::array<float, osc2ParameterCount>;
    void setOsc2Parameters (const Osc2Values& values) noexcept;
    void midi (const juce::MidiMessage& event);
    void releaseNote() noexcept; // stop a direct note when switching to sequencer mode
    std::pair<float, float> tick (float inputL, float inputR);
    // Shared FX instance only: the original JSFX classic delay on an external stereo bus.
    std::pair<float, float> processClassicDelay (float synthL, float synthR);

private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        void lowpass (double freq, double q, double sampleRate);
        void highpass (double freq, double q, double sampleRate);
        void peak (double freq, double q, double dB, double sampleRate);
        float process (float input);
        void clear();
    };
    struct DelayLine
    {
        std::vector<float> memory;
        size_t writeIndex = 0;
        void resize (size_t samples);
        float readCubic (double samples) const;
        void write (float sample);
        void clear();
    };
    struct PitchShift
    {
        DelayLine left, right;
        double phase = 0;
        void prepare();
        void reset();
        std::pair<float, float> process (float l, float r, int semitones);
    };
    double fs = 44100.0, hostFs = 44100.0, tempo = 120.0;
    int oversamplingFactor = 1;
    // Routing is selected when the host/step-lock values arrive, not while
    // synthesising every 2x/4x sub-sample. No LFO waveform is evaluated when
    // no destination has depth. Existing saved parameter IDs are unchanged.
    bool lfoHasDestination = false;
    double glideFreeCoeff = 1.0, glideLegatoCoeff = 1.0;
    double lpgSpeedCoeff = 0.03, accentDecayCoeff = 0.0;
    double staticPitchRatio = 1.0;
    void updateRateCoefficients() noexcept;
    void updateLpgCoefficient() noexcept;
    Values p {};
    LfoValues lfoParameters {};
    Osc2Values osc2Parameters {};
    struct LfoState
    {
        double phase = 0.0, shotPosition = 0.0;
        double previousPhase = 0.0, held = 0.0, smooth = 0.0, delayEnvelope = 0.0;
        std::uint32_t rng = 0x9e3779b9u;
        bool shotDone = false;
    } lfo;
    double nextLfoSample() noexcept;
    double freq = 0, targetFreq = 0, phase = 0, pwmPhase = 0;
    double osc2Phase = 0, osc2GainSmooth = 0, osc2DetuneSmooth = 10;
    double envelope = 0, velNorm = 1, accentEnvelope = 0, lpgMemory = 0;
    double stage3 = 0, stage4 = 0;
    int envelopeStage = 3, lastNote = -1;
    bool noteActive = false, legato = false, accent = false;
    double smoothDelay = 0, delayTargetSamples = 0, delayPhase = 0;
    DelayLine delayL, delayR;
    PitchShift pitchShift;
    Biquad airPeakL, airPeakR, outputLpfL, outputLpfR;
    Biquad delayLpL, delayLpR, delayHpL, delayHpR;
    static double polyblep (double t, double dt);
};
}
