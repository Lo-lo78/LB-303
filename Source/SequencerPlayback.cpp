// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SequencerPlayback.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace lb303
{
namespace
{
int positiveModulo (std::int64_t number, int divisor)
{
    const auto result = number % divisor;
    return static_cast<int> (result < 0 ? result + divisor : result);
}

// Stable deterministic probability so the same timeline position renders the
// same result after looped playback, rather than depending on audio block sizes.
float randomValue (int track, int pattern, std::int64_t step, std::uint32_t salt)
{
    std::uint32_t x = salt ^ static_cast<std::uint32_t> (step);
    x ^= static_cast<std::uint32_t> (step >> 32) * 0x85ebca6bu;
    x ^= static_cast<std::uint32_t> (track + 1) * 0x27d4eb2du;
    x ^= static_cast<std::uint32_t> (pattern + 1) * 0x165667b1u;
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15;
    return static_cast<float> (static_cast<double> (x) / 4294967296.0);
}
bool shouldSkip (int track, int pattern, std::int64_t step, float chance)
{
    if (chance <= 0.0f) return false;
    if (chance >= 1.0f) return true;
    return randomValue (track, pattern, step, 0x9e3779b9u) < chance;
}
}
void SequencerPlayback::reset() noexcept
{
    oldPattern = { -1, -1 };
    oldEnabled = { false, false };
    lastNote = { -1, -1 };
    lastSourceNote = { -1, -1 };
    previousPpq = -1.0;
    previousPlaying = false;
}

void SequencerPlayback::generate (juce::MidiBuffer& output,
                                  const std::array<SequencerState, 2>& banks,
                                  const std::array<int, 2>& patterns,
                                  const std::array<bool, 2>& enabled,
                                  const std::array<double, 2>& triggerOrigins,
                                  const std::array<int, 2>& keyboardTransposes,
                                  const std::array<SequencerConfig, 2>& performanceControls,
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

    // Explicit note-off on stop, disabling a lane or switching its pattern.
    // Does not interfere with the other lane's MIDI channel.
    for (int lane = 0; lane < 2; ++lane)
    {
        if (lastNote[lane] >= 0 && (timelineJump || ! playing || ! enabled[lane]
                                    || ! oldEnabled[lane] || oldPattern[lane] != patterns[lane]))
        {
            output.addEvent (juce::MidiMessage::noteOff (lane + 1, lastNote[lane]), 0);
            lastNote[lane] = -1;
            lastSourceNote[lane] = -1;
        }
        oldPattern[lane] = patterns[lane];
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
        auto config = bank.getConfig (pattern);
        // These 10 VST controls are per bassline, regardless of its active
        // pattern. Pattern-specific start/end, launch and step data remain intact.
        const auto& controls = performanceControls[static_cast<size_t> (lane)];
        config.bpmDivision = controls.bpmDivision;
        config.shuffle = controls.shuffle;
        config.legato = controls.legato;
        config.noteSkipProbability = controls.noteSkipProbability;
        config.noteRandomDepth = controls.noteRandomDepth;
        config.velocityRandomDepth = controls.velocityRandomDepth;
        config.noteLengthRandomDepth = controls.noteLengthRandomDepth;
        config.repeatRandomDepth = controls.repeatRandomDepth;
        config.octaveShift = controls.octaveShift;
        config.semitoneShift = controls.semitoneShift;
        const int start = juce::jlimit (1, 128, config.startStep) - 1;
        const int finish = juce::jlimit (start + 1, 128, config.endStep);
        const int patternLength = finish - start;
        const auto division = juce::jlimit (0.03125, 64.0, static_cast<double> (config.bpmDivision));
        const double stepBeats = 1.0 / division;
        // LJuno's Launch Step is a delayed pattern start, not the note number
        // of the first step. Keep phase/launch independent for each bank.
        const double launchDelay = (juce::jlimit (1, 128, config.launchStep) - 1) * stepBeats
            + static_cast<double> (config.launchOffsetMs) * bpm / 60000.0;
        const std::int64_t first = static_cast<std::int64_t> (
            std::floor ((lanePpq - launchDelay) / stepBeats)) - 2;
        const std::int64_t last = static_cast<std::int64_t> (
            std::ceil ((laneEndPpq - launchDelay) / stepBeats)) + 2;
        // Bound CPU for pathological host jumps or extreme divisions.
        for (std::int64_t tick = first; tick <= last && tick < first + 1024; ++tick)
        {
            const int stepInPattern = positiveModulo (tick, patternLength);
            const int stepIndex = start + stepInPattern;
            const auto step = bank.getStep (pattern, stepIndex);
            const float repeatsNoise = (randomValue (lane, pattern, tick, 0x1b873593u) * 2 - 1)
                * juce::jlimit (0.0f, 1.0f, config.repeatRandomDepth);
            const int repeats = juce::jlimit (1, 16, juce::roundToInt (step.repeat + repeatsNoise * 8));
            const double repeatBeats = stepBeats / repeats;
            const double swing = (positiveModulo (tick, 2) != 0)
                ? juce::jlimit (-0.45, 0.45, static_cast<double> (config.shuffle)) * stepBeats : 0.0;
            const double shift = juce::jlimit (-0.45, 0.45,
                static_cast<double> (step.shift - 0.5f)) * repeatBeats;
            // Parameter Locks take effect at this step's first onset, before
            // its Note On, including on steps which contain a pause.
            const double firstOnPpq = launchDelay + tick * stepBeats + swing + shift;
            if (firstOnPpq >= lanePpq && firstOnPpq < laneEndPpq
                && stepEventCount < maximumStepEvents)
            {
                const int sample = juce::jlimit (0, samples - 1,
                    static_cast<int> (std::floor ((firstOnPpq - lanePpq) / beatsPerSample + 0.5)));
                stepEvents[static_cast<size_t> (stepEventCount++)] =
                    { blockOffset + sample, lane, pattern, stepIndex };
            }
            if (step.note >= 128 || step.velocity <= 0 || shouldSkip (
                lane, pattern, tick, config.noteSkipProbability)) continue;
            // Same C4-based keyboard transposition as LJuno. Each lane uses
            // its own physical MIDI channel and independent held-note stack.
            const int randomSemitones = juce::roundToInt (
                (randomValue (lane, pattern, tick, 0x3972bc53u) * 2 - 1) *
                12.0f * juce::jlimit (0.0f, 1.0f, config.noteRandomDepth));
            const int note = juce::jlimit (0, 127, step.note + randomSemitones + keyboardTransposes[lane]
                + config.semitoneShift + 12 * config.octaveShift);
            const int velocity = juce::jlimit (1, 127, juce::roundToInt (
                step.velocity + (randomValue (lane, pattern, tick, 0x23ab4e21u) * 2 - 1)
                * 63.0f * juce::jlimit (0.0f, 1.0f, config.velocityRandomDepth)));
            for (int repeat = 0; repeat < repeats; ++repeat)
            {
                const double onPpq = launchDelay + tick * stepBeats + repeat * repeatBeats + swing + shift;
                const double randomLength = (randomValue (lane, pattern, tick, 0x4f3b9e12u) * 2 - 1)
                    * juce::jlimit (0.0f, 1.0f, config.noteLengthRandomDepth) * 0.5;
                const double lengthFactor = (step.length > 100 || (config.legato && step.length >= 100))
                    ? 1.02 : juce::jlimit (0.01, 1.0,
                                         static_cast<double> (step.length) / 100.0 + randomLength);
                const double offPpq = onPpq + repeatBeats * lengthFactor;
                const auto emit = [&] (bool noteOn, double eventPpq)
                {
                    if (eventPpq < lanePpq || eventPpq >= laneEndPpq) return;
                    const int sample = juce::jlimit (0, samples - 1,
                        static_cast<int> (std::floor ((eventPpq - lanePpq) / beatsPerSample + 0.5)));
                    if (noteOn)
                    {
                        output.addEvent (juce::MidiMessage::noteOn (
                            lane + 1, note, static_cast<juce::uint8> (velocity)), sample);
                        lastNote[lane] = note;
                        lastSourceNote[lane] = step.note;
                    }
                    else if (lastNote[lane] >= 0 && lastSourceNote[lane] == step.note)
                    {
                        // Match the pitch emitted at Note On, not the current
                        // transpose: the player may have changed keys meanwhile.
                        output.addEvent (juce::MidiMessage::noteOff (
                            lane + 1, lastNote[lane]), sample);
                        lastNote[lane] = -1;
                        lastSourceNote[lane] = -1;
                    }
                };
                emit (true, onPpq);
                emit (false, offPpq);
            }
        }
    }
}
}
