// Verify a truly silent, note-free voice can bypass the DSP, but a note or
// a release/filter/oversampling tail prevents premature sleep.
#include "BassEngine.h"
#include "SharedEffects.h"
#include "Oversampling.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <array>

int main()
{
    constexpr double fs = 44100;
    for (int factor : {1, 2, 4})
    {
        lb303::BassEngine voice;
        lb303::VoiceDecimator decimator;
        lb303::SharedEffects fx;
        voice.prepare(fs);
        voice.setOversamplingFactor(factor);
        decimator.setFactor(factor);
        fx.prepare(fs);
        fx.setInsertOversamplingFactor(factor);
        lb303::BassEngine::Values p {};
        p[0] = 0; p[1] = .002f; p[2] = .1f; p[3] = .2f;
        p[4] = .03f; p[5] = 10; p[6] = 1; p[7] = .8f;
        p[8] = .6f; p[9] = .06f; p[10] = .3f; p[13] = 2;
        p[15] = 120; p[16] = .5f; p[18] = 1;
        voice.setParameters(p, 120);
        lb303::SharedEffects::Controls controls {};
        controls[lb303::fx::distortionType] = 8;
        controls[lb303::fx::distortionAmount] = 75;
        fx.setParameters(controls,p,120);
        assert(voice.isQuiescent());
        assert(decimator.isQuiescent());
        assert(fx.distortionQuiescent(0));
        assert(!fx.wetEffectsEnabled());
        voice.midi(juce::MidiMessage::noteOn(60, 127));
        assert(!voice.isQuiescent());
        double energy=0;
        for(int i=0;i<fs*2;++i)
        {
            if(i == fs/10)
            {
                voice.midi(juce::MidiMessage::noteOff(60));
                assert(!voice.isQuiescent());
            }
            if (voice.isQuiescent() && decimator.isQuiescent() && fx.distortionQuiescent(0))
                continue;
            std::pair<float,float> sample {};
            for (int sub=0; sub<factor; ++sub)
            {
                sample=voice.tick(0,0);
                if (factor!=1) {
                    sample=fx.processDistortionInsert(sample.first,sample.second,0);
                    sample=decimator.process(sample.first,sample.second);
                }
            }
            const auto out = factor == 1 ? fx.process(sample.first, sample.second,0,0)
                : fx.processPostDistortion(sample.first,sample.second,0,0);
            assert(std::isfinite(out.dry[0].first));
            energy += out.dry[0].first*out.dry[0].first;
        }
        assert(energy>0.0001);
        assert(voice.isQuiescent());
        assert(decimator.isQuiescent());
        assert(fx.distortionQuiescent(0));
        voice.midi(juce::MidiMessage::noteOn(72, 120));
        assert(!voice.isQuiescent());
        float maxSample=0;
        for(int i=0;i<4096;++i)
        {
            for(int sub=0;sub<factor;++sub)
            {
                auto sample=voice.tick(0,0);
                if(factor>1) sample=decimator.process(sample.first,sample.second);
                maxSample=std::max(maxSample,std::abs(sample.first));
            }
        }
        assert(maxSample>0.001);
        voice.midi(juce::MidiMessage::controllerEvent(123));
        assert(!voice.isQuiescent()); // normal release tail after All Notes Off
        for(int i=0;i<fs*2;++i)
        {
            if (voice.isQuiescent() && decimator.isQuiescent()) break;
            for(int sub=0;sub<factor;++sub)
            {
                auto s=voice.tick(0,0);
                if(factor>1) decimator.process(s.first,s.second);
            }
        }
        assert(voice.isQuiescent());
        assert(decimator.isQuiescent());
        std::cout << "PASS "<<factor<<"x cold sleep / note wake / release tail / hibernate / retrigger / all-notes-off\n";
    }
    return 0;
}
