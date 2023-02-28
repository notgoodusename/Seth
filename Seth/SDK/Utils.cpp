#include <cmath>

#include "../Memory.h"

#include "GlobalVars.h"
#include "Entity.h"
#include "matrix3x4.h"
#include "Utils.h"
#include "Vector.h"

std::tuple<float, float, float> rainbowColor(float speed) noexcept
{
    constexpr float pi = std::numbers::pi_v<float>;
    return std::make_tuple(std::sin(speed * memory->globalVars->realtime) * 0.5f + 0.5f,
                           std::sin(speed * memory->globalVars->realtime + 2 * pi / 3) * 0.5f + 0.5f,
                           std::sin(speed * memory->globalVars->realtime + 4 * pi / 3) * 0.5f + 0.5f);
}

int getMaxUserCmdProcessTicks() noexcept
{
    return 24;
}

void applyMatrix(Entity* entity, matrix3x4* boneCacheData, Vector origin, Vector absAngle, Vector mins, Vector maxs) noexcept
{
    memcpy(entity->getBoneCache().memory, boneCacheData, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));
    memory->setAbsOrigin(entity, origin);
    memory->setAbsAngle(entity, Vector{ 0.f, absAngle.y, 0.f });
    memory->setCollisionBounds(entity->getCollideable(), mins, maxs);
}

Vector calculateRelativeAngle(const Vector& source, const Vector& destination, const Vector& viewAngles) noexcept
{
    return ((destination - source).toAngle() - viewAngles).normalize();
}