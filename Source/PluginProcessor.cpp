// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

LB303AudioProcessor::LB303AudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput ("Audio input", juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "LB303State", lb303::createParameterLayout())
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
    previewLane.store (-1, std::memory_order_relaxed);
    sequencerPlayback.reset();
    for (auto& engine : engines) engine.prepare (sampleRate);
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
    const int previewingTrack = previewLane.load (std::memory_order_relaxed);
    const bool activePreview = previewingTrack >= 0 && previewingTrack < 2;
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

    // On changing modes, release a note held by the previous mode. This also
    // prevents the physical key that selected a pattern from being heard.
    for (size_t lane = 0; lane < lb303::basslineCount; ++lane)
    {
        if (sequencerOn[lane] == previousSequencerOn[lane]) continue;
        engines[lane].releaseNote();
        heldKeys[lane].fill (false);
        heldKeyAge[lane].fill (0);
        heldKeyCount[lane] = 0;
        keyboardTranspose[lane] = 0;
        previousSequencerOn[lane] = sequencerOn[lane];
    }

    // Route physical keys before generating each audio segment. Unlike the
    // earlier implementation, we never merge them into the sequenced notes.
    // Dividing at MIDI offsets makes trigger/release sample-accurate, including
    // when a key begins or ends in the middle of an audio block.
    const juce::MidiBuffer physicalInput (midi);
    midi.clear();
    auto generateSegment = [&] (int beginSample, int endSample)
    {
        const int segmentLength = endSample - beginSample;
        if (segmentLength <= 0) return;
        const std::array<bool, 2> audible {{
            (sequencerOn[0] && heldKeyCount[0] > 0) || previewingTrack == 0,
            (sequencerOn[1] && heldKeyCount[1] > 0) || previewingTrack == 1
        }};
        const std::array<double, 2> origins {{
            previewingTrack == 0 ? previewOriginPpq : triggerOriginPpq[0],
            previewingTrack == 1 ? previewOriginPpq : triggerOriginPpq[1]
        }};
        const std::array<int, 2> transposes {{
            previewingTrack == 0 ? lastKeyboardNote[0] - 60 : keyboardTranspose[0],
            previewingTrack == 1 ? lastKeyboardNote[1] - 60 : keyboardTranspose[1]
        }};
        juce::MidiBuffer generated;
        sequencerPlayback.generate (generated, sequenceBanks,
            { currentPattern (0), currentPattern (1) }, audible, origins, transposes,
            playing || activePreview || audible[0] || audible[1],
            ppq + beginSample * beatsPerSample, bpm, sampleRateHz, segmentLength);
        midi.addEvents (generated, 0, segmentLength, beginSample);
    };
    int previousSample = 0;
    for (const auto entry : physicalInput)
    {
        const int offset = std::clamp (entry.samplePosition, 0, count);
        generateSegment (previousSample, offset);
        previousSample = offset;
        const auto& message = entry.getMessage();
        const int midiChannel = message.getChannel();
        if (midiChannel < 1 || midiChannel > static_cast<int> (lb303::basslineCount))
        {
            midi.addEvent (message, offset); // other MIDI channels untouched
            continue;
        }
        const auto lane = static_cast<size_t> (midiChannel - 1);
        if (message.isNoteOn())
            lastKeyboardNote[lane] = juce::jlimit (0, 127, message.getNoteNumber());

        if (sequencerOn[lane] && message.isNoteOnOrOff())
        {
            const int note = juce::jlimit (0, 127, message.getNoteNumber());
            const auto key = static_cast<size_t> (note);
            auto& down = heldKeys[lane][key];
            if (message.isNoteOn())
            {
                if (! down)
                {
                    if (heldKeyCount[lane] == 0)
                        triggerOriginPpq[lane] = ppq + offset * beatsPerSample;
                    down = true;
                    ++heldKeyCount[lane];
                }
                // A newly struck key takes priority without restarting the
                // pattern while an earlier key remains held (LJuno Mono Legato).
                auto& nextAge = keyAgeCounter[lane];
                if (nextAge == 0) // guard the theoretical counter wraparound
                {
                    heldKeyAge[lane].fill (0);
                    nextAge = 1;
                }
                heldKeyAge[lane][key] = nextAge++;
                keyboardTranspose[lane] = note - 60;
            }
            else if (down)
            {
                down = false;
                heldKeyAge[lane][key] = 0;
                --heldKeyCount[lane];
                // On release, return to the newest key still held, as in LJuno.
                int newestNote = 60;
                std::uint64_t newestAge = 0;
                for (int candidate = 0; candidate < 128; ++candidate)
                {
                    const auto index = static_cast<size_t> (candidate);
                    if (heldKeys[lane][index] && heldKeyAge[lane][index] > newestAge)
                    {
                        newestAge = heldKeyAge[lane][index];
                        newestNote = candidate;
                    }
                }
                keyboardTranspose[lane] = newestNote - 60;
            }
            // Physical Note On/Off is consumed: only transposed pattern notes play.
            continue;
        }
        if (sequencerOn[lane] && message.isController()
            && (message.getControllerNumber() == 120
                || message.getControllerNumber() == 123))
        {
            heldKeys[lane].fill (false);
            heldKeyAge[lane].fill (0);
            heldKeyCount[lane] = 0;
            keyboardTranspose[lane] = 0;
        }
        midi.addEvent (message, offset); // controls remain usable in both modes
    }
    generateSegment (previousSample, count);

    // Separate DSP state, oscillator/envelopes/filter/delay and parameter
    // snapshots for BOTH basslines. Never share engine state between them.
    std::array<float, lb303::basslineCount> leftGain {}, rightGain {};
    for (size_t voice = 0; voice < lb303::basslineCount; ++voice)
    {
        lb303::BassEngine::Values snapshot {};
        for (size_t i = 0; i < snapshot.size(); ++i)
            snapshot[i] = value (lb303::parameterIndex (voice, i));
        engines[voice].setParameters (snapshot, bpm);
        const auto level = std::clamp (
            value (lb303::parameterIndex (voice, lb303::outputLevelOffset)), 0.0f, 1.0f);
        const auto pan = std::clamp (
            value (lb303::parameterIndex (voice, lb303::outputPanOffset)) / 100.0f,
            -1.0f, 1.0f);
        // Centre pan has unity gain on both channels, so Bassline 1 alone
        // remains sonically identical to TEST1.5 at its default settings.
        leftGain[voice] = level * (pan > 0.0f ? 1.0f - pan : 1.0f);
        rightGain[voice] = level * (pan < 0.0f ? 1.0f + pan : 1.0f);
    }

    if (audio.getNumChannels() < 2) { audio.clear(); return; }
    auto* l = audio.getWritePointer (0);
    auto* r = audio.getWritePointer (1);
    int nextSample = 0;
    const auto renderTo = [&] (int end)
    {
        for (; nextSample < end; ++nextSample)
        {
            float outL = l[nextSample], outR = r[nextSample];
            for (size_t voice = 0; voice < lb303::basslineCount; ++voice)
            {
                // Engine.tick adds incoming audio to its output. Pass zero to
                // both engines and mix the original host input only ONCE.
                const auto generated = engines[voice].tick (0.0f, 0.0f);
                outL += generated.first * leftGain[voice];
                outR += generated.second * rightGain[voice];
            }
            l[nextSample] = outL;
            r[nextSample] = outR;
        }
    };
    // Route generated sequencer notes or direct notes exclusively to their
    // respective engines (MIDI channel 1 or 2), preserving exact offsets.
    // MIDI output contains generated notes, never a doubled physical note.
    for (const auto entry : midi)
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
