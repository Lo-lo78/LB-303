// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "SequencerState.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace lb303::sequencerTiming
{
// One Repeat is one additional sequencer clock pulse, as in LJuno. Never
// subdivide the original step to squeeze all retriggers into its duration.
inline float random (std::uint32_t seed, int lane, int pattern,
                     std::int64_t tick, std::int64_t entry, std::uint32_t salt) noexcept
{
    std::uint32_t x = seed ^ salt ^ (static_cast<std::uint32_t> (tick) * 0x9e3779b9u);
    x ^= static_cast<std::uint32_t> (tick >> 32) * 0x85ebca6bu;
    x ^= static_cast<std::uint32_t> (entry) * 0xc2b2ae35u;
    x ^= static_cast<std::uint32_t> (entry >> 32) * 0x27d4eb2du;
    x ^= static_cast<std::uint32_t> (lane + 1) * 0x165667b1u;
    x ^= static_cast<std::uint32_t> (pattern + 1) * 0x7feb352du;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return static_cast<float> (x & 0x00ffffffu) / 16777216.0f;
}

inline int count (const SequencerStep& step, const SequencerConfig& config,
                  std::uint32_t seed, int lane, int pattern,
                  std::int64_t tick, std::int64_t entry) noexcept
{
    const float depth = juce::jlimit (0.0f, 1.0f, config.repeatRandomDepth);
    if (depth <= 0.0f) return juce::jlimit (1, 16, step.repeat);
    // Keep LJuno's 1..16 (scaled by depth) random choice, but choose once
    // per *step*, not once per audio buffer or per MIDI retrigger.
    return juce::jlimit (1, 16, 1 + static_cast<int> (
        random (seed, lane, pattern, tick, entry, 0x1b873593u) * 16.0f * depth));
}

inline double interval (double stepBeats, float shuffle, std::int64_t pulse) noexcept
{
    // LJuno alternates the actual CLOCK interval on every retrigger;
    // shuffle therefore applies to repeated notes, not only pattern steps.
    const double swing = 0.5 * juce::jlimit (0.0, 1.0, static_cast<double> (shuffle));
    return stepBeats * ((pulse & 1) == 0 ? (1.0 + swing) : (1.0 - swing));
}

inline double launchBeats (const SequencerConfig& config, double bpm) noexcept
{
    const double stepBeats = 1.0 / juce::jlimit (0.03125, 64.0,
                                       static_cast<double> (config.bpmDivision));
    return (juce::jlimit (1, 128, config.launchStep) - 1) * stepBeats
        + static_cast<double> (config.launchOffsetMs) * bpm / 60000.0;
}

inline double duration (const SequencerState& bank, int pattern,
                       std::uint32_t seed, int lane, std::int64_t entry,
                       std::int64_t cycle = 0, double bpm = 120.0) noexcept
{
    const auto config = bank.getConfig (pattern);
    const int first = juce::jlimit (1, 128, config.startStep) - 1;
    const int finish = juce::jlimit (first + 1, 128, config.endStep);
    const int length = finish - first;
    const double stepBeats = 1.0 / juce::jlimit (0.03125, 64.0,
                                    static_cast<double> (config.bpmDivision));
    double result = 0.0;
    std::int64_t pulse = 0;
    for (int index = 0; index < length; ++index)
    {
        const auto tick = cycle * static_cast<std::int64_t> (length) + index;
        const int repeats = count (bank.getStep (pattern, first + index), config,
                                   seed, lane, pattern, tick, entry);
        for (int r = 0; r < repeats; ++r)
            result += interval (stepBeats, config.shuffle, pulse++);
    }
    // The Launch Step delay applies once per Song entry. Pattern-mode loops
    // instead apply it once at the start of playback, outside this duration.
    return result;
}
}
