// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "BassEngine.h"
#include "SharedEffects.h"
#include "SequencerState.h"
#include "SequencerPlayback.h"
#include "PresetManager.h"
#include <array>
#include <atomic>
#include <cstdint>

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
    double getTailLengthSeconds() const override { return 30.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    lb303::PresetManager presetManager;
    float value (size_t index) const;
    juce::RangedAudioParameter* parameter (size_t index);
    // Independent patterns, independent MIDI input channels (Omni or 1..16).
    int currentPattern (size_t lane) const noexcept;
    // One preview transport for BOTH basslines, regardless of edited track.
    void requestSequencerPreview (bool enabled) noexcept
    { previewEnabled.store (enabled, std::memory_order_relaxed); }
    lb303::SequencerState& patternBank (size_t lane) noexcept { return sequenceBanks[lane == 0 ? 0 : 1]; }
    const lb303::SequencerState& patternBank (size_t lane) const noexcept { return sequenceBanks[lane == 0 ? 0 : 1]; }

private:
    std::array<lb303::BassEngine, lb303::basslineCount> engines;
    lb303::SharedEffects sharedFx;
    std::array<lb303::SequencerState, lb303::basslineCount> sequenceBanks;
    lb303::SequencerPlayback sequencerPlayback;
    // Last running step per lane. -1 means the raw, host-automated baseline.
    // These are not VST parameters and never overwrite host automation.
    std::array<int, lb303::basslineCount> activeLockStep {{ -1, -1 }};
    std::array<int, lb303::basslineCount> activeLockPattern {{ -1, -1 }};
    double sampleRateHz = 44100.0;
    double fallbackPpq = 0.0;
    double previewPpq = 0.0;
    bool previousPreview = false;
    double previewOriginPpq = 0.0;
    // Physical MIDI keys are only triggers when the corresponding sequencer
    // switch is On; the notes themselves must never reach that bassline.
    // Distinguish input channels in Omni mode: two identical pitches arriving
    // from different MIDI channels must not release one another.
    std::array<std::array<std::array<bool, 128>, 16>, lb303::basslineCount> heldKeys {};
    std::array<int, lb303::basslineCount> heldKeyCount {};
    // Last-note priority is independent for each bassline.
    std::array<std::array<std::array<std::uint64_t, 128>, 16>, lb303::basslineCount> heldKeyAge {};
    std::array<std::uint64_t, lb303::basslineCount> keyAgeCounter {{ 1, 1 }};
    std::array<int, lb303::basslineCount> keyboardTranspose {};
    std::array<int, lb303::basslineCount> lastKeyboardNote {{ 60, 60 }};
    std::array<double, lb303::basslineCount> triggerOriginPpq {};
    std::array<bool, lb303::basslineCount> previousSequencerOn {};
    std::array<int, lb303::basslineCount> previousMidiChannel {{ 1, 2 }};
    std::atomic<bool> previewEnabled { false };
    std::array<std::atomic<float>*, lb303::parameterCount> rawParameters {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LB303AudioProcessor)
};
