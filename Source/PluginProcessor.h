// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "BassEngine.h"
#include "SequencerState.h"
#include "SequencerPlayback.h"
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
    // Track 0: channel 1 / Pattern Bassline 1. Track 1: channel 2 / Pattern Bassline 2.
    int currentPattern (size_t lane) const noexcept;
    void requestSequencerPreview (int lane) noexcept
    { previewLane.store (lane >= 0 && lane < 2 ? lane : -1, std::memory_order_relaxed); }
    lb303::SequencerState& patternBank (size_t lane) noexcept { return sequenceBanks[lane == 0 ? 0 : 1]; }
    const lb303::SequencerState& patternBank (size_t lane) const noexcept { return sequenceBanks[lane == 0 ? 0 : 1]; }

private:
    std::array<lb303::BassEngine, lb303::basslineCount> engines;
    std::array<lb303::SequencerState, lb303::basslineCount> sequenceBanks;
    lb303::SequencerPlayback sequencerPlayback;
    double sampleRateHz = 44100.0;
    double fallbackPpq = 0.0;
    double previewPpq = 0.0;
    bool previousPreview = false;
    std::atomic<int> previewLane { -1 };
    std::array<std::atomic<float>*, lb303::parameterCount> rawParameters {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LB303AudioProcessor)
};
