// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SongArrangement.h"
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
    const SequencerState& bank, double beats) noexcept
{
    Position result;
    if (column.empty() || ! std::isfinite (beats) || beats < 0.0) return result;
    // Walk the song's pattern references, not its steps. Cache-free and fully
    // deterministic across block sizes, host seeks and repeat playback.
    std::array<double, SequencerState::maximumSequences> durations {};
    for (int p = 0; p < SequencerState::maximumSequences; ++p)
    {
        const auto c = bank.getConfig (p);
        const auto stepBeats = 1.0 / juce::jlimit (0.03125, 64.0,
            static_cast<double> (c.bpmDivision));
        durations[static_cast<size_t> (p)] =
            (juce::jlimit (c.startStep, 128, c.endStep) - c.startStep + 1) * stepBeats;
    }
    double cycle = 0.0;
    for (int p : column) cycle += durations[static_cast<size_t> (p)];
    if (cycle <= 0.0) return result;
    const double startCycle = repeat ? std::floor (beats / cycle) * cycle : 0.0;
    if (! repeat && beats >= cycle) return result;
    double cursor = startCycle;
    for (size_t index = 0; index < column.size(); ++index)
    {
        const int p = column[index];
        const double duration = durations[static_cast<size_t> (p)];
        if (beats < cursor + duration || (index + 1 == column.size() && repeat))
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
    return result;
}
}
