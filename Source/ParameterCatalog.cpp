// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ParameterCatalog.h"
#include <cmath>

namespace lb303
{
namespace
{
constexpr const char* waveChoices[] = { "Saw", "Square" };
constexpr const char* portaChoices[] = { "Always", "Legato" };
constexpr const char* transientChoices[] = { "Loose", "Neutral", "Tight" };
constexpr const char* syncChoices[] = { "Off", "1/16", "1/12T", "1/8", "1/6T", "1/4", "1/3T", "1/2", "1/1" };
constexpr const char* delayModeChoices[] = { "Normal", "Ping-Pong" };
constexpr const char* responseChoices[] = { "Gate", "Lowpass Gate" };
}

const std::array<ParameterInfo, parameterCount>& catalog()
{
    static const std::array<ParameterInfo, parameterCount> values {{
        { "slider001", "Waveform", 0, 1, 1, 1, "", waveChoices, 2 },
        { "slider002", "Attack", 0, 0.05f, 0.001f, 0, "s" },
        { "slider003", "Decay", 0, 0.2f, 0.001f, 0.1f, "s" },
        { "slider004", "Sustain (Filter only)", 0, 0.99f, 0.01f, 0.14f, "" },
        { "slider005", "Release", 0, 0.2f, 0.001f, 0.03f, "s" },
        { "slider006", "Portamento Time", 0, 100, 0.5f, 10, "" },
        { "slider007", "Portamento Mode", 0, 1, 1, 1, "", portaChoices, 2 },
        { "slider008", "Volume", 0, 1, 0.01f, 0.8f, "" },
        { "slider009", "Cutoff", 0, 1, 0.01f, 0.6f, "" },
        { "slider010", "Resonance", 0, 0.1f, 0.001f, 0.1f, "" },
        { "slider011", "Filter Env Depth", 0, 1, 0.01f, 0.3f, "" },
        { "slider012", "PWM Rate", 0, 128, 0.125f, 1, "" },
        { "slider013", "PWM Depth", 0, 0.5f, 0.001f, 0.1f, "" },
        { "slider014", "Soft Clip Drive", 0, 4, 0.01f, 2, "" },
        { "slider015", "Master Tone", -48, 48, 0.1f, 0, "st" },
        { "slider016", "Accent Threshold", 80, 120, 1, 120, "" },
        { "slider017", "Accent Filter Amount", 0, 0.7f, 0.01f, 0.5f, "" },
        { "slider018", "Velocity", 0, 127, 1, 0, "" },
        { "slider019", "Transient Response", 0, 2, 1, 1, "", transientChoices, 3 },
        { "slider020", "Delay Time", 1, 10000, 1, 200, "ms" },
        { "slider021", "Delay Tempo Sync", 0, 8, 1, 0, "", syncChoices, 9 },
        { "slider022", "Delay Feedback", 0, 100, 1, 10, "%" },
        { "slider023", "Delay Mode", 0, 1, 1, 1, "", delayModeChoices, 2 },
        { "slider024", "Delay Spread", -100, 100, 1, 0, "%" },
        { "slider025", "Delay Low Pass", 20, 20000, 1, 16000, "Hz", nullptr, 0, true },
        { "slider026", "Delay Low Pass Q", 0.707f, 4, 0.01f, 0.707f, "", nullptr, 0, true },
        { "slider027", "Delay High Pass", 20, 20000, 1, 60, "Hz", nullptr, 0, true },
        { "slider028", "Delay High Pass Q", 0.707f, 4, 0.01f, 0.707f, "", nullptr, 0, true },
        { "slider029", "Delay Modulation Amp", 0, 100, 1, 0, "" },
        { "slider030", "Delay Modulation Freq", 1, 8, 1, 5, "Hz" },
        { "slider031", "Delay Transpose", -12, 12, 1, 0, "st" },
        { "slider032", "Delay Dry", 0, 100, 1, 100, "%" },
        { "slider033", "Delay Wet", 0, 100, 1, 10, "%" },
        { "slider034", "Response Mode", 0, 1, 1, 0, "", responseChoices, 2 },
        { "slider035", "Gate Velocity Filter", 0, 127, 1, 0, "" }
    }};
    return values;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    for (const auto& p : catalog())
    {
        if (p.choiceCount)
        {
            juce::StringArray options;
            for (int i = 0; i < p.choiceCount; ++i)
                options.add (p.choices[i]);
            result.add (std::make_unique<juce::AudioParameterChoice> (
                juce::ParameterID { p.id, 1 }, p.name, options, static_cast<int> (p.initial)));
        }
        else
        {
            juce::NormalisableRange<float> range (p.minimum, p.maximum, p.step);
            if (p.logarithmic)
                range.setSkewForCentre (p.minimum == 20.0f ? 1000.0f : 1.5f);
            result.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { p.id, 1 }, p.name, range, p.initial));
        }
    }
    return result;
}

juce::String parameterText (size_t index, float plainValue)
{
    const auto& p = catalog().at (index);
    if (p.choiceCount != 0)
        return p.choices[juce::jlimit (0, p.choiceCount - 1, juce::roundToInt (plainValue))];
    int places = p.step >= 1.0f ? 0 : p.step >= 0.1f ? 1 : p.step >= 0.01f ? 2 : 3;
    juce::String result (plainValue, places);
    if (p.unit[0] != '\0')
        result += juce::String (" ") + p.unit;
    return result;
}
}
