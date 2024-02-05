#include "Aimbot.h"
#include "AimbotHitscan.h"
#include "AimbotMelee.h"
#include "AimbotProjectile.h"
#include "../Backtrack.h"

#include "../../SDK/Entity.h"
#include "../../SDK/UserCmd.h"
#include "../../SDK/Math.h"
#include "../../SDK/ModelInfo.h"
#include "../../SDK/Vector.h"

void Aimbot::run(UserCmd* cmd) noexcept
{
    if (!config->aimbotKey.isActive() ||
        (!config->aimbot.hitscan.enabled && !config->aimbot.projectile.enabled && !config->aimbot.melee.enabled))
        return;

    if (!localPlayer || !localPlayer->isAlive() || localPlayer->isTaunting() || localPlayer->isBonked() || localPlayer->isFeignDeathReady()
        || localPlayer->isCloaked() || localPlayer->isInBumperKart() || localPlayer->isAGhost())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    const auto weaponType = activeWeapon->getWeaponType();
    switch (weaponType)
    {
        case WeaponType::HITSCAN:
            AimbotHitscan::run(activeWeapon, cmd);
            break;
        case WeaponType::PROJECTILE:
            AimbotProjectile::run(activeWeapon, cmd);
            break;
        case WeaponType::MELEE:
            AimbotMelee::run(activeWeapon, cmd);
            break;
        default:
            break;
    }
}

std::vector<Vector> Aimbot::multiPoint(Entity* entity, const matrix3x4* matrix, StudioBbox* hitbox, Vector localEyePos, int _hitbox, bool doNotRunMultipoint) noexcept
{
    Vector min, max, center;
    Math::vectorTransform(hitbox->bbMin, &matrix[hitbox->bone], min);
    Math::vectorTransform(hitbox->bbMax, &matrix[hitbox->bone], max);
    center = (min + max) * 0.5f;

    std::vector<Vector> vecArray;

    vecArray.emplace_back(center);

    if (doNotRunMultipoint)
        return vecArray;

    Vector currentAngles = Math::calculateRelativeAngle(center, localEyePos, Vector{});

    Vector forward;
    Vector::fromAngle(currentAngles, &forward);

    Vector right = forward.cross(Vector{ 0, 0, 1 });
    Vector left = Vector{ -right.x, -right.y, right.z };

    Vector top = Vector{ 0, 0, 1 };
    Vector bottom = Vector{ 0, 0, -1 };

    float multiPoint = 50.0f * 0.01f;

    switch (_hitbox)
    {
    case Hitboxes::Head:
        for (auto i = 0; i < 4; ++i)
            vecArray.emplace_back(center);

        vecArray[1] += top * multiPoint;
        vecArray[2] += right * multiPoint;
        vecArray[3] += left * multiPoint;
        break;
    default://rest
        for (auto i = 0; i < 3; ++i)
            vecArray.emplace_back(center);

        vecArray[1] += right * multiPoint;
        vecArray[2] += left * multiPoint;
        break;
    }
    return vecArray;
}

void Aimbot::updateInput() noexcept
{
    config->aimbotKey.handleToggle();
}

void Aimbot::reset() noexcept
{
    AimbotHitscan::reset();
    AimbotMelee::reset();
    AimbotProjectile::reset();
}