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
    // Non-MIDI step boundary events for sample-accurate Parameter Locks.
    // Fixed capacity: no audio-thread allocation for the lock event queue.
    struct StepEvent
    {
        int sample = 0;
        int lane = 0;
        int pattern = 0;
        int step = 0;
    };
    static constexpr int maximumStepEvents = 2048;
    // The two lanes have independent patterns, lengths and timing configurations.
    // The host PPQ is the shared clock; changing a pattern never writes the other lane.
    void reset() noexcept;
    void generate (juce::MidiBuffer& output, const std::array<SequencerState, 2>& banks,
                   const std::array<int, 2>& patterns, const std::array<std::int64_t, 2>& songEntryIds,
                   const std::array<bool, 2>& enabled,
                   const std::array<double, 2>& triggerOrigins,
                   const std::array<int, 2>& keyboardTransposes,
                   const std::array<SequencerConfig, 2>& performanceControls,
                   bool playing, double ppq, double bpm, double sampleRate, int samples,
                   std::array<StepEvent, maximumStepEvents>& stepEvents,
                   int& stepEventCount, int blockOffset);
private:
    std::array<int, 2> oldPattern { -1, -1 };
    std::array<std::int64_t, 2> oldSongEntryIds {{ -3, -3 }};
    std::array<bool, 2> oldEnabled { false, false };
    std::array<int, 2> lastNote { -1, -1 };
    // Remember source-note identity so Note Off still matches the original
    // output pitch if the physical keyboard transpose changes mid-step.
    std::array<int, 2> lastSourceNote { -1, -1 };
    double previousPpq = -1.0;
    bool previousPlaying = false;
};
}
