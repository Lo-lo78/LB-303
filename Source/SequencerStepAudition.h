// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>

namespace lb303
{
// A bounded and non-cyclic preview of ONE sequencer lane. Zero-based steps.
enum class StepAuditionMode { current = 0, startToCurrent = 1, currentToEnd = 2 };
struct StepAuditionRange { int first, last; };
inline StepAuditionRange makeStepAuditionRange (int startStep, int endStep,
                                                int cursor, StepAuditionMode mode) noexcept
{
    const int first = std::clamp (startStep - 1, 0, 127);
    const int last = std::clamp (endStep - 1, first, 127);
    const int current = std::clamp (cursor, first, last);
    switch (mode)
    {
        case StepAuditionMode::startToCurrent: return { first, current };
        case StepAuditionMode::currentToEnd: return { current, last };
        default: return { current, current };
    }
}
// The audio-thread command is a single atomic word: lane (2 bits), start/end
// (7 bits each) and a nonzero validity marker. No dynamic allocation or lock.
// Atomic 32-bit command: lane (1 bit), pattern (6 bits), first/end (7 bits
// each). Nonzero validity marker. No cross-thread shared mutable packet.
inline int packStepAudition (int lane, int pattern, StepAuditionRange r) noexcept
{
    return 1 + (std::clamp (lane, 0, 1))
             + (std::clamp (pattern, 0, 63) << 1)
             + (std::clamp (r.first, 0, 127) << 7)
             + (std::clamp (r.last, 0, 127) << 14);
}
inline int unpackStepAuditionLane (int command) noexcept { return (command - 1) & 1; }
inline int unpackStepAuditionPattern (int command) noexcept { return ((command - 1) >> 1) & 63; }
inline StepAuditionRange unpackStepAuditionRange (int command) noexcept
{
    const int bits = command - 1;
    return { (bits >> 7) & 127, (bits >> 14) & 127 };
}
}
