#pragma once

#include <numbers>
#include <tuple>

#include "../Memory.h"

#include "../SDK/GlobalVars.h"

class Entity;
class matrix3x4;
struct Vector;
struct UserCmd;

static auto timeToTicks(float time) noexcept { return static_cast<int>(0.5f + time / memory->globalVars->intervalPerTick); }
static auto ticksToTime(int ticks) noexcept { return static_cast<float>(ticks * memory->globalVars->intervalPerTick); }

std::tuple<float, float, float> rainbowColor(float speed) noexcept;

bool canAttack(UserCmd* cmd, Entity* activeWeapon) noexcept;

bool isAttacking(UserCmd* cmd, Entity* activeWeapon) noexcept;

void resetUtil() noexcept;