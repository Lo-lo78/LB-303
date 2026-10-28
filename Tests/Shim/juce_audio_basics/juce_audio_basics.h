#pragma once
// Tiny deterministic MIDI subset for testing SequencerPlayback against the
// actual SequencerState and timing code without downloading JUCE.
#include <cstdint>
#include <vector>
#include <algorithm>
namespace juce {
using uint8 = std::uint8_t;
template <typename T> T jmax(T a,T b) {return std::max(a,b);}
struct MidiMessage {
 bool on = false; int channel=1, note=0, velocity=0;
 static MidiMessage noteOn(int ch,int n,uint8 vel) {return {true,ch,n,vel};}
 static MidiMessage noteOff(int ch,int n) {return {false,ch,n,0};}
 bool isNoteOn() const {return on;}
 bool isNoteOff() const {return !on;}
 int getNoteNumber() const {return note;}
 int getChannel() const {return channel;}
};
struct MidiBuffer {
 struct MidiMessageMetadata {
  MidiMessage message;
  int samplePosition;
  const MidiMessage& getMessage() const {return message;}
 };
 std::vector<MidiMessageMetadata> events;
 void addEvent(const MidiMessage& msg,int pos) {
  auto p=std::upper_bound(events.begin(),events.end(),pos,
    [](int time,const MidiMessageMetadata& e){return time < e.samplePosition;});
  events.insert(p, {msg,pos});
 }
 void clear(){events.clear();}
 auto begin() const {return events.begin();}
 auto end() const {return events.end();}
};
}
