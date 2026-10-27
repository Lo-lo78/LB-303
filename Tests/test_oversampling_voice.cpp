#include "BassEngine.h"
#include "SharedEffects.h"
#include "Oversampling.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <array>
int main() {
    std::array<lb303::BassEngine,2> e;
    lb303::SharedEffects fx;
    for (auto& engine : e) engine.prepare(44100);
    fx.prepare(44100);
    lb303::BassEngine::Values v{};
    v[0] = 0; v[1]=0.003f; v[2]=0.1f; v[3]=.2f; v[4]=.03f;
    v[5]=10; v[6]=1; v[7]=.8f; v[8]=.60f; v[9]=.1f; v[10]=.3f;
    v[13]=2.0f; v[15]=120; v[16]=.5f; v[18]=1;
    for (auto& engine : e) engine.setParameters(v,120);
    lb303::SharedEffects::Controls ctl{};
    ctl[lb303::fx::distortionType]=8; ctl[lb303::fx::distortionAmount]=80;
    ctl[lb303::fx::distortion2Type]=7; ctl[lb303::fx::distortion2Amount]=55;
    fx.setParameters(ctl,v,120);
    std::array<lb303::VoiceDecimator,2> ds;
    double energies[3]{};
    for (int mode=0;mode<3;++mode) {
        int n=1<<mode;
        fx.clearTails(); fx.setInsertOversamplingFactor(n);
        for (int lane=0;lane<2;++lane) {
            e[lane].reset();
            e[lane].setOversamplingFactor(n);
            ds[lane].setFactor(n); ds[lane].clear();
            e[lane].midi(juce::MidiMessage::noteOn(lane ? 48 : 72,127));
        }
        for (int i=0;i<44100/2;++i) {
            std::array<std::pair<float,float>,2> stems{};
            for (int j=0;j<2;++j) {
                for (int k=0;k<n;++k) {
                    auto s=e[j].tick(0,0);
                    if(n>1) {s=fx.processDistortionInsert(s.first,s.second,j); s=ds[j].process(s.first,s.second);}
                    stems[j]=s;
                }
            }
            auto out=n==1 ? fx.process(stems[0].first,stems[0].second,stems[1].first,stems[1].second)
                : fx.processPostDistortion(stems[0].first,stems[0].second,stems[1].first,stems[1].second);
            const auto l=out.dry[0].first+out.dry[1].first;
            if(!std::isfinite(l)) {std::cerr<<"NONFINITE mode "<<n<<" step "<<i<<'\n'; return 1; }
            energies[mode]+=l*l;
        }
        std::cout << n << "x synth + distort energy " << energies[mode] << '\n';
        assert(energies[mode]>1.0);
    }
    std::cout<<"PASS synth voices/filter/softclip/distortion at 1x/2x/4x\n";
}
