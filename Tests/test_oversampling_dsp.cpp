// Self-contained unit smoke test. Build the DSP modules with a minimal JUCE
// shim, or compile only VoiceDecimator against an unmodified toolchain.
#include "Oversampling.h"
#include <cmath>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <array>
#include <limits>
#include <utility>
constexpr double pi=3.1415926535897932384626433832795;
int main() {
    lb303::VoiceDecimator filter;
    assert(filter.getFactor() == 1);
    const auto untouched = filter.process(.5373f,-.819f);
    assert(untouched.first == .5373f && untouched.second == -.819f);
    for (int factor : {2,4}) {
        filter.setFactor(factor);
        assert(filter.getFactor() == factor);
        // DC unity after settling.
        for (int i=0; i<4000*factor; ++i) {
            auto out=filter.process(.3f,-.6f);
            assert(std::isfinite(out.first) && std::isfinite(out.second));
            if (i==4000*factor-1) {
                assert(std::abs(out.first-.3f)<1.e-3f);
                assert(std::abs(out.second+.6f)<1.e-3f);
            }
        }
        auto measure = [&](double f) {
            filter.clear();
            double square=0.0;
            const int n=4000, warm=1000;
            for(int t=0;t<(n+warm)*factor;++t) {
                const float s = static_cast<float>(std::sin(2*pi*f*t/factor));
                const auto out=filter.process(s,0);
                if(t%factor==factor-1 && t/factor>=warm) square += out.first*out.first;
                if(t%factor==factor-1) assert(std::abs(out.second)<1.e-9f);
            }
            return std::sqrt(square/n);
        };
        const double pass=measure(.05); // 0.05 * host fs
        const double reject=measure(.8); // would alias strongly into host Nyquist
        std::cout << factor << "x RMS pass " << pass << ", alias-prone component " << reject << '\n';
        assert(pass > .65 && pass < .75);
        assert(reject < .006);
    }
    filter.setFactor(1);
    assert(filter.process(.5123f,-.128f).first==.5123f);
    std::cout << "PASS: 1x passthrough, 2x/4x DC unity, stereo isolation, stopband attenuation\n";
}
