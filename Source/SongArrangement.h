// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "SequencerState.h"
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace lb303
{
// An entry refers to an existing pattern; no steps are copied. The two
// immutable song columns can have arbitrary and different lengths. Editing
// publishes a whole snapshot; the realtime player never iterates mutable data.
class SongArrangement
{
public:
    using Column = std::vector<int>; // 0..15, independent for each bassline
    using Snapshot = std::shared_ptr<const Column>;
    struct Position
    {
        int pattern = 0;
        int row = -1;
        double startBeat = 0.0;
        double endBeat = 0.0;
        bool active = false;
    };
    SongArrangement();
    Snapshot column (int lane) const noexcept;
    void replace (int lane, Column column);
    void restoreSnapshot (int lane, Snapshot snapshot) noexcept;
    void reset();
    bool loop (int lane) const noexcept;
    void setLoop (int lane, bool enabled) noexcept;
    bool songMode() const noexcept { return mode.load (std::memory_order_acquire); }
    void setSongMode (bool enabled) noexcept
    { mode.store (enabled, std::memory_order_release); revision.fetch_add (1, std::memory_order_relaxed); }
    std::uint64_t getRevision() const noexcept { return revision.load (std::memory_order_relaxed); }
    std::uint64_t getLoadEpoch() const noexcept { return loadEpoch.load (std::memory_order_relaxed); }
    juce::String serialise() const;
    bool restore (const juce::String&);
    static Position locate (const Column&, bool loop, const SequencerState&, double beats) noexcept;
private:
    std::array<Snapshot, 2> columns;
    std::array<std::atomic<bool>, 2> looping {};
    std::atomic<bool> mode { false };
    std::atomic<std::uint64_t> revision { 1 };
    std::atomic<std::uint64_t> loadEpoch { 1 };
};
}
