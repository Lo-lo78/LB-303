// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SongArrangement.h"
#include "SequencerTiming.h"
#include <cmath>
#include <limits>

namespace lb303
{
SongArrangement::SongArrangement() { reset(); }
SongArrangement::Snapshot SongArrangement::column (int lane) const noexcept
{
    return std::atomic_load_explicit (&columns[static_cast<size_t> (juce::jlimit (0, 1, lane))],
                                       std::memory_order_acquire);
}
void SongArrangement::restoreSnapshot (int lane, Snapshot snapshot) noexcept
{
    if (snapshot == nullptr) snapshot = std::make_shared<const Column>();
    std::atomic_store_explicit (&columns[static_cast<size_t> (juce::jlimit (0, 1, lane))],
                                std::move (snapshot), std::memory_order_release);
    revision.fetch_add (1, std::memory_order_relaxed);
}
void SongArrangement::replace (int lane, Column value)
{
    for (auto& p : value) p = juce::jlimit (0, 15, p);
    restoreSnapshot (lane, std::make_shared<const Column> (std::move (value)));
}
void SongArrangement::reset()
{
    replace (0, {});
    replace (1, {});
    looping[0].store (false, std::memory_order_release);
    looping[1].store (false, std::memory_order_release);
    setSongMode (false);
    loadEpoch.fetch_add (1, std::memory_order_relaxed);
}
bool SongArrangement::loop (int lane) const noexcept
{
    return looping[static_cast<size_t> (juce::jlimit (0, 1, lane))].load (std::memory_order_acquire);
}
void SongArrangement::setLoop (int lane, bool enabled) noexcept
{
    looping[static_cast<size_t> (juce::jlimit (0, 1, lane))].store (enabled, std::memory_order_release);
    revision.fetch_add (1, std::memory_order_relaxed);
}
juce::String SongArrangement::serialise() const
{
    juce::MemoryOutputStream out;
    out.writeInt (0x4c425331); // LBS1
    out.writeInt (1);
    out.writeByte (songMode() ? 1 : 0);
    for (int lane = 0; lane < 2; ++lane)
    {
        out.writeByte (loop (lane) ? 1 : 0);
        const auto snapshot = column (lane);
        out.writeInt (static_cast<int> (snapshot->size()));
        for (int p : *snapshot) out.writeByte (static_cast<char> (p));
    }
    return juce::Base64::toBase64 (out.getData(), out.getDataSize());
}
bool SongArrangement::restore (const juce::String& text)
{
    juce::MemoryOutputStream decoded;
    if (! juce::Base64::convertFromBase64 (decoded, text)) return false;
    juce::MemoryInputStream input (decoded.getData(), decoded.getDataSize(), false);
    if (input.getNumBytesRemaining() < 10 || input.readInt() != 0x4c425331
        || input.readInt() != 1) return false;
    const bool newMode = input.readByte() != 0;
    std::array<bool, 2> newLoops {};
    std::array<Column, 2> newColumns;
    for (int lane = 0; lane < 2; ++lane)
    {
        if (input.getNumBytesRemaining() < 5) return false;
        newLoops[lane] = input.readByte() != 0;
        const int n = input.readInt();
        if (n < 0 || static_cast<juce::int64> (n) > input.getNumBytesRemaining()) return false;
        // Bound untrusted preset size by the actual file size, not an arbitrary
        // fixed maximum number of song entries.
        auto& v = newColumns[lane];
        v.reserve (static_cast<size_t> (n));
        for (int i = 0; i < n; ++i)
        {
            const int p = input.readByte();
            if (p < 0 || p >= SequencerState::maximumSequences) return false;
            v.push_back (p);
        }
    }
    for (int lane = 0; lane < 2; ++lane)
    {
        replace (lane, std::move (newColumns[lane]));
        setLoop (lane, newLoops[lane]);
    }
    setSongMode (newMode);
    loadEpoch.fetch_add (1, std::memory_order_relaxed);
    return true;
}
SongArrangement::Position SongArrangement::locate (const Column& column, bool repeat,
    const SequencerState& bank, double beats, int lane, std::uint32_t seed, double bpm) noexcept
{
    Position result;
    if (column.empty() || ! std::isfinite (beats) || beats < 0.0) return result;
    // Each Repeat consumes one complete BPM Division clock pulse. The Song
    // duration must include every one, plus the selected pattern's launch delay.
    // Repeat Random uses the SAME deterministic musical event hash as the
    // playback engine so variable-length Song entries are never truncated.
    std::array<double, SequencerState::maximumSequences> fixedDurations {};
    bool variable = false;
    for (int p : column)
    {
        const auto c = bank.getConfig (p);
        variable |= c.repeatRandomDepth > 0.0f;
        if (c.repeatRandomDepth <= 0.0f)
            fixedDurations[static_cast<size_t> (p)] = sequencerTiming::duration (
                bank, p, seed, lane, -1) + sequencerTiming::launchBeats (c, bpm);
    }
    // Fast seek for patterns with fixed Repeat counts (the common case):
    // even very long looping playback is O(number of Song rows), not O(loops).
    double cursor = 0.0;
    if (repeat && ! variable)
    {
        double cycle = 0.0;
        for (int p : column) cycle += fixedDurations[static_cast<size_t> (p)];
        if (cycle <= 0.0) return result;
        cursor = std::floor (beats / cycle) * cycle;
    }
    // When Repeat Random is active, the musical duration of each entry can
    // vary with the absolute Song occurrence. Walk the true durations (never
    // infer them from buffer length or a fixed pattern-step count).
    for (std::int64_t songLoop = 0; songLoop < 65536; ++songLoop)
    {
        for (size_t index = 0; index < column.size(); ++index)
        {
            const int p = column[index];
            const auto c = bank.getConfig (p);
            const auto entry = static_cast<std::int64_t> (std::llround (cursor * 1000000.0));
            const double duration = c.repeatRandomDepth <= 0.0f
                ? fixedDurations[static_cast<size_t> (p)]
                : sequencerTiming::duration (bank, p, seed, lane, entry, 0, bpm)
                    + sequencerTiming::launchBeats (c, bpm);
            if (beats < cursor + duration)
            {
                result.pattern = p;
                result.row = static_cast<int> (index);
                result.startBeat = cursor;
                result.endBeat = cursor + duration;
                result.active = true;
                return result;
            }
            cursor += duration;
        }
        if (! repeat || (! variable && songLoop >= 1)) return result;
        // For a fixed-length song the floor-based seek above normally finds
        // the active cycle immediately. Checking one adjacent cycle avoids
        // a possible silent sample at floating-point cycle boundaries.
    }
    return result;
}
}
