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
    void setRandomSeed (std::uint32_t seed) noexcept { randomSeed = seed ? seed : 0x51e90001u; }
    std::uint32_t getRandomSeed() const noexcept { return randomSeed; }
    void generate (juce::MidiBuffer& output, const std::array<SequencerState, 2>& banks,
                   const std::array<int, 2>& patterns, const std::array<std::int64_t, 2>& songEntryIds,
                   const std::array<bool, 2>& enabled,
                   const std::array<double, 2>& triggerOrigins,
                   const std::array<int, 2>& keyboardTransposes,
                   bool playing, double ppq, double bpm, double sampleRate, int samples,
                   std::array<StepEvent, maximumStepEvents>& stepEvents,
                   int& stepEventCount, int blockOffset);
private:
    // LJuno-inspired xorshift32, hashed by musical onset and Song occurrence.
    // Never advance RNG based on audio buffer size: song loops change the
    // occurrence key, while all pieces of a split note get identical choices.
    float randomValue (int lane, int pattern, std::int64_t tick,
                       std::int64_t songEntryId, std::uint32_t salt) const noexcept;
    std::uint32_t randomSeed = 0x51e90001u;
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
