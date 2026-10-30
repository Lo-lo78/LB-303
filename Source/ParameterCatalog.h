// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace lb303
{
struct ParameterInfo
{
    juce::String id;
    juce::String name;
    float minimum, maximum, step, initial;
    const char* unit;
    const char* const* choices = nullptr;
    int choiceCount = 0;
    bool logarithmic = false;
};

constexpr size_t synthParameterCount = 35; // original JSFX DSP params per engine
constexpr size_t voiceParameterCount = synthParameterCount + 2; // plus output Level/Pan
constexpr size_t basslineCount = 2;
constexpr size_t audioParameterCount = voiceParameterCount * basslineCount;
constexpr size_t sequencerParameterCount = 7;
constexpr size_t legacyParameterCount = audioParameterCount + sequencerParameterCount;
// Append all new FX IDs; existing automation and 1-based sequencer lock slots stay stable.
constexpr size_t fxParameterCount = 35; // append FX Order after legacy 34
constexpr size_t sequencerPerformancePerLane = 10;
constexpr size_t sequencerPerformanceCount = basslineCount * sequencerPerformancePerLane;
constexpr size_t sequencerPerformanceBase = legacyParameterCount + fxParameterCount;
constexpr size_t sequencerDisplayParameterCount = sequencerParameterCount + sequencerPerformanceCount;
// Append the two LFOs AFTER every previously published VST3 parameter ID.
// Older voice local lock IDs 1..37 remain untouched; new LFO lock IDs start at 38.
constexpr size_t lfoParameterCount = 16;
constexpr size_t lfoParameterBase = sequencerPerformanceBase + sequencerPerformanceCount;
constexpr size_t separateOutputBase = lfoParameterBase + basslineCount * lfoParameterCount;
constexpr size_t separateOutputIndex (size_t lane) { return separateOutputBase + lane; }
// Additional oscillators are append-only host parameters. Do not renumber any
// of the historic Bassline, Sequencer, FX, LFO or Separate Output IDs.
constexpr size_t osc2ParameterCount = 2;
constexpr size_t osc2ParameterBase = separateOutputBase + basslineCount;
constexpr size_t osc2ParameterIndex (size_t lane, size_t control)
{ return osc2ParameterBase + lane * osc2ParameterCount + control; }
namespace osc2 { enum Control : size_t { volume, detune }; }
// Append-only host parameter. Values: 0 = 1x, 1 = 2x, 2 = 4x.
constexpr size_t oversamplingIndex = osc2ParameterBase + basslineCount * osc2ParameterCount;
// Host parameter indices MUST remain append-only. Never grow fxParameterCount:
// that would move all sequencer, LFO, OSC2 and oversampling host indices.
constexpr size_t reverbBassDecayIndex = oversamplingIndex + 1;
constexpr size_t reverbBassXoverIndex = reverbBassDecayIndex + 1;
// Spatial reverb controls append after TEST2.4.43 (no old VST3 IDs move).
constexpr size_t reverbMotionModeIndex = reverbBassXoverIndex + 1;
constexpr size_t reverbMotionDepthIndex = reverbMotionModeIndex + 1;
constexpr size_t reverbMotionRateIndex = reverbMotionDepthIndex + 1;
// Spatial excitation of the existing shared FDN: four new append-only host IDs.
constexpr size_t reverbSourcePan1Index = reverbMotionRateIndex + 1;
constexpr size_t reverbSourcePan2Index = reverbSourcePan1Index + 1;
constexpr size_t reverbLfoPan1Index = reverbSourcePan2Index + 1;
constexpr size_t reverbLfoPan2Index = reverbLfoPan1Index + 1;
// Separate early-reflection level: append-only, all older host IDs remain stable.
constexpr size_t reverbEarlyLevelIndex = reverbLfoPan2Index + 1;
constexpr size_t parameterCount = reverbEarlyLevelIndex + 1;
constexpr size_t lfoParameterIndex (size_t lane, size_t control)
{ return lfoParameterBase + lane * lfoParameterCount + control; }
namespace lfo
{
enum Control : size_t { mode, rate, bpmSync, wave, squarePwm, delay, phase,
                        upperSquash, lowerSquash, smooth, oneShot, oneShotPercent,
                        pitchDepth, volumeDepth, filterDepth, panDepth };
}
// Legacy lock slots 1..37 are synth/output; 38..53 are LFO.
// Append oscillator-2 slots 54..55 without changing any older lock IDs.
constexpr size_t osc2LocalBase = voiceParameterCount + lfoParameterCount;
constexpr size_t voiceLocalParameterCount = osc2LocalBase + osc2ParameterCount;
constexpr size_t voiceParameterGlobalIndex (size_t voice, size_t local)
{ return local < voiceParameterCount ? voice * voiceParameterCount + local
       : local < osc2LocalBase ? lfoParameterIndex (voice, local - voiceParameterCount)
       : osc2ParameterIndex (voice, local - osc2LocalBase); }
constexpr size_t seqPerformanceIndex (size_t lane, size_t control)
{ return sequencerPerformanceBase + lane * sequencerPerformancePerLane + control; }
namespace seqPerformance
{
enum Control : size_t { bpmDivision, shuffle, legato, skipProbability, noteRandom,
                         velocityRandom, lengthRandom, repeatRandom, octaveShift, semitoneShift };
}
constexpr size_t fxParameterIndex (size_t local) { return legacyParameterCount + local; }
constexpr size_t voiceVisibleParameterCount = 22 + lfoParameterCount + osc2ParameterCount; // old Level is hidden; Volume is first // voice parameters shown in parameter-lock picker
constexpr size_t voiceGridParameterCount = voiceVisibleParameterCount + 2; // MIDI channel + separate stereo output
constexpr int voiceMidiDisplayIndex = 5; // Volume, Waveform, Osc 2 Volume, Osc 2 Detune, Pan, MIDI, ...
constexpr size_t classicDelayParameterCount = 14;
constexpr size_t fxDisplayParameterCount = fxParameterCount + classicDelayParameterCount + 1 + 2 + 3 + 4 + 1; // append-only reverb bass and spatial host params
constexpr size_t fxContextIndex = basslineCount + 1;
size_t fxDisplayGlobalIndex (size_t displayIndex) noexcept;
namespace fx
{
enum Control : size_t {
    delayType, delaySend1, delaySend2,
    dualTime, dualSync, dualSpeed1, dualSpeed2, dualGlide,
    dualFeedback1, dualFeedback2, dualTone1, dualTone2, dualSpread, dualDrive,
    chorusOn, chorusLevel, chorusRate, chorusWidth, chorusSend1, chorusSend2,
    distortionType, distortionAmount, distortionHarmonics,
    reverbType, reverbSend1, reverbSend2, reverbPredelay, reverbDecay,
    reverbDamping, reverbWidth, reverbLevel,
    // Append-only controls: do not reindex the existing 31 FX parameters.
    distortion2Type, distortion2Amount, distortion2Harmonics,
    effectOrder // append only; 0 Delay > Reverb, 1 Reverb > Delay
};
// Extended DSP control slots, NOT contiguous VST3 host parameter indices.
// Host indices are the append-only reverb*Index constants above.
constexpr size_t reverbBassDecay = fxParameterCount;
constexpr size_t reverbBassXover = fxParameterCount + 1;
constexpr size_t reverbMotionMode = fxParameterCount + 2;
constexpr size_t reverbMotionDepth = fxParameterCount + 3;
constexpr size_t reverbMotionRate = fxParameterCount + 4;
constexpr size_t reverbSourcePan1 = fxParameterCount + 5;
constexpr size_t reverbSourcePan2 = fxParameterCount + 6;
constexpr size_t reverbLfoPan1 = fxParameterCount + 7;
constexpr size_t reverbLfoPan2 = fxParameterCount + 8;
constexpr size_t reverbEarlyLevel = fxParameterCount + 9;
}
constexpr size_t sequencerParameterIndex (size_t local) { return audioParameterCount + local; }
constexpr size_t pattern1Index = audioParameterCount;
constexpr size_t pattern2Index = audioParameterCount + 1;
constexpr size_t sequencer1EnabledIndex = audioParameterCount + 2;
constexpr size_t sequencer2EnabledIndex = audioParameterCount + 3;
constexpr size_t editTrackIndex = audioParameterCount + 4;
// Append to preserve all existing parameter indexes and VST IDs.
constexpr size_t midiChannel1Index = audioParameterCount + 5;
constexpr size_t midiChannel2Index = audioParameterCount + 6;
// Only the accessible GUI order changes, never engine/local indices or lock IDs.
size_t displayToLocal (size_t context, size_t displayIndex) noexcept;
size_t localToDisplay (size_t context, size_t localIndex) noexcept;
constexpr size_t sequencerContextIndex = basslineCount;
constexpr size_t outputLevelOffset = synthParameterCount;
constexpr size_t outputPanOffset = synthParameterCount + 1;
constexpr size_t parameterIndex (size_t bassline, size_t local)
{
    return bassline * voiceParameterCount + local;
}
const std::array<ParameterInfo, parameterCount>& catalog();
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
juce::String parameterText (size_t index, float plainValue);
}
