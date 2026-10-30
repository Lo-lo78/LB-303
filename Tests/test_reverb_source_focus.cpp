// TEST2.4.46: the source must be clearly localisable, not just spectrally different.
// Small DSP-only native regression; full JUCE VST3 compilation is via GitHub Actions.
#include "SharedEffects.h"
#include <array>
#include <cmath>
#include <iostream>
#include <vector>
using namespace lb303;
struct Result
{
    double earlyLeft = 0.0, earlyRight = 0.0, lateLeft = 0.0, lateRight = 0.0;
    double dryError = 0.0;
    std::vector<float> early;
};
Result render (int mode, size_t voice, float pan, float lfoDepth = 0.0f,
               float lfo = 0.0f)
{
    SharedEffects fx;
    fx.prepare (44100.0);
    SharedEffects::Controls c {};
    c[fx::reverbType] = static_cast<float> (mode);
    c[fx::reverbSend1] = voice == 0 ? 100.0f : 0.0f;
    c[fx::reverbSend2] = voice == 1 ? 100.0f : 0.0f;
    c[fx::reverbPredelay] = 30.0f;
    c[fx::reverbDecay] = 2.0f;
    c[fx::reverbDamping] = 9000.0f;
    c[fx::reverbWidth] = 100.0f;
    c[fx::reverbLevel] = 20.0f;
    c[fx::reverbBassDecay] = 1.0f;
    c[fx::reverbBassXover] = 360.0f;
    c[voice == 0 ? fx::reverbSourcePan1 : fx::reverbSourcePan2] = pan;
    c[voice == 0 ? fx::reverbLfoPan1 : fx::reverbLfoPan2] = lfoDepth;
    SharedEffects::ClassicParameters legacy {};
    fx.setParameters (c, legacy, 120.0);
    Result result;
    for (int n = 0; n < 44100; ++n)
    {
        const float input = n < 5000 ? .1f * (std::sin (n * .081f)
                           + .35f * std::sin (n * .213f)) : 0.0f;
        const float a = voice == 0 ? input : 0.0f;
        const float b = voice == 1 ? input : 0.0f;
        const auto out = fx.processPostDistortion (a, a, b, b, { lfo, lfo });
        if (!std::isfinite (out.wet.first) || !std::isfinite (out.wet.second))
            std::abort();
        const double left = static_cast<double> (out.wet.first) * out.wet.first;
        const double right = static_cast<double> (out.wet.second) * out.wet.second;
        if (n < 10000)
        {
            result.earlyLeft += left;
            result.earlyRight += right;
            result.early.push_back (out.wet.first);
            result.early.push_back (out.wet.second);
        }
        else
        {
            result.lateLeft += left;
            result.lateRight += right;
        }
        result.dryError += std::abs (out.dry[0].first - a) + std::abs (out.dry[0].second - a)
                         + std::abs (out.dry[1].first - b) + std::abs (out.dry[1].second - b);
    }
    return result;
}
static double balance (const Result& result)
{
    return 10.0 * std::log10 ((result.earlyRight + 1.0e-30)
                             / (result.earlyLeft + 1.0e-30));
}
int main()
{
    for (int mode = 1; mode <= 4; ++mode)
        for (size_t voice = 0; voice < basslineCount; ++voice)
        {
            const auto centre = render (mode, voice, 0.0f);
            const auto left = render (mode, voice, -100.0f);
            const auto right = render (mode, voice, 100.0f);
            const double leftDb = balance (left), rightDb = balance (right);
            if (leftDb > -5.0 || rightDb < 5.0)
            {
                std::cerr << "Insufficient positioning: mode=" << mode
                          << " voice=" << voice << " left=" << leftDb
                          << " right=" << rightDb << "\n";
                return 1;
            }
            if (left.dryError > 1.0e-6 || right.dryError > 1.0e-6)
                return 2;
            if (left.lateLeft + left.lateRight < 1.0e-8
                || right.lateLeft + right.lateRight < 1.0e-8)
                return 3; // old 8-line diffuse tail must continue
            if (centre.early == left.early || centre.early == right.early)
                return 4;
            // Even if the nominal pan is zero, LFO modulation reaches the
            // same left/right room-origin geometry using the real input.
            const auto lfoLeft = render (mode, voice, 0.0f, 100.0f, -1.0f);
            const auto lfoRight = render (mode, voice, 0.0f, 100.0f, 1.0f);
            if (balance (lfoLeft) > -5.0 || balance (lfoRight) < 5.0)
                return 5;
            const auto zeroLfo = render (mode, voice, 0.0f, 0.0f, 1.0f);
            if (centre.early != zeroLfo.early) return 6;
        }
    std::cout << "PASS: TEST2.4.46 distinct left/right source positions and LFO\n"
                 "across all four reverbs and both voices, diffuse tails, dry preserved\n";
}
