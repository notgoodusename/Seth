#pragma once

#include <array>
#include <deque>

#include "../ConfigStructs.h"

#include "../SDK/NetworkChannel.h"
#include "../SDK/matrix3x4.h"
#include "../SDK/ModelInfo.h"
#include "../SDK/Vector.h"

enum class FrameStage;
struct UserCmd;

namespace Backtrack
{
    void run(UserCmd*) noexcept;

    void addLatencyToNetwork(NetworkChannel*, float) noexcept;
    void updateIncomingSequences() noexcept;

    float getLerp() noexcept;

    struct IncomingSequence {
        int inReliableState;
        int inSequenceNr;
        float currentTime;
    };

    bool valid(float simtime) noexcept;
    void init() noexcept;
    void reset() noexcept;
    float getMaxUnlag() noexcept;
}
