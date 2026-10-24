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
    if (activePreview && ! previousPreview) previewPpq = 0.0;
    previousPreview = activePreview;
    if (! playing && activePreview) ppq = previewPpq;
    const double nextPpq = ppq + audio.getNumSamples() * bpm / (60.0 * sampleRateHz);
    if (! playing && activePreview) previewPpq = nextPpq;
    else fallbackPpq = nextPpq;
    juce::MidiBuffer generated;
    sequencerPlayback.generate (generated, sequenceBanks,
        { currentPattern (0), currentPattern (1) },
        { (playing && value (lb303::sequencer1EnabledIndex) > 0.5f) || previewingTrack == 0,
          (playing && value (lb303::sequencer2EnabledIndex) > 0.5f) || previewingTrack == 1 },
        playing || activePreview, ppq, bpm, sampleRateHz, audio.getNumSamples());
    // Generated events keep MIDI channels 1 and 2 and are merged with host MIDI.
    midi.addEvents (generated, 0, audio.getNumSamples(), 0);

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

    const auto count = audio.getNumSamples();
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
    // Match MIDI channel numbers to bassline numbers (one-based):
    //   channel 1 -> Bassline 1, channel 2 -> Bassline 2.
    // Route all channel messages (including note-off and CC) only to their
    // respective engine. Channel-less system messages and channels 3-16
    // are not sent to either synthesizer. Preserve sample-accurate offsets.
    // The host's MIDI buffer is forwarded unchanged, as in TEST1.6.
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
