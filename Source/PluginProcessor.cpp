// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

LB303AudioProcessor::LB303AudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput ("Audio input", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Master 1/2", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Bassline 1 3/4", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Bassline 2 5/6", juce::AudioChannelSet::stereo(), true)
          .withOutput ("FX Wet 7/8", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "LB303State", lb303::createParameterLayout()),
      presetManager (*this)
{
    for (size_t i = 0; i < rawParameters.size(); ++i)
        rawParameters[i] = parameters.getRawParameterValue (lb303::catalog()[i].id);
    for (int lane = 0; lane < 2; ++lane)
        for (size_t control = 0; control < lb303::sequencerPerformancePerLane; ++control)
            parameters.addParameterListener (lb303::catalog()[lb303::seqPerformanceIndex (
                static_cast<size_t> (lane), control)].id, this);
}

LB303AudioProcessor::~LB303AudioProcessor()
{
    for (int lane = 0; lane < 2; ++lane)
        for (size_t control = 0; control < lb303::sequencerPerformancePerLane; ++control)
            parameters.removeParameterListener (lb303::catalog()[lb303::seqPerformanceIndex (
                static_cast<size_t> (lane), control)].id, this);
}

namespace
{
using CP = lb303::SequencerState::ConfigParameter;
constexpr std::array<CP, lb303::sequencerPerformancePerLane> performanceConfigMap {{
    CP::bpmDivision, CP::shuffle, CP::legato, CP::noteSkipProbability,
    CP::noteRandomDepth, CP::velocityRandomDepth, CP::noteLengthRandomDepth,
    CP::repeatRandomDepth, CP::octaveShift, CP::semitoneShift
}};
}

void LB303AudioProcessor::parameterChanged (const juce::String& id, float newValue)
{
    if (mirroringPerformance.load (std::memory_order_acquire)
        || restoringPerformance.load (std::memory_order_acquire)) return;
    for (size_t lane = 0; lane < lb303::basslineCount; ++lane)
        for (size_t control = 0; control < performanceConfigMap.size(); ++control)
            if (id == lb303::catalog()[lb303::seqPerformanceIndex (lane, control)].id)
            {
                sequenceBanks[lane].setConfigValue (
                    currentPattern (lane), performanceConfigMap[control], newValue);
                return;
            }
}

void LB303AudioProcessor::invalidatePerformanceDisplay() noexcept
{
    mirroredPatterns = {{ -1, -1 }};
}

void LB303AudioProcessor::syncSelectedPatternPerformance (bool force)
{
    // Called on the message thread, not in processBlock. Neither the host
    // automation nor the APVTS slider attachment may overwrite another pattern.
    for (size_t lane = 0; lane < lb303::basslineCount; ++lane)
    {
        const int patternIndex = currentPattern (lane);
        if (! force && mirroredPatterns[lane] == patternIndex) continue;
        const auto& bank = sequenceBanks[lane];
        mirroringPerformance.store (true, std::memory_order_release);
        for (size_t control = 0; control < performanceConfigMap.size(); ++control)
        {
            const auto global = lb303::seqPerformanceIndex (lane, control);
            if (auto* param = parameter (global))
            {
                const float actual = bank.getConfigValue (patternIndex, performanceConfigMap[control]);
                if (std::abs (value (global) - actual) > 1.0e-6f)
                    param->setValueNotifyingHost (param->convertTo0to1 (actual));
            }
        }
        mirroredPatterns[lane] = patternIndex;
        mirroringPerformance.store (false, std::memory_order_release);
    }
}


void LB303AudioProcessor::auditionSongPattern (int lane, int pattern) noexcept
{
    if (lane == auditionLane.load (std::memory_order_acquire)
        && pattern == auditionPattern.load (std::memory_order_acquire)
        && isPreviewPlaying())
    {
        requestSequencerPreview (false);
        return;
    }
    auditionPattern.store (juce::jlimit (0, 15, pattern), std::memory_order_release);
    auditionLane.store (juce::jlimit (0, 1, lane), std::memory_order_release);
    requestSequencerPreview (true);
    restartSongPreviewClock();
}

void LB303AudioProcessor::prepareToPlay (double sampleRate, int)
{
    sampleRateHz = sampleRate;
    fallbackPpq = 0.0;
    previewPpq = 0.0;
    previousPreview = false;
    consumedPreviewRestartEpoch = previewRestartEpoch.load (std::memory_order_acquire);
    previewOriginPpq = 0.0;
    heldKeys = {};
    heldKeyCount = {};
    heldKeyAge = {};
    keyAgeCounter = { 1, 1 };
    keyboardTranspose = {};
    lastKeyboardNote = { 60, 60 };
    triggerOriginPpq = {};
    previousSequencerOn = {};
    previousMidiChannel = { 1, 2 };
    previewEnabled.store (false, std::memory_order_relaxed);
    auditionLane.store (-1, std::memory_order_release);
    sequencerPlayback.setRandomSeed (static_cast<std::uint32_t> (
        juce::Random::getSystemRandom().nextInt()));
    sequencerPlayback.reset();
    activeLockStep = { -1, -1 };
    activeLockPattern = { -1, -1 };
    previousLockSongEntry = { -9, -9 };
    for (auto& engine : engines) engine.prepare (sampleRate);
    sharedFx.prepare (sampleRate);
    currentOversamplingFactor = 1;
    wetTailGuardSamples = 0;
    for (auto& decimator : voiceDecimators) decimator.setFactor (1);
}
void LB303AudioProcessor::releaseResources() {}

bool LB303AudioProcessor::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto stereo = juce::AudioChannelSet::stereo();
    if (layout.getMainOutputChannelSet() != stereo ||
        (layout.getMainInputChannelSet() != stereo &&
         layout.getMainInputChannelSet() != juce::AudioChannelSet::disabled()))
        return false;
    for (int bus = 1; bus < layout.outputBuses.size(); ++bus)
        if (layout.outputBuses[bus] != stereo &&
            layout.outputBuses[bus] != juce::AudioChannelSet::disabled())
            return false;
    return true;
}

