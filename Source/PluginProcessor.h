// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "BassEngine.h"
#include <array>
#include <atomic>

class LB303AudioProcessor final : public juce::AudioProcessor
{
public:
    LB303AudioProcessor();
    ~LB303AudioProcessor() override = default;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "LB-303"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    float value (size_t index) const;
    juce::RangedAudioParameter* parameter (size_t index);

private:
    std::array<lb303::BassEngine, lb303::basslineCount> engines;
    std::array<std::atomic<float>*, lb303::parameterCount> rawParameters {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LB303AudioProcessor)
};
