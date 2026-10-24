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
constexpr size_t parameterCount = voiceParameterCount * basslineCount;
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
