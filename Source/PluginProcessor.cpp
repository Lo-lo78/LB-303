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
    engine.prepare (sampleRate);
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
    lb303::BassEngine::Values snapshot {};
    for (size_t i = 0; i < snapshot.size(); ++i) snapshot[i] = value (i);
    double bpm = 120.0;
    if (auto* playhead = getPlayHead())
        if (const auto position = playhead->getPosition())
            bpm = position->getBpm().value_or (120.0);
    engine.setParameters (snapshot, bpm);
    const auto count = audio.getNumSamples();
    if (audio.getNumChannels() < 2) { audio.clear(); return; }
    auto* l = audio.getWritePointer (0);
    auto* r = audio.getWritePointer (1);
    int nextSample = 0;
    const auto renderTo = [&] (int end)
    {
        for (; nextSample < end; ++nextSample)
        {
            const auto rendered = engine.tick (l[nextSample], r[nextSample]);
            l[nextSample] = rendered.first;
            r[nextSample] = rendered.second;
        }
    };
    // MIDI event offsets are preserved at single-sample resolution, including
    // note-off followed by note-on on the same audio block boundary.
    for (const auto entry : midi)
    {
        const auto offset = std::clamp (entry.samplePosition, 0, count);
        renderTo (offset);
        engine.midi (entry.getMessage());
    }
    renderTo (count);
    for (int channel = 2; channel < audio.getNumChannels(); ++channel)
        audio.clear (channel, 0, count);
    // Like the original JSFX, MIDI input is forwarded unchanged to the host.
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
