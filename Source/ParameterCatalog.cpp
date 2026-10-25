// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ParameterCatalog.h"
#include <cmath>

namespace lb303
{
namespace
{
constexpr const char* waveChoices[] = { "Saw", "Square" };
constexpr const char* portaChoices[] = { "Always", "Legato" };
constexpr const char* transientChoices[] = { "Loose", "Neutral", "Tight" };
constexpr const char* syncChoices[] = { "Off", "1/16", "1/12T", "1/8", "1/6T", "1/4", "1/3T", "1/2", "1/1" };
constexpr const char* delayModeChoices[] = { "Normal", "Ping-Pong" };
constexpr const char* responseChoices[] = { "Gate", "Lowpass Gate" };
constexpr const char* patternChoices[] = { "1", "2", "3", "4", "5", "6", "7", "8",
                                          "9", "10", "11", "12", "13", "14", "15", "16" };
constexpr const char* switchChoices[] = { "Off", "On" };
constexpr const char* trackChoices[] = { "Bassline 1", "Bassline 2" };
constexpr const char* fxDelayChoices[] = { "Off", "LB-303 Classic", "LJuno Dual Tape" };
constexpr const char* fxSyncChoices[] = { "Off", "1/32", "1/24T", "1/16", "1/12T", "1/8", "1/6T", "1/4", "1/3T", "1/2", "1/1" };
constexpr const char* fxDistChoices[] = { "Off", "Soft Clip", "Warm Tube", "Tape Drive", "Classic Overdrive", "Hard Clip", "Diode Crunch", "Thick Fuzz", "Wave Fold", "Rectifier", "Bit Crush", "Sine Warp", "Broken Radio" };
constexpr const char* fxOrderChoices[] = { "Delay then Reverb", "Reverb then Delay" };
constexpr const char* fxReverbChoices[] = { "Off", "Room", "Plate", "Hall", "Cathedral" };
constexpr const char* midiChannelChoices[] = { "Omni", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                                               "10", "11", "12", "13", "14", "15", "16" };
// Group tone, envelopes and response; delay is accessible only in shared FX.
constexpr std::array<size_t, voiceVisibleParameterCount> voiceDisplayOrder {{
    35, 0, 36, 7, 8, 9, 10, 14,
    1, 2, 3, 4, 5, 6, 15, 16,
    17, 18, 33, 34, 11, 12, 13
}};
// One classic delay: reuse B1's 14 historic, stable VST3 IDs; all other FX append new IDs.
// Historic B2 delay params remain in the state for legacy loading, not in either grid.
constexpr std::array<size_t, fxDisplayParameterCount> fxDisplayOrder {{
    31, // Dry Level for both basslines, never a delay parameter.
    // Physical insert order: distortion -> chorus -> delay/reverb.
    fxParameterIndex (fx::distortionType), fxParameterIndex (fx::distortionAmount),
    fxParameterIndex (fx::distortionHarmonics), fxParameterIndex (fx::distortion2Type),
    fxParameterIndex (fx::distortion2Amount), fxParameterIndex (fx::distortion2Harmonics),
    fxParameterIndex (fx::chorusOn), fxParameterIndex (fx::chorusLevel),
    fxParameterIndex (fx::chorusRate), fxParameterIndex (fx::chorusWidth),
    fxParameterIndex (fx::chorusSend1), fxParameterIndex (fx::chorusSend2),
    // Select which serial effect comes first; the one downstream also hears
    // the output of the upstream effect instead of two parallel returns.
    fxParameterIndex (fx::effectOrder),
    fxParameterIndex (fx::delayType), fxParameterIndex (fx::delaySend1), fxParameterIndex (fx::delaySend2),
    19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 32,
    fxParameterIndex (fx::dualTime), fxParameterIndex (fx::dualSync),
    fxParameterIndex (fx::dualSpeed1), fxParameterIndex (fx::dualSpeed2),
    fxParameterIndex (fx::dualGlide), fxParameterIndex (fx::dualFeedback1),
    fxParameterIndex (fx::dualFeedback2), fxParameterIndex (fx::dualTone1),
    fxParameterIndex (fx::dualTone2), fxParameterIndex (fx::dualSpread),
    fxParameterIndex (fx::dualDrive),
    fxParameterIndex (fx::reverbType), fxParameterIndex (fx::reverbSend1),
    fxParameterIndex (fx::reverbSend2), fxParameterIndex (fx::reverbPredelay),
    fxParameterIndex (fx::reverbDecay), fxParameterIndex (fx::reverbDamping),
    fxParameterIndex (fx::reverbWidth), fxParameterIndex (fx::reverbLevel)
}};
// All indices are relative to audioParameterCount, including append-only
// performance controls located AFTER the shared FX parameters.
constexpr size_t perfRelative (size_t lane, size_t control)
{ return seqPerformanceIndex (lane, control) - audioParameterCount; }
constexpr std::array<size_t, sequencerDisplayParameterCount> seqDisplayOrder {{
    2, 0, 5,
    perfRelative (0, seqPerformance::bpmDivision), perfRelative (0, seqPerformance::shuffle),
    perfRelative (0, seqPerformance::legato), perfRelative (0, seqPerformance::skipProbability),
    perfRelative (0, seqPerformance::noteRandom), perfRelative (0, seqPerformance::velocityRandom),
    perfRelative (0, seqPerformance::lengthRandom), perfRelative (0, seqPerformance::repeatRandom),
    perfRelative (0, seqPerformance::octaveShift), perfRelative (0, seqPerformance::semitoneShift),
    3, 1, 6,
    perfRelative (1, seqPerformance::bpmDivision), perfRelative (1, seqPerformance::shuffle),
    perfRelative (1, seqPerformance::legato), perfRelative (1, seqPerformance::skipProbability),
    perfRelative (1, seqPerformance::noteRandom), perfRelative (1, seqPerformance::velocityRandom),
    perfRelative (1, seqPerformance::lengthRandom), perfRelative (1, seqPerformance::repeatRandom),
    perfRelative (1, seqPerformance::octaveShift), perfRelative (1, seqPerformance::semitoneShift),
    4
}};
}};
}

