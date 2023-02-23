#include <atomic>
#include <cstring>
#include <list>
#include <mutex>

#include "Config.h"
#include "fnv.h"
#include "GameData.h"
#include "Interfaces.h"
#include "Memory.h"

#include "Resources/avatar_ct.h"
#include "Resources/avatar_tt.h"
#include "Resources/skillgroups.h"

#include "stb_image.h"

#include "SDK/ClientClass.h"
#include "SDK/Engine.h"
#include "SDK/EngineTrace.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/GlobalVars.h"
#include "SDK/Localize.h"
#include "SDK/LocalPlayer.h"
#include "SDK/ModelInfo.h"

static Matrix4x4 viewMatrix;
static LocalPlayerData localPlayerData;
static std::vector<PlayerData> playerData;


static auto playerByHandleWritable(int handle) noexcept
{
    const auto it = std::ranges::find(playerData, handle, &PlayerData::handle);
    return it != playerData.end() ? &(*it) : nullptr;
}

constexpr auto playerVisibilityUpdateDelay = 0.1f;
static float nextPlayerVisibilityUpdateTime = 0.0f;

static bool shouldUpdatePlayerVisibility() noexcept
{
    return nextPlayerVisibilityUpdateTime <= memory->globalVars->realtime;
}

void GameData::update() noexcept
{
    static int lastFrame;
    if (lastFrame == memory->globalVars->framecount)
        return;
    lastFrame = memory->globalVars->framecount;

    Lock lock;

    localPlayerData.update();

    if (!localPlayer) {
        playerData.clear();
        return;
    }

    viewMatrix = interfaces->engine->worldToScreenMatrix();

    const auto highestEntityIndex = interfaces->entityList->getHighestEntityIndex();
    for (int i = 1; i <= highestEntityIndex; ++i) {
        const auto entity = interfaces->entityList->getEntity(i);
        if (!entity)
            continue;

        if (entity->isPlayer()) {
            if (entity == localPlayer.get() || !entity->isAlive())
                continue;

            if (const auto player = playerByHandleWritable(entity->handle())) {
                player->update(entity);
            }
            else {
                playerData.emplace_back(entity);
            }
        }
    }

    std::sort(playerData.begin(), playerData.end());

    std::erase_if(playerData, [](const auto& player) { return interfaces->entityList->getEntityFromHandle(player.handle) == nullptr; });

    if (shouldUpdatePlayerVisibility())
        nextPlayerVisibilityUpdateTime = memory->globalVars->realtime + playerVisibilityUpdateDelay;
}

const Matrix4x4& GameData::toScreenMatrix() noexcept
{
    return viewMatrix;
}

const LocalPlayerData& GameData::local() noexcept
{
    return localPlayerData;
}

const std::vector<PlayerData>& GameData::players() noexcept
{
    return playerData;
}

const PlayerData* GameData::playerByHandle(int handle) noexcept
{
    return playerByHandleWritable(handle);
}

void LocalPlayerData::update() noexcept
{
    if (!localPlayer) {
        exists = false;
        return;
    }

    exists = true;
    alive = localPlayer->isAlive();
    team = localPlayer->teamNumber();

    fov = localPlayer->fov() ? localPlayer->fov() : localPlayer->defaultFov();
    handle = localPlayer->handle();

    origin = localPlayer->getAbsOrigin();
}

BaseData::BaseData(Entity* entity) noexcept
{
    distanceToLocal = entity->getAbsOrigin().distTo(localPlayerData.origin);

    if (entity->isPlayer()) {
        const auto collideable = entity->getCollideable();
        obbMins = collideable->obbMins();
        obbMaxs = collideable->obbMaxs();
    }
    else if (const auto model = entity->getModel()) {
        obbMins = model->mins;
        obbMaxs = model->maxs;
    }

    coordinateFrame = entity->toWorldTransform();
}

PlayerData::PlayerData(Entity* entity) noexcept : BaseData{ entity }, handle{ entity->handle() }
{
    update(entity);
}

void PlayerData::update(Entity* entity) noexcept
{
    name = entity->getPlayerName();
    const auto idx = entity->index();

    dormant = entity->isDormant();
    if (dormant) {
        return;
    }

    team = entity->teamNumber();
    static_cast<BaseData&>(*this) = { entity };
    origin = entity->getAbsOrigin();
    inViewFrustum = !interfaces->engine->cullBox(obbMins + origin, obbMaxs + origin);
    alive = entity->isAlive();
    lastContactTime = alive ? memory->globalVars->realtime : 0.0f;

    if (localPlayer) {
        enemy = entity->isEnemy(localPlayer.get());
    }

    health = entity->health();
    maxHealth = entity->getMaxHealth();

    isCloaked = entity->isCloaked();

    if (const auto weapon = entity->getActiveWeapon())
        activeWeapon = interfaces->localize->findAsUTF8(weapon->getPrintName());

    if (!alive || !inViewFrustum)
        return;

    const auto model = entity->getModel();
    if (!model)
        return;

    const auto studioModel = interfaces->modelInfo->getStudioModel(model);
    if (!studioModel)
        return;

    if (!entity->getBoneCache().memory)
        return;

    matrix3x4 boneMatrices[MAXSTUDIOBONES];
    memcpy(boneMatrices, entity->getBoneCache().memory, std::clamp(entity->getBoneCache().size, 0, MAXSTUDIOBONES) * sizeof(matrix3x4));

    bones.clear();
    bones.reserve(20);

    for (int i = 0; i < studioModel->numBones; ++i) {
        const auto bone = studioModel->getBone(i);

        if (!bone || bone->parent == -1 || !(bone->flags & BONE_USED_BY_HITBOX))
            continue;

        bones.emplace_back(boneMatrices[i].origin(), boneMatrices[bone->parent].origin());
    }
}

float PlayerData::fadingAlpha() const noexcept
{
    constexpr float fadeTime = 1.50f;
    return std::clamp(1.0f - (memory->globalVars->realtime - lastContactTime - 0.25f) / fadeTime, 0.0f, 1.0f);
}