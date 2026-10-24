// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "SequencerState.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace lb303
{
class SequencerPlayback
{
public:
    // The two lanes have independent patterns, lengths and timing configurations.
    // The host PPQ is the shared clock; changing a pattern never writes the other lane.
    void reset() noexcept;
    void generate (juce::MidiBuffer& output, const std::array<SequencerState, 2>& banks,
                   const std::array<int, 2>& patterns, const std::array<bool, 2>& enabled,
                   bool playing, double ppq, double bpm, double sampleRate, int samples);
private:
    std::array<int, 2> oldPattern { -1, -1 };
    std::array<bool, 2> oldEnabled { false, false };
    std::array<int, 2> lastNote { -1, -1 };
    double previousPpq = -1.0;
    bool previousPlaying = false;
};
}
