// TEST2.4.76: white-box assertion on real BassEngine state, not an emulation.
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
int main()
{
    lb303::BassEngine e;
    e.prepare(8000.0);
    lb303::BassEngine::Values v {};
    v[15]=110.0f;
    v[7]=0.8f;
    e.setParameters(v,120.0);
    e.midi(juce::MidiMessage::noteOn(60,80));
    assert(e.noteActive && !e.accent && e.lastNote==60);
    e.tick(0,0);
    const int stage=e.envelopeStage;
    const double elapsed=e.holdElapsedSamples;
    e.midi(juce::MidiMessage::aftertouchChange(60,127));
    assert(e.noteActive && e.accent && e.accentEnvelope==1.0);
    assert(e.lastNote==60 && e.envelopeStage==stage && e.holdElapsedSamples==elapsed);
    e.midi(juce::MidiMessage::aftertouchChange(61,0));
    assert(e.accent && e.noteActive); // unrelated pitch ignored
    e.midi(juce::MidiMessage::aftertouchChange(60,70));
    assert(!e.accent && e.noteActive && e.lastNote==60);
    assert(e.envelopeStage==stage && e.holdElapsedSamples==elapsed);
    e.midi(juce::MidiMessage::noteOff(60));
    assert(!e.noteActive);
    std::cout<<"PASS: tied accent updates without retriggering/releasing AHDSR\n";
}