float LB303AudioProcessor::value (size_t i) const
{
    return i < rawParameters.size() && rawParameters[i] != nullptr
        ? rawParameters[i]->load (std::memory_order_relaxed) : 0.0f;
}
juce::RangedAudioParameter* LB303AudioProcessor::parameter (size_t i)
{
    return i < lb303::parameterCount ? parameters.getParameter (lb303::catalog()[i].id) : nullptr;
}
int LB303AudioProcessor::currentPattern (size_t lane) const noexcept
{
    const auto slot = lane == 0 ? lb303::pattern1Index : lb303::pattern2Index;
    return juce::jlimit (0, lb303::SequencerState::maximumSequences - 1,
                         juce::roundToInt (value (slot)));
}

void LB303AudioProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    if (resetRuntimeRequested.exchange (false, std::memory_order_acq_rel))
    {
        // Clear running notes and all FX tails on the audio thread, avoiding
        // concurrent writes into the synthesizer's non-atomic DSP state.
        for (auto& engine : engines) engine.reset();
        sharedFx.clearTails();
        for (auto& decimator : voiceDecimators) decimator.clear();
        wetTailGuardSamples = 0;
        sequencerPlayback.reset();
        fallbackPpq = previewPpq = previewOriginPpq = 0.0;
        previousPreview = false;
        auditionLane.store (-1, std::memory_order_release);
        heldKeys = {};
        heldKeyCount = {};
        heldKeyAge = {};
        keyAgeCounter = { 1, 1 };
        keyboardTranspose = {};
        lastKeyboardNote = { 60, 60 };
        triggerOriginPpq = {};
        previousSequencerOn = {};
        previousMidiChannel = { 1, 2 };
        activeLockStep = { -1, -1 };
        activeLockPattern = { -1, -1 };
        previousLockSongEntry = { -9, -9 };
    }
    double bpm = 120.0;
    double ppq = fallbackPpq;
    bool playing = false;
    if (auto* playhead = getPlayHead())
        if (const auto position = playhead->getPosition())
        {
            if (const auto hostBpm = position->getBpm()) bpm = *hostBpm;
            if (const auto hostPpq = position->getPpqPosition()) ppq = *hostPpq;
            playing = position->getIsPlaying();
        }
    if (! std::isfinite (bpm) || bpm <= 0.0) bpm = 120.0;
    if (! std::isfinite (ppq)) ppq = 0.0;
    const bool activePreview = previewEnabled.load (std::memory_order_relaxed);
    const auto restartEpoch = previewRestartEpoch.load (std::memory_order_acquire);
    if (activePreview && (! previousPreview || consumedPreviewRestartEpoch != restartEpoch))
    {
        previewPpq = 0.0;
        previewOriginPpq = 0.0;
        sequencerPlayback.reset(); // restart audition / song on step one
        activeLockStep = { -1, -1 };
        activeLockPattern = { -1, -1 };
        previousLockSongEntry = { -9, -9 };
    }
    consumedPreviewRestartEpoch = restartEpoch;
    previousPreview = activePreview;
    // Preview has its OWN clock even if REAPER's transport is playing. This
    // prevents a host Play/Stop transition from jumping song time backwards.
    // Held MIDI notes without preview retain the original fallback clock.
    if (activePreview) ppq = previewPpq;
    else if (! playing) ppq = fallbackPpq;
    const auto songColumns = std::array<lb303::SongArrangement::Snapshot, 2> {{
        songs.column (0), songs.column (1)
    }};
    const bool useSong = songs.songMode();
    const int audition = activePreview ? auditionLane.load (std::memory_order_acquire) : -1;
    const int auditionPat = auditionPattern.load (std::memory_order_acquire);
    std::array<int, 2> playingPatterns {{ currentPattern (0), currentPattern (1) }};
    const int count = audio.getNumSamples();
    const double beatsPerSample = bpm / (60.0 * sampleRateHz);
    const double nextPpq = ppq + count * beatsPerSample;
    if (activePreview) previewPpq = nextPpq;
    else fallbackPpq = nextPpq;

    const std::array<bool, 2> sequencerOn {{
        value (lb303::sequencer1EnabledIndex) > 0.5f,
        value (lb303::sequencer2EnabledIndex) > 0.5f
    }};
    // 0=Omni, 1..16=single MIDI input channel. Defaults remain 1 and 2.
    const std::array<int, 2> inputChannels {{
        juce::jlimit (0, 16, juce::roundToInt (value (lb303::midiChannel1Index))),
        juce::jlimit (0, 16, juce::roundToInt (value (lb303::midiChannel2Index)))
    }};
    const auto clearKeys = [&] (size_t lane)
    {
        for (auto& channel : heldKeys[lane]) channel.fill (false);
        for (auto& channel : heldKeyAge[lane]) channel.fill (0);
        heldKeyCount[lane] = 0;
        keyboardTranspose[lane] = 0;
    };

    // On changing modes, release a note held by the previous mode. This also
    // prevents the physical key that selected a pattern from being heard.
    for (size_t lane = 0; lane < lb303::basslineCount; ++lane)
    {
        if (sequencerOn[lane] == previousSequencerOn[lane]
            && inputChannels[lane] == previousMidiChannel[lane]) continue;
        // Change of MIDI route or Direct/Sequencer mode: release old note/gate.
        engines[lane].releaseNote();
        clearKeys (lane);
        previousSequencerOn[lane] = sequencerOn[lane];
        previousMidiChannel[lane] = inputChannels[lane];
        activeLockStep[lane] = -1;
        activeLockPattern[lane] = -1;
    }
    for (size_t lane = 0; lane < lb303::basslineCount; ++lane)
    {
        const bool gated = (sequencerOn[lane] && heldKeyCount[lane] > 0)
                            || activePreview;
        if (! gated || (activeLockStep[lane] >= 0
                         && ! useSong && activeLockPattern[lane] != currentPattern (lane)))
        {
            activeLockStep[lane] = -1;
            activeLockPattern[lane] = -1;
        }
    }

    // Route physical keys before generating each audio segment. Unlike the
    // earlier implementation, we never merge them into the sequenced notes.
    // Dividing at MIDI offsets makes trigger/release sample-accurate, including
    // when a key begins or ends in the middle of an audio block.
    std::array<lb303::SequencerPlayback::StepEvent,
               lb303::SequencerPlayback::maximumStepEvents> stepEvents {};
    int stepEventCount = 0;
    const juce::MidiBuffer physicalInput (midi);
    juce::MidiBuffer engineMidi; // channel 1/2 are INTERNAL lane IDs only
    midi.clear(); // VST MIDI output; always preserve original external channel once
    auto generateSegment = [&] (int beginSample, int endSample)
    {
        if (endSample <= beginSample) return;
        const std::array<double, 2> origins {{
            activePreview ? previewOriginPpq : triggerOriginPpq[0],
            activePreview ? previewOriginPpq : triggerOriginPpq[1]
        }};
        const std::array<int, 2> transposes {{
            activePreview ? lastKeyboardNote[0] - 60 : keyboardTranspose[0],
            activePreview ? lastKeyboardNote[1] - 60 : keyboardTranspose[1]
        }};
        // Audio always reads the performance controls belonging to the actual
        // pattern being played, not the editing-knob values of another pattern.
        // Divide a host audio segment at each Song pattern boundary. This
        // prevents a buffer straddling the boundary from delaying the change
        // until the next callback. The two columns never wait for one another.
        int cursor = beginSample;
        while (cursor < endSample)
        {
            const double nowPpq = ppq + cursor * beatsPerSample;
            auto laneOrigins = origins;
            std::array<int, 2> patterns {{ currentPattern (0), currentPattern (1) }};
            std::array<std::int64_t, 2> entries {{ -1, -1 }};
            std::array<bool, 2> audible {{
                (sequencerOn[0] && heldKeyCount[0] > 0) || activePreview,
                (sequencerOn[1] && heldKeyCount[1] > 0) || activePreview
            }};
            int segmentEnd = endSample;
            for (int lane = 0; lane < 2; ++lane)
            {
                if (audition >= 0)
                {
                    audible[static_cast<size_t> (lane)] = lane == audition;
                    if (lane == audition) patterns[static_cast<size_t> (lane)] = auditionPat;
                    entries[static_cast<size_t> (lane)] = -2;
                }
                else if (useSong && audible[static_cast<size_t> (lane)])
                {
                    const double localBeat = nowPpq - origins[static_cast<size_t> (lane)];
                    const auto pos = lb303::SongArrangement::locate (
                        *songColumns[static_cast<size_t> (lane)], songs.loop (lane),
                        sequenceBanks[static_cast<size_t> (lane)], localBeat, lane,
                        sequencerPlayback.getRandomSeed(), bpm);
                    if (! pos.active)
                    {
                        audible[static_cast<size_t> (lane)] = false;
                        entries[static_cast<size_t> (lane)] = -3;
                    }
                    else
                    {
                        patterns[static_cast<size_t> (lane)] = pos.pattern;
                        laneOrigins[static_cast<size_t> (lane)] += pos.startBeat;
                        entries[static_cast<size_t> (lane)] = static_cast<std::int64_t> (
                            std::llround (pos.startBeat * 1000000.0));
                        const double leftBeats = origins[static_cast<size_t> (lane)]
                            + pos.endBeat - nowPpq;
                        const int untilNext = juce::jmax (1, static_cast<int> (
                            std::ceil (leftBeats / beatsPerSample - 1.0e-8)));
                        segmentEnd = juce::jmin (segmentEnd, cursor + untilNext);
                    }
                }
                const auto index = static_cast<size_t> (lane);
                if (! audible[index]
                    || playingPatterns[index] != patterns[index]
                    || previousLockSongEntry[index] != entries[index])
                {
                    activeLockStep[index] = -1;
                    activeLockPattern[index] = -1;
                }
                previousLockSongEntry[index] = entries[index];
                playingPatterns[index] = patterns[index];
            }
            const int segmentLength = segmentEnd - cursor;
            juce::MidiBuffer generated;
            sequencerPlayback.generate (generated, sequenceBanks,
                patterns, entries, audible, laneOrigins, transposes,
                playing || activePreview || audible[0] || audible[1],
                nowPpq, bpm, sampleRateHz, segmentLength,
                stepEvents, stepEventCount, cursor);
            engineMidi.addEvents (generated, 0, segmentLength, cursor);
            for (const auto event : generated)
            {
                const int lane = event.getMessage().getChannel() - 1;
                if (lane < 0 || lane >= 2) continue;
                const int outputChannel = inputChannels[static_cast<size_t> (lane)] == 0
                    ? lane + 1 : inputChannels[static_cast<size_t> (lane)];
                auto outputMessage = event.getMessage();
                outputMessage.setChannel (outputChannel);
                midi.addEvent (outputMessage, cursor + event.samplePosition);
            }
            cursor = segmentEnd;
        }
    };
    int previousSample = 0;
    for (const auto entry : physicalInput)
    {
        const int offset = std::clamp (entry.samplePosition, 0, count);
        generateSegment (previousSample, offset);
        previousSample = offset;
        const auto& message = entry.getMessage();
        const int midiChannel = message.getChannel();
        bool matched = false;
        bool directReceiver = false;
        // A physical MIDI event may address both voices (same input channel
        // or Omni). Copy it to each *internal* lane without changing the
        // other lane's sequencer state, and forward it just once to REAPER.
        for (size_t lane = 0; lane < lb303::basslineCount; ++lane)
        {
            if (midiChannel < 1 || midiChannel > 16
                || (inputChannels[lane] != 0 && inputChannels[lane] != midiChannel))
                continue;
            matched = true;
            if (message.isNoteOn())
                lastKeyboardNote[lane] = juce::jlimit (0, 127, message.getNoteNumber());
            if (sequencerOn[lane] && message.isNoteOnOrOff())
            {
                const int note = juce::jlimit (0, 127, message.getNoteNumber());
                const auto channelSlot = static_cast<size_t> (midiChannel - 1);
                const auto key = static_cast<size_t> (note);
                auto& down = heldKeys[lane][channelSlot][key];
                if (message.isNoteOn())
                {
                    if (! down)
                    {
                        if (heldKeyCount[lane] == 0)
                            triggerOriginPpq[lane] = ppq + offset * beatsPerSample;
                        down = true;
                        ++heldKeyCount[lane];
                    }
                    auto& nextAge = keyAgeCounter[lane];
                    if (nextAge == 0)
                    {
                        for (auto& ages : heldKeyAge[lane]) ages.fill (0);
                        nextAge = 1;
                    }
                    heldKeyAge[lane][channelSlot][key] = nextAge++;
                    keyboardTranspose[lane] = note - 60;
                }
                else if (down)
                {
                    down = false;
                    heldKeyAge[lane][channelSlot][key] = 0;
                    --heldKeyCount[lane];
                    int newestNote = 60;
                    std::uint64_t newestAge = 0;
                    for (int channel = 0; channel < 16; ++channel)
                        for (int candidate = 0; candidate < 128; ++candidate)
                        {
                            const auto age = heldKeyAge[lane][static_cast<size_t> (channel)]
                                                           [static_cast<size_t> (candidate)];
                            if (age > newestAge)
                            {
                                newestAge = age;
                                newestNote = candidate;
                            }
                        }
                    keyboardTranspose[lane] = newestNote - 60;
                }
                // Note is a trigger, never a direct oscillator note for this lane.
            }
            else
            {
                if (message.isNoteOnOrOff()) directReceiver = true;
                // Map the physical source to the unique internal engine lane.
                auto routedMessage = message;
                routedMessage.setChannel (static_cast<int> (lane) + 1);
                engineMidi.addEvent (routedMessage, offset);
            }
            if (sequencerOn[lane] && message.isController()
                && (message.getControllerNumber() == 120
                    || message.getControllerNumber() == 123))
                clearKeys (lane);
        }
        // Do not forward a consumed trigger unless another voice is in direct
        // mode on this channel. Non-note events preserve normal MIDI thru.
        if (! message.isNoteOnOrOff() || directReceiver || ! matched)
            midi.addEvent (message, offset);
    }
    generateSegment (previousSample, count);

    // Each voice takes its host-automated parameter baseline, then overlays
    // ONLY the locks of the current sequencer step. No APVTS parameter writes:
    // REAPER automation and the two Bassline grids retain their original values.
    std::array<float, lb303::basslineCount> leftGain {}, rightGain {};
    const auto applyVoiceParameters = [&] (size_t voice)
    {
        std::array<float, lb303::voiceParameterCount> effective {};
        for (size_t i = 0; i < effective.size(); ++i)
            effective[i] = value (lb303::parameterIndex (voice, i));
        if (activeLockStep[voice] >= 0 && activeLockPattern[voice] >= 0)
        {
            const auto step = sequenceBanks[voice].getStep (
                activeLockPattern[voice], activeLockStep[voice]);
            for (int i = 0; i < step.parameterLockCount; ++i)
            {
                const auto& lock = step.parameterLocks[static_cast<size_t> (i)];
                const int index = lock.sliderNumber - 1; // 1..37 per bassline
                if (! juce::isPositiveAndBelow (index, static_cast<int> (effective.size())))
                    continue;
                const auto& info = lb303::catalog()[lb303::parameterIndex (voice,
                                                static_cast<size_t> (index))];
                if (! std::isfinite (lock.value)) continue;
                effective[static_cast<size_t> (index)] =
                    juce::jlimit (info.minimum, info.maximum, lock.value);
            }
        }
        lb303::BassEngine::Values synth {};
        for (size_t i = 0; i < synth.size(); ++i) synth[i] = effective[i];
        // Voice engine processes only the oscillator/filter/envelopes; the
        // 14 historic delay controls are routed to one shared classic FX bus.
        synth[31] = 100.0f; synth[32] = 0.0f;
        engines[voice].setParameters (synth, bpm);
        lb303::BassEngine::LfoValues voiceLfo {};
        for (size_t c = 0; c < voiceLfo.size(); ++c)
            voiceLfo[c] = value (lb303::lfoParameterIndex (voice, c));
        if (activeLockStep[voice] >= 0 && activeLockPattern[voice] >= 0)
        {
            const auto step = sequenceBanks[voice].getStep (
                activeLockPattern[voice], activeLockStep[voice]);
            for (int i = 0; i < step.parameterLockCount; ++i)
            {
                const auto& lock = step.parameterLocks[static_cast<size_t> (i)];
                const int c = lock.sliderNumber - 1 - static_cast<int> (lb303::voiceParameterCount);
                if (! juce::isPositiveAndBelow (c, static_cast<int> (voiceLfo.size()))
                    || ! std::isfinite (lock.value)) continue;
                const auto& info = lb303::catalog()[lb303::lfoParameterIndex (
                    voice, static_cast<size_t> (c))];
                voiceLfo[static_cast<size_t> (c)] =
                    juce::jlimit (info.minimum, info.maximum, lock.value);
            }
        }
        engines[voice].setLfoParameters (voiceLfo);
        lb303::BassEngine::Osc2Values layerValues {};
        for (size_t control = 0; control < layerValues.size(); ++control)
            layerValues[control] = value (lb303::osc2ParameterIndex (voice, control));
        if (activeLockStep[voice] >= 0 && activeLockPattern[voice] >= 0)
        {
            const auto step = sequenceBanks[voice].getStep (
                activeLockPattern[voice], activeLockStep[voice]);
            for (int i = 0; i < step.parameterLockCount; ++i)
            {
                const auto& lock = step.parameterLocks[static_cast<size_t> (i)];
                const int control = lock.sliderNumber - 1 - static_cast<int> (lb303::osc2LocalBase);
                if (! juce::isPositiveAndBelow (control, static_cast<int> (layerValues.size()))
                    || ! std::isfinite (lock.value)) continue;
                const auto& info = lb303::catalog()[lb303::osc2ParameterIndex (
                    voice, static_cast<size_t> (control))];
                layerValues[static_cast<size_t> (control)] =
                    juce::jlimit (info.minimum, info.maximum, lock.value);
            }
        }
        engines[voice].setOsc2Parameters (layerValues);
        // The historical Level ID remains loadable for old sessions but no
        // longer changes gain: slider008 Volume is the sole bassline level.
        const auto level = 1.0f;
        const auto pan = std::clamp (
            effective[lb303::outputPanOffset] / 100.0f, -1.0f, 1.0f);
        leftGain[voice] = level * (pan > 0.0f ? 1.0f - pan : 1.0f);
        rightGain[voice] = level * (pan < 0.0f ? 1.0f + pan : 1.0f);
    };
    const auto applyFxParameters = [&]
    {
        lb303::SharedEffects::Controls fxControls {};
        // The historic 35 FX VST3 IDs are followed in the DSP array by
        // TWO controls whose host IDs are appended after oversampling.
        for (size_t i = 0; i < lb303::fxParameterCount; ++i)
            fxControls[i] = value (lb303::fxParameterIndex (i));
        fxControls[lb303::fx::reverbBassDecay] = value (lb303::reverbBassDecayIndex);
        fxControls[lb303::fx::reverbBassXover] = value (lb303::reverbBassXoverIndex);
        fxControls[lb303::fx::reverbMotionMode] = value (lb303::reverbMotionModeIndex);
        fxControls[lb303::fx::reverbMotionDepth] = value (lb303::reverbMotionDepthIndex);
        fxControls[lb303::fx::reverbMotionRate] = value (lb303::reverbMotionRateIndex);
        fxControls[lb303::fx::reverbSourcePan1] = value (lb303::reverbSourcePan1Index);
        fxControls[lb303::fx::reverbSourcePan2] = value (lb303::reverbSourcePan2Index);
        fxControls[lb303::fx::reverbLfoPan1] = value (lb303::reverbLfoPan1Index);
        fxControls[lb303::fx::reverbLfoPan2] = value (lb303::reverbLfoPan2Index);
        // Historic B1 delay parameter IDs now control ONE master classic delay.
        lb303::SharedEffects::ClassicParameters legacy {};
        for (size_t i = 0; i < legacy.size(); ++i)
            legacy[i] = value (lb303::parameterIndex (0, i));
        // Preserve older B1 per-step delay locks as automations of that shared delay.
        if (activeLockStep[0] >= 0 && activeLockPattern[0] >= 0)
        {
            const auto step = sequenceBanks[0].getStep (activeLockPattern[0], activeLockStep[0]);
            for (int i = 0; i < step.parameterLockCount; ++i)
            {
                const auto& lock = step.parameterLocks[static_cast<size_t> (i)];
                const int local = lock.sliderNumber - 1;
                if (local >= 19 && local <= 32 && std::isfinite (lock.value))
                {
                    const auto& info = lb303::catalog()[static_cast<size_t> (local)];
                    legacy[static_cast<size_t> (local)] =
                        juce::jlimit (info.minimum, info.maximum, lock.value);
                }
            }
        }
        sharedFx.setParameters (fxControls, legacy, bpm);
    };
    for (size_t voice = 0; voice < lb303::basslineCount; ++voice)
        applyVoiceParameters (voice);
    applyFxParameters();
    // The reverb takes its LFO from the SAME per-voice generator used for
    // pitch/filter/pan: BPM sync, One Shot, Trigger, wave and step locks agree.
    for (size_t voice = 0; voice < lb303::basslineCount; ++voice)
    {
        const size_t depthIndex = voice == 0 ? lb303::reverbLfoPan1Index
                                             : lb303::reverbLfoPan2Index;
        const size_t sendIndex = voice == 0 ? lb303::fx::reverbSend1
                                            : lb303::fx::reverbSend2;
        const bool connected = value (lb303::fxParameterIndex (lb303::fx::reverbType)) > .5f
            && value (lb303::fxParameterIndex (sendIndex)) > 0.0f
            && value (depthIndex) != 0.0f;
        engines[voice].setExternalLfoDestination (connected);
    }

    const int quality = juce::jlimit (0, 2,
        juce::roundToInt (value (lb303::oversamplingIndex)));
    const int oversamplingFactor = quality == 0 ? 1 : (quality == 1 ? 2 : 4);
    if (oversamplingFactor != currentOversamplingFactor)
    {
        // No allocations or DSP preparation on the audio thread.
        currentOversamplingFactor = oversamplingFactor;
        for (size_t voice = 0; voice < lb303::basslineCount; ++voice)
        {
            engines[voice].setOversamplingFactor (oversamplingFactor);
            voiceDecimators[voice].setFactor (oversamplingFactor);
        }
        sharedFx.setInsertOversamplingFactor (oversamplingFactor);
    }

    // Independently scheduled lock boundaries may be emitted for paused steps,
    // too. Sort them for exact sample order between the two MIDI channels.
    std::sort (stepEvents.begin(), stepEvents.begin() + stepEventCount,
        [] (const auto& a, const auto& b)
        {
            if (a.sample != b.sample) return a.sample < b.sample;
            return a.lane < b.lane;
        });

    if (audio.getNumChannels() < 2) { audio.clear(); return; }
    // Four output buses: Master 1/2, Bassline 1 3/4, Bassline 2 5/6,
    // and shared FX wet-only 7/8. JUCE packs *enabled* output buses in
    // the host's process buffer, so do not assume fixed channel indexes.
    const auto stereoBusStart = [&] (int bus) noexcept
    {
        if (getChannelCountOfBus (false, bus) != 2) return -1;
        const int channel = getChannelIndexInProcessBlockBuffer (false, bus, 0);
        return channel >= 2 && channel + 1 < audio.getNumChannels() ? channel : -1;
    };
    const int b1Index = stereoBusStart (1);
    const int b2Index = stereoBusStart (2);
    const int wetIndex = stereoBusStart (3);
    // A disabled stem bus simply produces no stem. The full master mix
    // remains audible on 1/2, including both basslines and the wet FX.
    const bool toB1 = value (lb303::separateOutputIndex (0)) > .5f && b1Index >= 0;
    const bool toB2 = value (lb303::separateOutputIndex (1)) > .5f && b2Index >= 0;

    auto* l = audio.getWritePointer (0);
    auto* r = audio.getWritePointer (1);
    auto* b1L = b1Index >= 0 ? audio.getWritePointer (b1Index) : nullptr;
    auto* b1R = b1Index >= 0 ? audio.getWritePointer (b1Index + 1) : nullptr;
    auto* b2L = b2Index >= 0 ? audio.getWritePointer (b2Index) : nullptr;
    auto* b2R = b2Index >= 0 ? audio.getWritePointer (b2Index + 1) : nullptr;
    auto* wetL = wetIndex >= 0 ? audio.getWritePointer (wetIndex) : nullptr;
    auto* wetR = wetIndex >= 0 ? audio.getWritePointer (wetIndex + 1) : nullptr;

    // Synth audio does not use the host audio input. Clear the output bus
    // explicitly: += on uncleared input channels could otherwise mix in an
    // unwanted duplicate dry signal from the track feeding this instrument.
    audio.clear (0, 0, count);
    audio.clear (1, 0, count);
    if (b1L != nullptr) { audio.clear (b1Index, 0, count); audio.clear (b1Index + 1, 0, count); }
    if (b2L != nullptr) { audio.clear (b2Index, 0, count); audio.clear (b2Index + 1, 0, count); }
    if (wetL != nullptr) { audio.clear (wetIndex, 0, count); audio.clear (wetIndex + 1, 0, count); }
    int nextSample = 0;
    int nextStepEvent = 0;
    const auto renderSamplesTo = [&] (int end)
    {
        if (end <= nextSample) return;
        // The graph is chosen once per MIDI/lock-bounded segment. Parameter
        // updates, transport/preview and sequencer events still run each block.
        // Notes arriving at an offset wake their own lane BEFORE rendering it.
        const std::array<bool, lb303::basslineCount> voiceWorking {{
            !engines[0].isQuiescent() || !voiceDecimators[0].isQuiescent()
                || !sharedFx.distortionQuiescent (0),
            !engines[1].isQuiescent() || !voiceDecimators[1].isQuiescent()
                || !sharedFx.distortionQuiescent (1)
        }};
        const bool anyVoiceWorking = voiceWorking[0] || voiceWorking[1];
        const bool wetEnabled = sharedFx.wetEffectsEnabled();
        if (!wetEnabled) wetTailGuardSamples = 0;
        // A completely cold graph never enters any 1x/2x/4x synthesis,
        // distortion, oversampling or wet FX code. All output buses were
        // already cleared. Remaining MIDI/step scheduling is untouched.
        if (!anyVoiceWorking && wetTailGuardSamples == 0)
        {
            nextSample = end;
            return;
        }
        const int wetGuardLimit = static_cast<int> (
            std::min (sampleRateHz * sharedFx.wetTailGuardSeconds(),
                      static_cast<double> (0x7fffffff)));
        // Decisions about active distortion graphs are made once per segment
        // (including a segment split by a parameter lock), not per sub-sample.
        const std::array<bool, lb303::basslineCount> distortionOn {{
            sharedFx.distortionActive (0), sharedFx.distortionActive (1)
        }};
        for (; nextSample < end; ++nextSample)
        {
            std::array<std::pair<float, float>, lb303::basslineCount> stems {};
            for (size_t voice = 0; voice < lb303::basslineCount; ++voice)
            {
                if (!voiceWorking[voice])
                {
                    engines[voice].advanceSilentLfo (oversamplingFactor);
                    continue;
                }
                if (oversamplingFactor == 1)
                {
                    // Identical original synth + insert routing at 1x.
                    const auto generated = engines[voice].tick (0.0f, 0.0f);
                    stems[voice] = { generated.first * leftGain[voice],
                                     generated.second * rightGain[voice] };
                }
                else
                {
                    // Each bassline runs its own oscillator, filter, Drive AND
                    // distortion insert at 2x/4x. Decimate only after shaping.
                    for (int sub = 0; sub < oversamplingFactor; ++sub)
                    {
                        const auto generated = engines[voice].tick (0.0f, 0.0f);
                        const auto scaled = std::pair<float,float> {
                            generated.first * leftGain[voice],
                            generated.second * rightGain[voice] };
                        const auto distorted = distortionOn[voice]
                            ? sharedFx.processDistortionInsert (
                                scaled.first, scaled.second, voice)
                            : scaled;
                        stems[voice] = voiceDecimators[voice].process (
                            distorted.first, distorted.second);
                    }
                }
            }
            const lb303::SharedEffects::LfoSamples sourceLfo {{
                engines[0].currentLfoSample(), engines[1].currentLfoSample()
            }};
            const auto fx = oversamplingFactor == 1
                ? sharedFx.process (stems[0].first, stems[0].second,
                                    stems[1].first, stems[1].second, sourceLfo)
                : sharedFx.processPostDistortion (stems[0].first, stems[0].second,
                                                   stems[1].first, stems[1].second, sourceLfo);
            if (wetEnabled)
            {
                if (anyVoiceWorking || std::abs (fx.wet.first) > 1.0e-8f
                                    || std::abs (fx.wet.second) > 1.0e-8f)
                    wetTailGuardSamples = std::max (wetTailGuardSamples, wetGuardLimit);
                else if (wetTailGuardSamples > 0)
                    --wetTailGuardSamples;
            }
            // Shared FX are independent of stem routing and are never mixed
            // into the individual voice buses. The master output contains
            // one direct contribution per bassline and one shared wet return.
            if (toB1)
            {
                b1L[nextSample] += fx.dry[0].first;
                b1R[nextSample] += fx.dry[0].second;
            }
            if (toB2)
            {
                b2L[nextSample] += fx.dry[1].first;
                b2R[nextSample] += fx.dry[1].second;
            }
            if (wetL != nullptr)
            {
                wetL[nextSample] += fx.wet.first;
                wetR[nextSample] += fx.wet.second;
            }
            const float mainDry = sharedFx.dryLevel();
            l[nextSample] += mainDry * (fx.dry[0].first + fx.dry[1].first) + fx.wet.first;
            r[nextSample] += mainDry * (fx.dry[0].second + fx.dry[1].second) + fx.wet.second;
        }
    };
    const auto renderTo = [&] (int end)
    {
        // Locks at an exact MIDI Note On offset must be installed BEFORE
        // sending that Note On to the corresponding BassEngine.
        while (nextStepEvent < stepEventCount && stepEvents[static_cast<size_t> (nextStepEvent)].sample <= end)
        {
            const auto event = stepEvents[static_cast<size_t> (nextStepEvent++)];
            renderSamplesTo (event.sample);
            const auto lane = static_cast<size_t> (event.lane);
            activeLockStep[lane] = event.step;
            activeLockPattern[lane] = event.pattern;
            applyVoiceParameters (lane);
            applyFxParameters();
        }
        renderSamplesTo (end);
    };
    // The internal stream uses fixed lane IDs 1/2 even if both instruments
    // receive external MIDI channel 1 (or Omni). Thus generated notes never
    // bleed into the wrong engine when user-configured channels overlap.
    for (const auto entry : engineMidi)
    {
        const auto offset = std::clamp (entry.samplePosition, 0, count);
        renderTo (offset);
        const auto& message = entry.getMessage();
        const int midiChannel = message.getChannel();
        if (midiChannel >= 1 && midiChannel <= static_cast<int> (lb303::basslineCount))
            engines[static_cast<size_t> (midiChannel - 1)].midi (message);
    }
    renderTo (count);
    // Extra outputs contain two optional post-distortion voice stems and
    // the independently accessible shared FX wet return. Do not clear them
    // after rendering. They are taps of the master mix: do not sum all four
    // buses together in REAPER or the audio will deliberately be doubled.
}

