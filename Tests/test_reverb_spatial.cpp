// TEST2.4.44 DSP-only smoke test (JUCE shim): no REAPER or VST build required.
// Compare moving FDN tails with identical Off output and check safe feedback.
#include "SharedEffects.h"
#include <array>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <algorithm>

using namespace lb303;
struct Render
{
    std::vector<float> l, r;
    double energy = 0.0;
};

Render render (int mode, float depth, float rate, bool midAutomation = false)
{
    SharedEffects fx;
    fx.prepare (44100.0);
    SharedEffects::Controls c {};
    c[fx::reverbType] = 3;
    c[fx::reverbSend1] = 100;
    c[fx::reverbPredelay] = 0;
    c[fx::reverbDecay] = 3.0f;
    c[fx::reverbDamping] = 8000;
    c[fx::reverbWidth] = 100;
    c[fx::reverbLevel] = 20;
    c[fx::reverbBassDecay] = .7f;
    c[fx::reverbBassXover] = 360;
    c[fx::reverbMotionMode] = static_cast<float> (mode);
    c[fx::reverbMotionDepth] = depth;
    c[fx::reverbMotionRate] = rate;
    SharedEffects::ClassicParameters legacy {};
    fx.setParameters (c, legacy, 120.0);
    Render out;
    constexpr int frames = 44100; // 1 sec to detect tail evolution
    out.l.reserve (frames); out.r.reserve (frames);
    for (int n = 0; n < frames; ++n)
    {
        if (midAutomation && n == 15000)
        {
            c[fx::reverbMotionMode] = 2;
            c[fx::reverbMotionDepth] = 100;
            c[fx::reverbMotionRate] = 1.4f;
            fx.setParameters (c, legacy, 120.0);
        }
        // 60 ms mono, then a genuine reverb tail; exercise expansion.
        const float source = (n < 2646) ? .12f * std::sin (n * .081f) : 0.0f;
        const auto wet = fx.processPostDistortion (source, source, 0.0f, 0.0f).wet;
        if (! std::isfinite (wet.first) || ! std::isfinite (wet.second) ||
            std::max (std::abs (wet.first), std::abs (wet.second)) > 8.0f) std::abort();
        out.l.push_back (wet.first); out.r.push_back (wet.second);
        out.energy += double (wet.first) * wet.first + double (wet.second) * wet.second;
    }
    return out;
}

int main()
{
    const auto original = render (0, 0, .35f);
    const auto offWithDepth = render (0, 100, 1.5f);
    const auto zeroDepth = render (1, 0, .35f);
    // Exact Off and 0 Depth invariance, including the bass RT60 split.
    if (original.l != offWithDepth.l || original.r != offWithDepth.r ||
        original.l != zeroDepth.l || original.r != zeroDepth.r ||
        original.energy < 1.0e-6) return 10;

    for (int mode = 1; mode <= 6; ++mode)
    {
        const auto moving = render (mode, 100, .9f);
        double delta = 0.0, tailDelta = 0.0;
        for (int n = 0; n < static_cast<int> (moving.l.size()); ++n)
        {
            const double d = std::abs (moving.l[n] - original.l[n])
                           + std::abs (moving.r[n] - original.r[n]);
            delta += d;
            if (n > 12000) tailDelta += d;
        }
        if (delta < 1.0e-4 || tailDelta < 1.0e-4 || moving.energy < 1.0e-7 ||
            moving.energy > original.energy * 30.0) return 20 + mode;
        std::cout << "Mode " << mode << " tail delta " << tailDelta << '\n';
    }
    const auto clockwise = render (1, 100, .7f);
    const auto anticlockwise = render (2, 100, .7f);
    if (clockwise.l == anticlockwise.l && clockwise.r == anticlockwise.r) return 30;
    const auto automated = render (1, 100, .3f, true);
    if (automated.energy < 1.0e-7 || automated.energy > original.energy * 30.0) return 31;
    std::cout << "PASS: spatial FDN movement, original Off/depth-zero audio, 6 distinct modes, finite automations.\n";
}
