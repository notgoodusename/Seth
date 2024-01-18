#include "TargetSystem.h"

#include "../SDK/Entity.h"
#include "../SDK/LocalPlayer.h"
#include "../SDK/UserCmd.h"
#include "../SDK/Math.h"

LocalPlayerInfo localPlayerInfo;
std::vector<PlayerTarget> playersTargets;

static auto playerTargetByHandle(int handle) noexcept
{
    const auto it = std::ranges::find(playersTargets, handle, &PlayerTarget::handle);
    return it != playersTargets.end() ? &(*it) : nullptr;
}

void TargetSystem::updateFrame() noexcept
{
    localPlayerInfo.update();

    if (!localPlayer) {
        playersTargets.clear();
        return;
    }

    for (int i = 1; i <= interfaces->engine->getMaxClients(); i++)
    {
        const auto entity = interfaces->entityList->getEntity(i);
        if (!entity || !entity->isPlayer() || entity == localPlayer.get() 
            || entity->isDormant())
            continue;

        if (const auto player = playerTargetByHandle(entity->handle())) {
            player->update(entity);
        }
        else {
            playersTargets.emplace_back(entity);
        }
    }

    std::erase_if(playersTargets, [](const auto& player) { 
        return interfaces->entityList->getEntityFromHandle(player.handle) == nullptr; });
}

void TargetSystem::updateTick(UserCmd* cmd) noexcept
{

}

void TargetSystem::reset() noexcept
{
    playersTargets.clear();
}

void TargetSystem::setPriority(int handle, int priority) noexcept
{
    if (auto target = playerTargetByHandle(handle))
        target->priority = priority;
}

void LocalPlayerInfo::update() noexcept
{
    if (!localPlayer)
        return;

    handle = localPlayer->handle();

    origin = localPlayer->getAbsOrigin();
    eyePosition = localPlayer->getEyePosition();
    viewAngles = interfaces->engine->getViewAngles();
}

Target::Target(Entity* entity) noexcept
{
    handle = entity->handle();
}

PlayerTarget::PlayerTarget(Entity* entity) noexcept : Target{ entity }
{
    update(entity);
}

void PlayerTarget::update(Entity* entity) noexcept
{
    if (simulationTime >= entity->simulationTime())
        return;

    simulationTime = entity->simulationTime();
    isAlive = entity->isAlive();

    Record newRecord{ };

    bool isValid = entity->setupBones(newRecord.matrix.data(), entity->getBoneCache().size, 0x7FF00, memory->globalVars->currentTime);
    if (isValid)
    {
        const auto angle = Math::calculateRelativeAngle(localPlayerInfo.eyePosition, newRecord.matrix[6].origin(), localPlayerInfo.viewAngles);

        newRecord.simulationTime = simulationTime;
    
        distanceToLocal = entity->getAbsOrigin().distTo(localPlayerInfo.origin);
        fovFromLocal = angle.length2D();

        newRecord.origin = entity->origin();
        newRecord.absAngle = entity->getAbsAngle();
        newRecord.eyeAngle = entity->eyeAngles();
        newRecord.worldSpaceCenter = entity->getWorldSpaceCenter();

        newRecord.mins = entity->getCollideable()->obbMinsPreScaled();
        newRecord.maxs = entity->getCollideable()->obbMaxsPreScaled();

        if(const Vector headPos = Math::getCenterOfHitbox(entity, newRecord.matrix.data(), Hitboxes::Head); headPos.notNull())
            newRecord.headPositions.push_back(headPos);
        
        if (const Vector spinePos = Math::getCenterOfHitbox(entity, newRecord.matrix.data(), Hitboxes::Spine1); spinePos.notNull())
            newRecord.bodyPositions.push_back(spinePos);

        if (const Vector pelvisPos = Math::getCenterOfHitbox(entity, newRecord.matrix.data(), Hitboxes::Pelvis); pelvisPos.notNull())
            newRecord.bodyPositions.push_back(pelvisPos);

        playerData.push_back(newRecord);
    }

    while (playerData.size() > 3U && static_cast<int>(playerData.size()) > static_cast<int>(round(1.0f / memory->globalVars->intervalPerTick)))
        playerData.pop_front();
}

const LocalPlayerInfo& TargetSystem::local() noexcept
{
    return localPlayerInfo;
}

const std::vector<PlayerTarget>& TargetSystem::playerTargets(int sortType) noexcept
{
    switch (sortType)
    {
    case 0:
        std::sort(playersTargets.begin(), playersTargets.end(),
            [&](const PlayerTarget& a, const PlayerTarget& b) { return a.distanceToLocal < b.distanceToLocal; });
        break;
    case 1:
        std::sort(playersTargets.begin(), playersTargets.end(),
            [&](const PlayerTarget& a, const PlayerTarget& b) { return a.fovFromLocal < b.fovFromLocal; });
        break;
    default:
        return playersTargets;
    }

    std::sort(playersTargets.begin(), playersTargets.end(),
        [&](const PlayerTarget& a, const PlayerTarget& b) { return a.priority > b.priority; });

    return playersTargets;
}

const PlayerTarget* TargetSystem::playerByHandle(int handle) noexcept
{
    return playerTargetByHandle(handle);
}