void LB303AudioProcessor::resetToInitialState()
{
    // Restore the catalog's initial plain values. Keep existing parameter IDs
    // and notify REAPER so its exposed parameter values stay synchronized.
    previewEnabled.store (false, std::memory_order_release);
    for (size_t i = 0; i < lb303::parameterCount; ++i)
    {
        if (auto* p = parameter (i))
        {
            const auto& info = lb303::catalog()[i];
            const auto current = value (i);
            if (std::abs (current - info.initial) > 1.0e-6f)
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (p->convertTo0to1 (info.initial));
                p->endChangeGesture();
            }
        }
    }
    for (auto& bank : sequenceBanks) bank.reset();
    invalidatePerformanceDisplay();
    songs.reset();
    auditionLane.store (-1, std::memory_order_release);
    // Alt+I resets this instance's editing increment, without writing UI
    // history into the state saved by REAPER or the preset browser.
    editorSession.stepWidthIndex = 0;
    presetManager.forgetCurrent();
    // Reset actual running DSP at the next audio callback (thread safe).
    resetRuntimeRequested.store (true, std::memory_order_release);
}

juce::AudioProcessorEditor* LB303AudioProcessor::createEditor()
{
    return new LB303AudioProcessorEditor (*this);
}
namespace
{
// TEST2.4.5: Drop navigation metadata from older TEST2.4.4 states as well.
// Audio parameters, pattern banks, and preset manager data remain intact.
constexpr const char* transientEditorKeys[] {
    "editorActiveContext", "editorSelectedBassline1", "editorSelectedBassline2",
    "editorSelectedSequencer", "editorSelectedFx", "editorSequencerPage",
    "editorStepWidthIndex"
};
void removeLegacyEditorProperties (juce::ValueTree& state)
{
    for (const char* key : transientEditorKeys)
        state.removeProperty (juce::Identifier (key), nullptr);
}
}

