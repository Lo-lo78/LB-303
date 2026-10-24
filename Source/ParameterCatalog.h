// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace lb303
{
struct ParameterInfo
{
    const char* id;
    const char* name;
    float minimum, maximum, step, initial;
    const char* unit;
    const char* const* choices = nullptr;
    int choiceCount = 0;
    bool logarithmic = false;
};

constexpr size_t parameterCount = 35;
const std::array<ParameterInfo, parameterCount>& catalog();
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
juce::String parameterText (size_t index, float plainValue);
}
