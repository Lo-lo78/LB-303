#include "SequencerPlayback.h"
#include <cassert>
#include <iostream>
#include <vector>
using namespace lb303;
struct Ev {int absoluteSample; int note; bool on; int ch;};
int main() {
 std::array<SequencerState,2> banks;
 auto& bank=banks[0];
 bank.setConfigValue(0,SequencerState::ConfigParameter::startStep,1);
 bank.setConfigValue(0,SequencerState::ConfigParameter::endStep,16);
 for(int i=0;i<16;++i){
  bank.setStepValue(0,i,SequencerLayer::note,60);
  bank.setStepValue(0,i,SequencerLayer::length,101);
 }
 SequencerPlayback player;
 auto collect=[&](int blocks){
  std::vector<Ev> events;
  for(int block=0;block<blocks;++block){
   juce::MidiBuffer midi;
   std::array<SequencerPlayback::StepEvent,SequencerPlayback::maximumStepEvents> stepEvents{};
   int count=0;
   player.generate(midi,banks,{{0,0}},{{-1,-1}},{{true,false}},{{0,0}},{{0,0}},true,
      block*.25,120,1000,125,stepEvents,count,0);
   for(const auto& e:midi) events.push_back({block*125+e.samplePosition,
      e.message.note,e.message.on,e.message.channel});
  }
  return events;
 };
 // All 16 steps tied to the same C4 across TWO complete cycles: one gate.
 auto ev=collect(32);
 assert(ev.size()==1 && ev[0].on && ev[0].note==60);
 // New phrase with four tied steps followed by ordinary 50% length.
 player.reset();
 for(int i=4;i<16;++i)bank.setStepValue(0,i,SequencerLayer::note,128);
 bank.setStepValue(0,4,SequencerLayer::note,60);
 bank.setStepValue(0,4,SequencerLayer::length,50);
 ev=collect(6);
 assert(ev.size()==2 && ev[0].on && !ev[1].on && ev[1].absoluteSample==563);
 // Pitch change at step 2: overlap on the boundary with old off afterwards.
 player.reset();
 bank.setStepValue(0,1,SequencerLayer::note,62);
 ev=collect(6);
 assert(ev.size()==6);
 assert(ev[0].on && ev[0].note==60);
 assert(ev[1].on && ev[1].note==62);
 assert(!ev[2].on && ev[2].note==60);
 assert(ev[1].absoluteSample==ev[2].absoluteSample);
 assert(ev[3].on && ev[3].note==60);
 assert(!ev[4].on && ev[4].note==62);
 assert(ev[3].absoluteSample==ev[4].absoluteSample);
 assert(!ev[5].on && ev[5].note==60);
 // A silent step ends the previous tie, but a later new note is independent.
 player.reset();
 bank.setStepValue(0,2,SequencerLayer::note,128);
 ev=collect(5);
 assert(ev.size()==6);
 assert(!ev[3].on && ev[3].absoluteSample==250 && ev[3].note==62);
 // An explicit transport stop must not leave a held note hanging.
 player.reset();
 juce::MidiBuffer stopped;
 std::array<SequencerPlayback::StepEvent,SequencerPlayback::maximumStepEvents> lockEvents{};
 int lockCount=0;
 player.generate(stopped,banks,{{0,0}},{{-1,-1}},{{true,false}},{{0,0}},{{0,0}},false,
       0.0,120,1000,125,lockEvents,lockCount,0);
 assert(stopped.events.empty());
 std::cout<<"PASS: 101 ties persist across 16->1, 4-step held notes, normal end, pitch hand-off\n";
}