void LB303AudioProcessor::getStateInformation (juce::MemoryBlock& data)
{
    // Work on a copy; never change the live state merely to save a preset.
    auto snapshot = parameters.copyState();
    removeLegacyEditorProperties (snapshot);
    if (auto xml = snapshot.createXml())
    {
        // Two independent banks, not 16 joint patterns. Save all 32 patterns.
        // TEST2.4.22 marker: older builds treated the performance VST knobs
        // as lane-wide. Their values must be migrated into EACH old pattern.
        xml->createNewChildElement ("LB303PatternPerformancePerPattern")->setAttribute ("version", 1);
        auto* bank1 = xml->createNewChildElement ("LB303PatternBank1");
        bank1->setAttribute ("data", sequenceBanks[0].serialiseToBase64());
        auto* bank2 = xml->createNewChildElement ("LB303PatternBank2");
        bank2->setAttribute ("data", sequenceBanks[1].serialiseToBase64());
        auto* song = xml->createNewChildElement ("LB303SongArrangement");
        song->setAttribute ("data", songs.serialise());
        copyXmlToBinary (*xml, data);
    }
}
void LB303AudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
        {
            restoringPerformance.store (true, std::memory_order_release);
            const bool oldLaneWidePerformance =
                xml->getChildByName ("LB303PatternPerformancePerPattern") == nullptr;
            if (auto* marker = xml->getChildByName ("LB303PatternPerformancePerPattern"))
                xml->removeChildElement (marker, true);
            for (int lane = 0; lane < 2; ++lane)
            {
                const auto tag = juce::String (lane == 0 ? "LB303PatternBank1" : "LB303PatternBank2");
                if (auto* bank = xml->getChildByName (tag))
                    sequenceBanks[static_cast<size_t> (lane)].restoreFromBase64 (
                        bank->getStringAttribute ("data"));
                else
                    sequenceBanks[static_cast<size_t> (lane)].reset(); // legacy TEST1 projects
            }
            if (auto* song = xml->getChildByName ("LB303SongArrangement"))
            {
                if (! songs.restore (song->getStringAttribute ("data"))) songs.reset();
                xml->removeChildElement (song, true);
            }
            else songs.reset(); // old .lb presets have no arrangement
            if (auto* a = xml->getChildByName ("LB303PatternBank1")) xml->removeChildElement (a, true);
            if (auto* b = xml->getChildByName ("LB303PatternBank2")) xml->removeChildElement (b, true);
            auto restored = juce::ValueTree::fromXml (*xml);
            removeLegacyEditorProperties (restored);
            parameters.replaceState (restored);
            if (oldLaneWidePerformance)
            {
                // Exactly preserve the sound of pre-TEST2.4.22 presets: their
                // global sequencer controls applied equally to every pattern.
                for (size_t lane = 0; lane < lb303::basslineCount; ++lane)
                    for (int patternIndex = 0; patternIndex < 16; ++patternIndex)
                        for (size_t control = 0; control < performanceConfigMap.size(); ++control)
                            sequenceBanks[lane].setConfigValue (patternIndex,
                                performanceConfigMap[control],
                                value (lb303::seqPerformanceIndex (lane, control)));
            }
            invalidatePerformanceDisplay();
            restoringPerformance.store (false, std::memory_order_release);
            sequencerPlayback.reset();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LB303AudioProcessor();
}
