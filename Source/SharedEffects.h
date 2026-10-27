// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "BassEngine.h"
#include <array>
#include <vector>
#include <utility>

namespace lb303
{
// ONE stereo FX rack shared by the two independent monophonic voices.
// All storage is allocated in prepare() and the audio callback allocates nothing.
class SharedEffects final
{
public:
    using Controls = std::array<float, fxParameterCount>;
    using ClassicParameters = BassEngine::Values;
    void prepare (double newSampleRate);
    void clearTails() noexcept; // no allocation: reset FX after Alt+I
    void setParameters (const Controls&, const ClassicParameters&, double newTempo);
    float dryLevel() const noexcept { return classicDryLevel; }
    struct Output
    {
        std::pair<float, float> wet; // Shared chorus, delay, reverb: Master 1/2 + isolated FX bus 7/8
        std::array<std::pair<float, float>, basslineCount> dry;
        // Post-distortion voice, no FX tails, BEFORE global FX Dry Level.
    };
    Output process (float b1l, float b1r, float b2l, float b2r) noexcept;
    void setInsertOversamplingFactor (int factor) noexcept;
    std::pair<float, float> processDistortionInsert (float l, float r, size_t voice) noexcept;
    Output processPostDistortion (float b1l, float b1r, float b2l, float b2r) noexcept;
private:
    struct Ring
    {
        std::vector<float> data;
        int pos = 0;
        void initialise (int size) { data.assign (static_cast<size_t> (size), 0.0f); pos = 0; }
        float read (double samplesAgo) const noexcept;
        void push (float x) noexcept
        {
            data[static_cast<size_t> (pos)] = x;
            if (++pos == static_cast<int> (data.size())) pos = 0;
        }
    };
    struct DistSide { float previous = 0, prevEven = 0, evenDc = 0; };
    static float fxShape (int mode, float x) noexcept;
    std::pair<float, float> distort (float, float, size_t voice) noexcept;
    std::pair<float, float> chorus (float, float, size_t voice) noexcept;
    std::pair<float, float> dualDelay (float, float) noexcept;
    std::pair<float, float> reverb (float, float) noexcept;
    float toneFeedback (float x, float tone, size_t lane) noexcept;
    float saturateTape (float) const noexcept;
    static float bounded (float v) noexcept;

    BassEngine classic; // the original JSFX delay engine, instantiated ONCE
    Controls c {};
    double fs = 44100.0, bpm = 120.0;
    // Smoothed fractional read position for the shared reverb pre-delay.
    // Parameter changes must not jump directly to another point in the ring.
    double reverbPredelaySamples = 0.0;
    double reverbPredelaySmoothCoeff = 0.0;
    bool reverbPredelayReady = false;
    float classicDryGain = 1.0f;
    float classicDryLevel = 1.0f;
    int lastDelayMode = 0;
    std::array<Ring, basslineCount> chorusL, chorusR;
    Ring tapeL, tapeR, preL, preR;
    std::array<Ring, 8> verb {};
    std::array<float, 8> verbDamped {};
    std::array<float, 8> verbTimes {};
    int verbIndex = 0;
    float chorusRateSmoothed = 0, chorusHpCoefficient = 0, chorusTail = 0;
    std::array<float, basslineCount> chorusHpXL {}, chorusHpYL {},
                                       chorusHpXR {}, chorusHpYR {};
    double chorusPhase = 0;
    struct DistortionState
    {
        float mix = 0, gain = 1, dryPower = 0, processedPower = 0;
        DistSide left, right;
    };
    // Dedicated state per bassline: no shared DC filters, envelopes or gain smoothing.
    std::array<DistortionState, basslineCount> distortions {};
    int insertOversamplingFactor = 1;
    float distDcCoeff = 0, distPowerCoeff = 0, distGainCoeff = 0, distMixCoeff = 0;
    std::array<float, 2> tapeTime {}, tapeLow {}, tapeHighX {}, tapeHighY {};
    std::array<float, 2> tapeWetX {}, tapeWetY {};
    std::array<float, 4> tapePan {{ 0.980785f, 0.19509f, 0.19509f, 0.980785f }};
};
}