const std::array<ParameterInfo, parameterCount>& catalog()
{
    static const std::array<ParameterInfo, synthParameterCount> originals {{
        { "slider001", "Waveform", 0, 1, 1, 1, "", waveChoices, 2 },
        { "slider002", "Attack", 0, 0.05f, 0.001f, 0, "s" },
        { "slider003", "Decay", 0, 0.2f, 0.001f, 0.1f, "s" },
        { "slider004", "Sustain (Filter only)", 0, 0.99f, 0.01f, 0.14f, "" },
        { "slider005", "Release", 0, 0.2f, 0.001f, 0.03f, "s" },
        { "slider006", "Portamento Time", 0, 100, 0.5f, 10, "" },
        { "slider007", "Portamento Mode", 0, 1, 1, 1, "", portaChoices, 2 },
        { "slider008", "Volume", 0, 1, 0.01f, 0.8f, "" },
        { "slider009", "Cutoff", 0, 1, 0.01f, 0.6f, "" },
        { "slider010", "Resonance", 0, 0.1f, 0.001f, 0.1f, "" },
        { "slider011", "Filter Env Depth", 0, 1, 0.01f, 0.3f, "" },
        { "slider012", "PWM Rate", 0, 128, 0.125f, 1, "" },
        { "slider013", "PWM Depth", 0, 0.5f, 0.001f, 0.1f, "" },
        { "slider014", "Soft Clip Drive", 0, 4, 0.01f, 2, "" },
        { "slider015", "Master Tone", -48, 48, 0.1f, 0, "st" },
        { "slider016", "Accent Threshold", 80, 120, 1, 120, "" },
        { "slider017", "Accent Filter Amount", 0, 0.7f, 0.01f, 0.5f, "" },
        { "slider018", "Velocity", 0, 127, 1, 0, "" },
        { "slider019", "Transient Response", 0, 2, 1, 1, "", transientChoices, 3 },
        { "slider020", "Delay Time", 1, 10000, 1, 200, "ms" },
        { "slider021", "Delay Tempo Sync", 0, 8, 1, 0, "", syncChoices, 9 },
        { "slider022", "Delay Feedback", 0, 100, 1, 10, "%" },
        { "slider023", "Delay Mode", 0, 1, 1, 1, "", delayModeChoices, 2 },
        { "slider024", "Delay Spread", -100, 100, 1, 0, "%" },
        { "slider025", "Delay Low Pass", 20, 20000, 1, 16000, "Hz", nullptr, 0, true },
        { "slider026", "Delay Low Pass Q", 0.707f, 4, 0.01f, 0.707f, "", nullptr, 0, true },
        { "slider027", "Delay High Pass", 20, 20000, 1, 60, "Hz", nullptr, 0, true },
        { "slider028", "Delay High Pass Q", 0.707f, 4, 0.01f, 0.707f, "", nullptr, 0, true },
        { "slider029", "Delay Modulation Amp", 0, 100, 1, 0, "" },
        { "slider030", "Delay Modulation Freq", 1, 8, 1, 5, "Hz" },
        { "slider031", "Delay Transpose", -12, 12, 1, 0, "st" },
        { "slider032", "Dry Level", 0, 100, 1, 100, "%" },
        { "slider033", "Delay Wet", 0, 100, 1, 10, "%" },
        { "slider034", "Response Mode", 0, 1, 1, 0, "", responseChoices, 2 },
        { "slider035", "Gate Velocity Filter", 0, 127, 1, 0, "" }
    }};
    // The first 35 IDs are intentionally unchanged for old REAPER projects.
    // Bassline 2 gets entirely independent IDs; never duplicate the originals.
    static const std::array<ParameterInfo, parameterCount> values = [&]
    {
        std::array<ParameterInfo, parameterCount> result {};
        for (size_t voice = 0; voice < basslineCount; ++voice)
        {
            const auto base = parameterIndex (voice, 0);
            for (size_t i = 0; i < synthParameterCount; ++i)
            {
                result[base + i] = originals[i];
                if (voice == 1)
                    result[base + i].id = juce::String ("b2_") + originals[i].id;
            }
            result[base + outputLevelOffset] = {
                voice == 0 ? "b1_level" : "b2_level", "Level",
                0.0f, 1.0f, 0.01f, 1.0f, "" };
            result[base + outputPanOffset] = {
                voice == 0 ? "b1_pan" : "b2_pan", "Pan",
                -100.0f, 100.0f, 1.0f, 0.0f, "%" };
        }
        result[pattern1Index] = { "seq_b1_pattern", "Pattern Bassline 1", 0, 15, 1, 0, "", patternChoices, 16 };
        result[pattern2Index] = { "seq_b2_pattern", "Pattern Bassline 2", 0, 15, 1, 0, "", patternChoices, 16 };
        result[sequencer1EnabledIndex] = { "seq_b1_enable", "Sequencer Bassline 1", 0, 1, 1, 0, "", switchChoices, 2 };
        result[sequencer2EnabledIndex] = { "seq_b2_enable", "Sequencer Bassline 2", 0, 1, 1, 0, "", switchChoices, 2 };
        result[editTrackIndex] = { "seq_edit_track", "Editor Track", 0, 1, 1, 0, "", trackChoices, 2 };
        result[midiChannel1Index] = { "b1_midi_channel", "MIDI Channel Bassline 1", 0, 16, 1, 1, "", midiChannelChoices, 17 };
        result[midiChannel2Index] = { "b2_midi_channel", "MIDI Channel Bassline 2", 0, 16, 1, 2, "", midiChannelChoices, 17 };
        const auto addFx = [&] (size_t i, const char* id, const char* name,
                                float lo, float hi, float step, float initial,
                                const char* unit = "", const char* const* choices = nullptr, int count = 0)
        {
            result[fxParameterIndex (i)] = { id, name, lo, hi, step, initial, unit, choices, count };
        };
        addFx (fx::delayType, "fx_delay_type", "Delay Type", 0, 2, 1, 1, "", fxDelayChoices, 3);
        addFx (fx::delaySend1, "fx_delay_send1", "Delay Send Bassline 1", 0, 100, 1, 100, "%");
        addFx (fx::delaySend2, "fx_delay_send2", "Delay Send Bassline 2", 0, 100, 1, 100, "%");
        addFx (fx::dualTime, "fx_dual_time", "Dual Delay Time", 20, 2000, 1, 270, "ms");
        addFx (fx::dualSync, "fx_dual_sync", "Dual Delay Sync", 0, 10, 1, 6, "", fxSyncChoices, 11);
        addFx (fx::dualSpeed1, "fx_dual_speed1", "Dual Delay Speed 1", 0.25f, 4, 0.01f, 1, "x");
        addFx (fx::dualSpeed2, "fx_dual_speed2", "Dual Delay Speed 2", 0.25f, 4, 0.01f, 1.5f, "x");
        addFx (fx::dualGlide, "fx_dual_glide", "Dual Delay Glide", 1, 1500, 1, 180, "ms");
        addFx (fx::dualFeedback1, "fx_dual_feedback1", "Dual Delay Feedback 1", 0, 200, 1, 75, "%");
        addFx (fx::dualFeedback2, "fx_dual_feedback2", "Dual Delay Feedback 2", 0, 200, 1, 75, "%");
        addFx (fx::dualTone1, "fx_dual_tone1", "Dual Delay Tone 1", 0, 200, 1, 50, "%");
        addFx (fx::dualTone2, "fx_dual_tone2", "Dual Delay Tone 2", 0, 200, 1, 50, "%");
        addFx (fx::dualSpread, "fx_dual_spread", "Dual Delay Stereo Spread", -100, 100, 1, 75, "%");
        addFx (fx::dualDrive, "fx_dual_drive", "Dual Delay Tape Drive", 0, 100, 1, 20, "%");
        addFx (fx::chorusOn, "fx_chorus_on", "Chorus", 0, 1, 1, 0, "", switchChoices, 2);
        addFx (fx::chorusLevel, "fx_chorus_level", "Chorus Level", 0, 100, 1, 50, "%");
        addFx (fx::chorusRate, "fx_chorus_rate", "Chorus Rate", 0, 100, 1, 30, "%");
        addFx (fx::chorusWidth, "fx_chorus_width", "Chorus Width", 0, 100, 1, 100, "%");
        addFx (fx::chorusSend1, "fx_chorus_send1", "Chorus Send Bassline 1", 0, 100, 1, 35, "%");
        addFx (fx::chorusSend2, "fx_chorus_send2", "Chorus Send Bassline 2", 0, 100, 1, 35, "%");
        // Keep original IDs for Bassline 1: old projects and .lb presets remain compatible.
        addFx (fx::distortionType, "fx_distortion_type", "Distortion 1 Type", 0, 12, 1, 0, "", fxDistChoices, 13);
        addFx (fx::distortionAmount, "fx_distortion_amount", "Distortion 1 Amount", 0, 100, 1, 0, "%");
        addFx (fx::distortionHarmonics, "fx_distortion_harmonics", "Distortion 1 Harmonics Even Odd", -100, 100, 1, 0, "%");
        // New voice-specific processor. Append rather than shifting any existing ID.
        addFx (fx::distortion2Type, "fx_distortion_2_type", "Distortion 2 Type", 0, 12, 1, 0, "", fxDistChoices, 13);
        addFx (fx::distortion2Amount, "fx_distortion_2_amount", "Distortion 2 Amount", 0, 100, 1, 0, "%");
        addFx (fx::distortion2Harmonics, "fx_distortion_2_harmonics", "Distortion 2 Harmonics Even Odd", -100, 100, 1, 0, "%");
        addFx (fx::reverbType, "fx_reverb_type", "Reverb Type", 0, 4, 1, 0, "", fxReverbChoices, 5);
        addFx (fx::reverbSend1, "fx_reverb_send1", "Reverb Send Bassline 1", 0, 100, 1, 25, "%");
        addFx (fx::reverbSend2, "fx_reverb_send2", "Reverb Send Bassline 2", 0, 100, 1, 25, "%");
        addFx (fx::reverbPredelay, "fx_reverb_predelay", "Reverb Predelay", 0, 180, 1, 30, "ms");
        addFx (fx::reverbDecay, "fx_reverb_decay", "Reverb Decay", 0.2f, 20, 0.1f, 3.0f, "s");
        addFx (fx::reverbDamping, "fx_reverb_damping", "Reverb Damping", 500, 16000, 100, 7500, "Hz");
        addFx (fx::reverbWidth, "fx_reverb_width", "Reverb Width", 0, 100, 1, 100, "%");
        addFx (fx::reverbLevel, "fx_reverb_level", "Reverb Level", 0, 100, 1, 50, "%");
        addFx (fx::effectOrder, "fx_delay_reverb_order", "Delay Reverb Order",
               0, 1, 1, 0, "", fxOrderChoices, 2);
        for (size_t lane = 0; lane < basslineCount; ++lane)
        {
            const auto base = lane == 0 ? "seq_b1_" : "seq_b2_";
            const auto suffix = lane == 0 ? " Bassline 1" : " Bassline 2";
            const auto addSeq = [&] (size_t control, const char* shortId, const char* name,
                                     float lo, float hi, float step, float initial,
                                     const char* const* choices = nullptr, int count = 0)
            {
                result[seqPerformanceIndex (lane, control)] = {
                    juce::String (base) + shortId, juce::String (name) + suffix,
                    lo, hi, step, initial, "", choices, count };
            };
            addSeq (seqPerformance::bpmDivision, "bpm_division", "BPM Division", .03125f, 64, .03125f, 4);
            addSeq (seqPerformance::shuffle, "shuffle", "Shuffle", 0, 1, .01f, 0);
            addSeq (seqPerformance::legato, "legato", "Legato", 0, 1, 1, 0, switchChoices, 2);
            addSeq (seqPerformance::skipProbability, "skip_probability", "Note Skip Probability", 0, 1, .01f, 0);
            addSeq (seqPerformance::noteRandom, "note_random", "Note Random Depth", 0, 1, .01f, 0);
            addSeq (seqPerformance::velocityRandom, "velocity_random", "Velocity Random Depth", 0, 1, .01f, 0);
            addSeq (seqPerformance::lengthRandom, "length_random", "Length Random Depth", 0, 1, .01f, 0);
            addSeq (seqPerformance::repeatRandom, "repeat_random", "Repeat Random Depth", 0, 1, .01f, 0);
            addSeq (seqPerformance::octaveShift, "octave_shift", "Global Octave Shift", -4, 4, 1, 0);
            addSeq (seqPerformance::semitoneShift, "semitone_shift", "Global Semitone Shift", -12, 12, 1, 0);
        }
        return result;
    }();
    return values;
}

