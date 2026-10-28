// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SequencerPlayback.h"
#include "SequencerTiming.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace lb303
{
float SequencerPlayback::randomValue (int lane, int pattern, std::int64_t tick,
                                      std::int64_t entryId, std::uint32_t salt) const noexcept
{
    return sequencerTiming::random (randomSeed, lane, pattern, tick, entryId, salt);
}
void SequencerPlayback::reset() noexcept
{
    // Restarting or seeking advances the random stream even at identical PPQ.
    randomSeed += 0x9e3779b9u;
    randomSeed ^= randomSeed << 13;
    randomSeed ^= randomSeed >> 17;
    randomSeed ^= randomSeed << 5;
    if (randomSeed == 0) randomSeed = 0x51e90001u;
    oldPattern = { -1, -1 };
    oldSongEntryIds = { -3, -3 };
    oldEnabled = { false, false };
    lastNote = { -1, -1 };
    lastSourceNote = { -1, -1 };
    pendingNoteOffPpq = { -1.0, -1.0 };
    previousPpq = -1.0;
    previousPlaying = false;
}

void SequencerPlayback::generate (juce::MidiBuffer& output,
                                  const std::array<SequencerState, 2>& banks,
                                  const std::array<int, 2>& patterns,
                                  const std::array<std::int64_t, 2>& songEntryIds,
                                  const std::array<bool, 2>& enabled,
                                  const std::array<double, 2>& triggerOrigins,
                                  const std::array<int, 2>& keyboardTransposes,
                                  bool playing, double ppq, double bpm,
                                  double sampleRate, int samples,
                                  std::array<StepEvent, maximumStepEvents>& stepEvents,
                                  int& stepEventCount, int blockOffset)
{
    if (samples <= 0 || ! std::isfinite (ppq) || bpm <= 0.0 || sampleRate <= 0.0)
        return;
    const auto beatsPerSample = bpm / (60.0 * sampleRate);
    const auto endPpq = ppq + samples * beatsPerSample;
    const bool timelineJump = previousPlaying && (previousPpq < 0.0
        || std::abs (ppq - previousPpq) > 0.025); // loop, seek or discontinuity

    // A Song boundary normally falls between sample frames. The processor
    // starts the new row on the first sample at/after that beat; allow the
    // first note/lock to land on that frame instead of silently discarding it.
    // Grace applies ONCE on entry, not to every buffer (no double attacks).
    std::array<bool, 2> newSongEntry {{ false, false }};
    for (int lane = 0; lane < 2; ++lane)
        newSongEntry[static_cast<size_t> (lane)] = songEntryIds[lane] >= 0
            && (oldSongEntryIds[lane] != songEntryIds[lane]
                || oldPattern[lane] != patterns[lane] || ! oldEnabled[lane]);

    // Explicit note-off on stop, disabling a lane or switching its pattern.
    // Does not interfere with the other lane's MIDI channel.
    for (int lane = 0; lane < 2; ++lane)
    {
        if (lastNote[lane] >= 0 && (timelineJump || ! playing || ! enabled[lane]
                                    || ! oldEnabled[lane] || oldPattern[lane] != patterns[lane]
                                    || oldSongEntryIds[lane] != songEntryIds[lane]))
        {
            output.addEvent (juce::MidiMessage::noteOff (lane + 1, lastNote[lane]), 0);
            lastNote[lane] = -1;
            lastSourceNote[lane] = -1;
            pendingNoteOffPpq[lane] = -1.0;
        }
        oldPattern[lane] = patterns[lane];
        oldSongEntryIds[lane] = songEntryIds[lane];
        oldEnabled[lane] = enabled[lane];
    }
    previousPpq = endPpq;
    previousPlaying = playing;
    if (! playing) return;

    for (int lane = 0; lane < 2; ++lane)
    {
        if (! enabled[lane]) continue;
        // Each MIDI-triggered bassline has its own song-time origin. A new key
        // begins its own pattern at step 1, without restarting the other lane.
        const double lanePpq = ppq - triggerOrigins[static_cast<size_t> (lane)];
        const double laneEndPpq = endPpq - triggerOrigins[static_cast<size_t> (lane)];
        const int pattern = juce::jlimit (0, SequencerState::maximumSequences - 1, patterns[lane]);
        const auto& bank = banks[static_cast<size_t> (lane)];
        const auto config = bank.getConfig (pattern); // all ten values per pattern
        const int start = juce::jlimit (1, 128, config.startStep) - 1;
        const int finish = juce::jlimit (start + 1, 128, config.endStep);
        const int patternLength = finish - start;
        const double division = juce::jlimit (0.03125, 64.0, static_cast<double> (config.bpmDivision));
        const double stepBeats = 1.0 / division;
        const double launchDelay = sequencerTiming::launchBeats (config, bpm);
        const std::int64_t entry = songEntryIds[static_cast<size_t> (lane)];
        const bool songEntry = entry >= 0;
        const double entryGrace = newSongEntry[static_cast<size_t> (lane)]
            ? beatsPerSample * 1.01 : 0.0;
        const auto inBlock = [&] (double eventBeat)
        {
            return eventBeat >= lanePpq - entryGrace && eventBeat < laneEndPpq;
        };
        const auto emitNote = [&] (bool on, int note, int velocity, double eventBeat)
        {
            const int sample = juce::jlimit (0, samples - 1,
                static_cast<int> (std::floor ((eventBeat - lanePpq)
                                               / beatsPerSample + 0.5)));
            if (on)
                output.addEvent (juce::MidiMessage::noteOn (
                    lane + 1, note, static_cast<juce::uint8> (velocity)), sample);
            else
                output.addEvent (juce::MidiMessage::noteOff (lane + 1, note), sample);
        };
        // Handle the pending gate deadline before the next onset. Unlike a
        // 102%-long note, Length 101 has NO deadline: it stays held until
        // an actual new note/pause or a normal-length step ends the phrase.
        const auto flushNoteOff = [&] (double untilBeat)
        {
            const auto deadline = pendingNoteOffPpq[lane];
            if (lastNote[lane] < 0 || deadline < 0.0 || deadline > untilBeat
                || deadline >= laneEndPpq) return;
            emitNote (false, lastNote[lane], 0,
                      juce::jmax (lanePpq, deadline));
            lastNote[lane] = -1;
            lastSourceNote[lane] = -1;
            pendingNoteOffPpq[lane] = -1.0;
        };
        const auto releaseAt = [&] (double eventBeat)
        {
            if (lastNote[lane] >= 0)
                emitNote (false, lastNote[lane], 0, eventBeat);
            lastNote[lane] = -1;
            lastSourceNote[lane] = -1;
            pendingNoteOffPpq[lane] = -1.0;
        };
        flushNoteOff (lanePpq);
        // A pattern in the Song plays once; a freely selected pattern repeats.
        // The SONG locator uses this exact same clock/repeat-count law.
        const double baseTime = launchDelay;
        if (laneEndPpq < baseTime - stepBeats * 3.0) continue;

        std::int64_t firstCycle = 0;
        double cycleBeat = baseTime;
        const bool randomRepeats = config.repeatRandomDepth > 0.0f;
        if (! songEntry && ! randomRepeats)
        {
            const double cycleLength = sequencerTiming::duration (
                bank, pattern, randomSeed, lane, entry, 0, bpm);
            if (cycleLength <= 0.0) continue;
            // Include the preceding pattern pass for Note Offs that extend
            // across the cycle boundary. A host seek does not need a scan
            // of thousands of old pattern cycles.
            firstCycle = juce::jmax (static_cast<std::int64_t> (0),
                static_cast<std::int64_t> (std::floor (
                    juce::jmax (0.0, lanePpq - baseTime) / cycleLength)) - 1);
            cycleBeat += static_cast<double> (firstCycle) * cycleLength;
        }
        // With random Repeat, the duration itself can change on every pass.
        // Walk those durations in musical beats; never derive the event count
        // or location from host audio-buffer boundaries.
        for (std::int64_t cycle = firstCycle; cycle < firstCycle + 65536; ++cycle)
        {
            if (songEntry && cycle > 0) break;
            const auto cycleLength = sequencerTiming::duration (
                bank, pattern, randomSeed, lane, entry, cycle, bpm);
            if (cycleLength <= 0.0) break;
            if (cycleBeat - stepBeats > laneEndPpq) break;
            if (cycleBeat + cycleLength + stepBeats * 3.0 < lanePpq)
            {
                cycleBeat += cycleLength;
                continue;
            }
            double pulseBeat = cycleBeat;
            std::int64_t pulseIndex = 0;
            for (int index = 0; index < patternLength; ++index)
            {
                const int stepIndex = start + index;
                const auto step = bank.getStep (pattern, stepIndex);
                const std::int64_t tick = cycle * static_cast<std::int64_t> (patternLength) + index;
                const int repeats = sequencerTiming::count (
                    step, config, randomSeed, lane, pattern, tick, entry);
                const double shift = juce::jlimit (-0.45, 0.45,
                    static_cast<double> (step.shift - 0.5f)) * stepBeats;
                const auto random = [&] (std::uint32_t salt)
                    { return randomValue (lane, pattern, tick, entry, salt); };
                // One parameter-lock event per original step, not per
                // retrigger. Even a paused step can contain Parameter Locks.
                // A negative shift on the FIRST Song pulse would place the
                // attack before the Song row exists. Keep that attack at the
                // row start; later notes retain their programmed shift.
                const double firstOnPpq = songEntry && index == 0
                    ? juce::jmax (cycleBeat, pulseBeat + shift) : pulseBeat + shift;
                if (inBlock (firstOnPpq)
                    && stepEventCount < maximumStepEvents)
                {
                    const int sample = juce::jlimit (0, samples - 1,
                        static_cast<int> (std::floor ((firstOnPpq - lanePpq)
                                                      / beatsPerSample + 0.5)));
                    stepEvents[static_cast<size_t> (stepEventCount++)] =
                        { blockOffset + sample, lane, pattern, stepIndex };
                }
                for (int repeat = 0; repeat < repeats; ++repeat)
                {
                    // Each Repeat consumes a complete tempo-synchronised
                    // clock pulse; Shuffle alternates on every pulse, exactly
                    // as LJuno's repeatCounter/stepInterval scheduler does.
                    const double intervalBeats = sequencerTiming::interval (
                        stepBeats, config.shuffle, pulseIndex++);
                    const double onPpq = songEntry && index == 0 && repeat == 0
                        ? juce::jmax (cycleBeat, pulseBeat + shift) : pulseBeat + shift;
                    pulseBeat += intervalBeats;
                    if (! inBlock (onPpq)) continue;
                    flushNoteOff (onPpq);
                    // A pause, Velocity 0 or a skipped step ends a held tie.
                    if (step.note >= 128 || step.velocity <= 0)
                    {
                        releaseAt (onPpq);
                        continue;
                    }
                    const auto repeatSalt = static_cast<std::uint32_t> (repeat) * 0x85ebca6bu;
                    if (random (0x9e3779b9u ^ repeatSalt)
                        < juce::jlimit (0.0f, 1.0f, config.noteSkipProbability))
                    {
                        releaseAt (onPpq);
                        continue;
                    }
                    const int randomSemitones = juce::roundToInt (
                        (random (0x3972bc53u ^ repeatSalt) * 2 - 1) * 12.0f
                        * juce::jlimit (0.0f, 1.0f, config.noteRandomDepth));
                    const int note = juce::jlimit (0, 127, step.note + randomSemitones + keyboardTransposes[lane] + config.semitoneShift + 12 * config.octaveShift);
                    float gaussian = 0.0f;
                    for (int n = 0; n < 6; ++n)
                        gaussian += random (0x23ab4e21u ^ repeatSalt
                            ^ (static_cast<std::uint32_t> (n) * 0x9e3779b9u)) - 0.5f;
                    gaussian = juce::jlimit (-1.0f, 1.0f, gaussian / 3.0f);
                    const int velocity = juce::jlimit (1, 127, juce::roundToInt (
                        step.velocity + gaussian * 48.0f
                        * juce::jlimit (0.0f, 1.0f, config.velocityRandomDepth)));
                    const bool tie = step.length == 101
                        || (config.legato && step.length >= 100);
                    double lengthFactor = tie ? 1.0
                        : juce::jmax (0.01, static_cast<double> (step.length) / 100.0);
                    const float lengthDepth = juce::jlimit (0.0f, 1.0f,
                        config.noteLengthRandomDepth);
                    if (! tie && lengthDepth > 0.0f
                        && random (0x4f3b9e12u ^ repeatSalt) < lengthDepth)
                    {
                        float r = 0.0f;
                        for (int n = 0; n < 6; ++n)
                            r += random (0x5f356495u ^ repeatSalt
                                    ^ (static_cast<std::uint32_t> (n) * 0x9e3779b9u)) - 0.5f;
                        lengthFactor = juce::jlimit (0.01, 2.5,
                            lengthFactor * static_cast<double> ((r / 3.0f + 0.5f) * 3.0f));
                    }
                    // An identical pitch across consecutive tied steps is ONE
                    // sustained note, including step 16 -> step 1. A changed
                    // pitch uses an overlap hand-off (new On before old Off)
                    // to keep the bass engine's envelope and portamento alive.
                    if (lastNote[lane] >= 0 && lastNote[lane] != note
                        && pendingNoteOffPpq[lane] < 0.0)
                    {
                        emitNote (true, note, velocity, onPpq);
                        emitNote (false, lastNote[lane], 0, onPpq);
                    }
                    else if (lastNote[lane] != note
                             || pendingNoteOffPpq[lane] >= 0.0)
                    {
                        releaseAt (onPpq);
                        emitNote (true, note, velocity, onPpq);
                    }
                    // If the pitch is unchanged and no gate is pending, the
                    // previous Length 101 is still open: never retrigger it.
                    lastNote[lane] = note;
                    lastSourceNote[lane] = step.note;
                    pendingNoteOffPpq[lane] = tie ? -1.0
                        : onPpq + intervalBeats * lengthFactor;
                }
            }
            cycleBeat += cycleLength;
            if (songEntry || cycleBeat > laneEndPpq + stepBeats * 3.0) break;
        }
        // A normal note may finish in this buffer even when its onset was in
        // an earlier callback. Tied notes deliberately have no deadline.
        flushNoteOff (laneEndPpq);
    }
}
}
