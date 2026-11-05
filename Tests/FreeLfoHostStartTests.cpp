// TEST2.4.68: real BassEngine LFO code, no JUCE dependency or test-only DSP.
// Validate only Free LFO host-start reset, including deterministic Sample & Hold.
#include "BassEngine.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

using lb303::BassEngine;

static BassEngine::LfoValues lfoSettings (int mode, int wave)
{
    BassEngine::LfoValues values {};
    values[lb303::lfo::mode] = static_cast<float> (mode);
    values[lb303::lfo::wave] = static_cast<float> (wave);
    values[lb303::lfo::rate] = 99.0f;
    values[lb303::lfo::phase] = 0.10f;
    return values;
}

static std::vector<float> capture (BassEngine& engine, int samples)
{
    std::vector<float> output;
    output.reserve (static_cast<size_t> (samples));
    for (int n = 0; n < samples; ++n)
    {
        engine.advanceSilentLfo (1);
        output.push_back (engine.currentLfoSample());
    }
    return output;
}

static void prepare (BassEngine& engine, const BassEngine::LfoValues& values)
{
    engine.prepare (44100.0);
    engine.setLfoParameters (values);
    engine.setExternalLfoDestination (true);
}

int main()
{
    for (int wave : { 0, 5 }) // Sine + deterministic Sample & Hold
    {
        BassEngine engine;
        prepare (engine, lfoSettings (0, wave));
        capture (engine, 973); // an arbitrary previous run, no note reset
        engine.restartFreeLfoFromHostTransport();
        const auto first = capture (engine, 1873);
        capture (engine, 347);
        engine.restartFreeLfoFromHostTransport();
        const auto second = capture (engine, 1873);
        assert (first == second);
        assert (first[0] != first[501]);
        if (wave == 5) assert (std::abs (first[0]) > .01f);
    }
    // Triggered LFO is NOT reset by DAW Play; only its MIDI notes reset it.
    {
        BassEngine unchanged, attemptedRestart;
        const auto values = lfoSettings (1, 0);
        prepare (unchanged, values);
        prepare (attemptedRestart, values);
        capture (unchanged, 701);
        capture (attemptedRestart, 701);
        attemptedRestart.restartFreeLfoFromHostTransport();
        assert (capture (unchanged, 507) == capture (attemptedRestart, 507));
    }
    // Free mode is not retriggered by MIDI note-on (One Shot disabled).
    {
        BassEngine withoutNote, withNote;
        const auto values = lfoSettings (0, 0);
        prepare (withoutNote, values);
        prepare (withNote, values);
        capture (withoutNote, 129);
        capture (withNote, 129);
        withNote.midi (juce::MidiMessage::noteOn (60, 115));
        assert (capture (withoutNote, 320) == capture (withNote, 320));
    }
    // Free One Shot remains at its end until DAW Play, then restarts.
    {
        BassEngine engine;
        auto values = lfoSettings (0, 0);
        values[lb303::lfo::oneShot] = 1.0f;
        values[lb303::lfo::oneShotPercent] = 25.0f;
        prepare (engine, values);
        capture (engine, 1000);
        const auto stoppedAtEnd = capture (engine, 10);
        assert (stoppedAtEnd.front() == stoppedAtEnd.back());
        engine.restartFreeLfoFromHostTransport();
        const auto a = capture (engine, 201);
        engine.restartFreeLfoFromHostTransport();
        const auto b = capture (engine, 201);
        assert (a == b);
        assert (a.front() != stoppedAtEnd.front());
    }
    std::cout << "PASS TEST2.4.68: Free mode sine/S&H/One Shot restart; "
                 "Triggered and MIDI behavior preserved\n";
}
