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
               float lfo = 0.0f, float earlyLevel = 100.0f)
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
    c[fx::reverbEarlyLevel] = earlyLevel;
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
            // TEST2.4.47: early reflections exist at centre as well as edges.
            // With Early Level zero only the existing FDN remains. The delta
            // is the pure early-reflection field (plus finite-precision mix).
            const auto centreNoEarly = render (mode, voice, 0.0f, 0.0f, 0.0f, 0.0f);
            const auto leftNoEarly = render (mode, voice, -100.0f, 0.0f, 0.0f, 0.0f);
            const auto rightNoEarly = render (mode, voice, 100.0f, 0.0f, 0.0f, 0.0f);
            auto earlyEnergy = [] (const Result& full, const Result& none)
            {
                double sum = 0.0;
                for (size_t k = 0; k < full.early.size(); ++k)
                {
                    const double d = static_cast<double> (full.early[k]) - none.early[k];
                    sum += d * d;
                }
                return sum;
            };
            const double ec = earlyEnergy (centre, centreNoEarly);
            const double el = earlyEnergy (left, leftNoEarly);
            const double er = earlyEnergy (right, rightNoEarly);
            if (ec < 1.0e-7 || el < 1.0e-7 || er < 1.0e-7)
                return 7; // centre cannot have vanishing reflections
            if (ec / el < 0.60 || ec / el > 1.60
                || ec / er < 0.60 || ec / er > 1.60)
                return 8; // no pan-triggered increase/decrease of ER loudness
            const auto centreHalf = render (mode, voice, 0.0f, 0.0f, 0.0f, 50.0f);
            const double halfEnergy = earlyEnergy (centreHalf, centreNoEarly);
            if (halfEnergy / ec < 0.20 || halfEnergy / ec > 0.30)
                return 9; // 50% = half the amplitude, quarter the energy
            // Even if the nominal pan is zero, LFO modulation reaches the
            // same left/right room-origin geometry using the real input.
            const auto lfoLeft = render (mode, voice, 0.0f, 100.0f, -1.0f);
            const auto lfoRight = render (mode, voice, 0.0f, 100.0f, 1.0f);
            if (balance (lfoLeft) > -5.0 || balance (lfoRight) < 5.0)
                return 5;
            const auto zeroLfo = render (mode, voice, 0.0f, 0.0f, 1.0f);
            if (centre.early != zeroLfo.early) return 6;
        }
    std::cout << "PASS: TEST2.4.47 directional source positions, centred early reflections, independent level and LFO\n"
                 "across all four reverbs and both voices, diffuse tails, dry preserved\n";
}
