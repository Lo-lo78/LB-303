// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <utility>

namespace lb303
{
// A causal 8th-order Butterworth antialias filter for 2x/4x synthesis and
// distortion. 1x is bit-for-bit bypassed. No allocations or look-ahead.
// It has nonlinear phase (typical of low-latency IIR oversampling), not a
// fixed integer delay. Avoid changing quality mid-note when recording.
class VoiceDecimator final
{
public:
    void setFactor (int value) noexcept
    {
        value = value == 2 || value == 4 ? value : 1;
        if (value == factor) return;
        factor = value;
        clear();
        if (factor == 1) return;
        constexpr double pi = 3.1415926535897932384626433832795;
        // Cutoff at 45% of the original Nyquist: usable bassline brightness
        // with a steep stopband above the host Nyquist before decimation.
        const double w = 2.0 * pi * (0.225 / factor);
        const double c = std::cos (w);
        for (int i = 0; i < 4; ++i)
        {
            const double q = 1.0 / (2.0 * std::cos ((2.0*i + 1.0) * pi / 16.0));
            const double alpha = std::sin (w) / (2.0 * q);
            const double a0 = 1.0 / (1.0 + alpha);
            for (int ch = 0; ch < 2; ++ch)
            {
                auto& b = channels[static_cast<size_t> (ch)][static_cast<size_t> (i)];
                b.b0 = (1.0 - c) * 0.5 * a0;
                b.b1 = (1.0 - c) * a0;
                b.b2 = b.b0;
                b.a1 = -2.0 * c * a0;
                b.a2 = (1.0 - alpha) * a0;
            }
        }
    }
    void clear() noexcept
    {
        for (auto& channel : channels)
            for (auto& s : channel)
                s.z1 = s.z2 = 0.0;
    }
    // Feed EVERY high-rate sample (including all intermediate samples);
    // use the output only after factor calls for a base-rate output frame.
    std::pair<float, float> process (float l, float r) noexcept
    {
        if (factor == 1) return { l, r };
        return { channel (l, channels[0]), channel (r, channels[1]) };
    }
    int getFactor() const noexcept { return factor; }
    bool isQuiescent() const noexcept
    {
        // The downsampler also stores ringing after the synth reaches zero.
        if (factor == 1) return true;
        constexpr double threshold = 1.0e-10;
        for (const auto& channel : channels)
            for (const auto& stage : channel)
                if (std::abs (stage.z1) >= threshold
                    || std::abs (stage.z2) >= threshold) return false;
        return true;
    }
private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double z1 = 0, z2 = 0;
    };
    static float channel (float x, std::array<Biquad, 4>& cascade) noexcept
    {
        double value = std::isfinite (x) ? x : 0.0;
        for (auto& b : cascade)
        {
            const double out = b.b0 * value + b.z1;
            b.z1 = b.b1 * value - b.a1 * out + b.z2;
            b.z2 = b.b2 * value - b.a2 * out;
            value = out;
        }
        return static_cast<float> (std::isfinite (value)
            ? std::clamp (value, -1000.0, 1000.0) : 0.0);
    }
    std::array<std::array<Biquad, 4>, 2> channels {};
    int factor = 1;
};
}
