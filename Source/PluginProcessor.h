// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "BassEngine.h"
#include "SharedEffects.h"
#include "Oversampling.h"
#include "SequencerState.h"
#include "SequencerPlayback.h"
#include "SongArrangement.h"
#include "PresetManager.h"
#include <array>
#include <atomic>
#include <cstdint>

class LB303AudioProcessor final : public juce::AudioProcessor,
                                 private juce::AudioProcessorValueTreeState::Listener
{
public:
    LB303AudioProcessor();
    ~LB303AudioProcessor() override;
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
    void resetToInitialState(); // Alt+I: full synth, FX, MIDI, all 32 patterns

    // Editor navigation belongs to this live processor instance, not to
    // the host project or a .lb preset. Recreating this instance starts clean.
    struct EditorSessionState
    {
        int activeContext = 0;
        std::array<int, lb303::basslineCount + 2> selectedParameters {};
        int sequencerPage = 0;
        int stepWidthIndex = 0;
        size_t fxSelectedGlobal = lb303::parameterIndex (0, 31); // per-instance, never serialized
        int fxSelectedColumn = 0;
        // Space routing is UI history for THIS processor instance only.
        // Never write it into VST project state or .lb preset snapshots.
        bool spaceControlsPreview = false; // default: REAPER transport
    };
    EditorSessionState editorSession;

    juce::AudioProcessorValueTreeState parameters;
    lb303::PresetManager presetManager;
    lb303::SongArrangement songs;
    // Alt+Shift+M auditions the focused Song row; Alt+M returns to the global Pattern/Song preview.
    void auditionSongPattern (int lane, int pattern) noexcept;
    void endSongAudition() noexcept
    { auditionLane.store (-1, std::memory_order_release); }
    int songAuditionLane() const noexcept { return auditionLane.load (std::memory_order_acquire); }
    bool isPreviewPlaying() const noexcept { return previewEnabled.load (std::memory_order_acquire); }
    void restartSongPreviewClock() noexcept
    { previewRestartEpoch.fetch_add (1, std::memory_order_release); }
    float value (size_t index) const;
    juce::RangedAudioParameter* parameter (size_t index);
    // Independent patterns, independent MIDI input channels (Omni or 1..16).
    int currentPattern (size_t lane) const noexcept;
    // Ten exposed sequencer knobs per lane edit the currently selected pattern,
    // while the 32 independent SequencerConfig snapshots remain authoritative.
    void syncSelectedPatternPerformance (bool force = false);
    void invalidatePerformanceDisplay() noexcept;
    // One preview transport for BOTH basslines, regardless of edited track.
    void requestSequencerPreview (bool enabled) noexcept
    { previewEnabled.store (enabled, std::memory_order_release);
      if (! enabled) auditionLane.store (-1, std::memory_order_release); }
    bool spaceControlsSequencerPreview() const noexcept
    { return editorSession.spaceControlsPreview; }
    // Returns true when Space now controls the two-bassline preview.
    bool toggleSpaceTarget() noexcept
    {
        editorSession.spaceControlsPreview = ! editorSession.spaceControlsPreview;
        // Never leave a preview sounding with its stop key assigned to REAPER.
        if (! editorSession.spaceControlsPreview) requestSequencerPreview (false);
        return editorSession.spaceControlsPreview;
    }
    // Preview belongs to the processor, not to the Alt+Q editor window.
    bool toggleSequencerPreview() noexcept
    {
        const bool next = ! previewEnabled.load (std::memory_order_acquire);
        previewEnabled.store (next, std::memory_order_release);
        if (! next) auditionLane.store (-1, std::memory_order_release);
        return next;
    }
    lb303::SequencerState& patternBank (size_t lane) noexcept { return sequenceBanks[lane == 0 ? 0 : 1]; }
    const lb303::SequencerState& patternBank (size_t lane) const noexcept { return sequenceBanks[lane == 0 ? 0 : 1]; }

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    std::atomic<bool> mirroringPerformance { false };
    std::atomic<bool> restoringPerformance { false };
    std::array<int, 2> mirroredPatterns {{ -1, -1 }}; // message/UI thread only
    std::array<lb303::BassEngine, lb303::basslineCount> engines;
    lb303::SharedEffects sharedFx;
    std::array<lb303::VoiceDecimator, lb303::basslineCount> voiceDecimators;
    int currentOversamplingFactor = 1; // audio thread only
    // Once a voice is released, continue the wet graph until delayed repeats
    // and reverberation have had ample time to decay. Never sleep while a
    // live voice or measurable wet output exists.
    int wetTailGuardSamples = 0; // audio thread only, no allocation
    std::array<lb303::SequencerState, lb303::basslineCount> sequenceBanks;
    lb303::SequencerPlayback sequencerPlayback;
    // Last running step per lane. -1 means the raw, host-automated baseline.
    // These are not VST parameters and never overwrite host automation.
    std::array<int, lb303::basslineCount> activeLockStep {{ -1, -1 }};
    std::array<int, lb303::basslineCount> activeLockPattern {{ -1, -1 }};
    // Detect new Song rows even when two consecutive rows both reference P1.
    std::array<std::int64_t, lb303::basslineCount> previousLockSongEntry {{ -9, -9 }};
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
    std::atomic<int> auditionLane { -1 };
    std::atomic<int> auditionPattern { 0 };
    std::atomic<unsigned int> previewRestartEpoch { 0 };
    unsigned int consumedPreviewRestartEpoch = 0; // audio thread only
    std::atomic<bool> resetRuntimeRequested { false };
    std::array<std::atomic<float>*, lb303::parameterCount> rawParameters {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LB303AudioProcessor)
};
