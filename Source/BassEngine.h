// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "ParameterCatalog.h"
#include <array>
#include <vector>
#include <utility>
#include <cstddef>

namespace lb303
{
class BassEngine
{
public:
    using Values = std::array<float, synthParameterCount>;
    void prepare (double sampleRate);
    void reset();
    void setParameters (const Values& values, double bpm);
    void midi (const juce::MidiMessage& event);
    void releaseNote() noexcept; // stop a direct note when switching to sequencer mode
    std::pair<float, float> tick (float inputL, float inputR);

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
    double fs = 44100.0, tempo = 120.0;
    Values p {};
    double freq = 0, targetFreq = 0, phase = 0, pwmPhase = 0;
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
