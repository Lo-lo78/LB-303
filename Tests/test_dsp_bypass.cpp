// Regression test for TEST2.4.34's dormant audio graphs. This same test
// can be compiled against the original TEST2.4.33 to compare sample hashes.
#include "BassEngine.h"
#include "SharedEffects.h"
#include "Oversampling.h"
#include <iostream>
#include <cstring>
#include <cstdint>
#include <array>
#include <cmath>
#include <chrono>
#include <cstdlib>

static std::uint64_t hash = 1469598103934665603ULL;
static void hashf(float f)
{
    if (!std::isfinite(f)) std::abort();
    std::uint32_t bits;
    std::memcpy(&bits, &f, 4);
    hash ^= bits;
    hash *= 1099511628211ULL;
}
int main(int argc, char** argv)
{
    // 0=all inactive, 1=active LFO, 2=active OSC2, 3=both LFO+OSC2
    // 4=FX enabled, 5=effect bypass toggling mid-note
    // 6=glide always, 7=static transpose, 8=LFO activated mid-note,
    // 9=second oscillator activated mid-note
    const int scenario = argc > 1 ? std::atoi(argv[1]) : 0;
    const int quality = argc > 2 ? std::atoi(argv[2]) : 1;
    const int samples = argc > 3 ? std::atoi(argv[3]) : 22050;
    if (quality != 1 && quality != 2 && quality != 4) std::abort();
    std::array<lb303::BassEngine,2> engines;
    std::array<lb303::VoiceDecimator,2> ds;
    lb303::SharedEffects effects;
    for (auto& v: engines) { v.prepare(44100); v.setOversamplingFactor(quality); }
    effects.prepare(44100);
    effects.setInsertOversamplingFactor(quality);
    for (auto& d : ds) d.setFactor(quality);
    lb303::BassEngine::Values p {};
    p[0]=0; p[1]=.002f; p[2]=.1f; p[3]=.2f; p[4]=.03f; p[5]=10;
    p[6]=scenario==6 ? 0 : 1; p[7]=.8f; p[8]=.6f; p[9]=.1f; p[10]=.3f;
    p[13]=2; p[14]=scenario==7 ? 7 : 0; p[15]=120; p[16]=.5f; p[18]=1;
    for (auto& v:engines) v.setParameters(p,120);
    lb303::BassEngine::LfoValues lfo {};
    if (scenario == 1 || scenario == 3) {
        lfo[lb303::lfo::rate]=2.0f; lfo[lb303::lfo::wave]=0;
        lfo[lb303::lfo::filterDepth]=.25f; lfo[lb303::lfo::pitchDepth]=.3f;
        lfo[lb303::lfo::volumeDepth]=.1f; lfo[lb303::lfo::panDepth]=.25f;
    }
    if (scenario == 8) lfo[lb303::lfo::rate] = 5.0f;
    lb303::BassEngine::Osc2Values osc {};
    if (scenario==2 || scenario==3) {osc[lb303::osc2::volume]=45;osc[lb303::osc2::detune]=23;}
    for (auto& v:engines) {v.setLfoParameters(lfo);v.setOsc2Parameters(osc);}
    lb303::SharedEffects::Controls fx {};
    if (scenario==4 || scenario==5) {
        fx[lb303::fx::distortionType]=8;fx[lb303::fx::distortionAmount]=80;
        fx[lb303::fx::distortion2Type]=7;fx[lb303::fx::distortion2Amount]=55;
        fx[lb303::fx::chorusOn]=1;fx[lb303::fx::chorusSend1]=30;
        fx[lb303::fx::chorusLevel]=55;fx[lb303::fx::delayType]=2;
        fx[lb303::fx::delaySend1]=35;fx[lb303::fx::delaySend2]=30;
        fx[lb303::fx::reverbType]=2;fx[lb303::fx::reverbSend1]=30;
        fx[lb303::fx::reverbLevel]=40;
    }
    effects.setParameters(fx,p,120);
    engines[0].midi(juce::MidiMessage::noteOn(72,127));
    engines[1].midi(juce::MidiMessage::noteOn(48,110));
    const auto begin=std::chrono::steady_clock::now();
    for (int i=0; i<samples; ++i) {
        if (i==samples/2) engines[0].midi(juce::MidiMessage::noteOff(72));
        if (scenario==6 && i==samples/4) {
            engines[0].midi(juce::MidiMessage::noteOn(74,99));
        }
        if (scenario==8 && i==samples/2) {
            lfo[lb303::lfo::rate]=5.0f;
            lfo[lb303::lfo::pitchDepth]=.6f;
            for (auto& v:engines) v.setLfoParameters(lfo);
        }
        if (scenario==9 && i==samples/2) {
            osc[lb303::osc2::volume]=62;
            osc[lb303::osc2::detune]=-33;
            for (auto& v:engines) v.setOsc2Parameters(osc);
        }
        if (scenario==5 && i==samples/2) {
            fx[lb303::fx::distortionAmount]=0;
            fx[lb303::fx::distortion2Amount]=0;
            fx[lb303::fx::chorusOn]=0;
            fx[lb303::fx::delayType]=0;
            fx[lb303::fx::reverbType]=0;
            effects.setParameters(fx,p,120);
        }
        std::array<std::pair<float,float>,2> stem {};
        for (int voice=0; voice<2; ++voice) {
            for (int sub=0; sub<quality; ++sub) {
                auto s=engines[static_cast<size_t>(voice)].tick(0,0);
                if (quality>1) {
                    s=effects.processDistortionInsert(s.first,s.second,static_cast<size_t>(voice));
                    s=ds[static_cast<size_t>(voice)].process(s.first,s.second);
                }
                stem[static_cast<size_t>(voice)]=s;
            }
        }
        const auto out = quality==1
            ? effects.process(stem[0].first,stem[0].second,stem[1].first,stem[1].second)
            : effects.processPostDistortion(stem[0].first,stem[0].second,stem[1].first,stem[1].second);
        hashf(out.dry[0].first);hashf(out.dry[0].second);
        hashf(out.dry[1].first);hashf(out.dry[1].second);
        hashf(out.wet.first);hashf(out.wet.second);
    }
    const auto end=std::chrono::steady_clock::now();
    std::cout << "scenario " << scenario << ", " << quality << "x, hash " << hash
              << ", ms " << std::chrono::duration<double,std::milli>(end-begin).count() << "\n";
}
