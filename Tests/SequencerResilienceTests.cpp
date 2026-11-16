// TEST2.4.76: actual sequencer timing and Song locator, no full JUCE dependency.
#include "SequencerPlayback.h"
#include "SongArrangement.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <tuple>
#include <vector>

using namespace lb303;
using Ev = std::tuple<int,int,int,int,bool>; // sample, ch, note, velocity, aftertouch
static std::vector<Ev> render(SequencerPlayback& player,
                              const std::array<SequencerState,2>& banks,
                              int blockSamples, int blocks)
{
    std::vector<Ev> values;
    auto events=std::make_unique<std::array<SequencerPlayback::StepEvent,
                        SequencerPlayback::maximumStepEvents>>();
    for(int block=0;block<blocks;++block)
    {
        int n=0;
        juce::MidiBuffer midi;
        player.generate(midi,banks,{{0,0}},{{-1,-1}},{{true,false}},
                        {{0,0}},{{0,0}},true,
                        static_cast<double>(block)*blockSamples/500.0,
                        120.0,1000.0,blockSamples,*events,n,0);
        for (const auto& m : midi)
        {
            const auto& msg=m.getMessage();
            values.emplace_back(block*blockSamples+m.samplePosition,
                                msg.channel,msg.note,msg.velocity,msg.aftertouch);
        }
    }
    return values;
}
int main()
{
    auto banks=std::make_unique<std::array<SequencerState,2>>();
    auto& bank=(*banks)[0];
    bank.setConfigValue(0,SequencerState::ConfigParameter::endStep,4);
    for(int i=0;i<4;++i)
    {
        SequencerStep s; s.note=60+i; s.length=60; s.velocity=85;
        bank.replaceStep(0,i,s);
    }
    // Random Repeat used to re-scan all previous cycles at every block.
    bank.setConfigValue(0,SequencerState::ConfigParameter::repeatRandomDepth,1.0f);
    SequencerPlayback longRun, chunked;
    auto all=render(longRun,*banks,500000,1);
    auto split=render(chunked,*banks,1000,500);
    assert(!all.empty() && all==split);

    // Length 101 ties must keep the gate but still update/re-trigger Accent.
    bank.setConfigValue(0,SequencerState::ConfigParameter::repeatRandomDepth,0.0f);
    bank.setConfigValue(0,SequencerState::ConfigParameter::velocityRandomDepth,1.0f);
    for(int i=0;i<4;++i)
    {
        SequencerStep s; s.note=i==3?128:60; s.velocity=i==1?127:i==2?70:80;
        s.length=101;
        bank.replaceStep(0,i,s);
    }
    SequencerPlayback ties;
    juce::MidiBuffer output;
    auto stepEvents=std::make_unique<std::array<SequencerPlayback::StepEvent,
                      SequencerPlayback::maximumStepEvents>>();
    int count=0;
    ties.generate(output,*banks,{{0,0}},{{-1,-1}},{{true,false}},{{0,0}},{{0,0}},
                  true,0.0,120.0,1000.0,500,*stepEvents,count,0);
    int ons=0,offs=0,pressure=0;
    for(const auto& e:output)
    {
        if(e.message.isNoteOn()) {++ons;assert(e.samplePosition==0);}
        else if(e.message.isNoteOff()) {++offs;assert(e.samplePosition==375);}
        else if(e.message.isAftertouch())
        {
            ++pressure;
            if(e.samplePosition==125) assert(e.message.velocity==127);
            else if(e.samplePosition==250) assert(e.message.velocity>=1 && e.message.velocity<=127);
            else assert(false);
        }
    }
    assert(ons==1 && offs==1 && pressure==2);

    // Song hints must produce EXACTLY the same row and duration as seeking
    // from beat zero, even after many looping Repeat Random song entries.
    SongArrangement::Column song{0,0,0,0,0};
    SongArrangement::Position previous;
    for(double beat=0.0;beat<200.0;beat+=0.25)
    {
        auto slow=SongArrangement::locate(song,true,bank,beat,0,123456,120);
        auto fast=SongArrangement::locate(song,true,bank,beat,0,123456,120,
                                        previous.active?&previous:nullptr);
        assert(slow.active==fast.active && slow.pattern==fast.pattern
               && slow.row==fast.row && slow.startBeat==fast.startBeat
               && slow.endBeat==fast.endBeat);
        previous=fast;
    }
    std::cout<<"PASS: random-repeat buffer independence, tied accent, long-song exact cursor\n";
}
