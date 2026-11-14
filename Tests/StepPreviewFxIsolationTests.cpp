// TEST2.4.73: The shared delay must keep its time glide, feedback and
// existing echoes while individual sequencer steps are auditioned.
#include "SharedEffects.h"
#include <cmath>
#include <cstdio>
#include <algorithm>

static lb303::BassEngine::Values classicSettings()
{
    lb303::BassEngine::Values p {};
    p[19] = 160.0f; // 200 ms at the JSFX's 1.25x free-time scale
    p[20] = 0.0f;
    p[21] = 55.0f;
    p[22] = 1.0f;
    p[24] = 20000.0f; p[25] = 0.707f;
    p[26] = 20.0f; p[27] = 0.707f;
    p[31] = 0.0f; p[32] = 100.0f; // wet only
    return p;
}

int main()
{
    constexpr double fs = 48000.0;
    auto legacy = classicSettings();
    lb303::SharedEffects::Controls controls {};
    controls[lb303::fx::delayType] = 1.0f;
    controls[lb303::fx::delaySend1] = 100.0f;
    controls[lb303::fx::delaySend2] = 0.0f;
    lb303::SharedEffects reference, audition;
    reference.prepare (fs);
    audition.prepare (fs);
    reference.setParameters (controls, legacy, 120.0);
    audition.setParameters (controls, legacy, 120.0);

    // The real classic delay intentionally glides very slowly from zero on
    // initial preparation. Warm both instances before auditioning a step.
    for (int i = 0; i < 144000; ++i)
    {
        reference.process (0.0f, 0.0f, 0.0f, 0.0f);
        audition.process (0.0f, 0.0f, 0.0f, 0.0f);
    }

    double echoEnergy = 0.0;
    for (int i = 0; i < 40000; ++i)
    {
        // Repeated step starts: only the normal Send and unchanged parameters
        // are applied. There must be no FX reset at these boundaries.
        if (i == 2200 || i == 5900 || i == 10100 || i == 16800)
        {
            audition.setParameters (controls, legacy, 120.0);
            audition.setStepSendGains ({{ 1.0f, 1.0f }}, {{ 1.0f, 1.0f }});
            audition.setStepSendOverrides ({{ -1.0f, -1.0f }}, {{ -1.0f, -1.0f }});
        }
        const float input = i == 0 ? 0.9f : (i == 2200 ? 0.6f : 0.0f);
        const auto a = reference.process (input, 0.0f, 0.0f, 0.0f);
        const auto b = audition.process (input, 0.0f, 0.0f, 0.0f);
        if (std::abs (a.wet.first - b.wet.first) > 1.0e-5f
            || std::abs (a.wet.second - b.wet.second) > 1.0e-5f)
        {
            std::fprintf (stderr, "FAIL: delay state altered by a step at sample %d: %.9f vs %.9f\n",
                          i, a.wet.first, b.wet.first);
            return 1;
        }
        if (i > 8000 && i < 28000)
            echoEnergy += std::abs (b.wet.first) + std::abs (b.wet.second);
    }
    if (echoEnergy < 0.001)
    {
        std::fprintf (stderr, "FAIL: single-step audition has no audible delay tail: %.9f\n", echoEnergy);
        return 2;
    }
    std::puts ("PASS TEST2.4.73: step auditions retain delay speed, glide, feedback and audible tails");
    return 0;
}
