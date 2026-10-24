// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

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

void LB303AudioProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    double bpm = 120.0;
    if (auto* playhead = getPlayHead())
        if (const auto position = playhead->getPosition())
            if (const auto hostBpm = position->getBpm())
                bpm = *hostBpm;

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
        copyXmlToBinary (*xml, data);
}
void LB303AudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LB303AudioProcessor();
}
