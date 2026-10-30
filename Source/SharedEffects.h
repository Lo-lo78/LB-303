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
    using Controls = std::array<float, fxParameterCount + 9>; // 35 historic + 2 bass + 3 motion + 4 source-position controls
    using ClassicParameters = BassEngine::Values;
    using LfoSamples = std::array<float, basslineCount>;
    void prepare (double newSampleRate);
    void clearTails() noexcept; // no allocation: reset FX after Alt+I
    void setParameters (const Controls&, const ClassicParameters&, double newTempo);
    float dryLevel() const noexcept { return classicDryLevel; }
    bool wetEffectsEnabled() const noexcept { return enabledSharedWetFx; }
    double wetTailGuardSeconds() const noexcept { return wetGuardSeconds; }
    bool distortionActive (size_t voice) const noexcept { return enabledDistortion[voice]; }
    bool distortionQuiescent (size_t voice) const noexcept;

    struct Output
    {
        std::pair<float, float> wet; // Shared chorus, delay, reverb: Master 1/2 + isolated FX bus 7/8
        std::array<std::pair<float, float>, basslineCount> dry;
        // Post-distortion voice, no FX tails, BEFORE global FX Dry Level.
    };
    Output process (float b1l, float b1r, float b2l, float b2r,
                    const LfoSamples& = {}) noexcept;
    void setInsertOversamplingFactor (int factor) noexcept;
    std::pair<float, float> processDistortionInsert (float l, float r, size_t voice) noexcept;
    Output processPostDistortion (float b1l, float b1r, float b2l, float b2r,
                                  const LfoSamples& = {}) noexcept;
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
    std::pair<float, float> reverb (float, float,
         const std::array<std::pair<float, float>, basslineCount>& voiceSends,
         const LfoSamples&) noexcept;
    float toneFeedback (float x, float tone, size_t lane) noexcept;
    float saturateTape (float) const noexcept;
    static float bounded (float v) noexcept;

    BassEngine classic; // the original JSFX delay engine, instantiated ONCE
    Controls c {};
    ClassicParameters lastClassicParameters {};
    bool settingsPrepared = false;
    double wetGuardSeconds = 0.5;
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
    // Per-voice pre-delay taps retain each source independently through the
    // original predelay. Only the FDN INPUT uses spatial position / LFO.
    std::array<Ring, basslineCount> sourcePreL, sourcePreR;
    LfoSamples sourcePositionsSmoothed {};
    float sourcePositionSmoothCoeff = 0.0f;
    std::array<Ring, 8> verb {};
    std::array<float, 8> verbDamped {};
    // In-loop bass RT60 control. The LP state isolates bass energy in each
    // feedback line; the difference gain changes its decay, not the wet EQ.
    std::array<float, 8> verbBassLow {}, verbBassGainDeltaTarget {}, verbBassGainDeltaSmoothed {};
    float verbBassXoverTarget = 0.0f, verbBassXoverSmoothed = 0.0f;
    float verbBassSmoothCoeff = 0.0f;
    // Motion rotates reflection paths (orthogonal cross-coupling) INSIDE the FDN.
    // Smoothed in radians so changing direction/mode cannot jump feedback taps.
    std::array<float, 8> verbMotionAngles {};
    float verbMotionDepthSmoothed = 0.0f, verbMotionRateSmoothed = 0.35f;
    float verbMotionSmoothCoeff = 0.0f;
    double verbMotionPhase = 0.0, verbMotionAge = 0.0;
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
    struct DistortionConfig
    {
        int mode = 0;
        float amount = 0, oddGain = 1, evenGain = 1;
        float drive = 1, bias = 0, centre = 0;
    };
    std::array<DistortionConfig, basslineCount> distortionConfigs {};
    std::array<bool, basslineCount> enabledDistortion {{ false, false }};
    bool enabledSharedWetFx = false;
    int insertOversamplingFactor = 1;
    float distDcCoeff = 0, distPowerCoeff = 0, distGainCoeff = 0, distMixCoeff = 0;
    std::array<float, 2> tapeTime {}, tapeLow {}, tapeHighX {}, tapeHighY {};
    std::array<float, 2> tapeWetX {}, tapeWetY {};
    std::array<float, 4> tapePan {{ 0.980785f, 0.19509f, 0.19509f, 0.980785f }};
};
}
