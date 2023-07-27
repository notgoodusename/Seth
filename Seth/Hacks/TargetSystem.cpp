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
            || entity->isDormant() || !entity->isAlive())
            continue;

        if (const auto player = playerTargetByHandle(entity->handle())) {
            player->update(entity);
        }
        else {
            playersTargets.emplace_back(entity);
        }
    }

    std::erase_if(playersTargets, [](const auto& player) { return interfaces->entityList->getEntityFromHandle(player.handle) == nullptr || !player.isValid; });
}

void TargetSystem::updateTick(UserCmd* cmd) noexcept
{

}

void TargetSystem::reset() noexcept
{
    playersTargets.clear();
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

Target::Target(Entity* entity) noexcept : handle{ entity->handle() }
{
    if (entity->isPlayer()) {
        const auto collideable = entity->getCollideable();
        obbMins = collideable->obbMins();
        obbMaxs = collideable->obbMaxs();
    }
    else {
        obbMins = entity->obbMins();
        obbMaxs = entity->obbMaxs();
    }
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

    isValid = entity->setupBones(matrix.data(), entity->getBoneCache().size, 0x7FF00, memory->globalVars->currenttime);
    if (!isValid)
        return;

    const auto angle = Math::calculateRelativeAngle(localPlayerInfo.eyePosition, matrix[6].origin(), localPlayerInfo.viewAngles);
    
    distanceToLocal = entity->getAbsOrigin().distTo(localPlayerInfo.origin);
    fovFromLocal = angle.length2D();

    origin = entity->origin();
    eyeAngle = entity->eyeAngles();
    absAngle = entity->getAbsAngle();

    mins = entity->getCollideable()->obbMinsPreScaled();
    maxs = entity->getCollideable()->obbMaxsPreScaled();

    //Handle backtrack
    if (!backtrackRecords.empty() && (backtrackRecords.front().simulationTime == entity->simulationTime()))
        return;

    Record record{ };
    record.origin = origin;
    record.absAngle = absAngle;
    record.eyeAngle = eyeAngle;
    record.simulationTime = simulationTime;
    record.mins = mins;
    record.maxs = maxs;
    std::copy(matrix.begin(), matrix.end(), record.matrix);
    for (auto bone : { 6, 0 }) {
        record.positions.push_back(record.matrix[bone].origin());
    }

    backtrackRecords.push_front(record);

    while (backtrackRecords.size() > 3U && static_cast<int>(backtrackRecords.size()) > static_cast<int>(round(1.0f / memory->globalVars->intervalPerTick)))
        backtrackRecords.pop_back();
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

    return playersTargets;
}

const PlayerTarget* TargetSystem::playerByHandle(int handle) noexcept
{
    return playerTargetByHandle(handle);
}