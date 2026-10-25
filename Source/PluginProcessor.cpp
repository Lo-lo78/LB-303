// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

LB303AudioProcessor::LB303AudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput ("Audio input", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "LB303State", lb303::createParameterLayout()),
      presetManager (*this)
{
    for (size_t i = 0; i < rawParameters.size(); ++i)
        rawParameters[i] = parameters.getRawParameterValue (lb303::catalog()[i].id);
}

void LB303AudioProcessor::prepareToPlay (double sampleRate, int)
{
    sampleRateHz = sampleRate;
    fallbackPpq = 0.0;
    previewPpq = 0.0;
    previousPreview = false;
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
    sequencerPlayback.reset();
    activeLockStep = { -1, -1 };
    activeLockPattern = { -1, -1 };
    for (auto& engine : engines) engine.prepare (sampleRate);
    sharedFx.prepare (sampleRate);
}
void LB303AudioProcessor::releaseResources() {}

bool LB303AudioProcessor::isBusesLayoutSupported (const BusesLayout& layout) const
{
    const auto stereo = juce::AudioChannelSet::stereo();
    return layout.getMainOutputChannelSet() == stereo
        && (layout.getMainInputChannelSet() == stereo ||
            layout.getMainInputChannelSet() == juce::AudioChannelSet::disabled());
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
    if (activePreview && ! previousPreview)
    {
        previewPpq = 0.0;
        previewOriginPpq = playing ? ppq : 0.0;
    }
    previousPreview = activePreview;
    // Many hosts report a fixed PPQ position while stopped. Keyboard-held
    // patterns must keep advancing on our own sample clock instead of replaying
    // their first step on every audio block.
    if (! playing) ppq = activePreview ? previewPpq : fallbackPpq;
    const int count = audio.getNumSamples();
    const double beatsPerSample = bpm / (60.0 * sampleRateHz);
    const double nextPpq = ppq + count * beatsPerSample;
    if (! playing && activePreview) previewPpq = nextPpq;
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
                         && activeLockPattern[lane] != currentPattern (lane)))
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
        const int segmentLength = endSample - beginSample;
        if (segmentLength <= 0) return;
        const std::array<bool, 2> audible {{
            (sequencerOn[0] && heldKeyCount[0] > 0) || activePreview,
            (sequencerOn[1] && heldKeyCount[1] > 0) || activePreview
        }};
        const std::array<double, 2> origins {{
            activePreview ? previewOriginPpq : triggerOriginPpq[0],
            activePreview ? previewOriginPpq : triggerOriginPpq[1]
        }};
        const std::array<int, 2> transposes {{
            activePreview ? lastKeyboardNote[0] - 60 : keyboardTranspose[0],
            activePreview ? lastKeyboardNote[1] - 60 : keyboardTranspose[1]
        }};
        juce::MidiBuffer generated;
        sequencerPlayback.generate (generated, sequenceBanks,
            { currentPattern (0), currentPattern (1) }, audible, origins, transposes,
            playing || activePreview || audible[0] || audible[1],
            ppq + beginSample * beatsPerSample, bpm, sampleRateHz, segmentLength,
            stepEvents, stepEventCount, beginSample);
        engineMidi.addEvents (generated, 0, segmentLength, beginSample);
        // Preserve each lane internally while sending MIDI out on its configured
        // channel. In Omni, keep the original lane default (1 or 2).
        for (const auto event : generated)
        {
            const auto lane = event.getMessage().getChannel() - 1;
            if (lane < 0 || lane >= 2) continue;
            const int outputChannel = inputChannels[static_cast<size_t> (lane)] == 0
                ? lane + 1 : inputChannels[static_cast<size_t> (lane)];
            auto outputMessage = event.getMessage();
            outputMessage.setChannel (outputChannel);
            midi.addEvent (outputMessage, beginSample + event.samplePosition);
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
        if (activeLockStep[voice] >= 0 && activeLockPattern[voice] == currentPattern (voice))
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
        const auto level = std::clamp (effective[lb303::outputLevelOffset], 0.0f, 1.0f);
        const auto pan = std::clamp (
            effective[lb303::outputPanOffset] / 100.0f, -1.0f, 1.0f);
        leftGain[voice] = level * (pan > 0.0f ? 1.0f - pan : 1.0f);
        rightGain[voice] = level * (pan < 0.0f ? 1.0f + pan : 1.0f);
    };
    const auto applyFxParameters = [&]
    {
        lb303::SharedEffects::Controls fxControls {};
        for (size_t i = 0; i < fxControls.size(); ++i)
            fxControls[i] = value (lb303::fxParameterIndex (i));
        // Historic B1 delay parameter IDs now control ONE master classic delay.
        lb303::SharedEffects::ClassicParameters legacy {};
        for (size_t i = 0; i < legacy.size(); ++i)
            legacy[i] = value (lb303::parameterIndex (0, i));
        // Preserve older B1 per-step delay locks as automations of that shared delay.
        if (activeLockStep[0] >= 0 && activeLockPattern[0] == currentPattern (0))
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

    // Independently scheduled lock boundaries may be emitted for paused steps,
    // too. Sort them for exact sample order between the two MIDI channels.
    std::sort (stepEvents.begin(), stepEvents.begin() + stepEventCount,
        [] (const auto& a, const auto& b)
        {
            if (a.sample != b.sample) return a.sample < b.sample;
            return a.lane < b.lane;
        });

    if (audio.getNumChannels() < 2) { audio.clear(); return; }
    auto* l = audio.getWritePointer (0);
    auto* r = audio.getWritePointer (1);
    int nextSample = 0;
    int nextStepEvent = 0;
    const auto renderSamplesTo = [&] (int end)
    {
        for (; nextSample < end; ++nextSample)
        {
            std::array<std::pair<float, float>, lb303::basslineCount> stems {};
            for (size_t voice = 0; voice < lb303::basslineCount; ++voice)
            {
                const auto generated = engines[voice].tick (0.0f, 0.0f);
                stems[voice] = { generated.first * leftGain[voice],
                                 generated.second * rightGain[voice] };
            }
            const auto fx = sharedFx.process (stems[0].first, stems[0].second,
                                              stems[1].first, stems[1].second);
            const float outL = l[nextSample] + fx.first;
            const float outR = r[nextSample] + fx.second;
            l[nextSample] = outL;
            r[nextSample] = outR;
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
    for (int channel = 2; channel < audio.getNumChannels(); ++channel)
        audio.clear (channel, 0, count);
}

juce::AudioProcessorEditor* LB303AudioProcessor::createEditor()
{
    return new LB303AudioProcessorEditor (*this);
}
void LB303AudioProcessor::getStateInformation (juce::MemoryBlock& data)
{
    if (auto xml = parameters.copyState().createXml())
    {
        // Two independent banks, not 16 joint patterns. Save all 32 patterns.
        auto* bank1 = xml->createNewChildElement ("LB303PatternBank1");
        bank1->setAttribute ("data", sequenceBanks[0].serialiseToBase64());
        auto* bank2 = xml->createNewChildElement ("LB303PatternBank2");
        bank2->setAttribute ("data", sequenceBanks[1].serialiseToBase64());
        copyXmlToBinary (*xml, data);
    }
}
void LB303AudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
        {
            for (int lane = 0; lane < 2; ++lane)
            {
                const auto tag = juce::String (lane == 0 ? "LB303PatternBank1" : "LB303PatternBank2");
                if (auto* bank = xml->getChildByName (tag))
                    sequenceBanks[static_cast<size_t> (lane)].restoreFromBase64 (
                        bank->getStringAttribute ("data"));
                else
                    sequenceBanks[static_cast<size_t> (lane)].reset(); // legacy TEST1 projects
            }
            if (auto* a = xml->getChildByName ("LB303PatternBank1")) xml->removeChildElement (a, true);
            if (auto* b = xml->getChildByName ("LB303PatternBank2")) xml->removeChildElement (b, true);
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
            sequencerPlayback.reset();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LB303AudioProcessor();
}
