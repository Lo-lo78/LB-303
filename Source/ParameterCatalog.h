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
constexpr size_t fxParameterCount = 31;
constexpr size_t parameterCount = legacyParameterCount + fxParameterCount;
constexpr size_t fxParameterIndex (size_t local) { return legacyParameterCount + local; }
constexpr size_t voiceVisibleParameterCount = 23; // no duplicated old delays in voice grids
constexpr size_t classicDelayParameterCount = 14;
constexpr size_t fxDisplayParameterCount = fxParameterCount + classicDelayParameterCount;
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
    reverbDamping, reverbWidth, reverbLevel
};
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
