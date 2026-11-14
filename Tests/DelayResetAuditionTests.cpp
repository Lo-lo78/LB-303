// Regression for TEST2.4.72: repeated one-shot step auditions must never
// leave the shared classic delay running at a stale near-zero delay time.
// Execute the real BassEngine and SharedEffects through the project's JUCE shim.
#include "BassEngine.h"
#include "SharedEffects.h"
#include <cmath>
#include <cstdio>

static lb303::BassEngine::Values classicValues()
{
    lb303::BassEngine::Values values {};
    values[19] = 200.0f; // distinctly nonzero delay (milliseconds)
    values[20] = 0.0f;    // free time, no BPM synchronisation
    values[21] = 35.0f;   // feedback, not a runaway value
    values[22] = 1.0f;    // ping-pong
    values[24] = 20000.0f; values[25] = 0.707f;
    values[26] = 20.0f; values[27] = 0.707f;
    values[31] = 0.0f; values[32] = 100.0f; // wet only: hear erroneous early delay
    return values;
}

static bool checkEngineReset()
{
    constexpr double fs = 48000.0;
    auto values = classicValues();
    lb303::BassEngine clean;
    lb303::BassEngine repeated;
    clean.prepare (fs);
    repeated.prepare (fs);
    clean.setParameters (values, 120.0);
    repeated.setParameters (values, 120.0);
    repeated.reset(); // Clear a previous step's FX tail, as preview does.
    repeated.setParameters (values, 120.0); // No user knob movement!

    // A correct reset must re-cook exactly the same delay as on first load.
    // Before the fix, reset() erased delayTargetSamples but the cached p/tempo
    // incorrectly bypassed setParameters(), producing an immediate echo.
    for (int i = 0; i < 384; ++i)
    {
        const float impulse = i == 0 ? 0.5f : 0.0f;
        const auto expected = clean.processClassicDelay (impulse, 0.0f);
        const auto actual = repeated.processClassicDelay (impulse, 0.0f);
        if (! std::isfinite (actual.first) || ! std::isfinite (actual.second)
            || std::abs (actual.first - expected.first) > 1.0e-6f
            || std::abs (actual.second - expected.second) > 1.0e-6f)
        {
            std::fprintf (stderr, "FAIL classic delay reapply-after-reset at sample %d: expected %.8f / %.8f, got %.8f / %.8f\n",
                          i, expected.first, expected.second, actual.first, actual.second);
            return false;
        }
    }
    return true;
}

static bool checkSharedFxResets()
{
    constexpr double fs = 48000.0;
    auto values = classicValues();
    lb303::SharedEffects::Controls controls {};
    controls[lb303::fx::delayType] = 1.0f;
    controls[lb303::fx::delaySend1] = 100.0f;
    controls[lb303::fx::delaySend2] = 100.0f;
    lb303::SharedEffects reused;
    reused.prepare (fs);
    reused.setParameters (controls, values, 120.0);
    for (int cycle = 0; cycle < 3; ++cycle)
    {
        // Compare a fresh FX rack with a rack re-used by repeated step
        // auditions. This catches exactly the bad cache left by clearTails().
        lb303::SharedEffects clean;
        clean.prepare (fs);
        clean.setParameters (controls, values, 120.0);
        reused.clearTails();
        reused.setParameters (controls, values, 120.0);
        for (int i = 0; i < 384; ++i)
        {
            const float impulse = i == 0 ? 0.5f : 0.0f;
            const auto a = clean.process (impulse, 0.0f, 0.0f, 0.0f);
            const auto b = reused.process (impulse, 0.0f, 0.0f, 0.0f);
            if (! std::isfinite (b.wet.first) || ! std::isfinite (b.wet.second)
                || std::abs (a.wet.first - b.wet.first) > 1.0e-6f
                || std::abs (a.wet.second - b.wet.second) > 1.0e-6f)
            {
                std::fprintf (stderr, "FAIL FX repeated preview cycle %d at sample %d\n", cycle, i);
                return false;
            }
        }
    }
    return true;
}

int main()
{
    if (!checkEngineReset() || !checkSharedFxResets()) return 1;
    std::puts ("PASS TEST2.4.72: shared classic delay reset and repeated preview keep original time/feedback");
    return 0;
}
