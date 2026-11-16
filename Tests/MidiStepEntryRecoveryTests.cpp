// TEST2.4.76: a missing Note Off must not poison the following Ctrl+K session.
#include "MidiStepEntryQueue.h"
#include <cassert>
#include <iostream>
int main()
{
    MidiStepEntryQueue q;
    MidiStepEntryQueue::Note n;
    q.begin();
    assert(q.consume(juce::MidiMessage::noteOn(1,60,100)));
    assert(q.pop(n) && n.pitch==60);
    q.finish(); // original Note Off was lost
    q.begin();  // new session must not inherit an eternally-held C4
    assert(q.consume(juce::MidiMessage::noteOn(1,62,114)));
    assert(q.pop(n) && n.pitch==62 && n.velocity==114);
    // Ordinary chord filtering still allows only first incoming note.
    assert(q.consume(juce::MidiMessage::noteOn(1,65,105)));
    assert(!q.pop(n));
    assert(q.consume(juce::MidiMessage::noteOff(1,62)));
    assert(q.consume(juce::MidiMessage::noteOff(1,65)));
    assert(q.consume(juce::MidiMessage::noteOn(1,67,111)));
    assert(q.pop(n) && n.pitch==67);
    std::cout<<"PASS: MIDI entry sessions recover from missing physical Note Off\n";
}
