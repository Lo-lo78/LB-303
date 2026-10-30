// Optional DSP smoke test using JUCE shim. The full Windows VST3 is built on GitHub Actions.
#include "SharedEffects.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace lb303;
struct Result
{
    std::vector<float> left, right;
    double energy = 0.0;
    double dryError = 0.0;
};
Result render (float pan1, float pan2, float depth1, float depth2, bool reverseLfo = false)
{
    SharedEffects fx;
    fx.prepare (44100.0);
    SharedEffects::Controls c {};
    c[fx::reverbType] = 3;
    c[fx::reverbSend1] = 70;
    c[fx::reverbSend2] = 70;
    c[fx::reverbPredelay] = 40;
    c[fx::reverbDecay] = 2.0f;
    c[fx::reverbDamping] = 8000;
    c[fx::reverbWidth] = 100;
    c[fx::reverbLevel] = 20;
    c[fx::reverbBassDecay] = .7f;
    c[fx::reverbBassXover] = 360;
    c[fx::reverbMotionMode] = 0;
    c[fx::reverbSourcePan1] = pan1;
    c[fx::reverbSourcePan2] = pan2;
    c[fx::reverbLfoPan1] = depth1;
    c[fx::reverbLfoPan2] = depth2;
    SharedEffects::ClassicParameters legacy {};
    fx.setParameters (c, legacy, 120.0);
    Result result;
    constexpr int frames = 44100;
    for (int n = 0; n < frames; ++n)
    {
        const float a = n < 3000 ? .10f * std::sin (n * .081f) : 0.0f;
        const float b = n < 5100 ? .07f * std::sin (n * .139f) : 0.0f;
        const float lfo1 = std::sin (n * .00017f);
        const float lfo2 = std::sin (n * .00031f);
        const auto out = fx.processPostDistortion (a, a, b, b,
            { reverseLfo ? -lfo1 : lfo1, lfo2 });
        if (!std::isfinite (out.wet.first) || !std::isfinite (out.wet.second)) std::abort();
        result.energy += double (out.wet.first) * out.wet.first
                       + double (out.wet.second) * out.wet.second;
        result.left.push_back (out.wet.first);
        result.right.push_back (out.wet.second);
        result.dryError += std::abs (out.dry[0].first-a) + std::abs (out.dry[0].second-a)
                        + std::abs (out.dry[1].first-b) + std::abs (out.dry[1].second-b);
    }
    return result;
}
int main()
{
    const auto base = render (0,0,0,0);
    const auto zeroLfo = render (0,0,0,0,true);
    const auto left1 = render (-85,0,0,0);
    const auto right1 = render (85,0,0,0);
    const auto right2 = render (0,85,0,0);
    const auto moving = render (0,0,85,-70);
    const auto reversed = render (0,0,85,-70,true);
    if (base.left != zeroLfo.left || base.right != zeroLfo.right) return 1;
    if (base.dryError > 1e-6 || moving.dryError > 1e-6) return 2;
    for (const auto* r : { &left1, &right1, &right2, &moving, &reversed })
        if (r->energy < 1.e-8 || r->energy > 30.0 * base.energy) return 3;
    auto distance = [] (const Result& x, const Result& y)
    {
        double sum = 0;
        for (size_t i = 0; i < x.left.size(); ++i)
            sum += std::abs (x.left[i] - y.left[i])
                 + std::abs (x.right[i] - y.right[i]);
        return sum;
    };
    if (distance (base, left1) < .01 || distance (base, right1) < .01
        || distance (left1, right1) < .01 || distance (right1, right2) < .01
        || distance (base, moving) < .01 || distance (moving, reversed) < .01)
        return 4;
    std::cout << "PASS: TEST2.4.45 independent spatial sends, LFO, dry isolation and zero identity\n";
}
