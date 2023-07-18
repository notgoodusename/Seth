#include "Animations.h"
#include "TargetSystem.h"

#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Math.h"

std::vector<TargetSystem::Enemy> enemies;

void TargetSystem::updateTargets(UserCmd* cmd) noexcept
{
    enemies.clear();

    if (!localPlayer || !localPlayer->isAlive() || localPlayer->isTaunting() || localPlayer->isBonked() || localPlayer->isFeignDeathReady()
        || localPlayer->isCloaked() || localPlayer->isInBumperKart() || localPlayer->isAGhost())
        return;

    const auto activeWeapon = localPlayer->getActiveWeapon();
    if (!activeWeapon || !canAttack(cmd, activeWeapon))
        return;

    const auto& localPlayerOrigin = localPlayer->getAbsOrigin();
    const auto& localPlayerEyePosition = localPlayer->getEyePosition();

    for (int i = 1; i <= interfaces->engine->getMaxClients(); ++i) {
        const auto entity{ interfaces->entityList->getEntity(i) };
        if (!entity || entity == localPlayer.get() || entity->isDormant() || !entity->isAlive())
            continue;

        if (!entity->getBoneCache().memory)
            continue;

        const auto angle{ Math::calculateRelativeAngle(localPlayerEyePosition, entity->getBoneCache()[6].origin(), cmd->viewangles)};
        const auto fov{ angle.length2D() }; //fov
        const auto distance{ localPlayerOrigin.distTo(entity->getAbsOrigin()) }; //distance
        enemies.emplace_back(i, distance, fov);
    }
}

const std::vector<TargetSystem::Enemy>& TargetSystem::getTargets(int sortType) noexcept
{
    switch (sortType)
    {
    case 0:
        std::sort(enemies.begin(), enemies.end(),
            [&](const TargetSystem::Enemy& a, const TargetSystem::Enemy& b) { return a.distance < b.distance; });
        break;
    case 1:
        std::sort(enemies.begin(), enemies.end(),
            [&](const TargetSystem::Enemy& a, const TargetSystem::Enemy& b) { return a.fov < b.fov; });
        break;
    default:
        break;
    }

	return enemies;
}

void TargetSystem::reset() noexcept
{

}