size_t fxDisplayGlobalIndex (size_t displayIndex) noexcept
{
    return fxDisplayOrder[juce::jmin (displayIndex, fxDisplayOrder.size() - 1)];
}

size_t displayToLocal (size_t context, size_t displayIndex) noexcept
{
    if (context < basslineCount)
        return voiceDisplayOrder[juce::jmin (displayIndex, voiceDisplayOrder.size() - 1)];
    return seqDisplayOrder[juce::jmin (displayIndex, seqDisplayOrder.size() - 1)];
}

size_t localToDisplay (size_t context, size_t localIndex) noexcept
{
    if (context < basslineCount)
    {
        for (size_t i = 0; i < voiceDisplayOrder.size(); ++i)
            if (voiceDisplayOrder[i] == localIndex) return i;
    }
    else
    {
        for (size_t i = 0; i < seqDisplayOrder.size(); ++i)
            if (seqDisplayOrder[i] == localIndex) return i;
    }
    return 0;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout result;
    for (size_t index = 0; index < catalog().size(); ++index)
    {
        const auto& p = catalog()[index];
        // Distinguish automated parameters in REAPER's global FX parameter list,
        // without adding redundant prefixes to the accessible per-bassline grids.
        const auto hostName = index == parameterIndex (0, 31)
            ? ("FX - " + p.name) // Historic slider032 ID, but now global direct bus.
            : (index < audioParameterCount
                ? ("Bassline " + juce::String (1 + index / voiceParameterCount) + " - " + p.name)
                : ((index < legacyParameterCount || index >= sequencerPerformanceBase)
                    ? ("Sequencer - " + p.name) : ("FX - " + p.name)));
        if (p.choiceCount)
        {
            juce::StringArray options;
            for (int i = 0; i < p.choiceCount; ++i)
                options.add (p.choices[i]);
            result.add (std::make_unique<juce::AudioParameterChoice> (
                juce::ParameterID { p.id, 1 }, hostName, options, static_cast<int> (p.initial)));
        }
        else
        {
            juce::NormalisableRange<float> range (p.minimum, p.maximum, p.step);
            if (p.logarithmic)
                range.setSkewForCentre (p.minimum == 20.0f ? 1000.0f : 1.5f);
            result.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { p.id, 1 }, hostName, range, p.initial));
        }
    }
    return result;
}

juce::String parameterText (size_t index, float plainValue)
{
    const auto& p = catalog().at (index);
    if (p.choiceCount != 0)
        return p.choices[juce::jlimit (0, p.choiceCount - 1, juce::roundToInt (plainValue))];
    int places = p.step >= 1.0f ? 0 : p.step >= 0.1f ? 1 : p.step >= 0.01f ? 2 : 3;
    juce::String result (plainValue, places);
    if (p.unit[0] != '\0')
        result += juce::String (" ") + p.unit;
    return result;
}
}
