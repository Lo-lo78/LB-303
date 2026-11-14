// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SequencerStepAudition.h"
#include "SequencerPlayback.h"
#include "SequencerTiming.h"
#include <cassert>
#include <cmath>
#include <memory>
#include <iostream>
int main()
{
    using namespace lb303;
    auto banks = std::make_unique<std::array<SequencerState, 2>>();
    for (int i=0; i<4; ++i)
    {
        SequencerStep s; s.note=60+i; s.velocity=100; s.length=50;
        (*banks)[0].replaceStep (0,i,s);
        s.note=80+i;
        (*banks)[1].replaceStep (0,i,s);
    }
    (*banks)[0].setConfigValue(0,SequencerState::ConfigParameter::endStep,4);
    const auto rg = makeStepAuditionRange(1,4,2,StepAuditionMode::current);
    assert(rg.first == 2 && rg.last == 2);
    assert(makeStepAuditionRange(1,4,2,StepAuditionMode::startToCurrent).first == 0);
    assert(makeStepAuditionRange(1,4,2,StepAuditionMode::currentToEnd).last == 3);
    for(int lane=0;lane<2;++lane)
        for(int first=0;first<4;++first)
            for(int last=first;last<4;++last)
            {
                const auto c=packStepAudition(lane,33,{first,last});
                const auto r=unpackStepAuditionRange(c);
                assert(unpackStepAuditionLane(c)==lane && unpackStepAuditionPattern(c)==33);
                assert(r.first==first && r.last==last);
            }
    assert(std::abs(sequencerTiming::auditionDuration((*banks)[0],0,0x51e90001u,0,2,2)-0.25)<1e-6);
    SequencerPlayback playback;
    std::array<SequencerPlayback::StepEvent, SequencerPlayback::maximumStepEvents> events{};
    int eventCount=0;
    juce::MidiBuffer out;
    auto origins=std::array<double,2>{{0.0,0.0}};
    auto transposes=std::array<int,2>{{0,0}};
    auto patterns=std::array<int,2>{{0,0}};
    auto entries=std::array<std::int64_t,2>{{0,-1}};
    auto enabled=std::array<bool,2>{{true,false}};
    playback.generate(out,*banks,patterns,entries,enabled,origins,transposes,
        true,0.0,120.0,48000.0,6000,events,eventCount,0,&rg);
    bool on=false,off=false;
    for(const auto& e:out)
    {
        assert(e.getMessage().getChannel()==1);
        assert(e.getMessage().getNoteNumber()==62);
        if(e.getMessage().isNoteOn()) on=true;
        else off=true;
    }
    assert(on && off && eventCount==1 && events[0].step==2 && events[0].lane==0);
    std::cout<<"PASS two-lane one-shot range, real step timing and MIDI source isolation\n";
}
