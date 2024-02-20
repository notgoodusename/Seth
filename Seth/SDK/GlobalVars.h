#pragma once

#include <cstddef>

#include "Pad.h"

struct UserCmd;

struct GlobalVars {
    const float realTime;
    int frameCount;
    const float absoluteFrameTime;
    float currentTime;
    float frameTime;
    const int maxClients;
    const int tickCount;
    const float intervalPerTick;
    float interpolationAmount;

    float serverTime(UserCmd* = nullptr) const noexcept;
};