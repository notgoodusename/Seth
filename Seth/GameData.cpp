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

#include "SDK/Client.h"
#include "SDK/ClientClass.h"
#include "SDK/Engine.h"
#include "SDK/EngineTrace.h"
#include "SDK/Entity.h"
#include "SDK/EntityList.h"
#include "SDK/GlobalVars.h"
#include "SDK/Localize.h"
#include "SDK/LocalPlayer.h"
#include "SDK/ModelInfo.h"
#include "SDK/RenderView.h"
#include "SDK/ViewSetup.h"

static Matrix4x4 viewMatrix;
static LocalPlayerData localPlayerData;
static std::vector<PlayerData> playerData;
static std::vector<BuildingsData> buildingsData;
static std::vector<WorldData> worldData;

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

    buildingsData.clear();
    worldData.clear();

    localPlayerData.update();

    if (!localPlayer) {
        playerData.clear();
        return;
    }

    if (ViewSetup viewSetup = {}; interfaces->client->getPlayerView(viewSetup))
    {
        Matrix4x4 worldToView = {}, viewToProjection = {}, worldToProjection = {}, worldToPixels = {};
        interfaces->renderView->getMatricesForView(viewSetup, &worldToView, &viewToProjection, &worldToProjection, &worldToPixels);
        viewMatrix = worldToProjection;
    }

    const auto highestEntityIndex = interfaces->entityList->getHighestEntityIndex();
    for (int i = 1; i <= highestEntityIndex; ++i) {
        const auto entity = interfaces->entityList->getEntity(i);
        if (!entity)
            continue;

        if (entity->isPlayer()) {
            if (entity == localPlayer.get())
                continue;

            if (const auto player = playerByHandleWritable(entity->handle())) {
                player->update(entity);
            }
            else {
                playerData.emplace_back(entity);
            }
        }
        else
        {
            switch (entity->getClassId())
            {
                case ClassId::ObjectSentrygun:
                case ClassId::ObjectDispenser:
                case ClassId::ObjectTeleporter:
                    buildingsData.emplace_back(entity);
                    break;
                case ClassId::BaseAnimating:
                    worldData.emplace_back(entity);
                    break;
                case ClassId::TFAmmoPack:
                    worldData.emplace_back(entity);
                    break;
                case ClassId::TFProjectile_Rocket:
                case ClassId::TFGrenadePipebombProjectile:
                case ClassId::TFProjectile_Jar:
                case ClassId::TFProjectile_JarGas:
                case ClassId::TFProjectile_JarMilk:
                case ClassId::TFProjectile_Arrow:
                case ClassId::TFProjectile_SentryRocket:
                case ClassId::TFProjectile_Flare:
                case ClassId::TFProjectile_GrapplingHook:
                case ClassId::TFProjectile_Cleaver:
                case ClassId::TFProjectile_EnergyBall:
                case ClassId::TFProjectile_EnergyRing:
                case ClassId::TFProjectile_HealingBolt:
                case ClassId::TFProjectile_ThrowableBreadMonster:
                case ClassId::TFStunBall:
                case ClassId::TFBall_Ornament:
                    break;
                case ClassId::HeadlessHatman:
                case ClassId::TFTankBoss:
                case ClassId::Merasmus:
                case ClassId::Zombie:
                case ClassId::EyeballBoss:
                    break;
            default:
                break;
            }
        }
    }

    std::sort(playerData.begin(), playerData.end());
    std::sort(buildingsData.begin(), buildingsData.end());
    std::sort(worldData.begin(), worldData.end());

    std::erase_if(playerData, [](const auto& player) { return interfaces->entityList->getEntityFromHandle(player.handle) == nullptr; });
    std::erase_if(worldData, [](const auto& world) { return world.name == ""; });

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

const std::vector<BuildingsData>& GameData::buildings() noexcept
{
    return buildingsData;
}

const std::vector<WorldData>& GameData::world() noexcept
{
    return worldData;
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
    else {
        obbMins = entity->obbMins();
        obbMaxs = entity->obbMaxs();
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

    if (!alive)
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

BuildingsData::BuildingsData(Entity* building) noexcept : BaseData{ building }
{
    if (localPlayer) {
        enemy = building->isEnemy(localPlayer.get());
    }

    alive = building->isAlive();
    health = building->objectHealth();
    maxHealth = building->objectMaxHealth();

    switch (building->objectType())
    {
        case 0:
            name = "Dispenser";
            break;
        case 1:
            name = building->objectMode() >= 1 ? "Teleporter Out" : "Teleporter In";
            break;
        case 2:
            name = building->isMini() ? "Mini Sentry" : "Sentry";
            break;
        default:
            name = "unknown";
            break;
    }

    if (!building->mapPlaced())
    {
        if (const auto buildingOwner = building->getObjectOwner())
            owner = buildingOwner->getPlayerName();
    }
    else
        owner = "Map placed";
}

WorldData::WorldData(Entity* worldEntity) noexcept : BaseData{ worldEntity }
{
    switch (fnv::hashRuntime(worldEntity->getModelName()))
    {
        case fnv::hash("models/items/medkit_small.mdl"):
        case fnv::hash("models/items/medkit_small_bday.mdl"):
        case fnv::hash("models/props_halloween/halloween_medkit_small.mdl"):
            name = "Pill";
            break;
        case fnv::hash("models/items/ammopack_small.mdl"):
        case fnv::hash("models/items/ammopack_small_bday.mdl"):
            name = "Ammo";
            break;
        case fnv::hash("models/items/medkit_medium.mdl"):
        case fnv::hash("models/items/medkit_medium_bday.mdl"):
        case fnv::hash("models/props_halloween/halloween_medkit_medium.mdl"):
            name = "Medkit";
            break;
        case fnv::hash("models/items/ammopack_medium.mdl"):
        case fnv::hash("models/items/ammopack_medium_bday.mdl"):
            name = "Ammo Pack";
            break;
        case fnv::hash("models/items/medkit_large.mdl"):
        case fnv::hash("models/items/medkit_large_bday.mdl"):
        case fnv::hash("models/props_halloween/halloween_medkit_large.mdl"):
            name = "Full Medkit";
            break;
        case fnv::hash("models/items/ammopack_large.mdl"):
        case fnv::hash("models/items/ammopack_large_bday.mdl"):
            name = "Full Ammo Pack";
            break;
        case fnv::hash("models/items/tf_gift.mdl"):
        case fnv::hash("models/props_halloween/halloween_gift.mdl"):
            name = "Gift";
            break;
        case fnv::hash("models/items/plate.mdl"):
        case fnv::hash("models/items/plate_steak.mdl"):
        case fnv::hash("models/items/plate_robo_sandwich.mdl"):
            name = "Sandwich";
            break;
        case fnv::hash("models/items/currencypack_small.mdl"):
        case fnv::hash("models/items/currencypack_medium.mdl"):
        case fnv::hash("models/items/currencypack_large.mdl"):
            name = "Money";
            break;
        default:
            name = "";
            break;
    }
}