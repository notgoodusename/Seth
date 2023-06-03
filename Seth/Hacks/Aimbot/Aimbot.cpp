#include "Aimbot.h"
#include "AimbotHitscan.h"
#include "AimbotMelee.h"
#include "AimbotProjectile.h"
#include "../Animations.h"
#include "../Backtrack.h"

#include "../../SDK/UserCmd.h"
#include "../../SDK/Math.h"
#include "../../SDK/ModelInfo.h"
#include "../../SDK/Vector.h"

std::vector<Aimbot::Enemy> enemies;

void Aimbot::run(UserCmd* cmd) noexcept
{
    if (!localPlayer || !localPlayer->isAlive() || localPlayer->isTaunting() || localPlayer->isBonked() || localPlayer->isFeignDeathReady()
        || localPlayer->isCloaked() || localPlayer->isInBumperKart() || localPlayer->isAGhost())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon)
        return;

    enemies.clear();

    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();
    for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
        const auto entity{ interfaces->entityList->getEntity(i) };
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
            continue;

        const auto player = Animations::getPlayer(i);
        if (!player.gotMatrix)
            continue;

        const auto angle{ Math::calculateRelativeAngle(localPlayerEyePosition, player.matrix[6].origin(), cmd->viewangles) };
        const auto fov{ angle.length2D() }; //fov
        const auto health{ entity->health() }; //health
        const auto distance{ localPlayerOrigin.distTo(entity->getAbsOrigin()) }; //distance
        enemies.emplace_back(i, health, distance, fov);
    }

    if (!config->aimbotKey.isActive() ||
        (!config->aimbot.hitscan.enabled && !config->aimbot.projectile.enabled && !config->aimbot.melee.enabled))
        return;

    if (enemies.empty())
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

std::vector<Vector> Aimbot::multiPoint(Entity* entity, const matrix3x4 matrix[MAXSTUDIOBONES], StudioBbox* hitbox, Vector localEyePos, int _hitbox, bool doNotRunMultipoint) noexcept
{
    auto VectorTransformWrapper = [](const Vector& in1, const matrix3x4 in2, Vector& out)
    {
        auto VectorTransform = [](const float* in1, const matrix3x4 in2, float* out)
        {
            auto dotProducts = [](const float* v1, const float* v2)
            {
                return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
            };
            out[0] = dotProducts(in1, in2[0]) + in2[0][3];
            out[1] = dotProducts(in1, in2[1]) + in2[1][3];
            out[2] = dotProducts(in1, in2[2]) + in2[2][3];
        };
        VectorTransform(&in1.x, in2, &out.x);
    };

    Vector min, max, center;
    VectorTransformWrapper(hitbox->bbMin, matrix[hitbox->bone], min);
    VectorTransformWrapper(hitbox->bbMax, matrix[hitbox->bone], max);
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

std::vector<Aimbot::Enemy>& Aimbot::getEnemies() noexcept
{
    return enemies;
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