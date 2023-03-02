#pragma once

#include <numbers>
#include <tuple>

#include "../Memory.h"

#include "../SDK/GlobalVars.h"

class Entity;
class matrix3x4;
struct Vector;

static auto timeToTicks(float time) noexcept { return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); }
static auto ticksToTime(int ticks) noexcept { return static_cast<float>(ticks * memory->globalVars->intervalPerTick); }

std::tuple<float, float, float> rainbowColor(float speed) noexcept;

int getMaxUserCmdProcessTicks() noexcept;

#define maxUserCmdProcessTicks getMaxUserCmdProcessTicks()

void applyMatrix(Entity* entity, matrix3x4* boneCacheData, Vector origin, Vector eyeAngle, Vector mins, Vector maxs) noexcept;

Vector calculateRelativeAngle(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept;