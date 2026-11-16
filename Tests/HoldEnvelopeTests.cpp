// TEST2.4.75: exercise the REAL BassEngine AHDSR with a minimal JUCE shim.
// Private access here is only for white-box assertions, never in production.
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#define private public
#include "BassEngine.h"
#undef private
#include <iostream>

using lb303::BassEngine;
static void configure(BassEngine& engine, float hold, double fs=8000.0)
{
    engine.prepare (fs);
    BassEngine::Values p {};
    p[0] = 0.0f;  // saw
    p[1] = 0.0f;  // instant Attack
    p[2] = 0.010f; // 10 ms Decay
    p[3] = 0.25f; // Sustain
    p[4] = 0.030f; // 30 ms Release
    p[7] = 0.8f;
    p[8] = 0.5f;
    p[13] = 1.0f;
    engine.setParameters(p, 120.0);
    engine.setHoldSeconds(hold);
}
static void tick(BassEngine& engine, int n=1)
{
    for (int i=0;i<n;++i) engine.tick(0.0f,0.0f);
}
int main()
{
    BassEngine noHold, holding;
    configure(noHold,0.0f);
    configure(holding,0.010f);
    noHold.midi(juce::MidiMessage::noteOn(60, 100));
    holding.midi(juce::MidiMessage::noteOn(60, 100));
    tick(noHold); tick(holding);
    assert(noHold.envelopeStage==1); // identical historic Attack -> Decay
    assert(holding.envelopeStage==4 && holding.envelope==1.0);
    tick(noHold, 40); tick(holding, 40);
    assert(noHold.envelope < .85 && noHold.envelopeStage==1);
    assert(holding.envelopeStage==4 && holding.envelope==1.0);
    tick(holding,40);
    assert(holding.envelopeStage==1 && holding.envelope==1.0);
    tick(holding,20);
    assert(holding.envelope < 1.0 && holding.envelopeStage==1);
    // Note Off during Hold must start Release at once, not finish Hold.
    BassEngine interrupted;
    configure(interrupted, 5.0f);
    interrupted.midi(juce::MidiMessage::noteOn(60,100)); tick(interrupted,20);
    assert(interrupted.envelopeStage==4);
    interrupted.midi(juce::MidiMessage::noteOff(60));
    assert(interrupted.envelopeStage==3);
    tick(interrupted,5);
    assert(interrupted.envelope < 1.0);
    // Legato reuses the envelope stage instead of restarting Hold.
    BassEngine legato;
    configure(legato,.05f);
    legato.midi(juce::MidiMessage::noteOn(60,90)); tick(legato,20);
    const auto elapsed=legato.holdElapsedSamples;
    legato.midi(juce::MidiMessage::noteOn(62,90));
    assert(legato.envelopeStage==4 && legato.holdElapsedSamples==elapsed);
    // Automation: immediately shortening Hold to zero exits the peak on next tick.
    legato.setHoldSeconds(0.0f); tick(legato);
    assert(legato.envelopeStage==1);
    // Oversampling switch mid Hold maintains elapsed physical time.
    BassEngine oversampled;
    configure(oversampled,.02f);
    oversampled.midi(juce::MidiMessage::noteOn(60,90)); tick(oversampled,41);
    const auto elapsed1x=oversampled.holdElapsedSamples;
    oversampled.setOversamplingFactor(4);
    assert(std::abs(oversampled.holdElapsedSamples-elapsed1x*4)<1e-9);
    tick(oversampled,4*120);
    assert(oversampled.envelopeStage==1);
    // Instances for Bassline 1 and 2 have completely separate clocks.
    BassEngine second;
    configure(second,0.0f);
    second.midi(juce::MidiMessage::noteOn(65,90)); tick(second,41);
    assert(second.envelopeStage==1 && second.envelope<1.0);
    assert(interrupted.envelopeStage==3);
    std::cout << "PASS TEST2.4.75: AHDSR zero bypass, timing, legato, release, "
                 "automation, oversampling, independent basslines\n";
}
